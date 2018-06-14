/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <sys/stat.h>

#include <sodium.h>

#include "common.h"
#include "config.h"

void die(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    putc('\n', stderr);

    exit(1);
}

void die_errno(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(errno));

    exit(1);
}

void warn(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    putc('\n', stderr);

    exit(1);
}

void version(const char *executable)
{
    printf("%s version "GOB_VERSION"\n"
           "\n"
           "block size: %d\n"
           "hash size:  %d\n", executable, BLOCK_LEN, HASH_LEN);
    exit(0);
}

ssize_t read_bytes(int fd, unsigned char *buf, size_t buflen)
{
    size_t total = 0;

    while (total != buflen) {
        ssize_t bytes = read(fd, buf + total, buflen - total);
        if (bytes < 0)
            return -1;
        if (bytes == 0)
            break;
        total += bytes;
    }

    return total;
}

int write_bytes(int fd, const unsigned char *buf, size_t buflen)
{
    size_t total = 0;

    while (total != buflen) {
        ssize_t bytes = write(fd, buf + total, buflen - total);
        if (bytes <= 0)
            return -1;
        total += bytes;
    }

    return 0;
}

int hash_from_bin(struct hash *out, const unsigned char *data, size_t len)
{
    if (len != HASH_LEN)
        return -1;
    memcpy(&out->bin[0], data, len);
    memset(&out->hex[0], 0, sizeof(out->hex));
    sodium_bin2hex(&out->hex[0], sizeof(out->hex), data, len);
    return 0;
}

int hash_from_str(struct hash *out, const char *str, size_t len)
{
    size_t parsed_len;

    if (!len)
        len = strlen(str);

    if (len != HASH_LEN * 2 || strspn(str, "0123456789abcdef") != len)
        return -1;
    if (sodium_hex2bin(&out->bin[0], sizeof(out->bin), str, len, NULL, &parsed_len, NULL) < 0)
        return -1;
    if (parsed_len != sizeof(out->bin))
        return -1;

    memset(&out->hex[0], 0, sizeof(out->hex));
    strncpy(&out->hex[0], str, len);

    return 0;
}

int hash_eq(const struct hash *a, const struct hash *b)
{
    return !memcmp(a->bin, b->bin, sizeof(a->bin));
}

int hash_compute(struct hash *out, const unsigned char *data, size_t len)
{
    struct hash_state state;

    if (hash_state_init(&state) < 0 ||
            hash_state_update(&state, data, len) < 0 ||
            hash_state_final(out, &state) < 0)
        return -1;
    return 0;
}

int hash_state_init(struct hash_state *state)
{
    if (crypto_generichash_init(&state->state, NULL, 0, HASH_LEN) < 0)
        return -1;
    return 0;
}

int hash_state_update(struct hash_state *state, const unsigned char *data, size_t len)
{
    if (crypto_generichash_update(&state->state, data, len) < 0)
        return -1;
    return 0;
}

int hash_state_final(struct hash *out, struct hash_state *state)
{
    unsigned char hash[HASH_LEN];
    if (crypto_generichash_final(&state->state, hash, sizeof(hash)) < 0)
        return -1;
    return hash_from_bin(out, hash, sizeof(hash));
}

int store_open(struct store *out, const char *path)
{
    struct stat st;
    int i, storefd, versionfd;
    uint32_t version;

    if ((storefd = open(path, O_RDONLY)) < 0)
        die_errno("Unable to open storage '%s'", path);

    if (fstat(storefd, &st) < 0 || !S_ISDIR(st.st_mode))
        die("Storage is not a directory");

    if ((versionfd = openat(storefd, BLOCK_STORE_VERSION_FILE, O_RDONLY)) < 0) {
        version = htonl(BLOCK_STORE_VERSION);

        if ((versionfd = openat(storefd, BLOCK_STORE_VERSION_FILE,
                O_CREAT|O_EXCL|O_WRONLY, 0644)) < 0)
            die_errno("Unable to create store version file");

        if (write_bytes(versionfd, (unsigned char *) &version, sizeof(version)) < 0)
            die_errno("Unable to write store version");
    } else {
        if (read_bytes(versionfd, (unsigned char *) &version, sizeof(version))
                != sizeof(version))
            die_errno("Unable to read store version");

        version = ntohl(version);
        if (version != BLOCK_STORE_VERSION)
            die_errno("Unable to open block store with version %"PRIu32, version);
    }

    close(versionfd);

    out->fd = storefd;
    for (i = 0; i < 256; i++)
        out->shardfds[i] = -1;

    return 0;
}

void store_close(struct store *store)
{
    int i;

    close(store->fd);
    for (i = 0; i < 256; i++)
        if (store->shardfds[i] >= 0)
            close(store->shardfds[i]);
}

static int open_shard(struct store *store, const struct hash *hash, int create)
{
    struct stat st;
    char shard[3];
    int shardfd;

    if ((shardfd = store->shardfds[hash->bin[0]]) >= 0)
        return shardfd;

    shard[0] = hash->hex[0];
    shard[1] = hash->hex[1];
    shard[2] = '\0';

    if ((shardfd = openat(store->fd, shard, O_RDONLY)) >= 0) {
        if (fstat(shardfd, &st) < 0 || !S_ISDIR(st.st_mode))
            die("Shard is not a directory");
        goto out;
    }

    if (!create)
        die_errno("Unable to open sharding directory '%s'", shard);

    if (mkdirat(store->fd, shard, 0755) < 0)
        die_errno("Unable to create sharding directory '%s'", shard);
    if ((shardfd = openat(store->fd, shard, O_RDONLY)) < 0)
        die_errno("Unable to open sharding directory '%s'", shard);

out:
    store->shardfds[hash->bin[0]] = shardfd;
    return shardfd;
}

int store_write(struct hash *out, struct store *store, const unsigned char *data, size_t datalen)
{
    struct hash hash;
    int fd, shardfd;

    if (hash_compute(&hash, data, datalen) < 0)
        die("Unable to hash block");

    if ((shardfd = open_shard(store, &hash, 1)) < 0)
        die("Unable to open shard");

    if ((fd = openat(shardfd, hash.hex + 2, O_CREAT|O_EXCL|O_WRONLY, 0644)) < 0) {
        if (errno == EEXIST)
            goto out;
        die_errno("Unable to create block '%s'", hash.hex);
    }

    if (write_bytes(fd, data, datalen) < 0 || close(fd) < 0) {
        die_errno("Unable to write block '%s'", hash.hex);
    }

out:
    if (out)
        memcpy(out, &hash, sizeof(*out));

    return 0;
}

int store_read(unsigned char *out, size_t outlen, struct store *store, const struct hash *hash)
{
    int fd, shardfd;
    ssize_t len;

    if ((shardfd = open_shard(store, hash, 0)) < 0)
        die("Unable to open shard");

    if ((fd = openat(shardfd, hash->hex + 2, O_RDONLY)) < 0)
        die_errno("Unable to open block '%s'", hash->hex);

    if ((len = read_bytes(fd, out, outlen)) < 0)
        die_errno("Unable to read block '%s'", hash->hex);

    close(fd);

    return len;
}

int read_keys(struct nonce_key *nout, struct encrypt_key *cout, const char *file)
{
    unsigned char masterkey[MASTER_KEY_LEN];
    char masterkey_hex[MASTER_KEY_LEN * 2];
    size_t parsed_len;
    ssize_t bytes;
    int fd;

    if ((fd = open(file, O_RDONLY)) < 0)
        die_errno("Unable to open keyfile '%s'", file);
    if ((bytes = read_bytes(fd, (unsigned char *) masterkey_hex, sizeof(masterkey_hex))) < 0)
        die_errno("Unable to read keyfile '%s'", file);
    if (bytes != MASTER_KEY_LEN * 2)
        die("Invalid key length: expected %"PRIuMAX", got %"PRIuMAX, MASTER_KEY_LEN * 2, bytes);
    if (sodium_hex2bin(masterkey, sizeof(masterkey), masterkey_hex, sizeof(masterkey_hex), NULL, &parsed_len, NULL) < 0)
        die("Unable to convert key to hex");
    if (parsed_len != MASTER_KEY_LEN)
        die("Key not fully parsed");

    if (cout && crypto_kdf_derive_from_key(cout->data, sizeof(cout->data), 1, "gobcrypt", masterkey) < 0)
        die("Unable do derive encryption key");

    if (nout && crypto_kdf_derive_from_key(nout->data, sizeof(nout->data), 2, "gobnonce", masterkey) < 0)
        die("Unable do derive nonce key");

    close(fd);

    return 0;
}
