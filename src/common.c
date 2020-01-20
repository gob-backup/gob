/*
 * Copyright (C) 2018 Patrick Steinhardt
 *
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

#include "common.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#ifdef HAVE_FPENDING
# include <stdio_ext.h>
#endif
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

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
}

int try_close(int fd)
{
  int error;
  do {
      error = close(fd);
  } while (error < 0 && errno == EINTR);
  return error;
}

int try_closedir(DIR *d)
{
  int error;
  do {
      error = closedir(d);
  } while (error < 0 && errno == EINTR);
  return error;
}

void close_stdout(void)
{
#ifdef HAVE_FPENDING
    char some_pending = (__fpending(stdout) != 0);
#else
    char some_pending = 0;
#endif
    char prev_fail = (ferror(stdout) != 0);
    char fclose_fail = (fclose(stdout) != 0);

    if (prev_fail || (fclose_fail && (some_pending || errno != EBADF))) {
        if (!fclose_fail)
            fprintf(stderr, "Error closing stdout: %s\n", strerror(errno));
        else
            fprintf(stderr, "Error closing stdout\n");
        _exit(1);
    }
}

ssize_t read_bytes(int fd, unsigned char *buf, size_t buflen)
{
    size_t total = 0;

    while (total != buflen) {
        ssize_t bytes = read(fd, buf + total, buflen - total);
        if (bytes < 0 && (errno == EAGAIN || errno == EINTR))
            continue;
        if (bytes < 0)
            return -1;
        if (bytes == 0)
            break;
        total += (size_t) bytes;
    }

    return (ssize_t) total;
}

int write_bytes(int fd, const unsigned char *buf, size_t buflen)
{
    size_t total = 0;

    while (total != buflen) {
        ssize_t bytes = write(fd, buf + total, buflen - total);
        if (bytes < 0 && (errno == EAGAIN || errno == EINTR))
            continue;
        if (bytes <= 0)
            return -1;
        total += (size_t) bytes;
    }

    return 0;
}

static char to_hex[] = "0123456789abcdef";

int hash_from_bin(struct hash *out, const unsigned char *data, size_t len)
{
    size_t i;
    char *p = &out->hex[0];

    if (len != HASH_LEN)
        return -1;

    memcpy(&out->bin[0], data, len);

    for (i = 0; i < HASH_LEN; i++) {
        *p++ = to_hex[((unsigned int) out->bin[i]) >> 4];
        *p++ = to_hex[((unsigned int) out->bin[i]) & 0xf];
    }
    *p = '\0';

    return 0;
}

static signed char from_hex[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 00 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 10 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 20 */
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1, /* 30 */
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 40 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 50 */
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 60 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 70 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 80 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 90 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* a0 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* b0 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* c0 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* d0 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* e0 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* f0 */
};

int hash_from_str(struct hash *out, const char *str, size_t len)
{
    size_t i;

    if (len != HASH_LEN * 2)
        return -1;

    memset(out, 0, sizeof(*out));

    for (i = 0; i < len; i++) {
        signed char c = from_hex[(unsigned char) str[i]];
        if (c < 0)
            return -1;
        out->bin[i / 2] |= (unsigned char)(c << (i % 2 ? 0 : 4));
    }

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
    if (blake2b_init(&state->state, HASH_LEN) < 0)
        return -1;
    return 0;
}

int hash_state_update(struct hash_state *state, const unsigned char *data, size_t len)
{
    if (blake2b_update(&state->state, data, len) < 0)
        return -1;
    return 0;
}

int hash_state_final(struct hash *out, struct hash_state *state)
{
    unsigned char hash[HASH_LEN];
    if (blake2b_final(&state->state, hash, sizeof(hash)) < 0)
        return -1;
    return hash_from_bin(out, hash, sizeof(hash));
}

int store_init(const char *path)
{
    int storefd, versionfd;
    uint32_t version;
    struct stat st;

    if ((stat(path, &st)) == 0)
        die("Path exists already: %s", path);

    if (mkdir(path, 0777) < 0 || (storefd = open(path, O_RDONLY)) < 0)
        die_errno("Cannot create store directory: %s", path);

    if ((versionfd = openat(storefd, BLOCK_STORE_VERSION_FILE, O_CREAT|O_EXCL|O_WRONLY, 0666)) < 0)
        die_errno("Unable to initialize store version");

    version = htonl(BLOCK_STORE_VERSION);

    if (write_bytes(versionfd, (unsigned char *) &version, sizeof(version)) < 0)
        die_errno("Unable to write store version");

    if (try_close(versionfd) < 0 || try_close(storefd) < 0)
        die_errno("Unable to finalize creation");

    return 0;
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

    if ((versionfd = openat(storefd, BLOCK_STORE_VERSION_FILE, O_RDONLY)) < 0)
        die_errno("Could not open store's version file");

    if (read_bytes(versionfd, (unsigned char *) &version, sizeof(version)) != sizeof(version))
        die_errno("Unable to read store version");

    version = ntohl(version);
    if (version != BLOCK_STORE_VERSION)
        die_errno("Unable to open block store with version %"PRIu32, version);

    if (try_close(versionfd) < 0)
        die_errno("Unable to close block's version file");

    out->fd = storefd;
    for (i = 0; i < 256; i++)
        out->shardfds[i] = -1;

    return 0;
}

int store_close(struct store *store)
{
    int i;

    if (try_close(store->fd) < 0)
        return -1;

    for (i = 0; i < 256; i++)
        if (store->shardfds[i] >= 0 && try_close(store->shardfds[i]) < 0)
            return -1;

    return 0;
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
    char name[sizeof(hash.hex) + 5];

    if (hash_compute(&hash, data, datalen) < 0)
        die("Unable to hash block");

    if (snprintf(name, sizeof(name), "%s.tmp", hash.hex + 2) < 0)
        die("Unable to compute block name");

    if ((shardfd = open_shard(store, &hash, 1)) < 0)
        die("Unable to open shard");

    if ((fd = openat(shardfd, name, O_CREAT|O_EXCL|O_WRONLY, 0644)) < 0) {
        if (errno == EEXIST)
            goto out;
        die_errno("Unable to create block '%s'", hash.hex);
    }

    if (write_bytes(fd, data, datalen) < 0 || try_close(fd) < 0) {
        unlinkat(shardfd, name, 0);
        die_errno("Unable to write block '%s'", hash.hex);
    }

    if (renameat(shardfd, name, shardfd, hash.hex + 2) < 0) {
        unlinkat(shardfd, name, 0);
        die_errno("Unable to move temporary block '%s'", hash.hex);
    }

out:
    if (out)
        memcpy(out, &hash, sizeof(*out));

    return 0;
}

ssize_t store_read(unsigned char *out, size_t outlen, struct store *store, const struct hash *hash)
{
    int fd, shardfd;
    ssize_t len;

    if ((shardfd = open_shard(store, hash, 0)) < 0)
        die("Unable to open shard");

    if ((fd = openat(shardfd, hash->hex + 2, O_RDONLY)) < 0)
        die_errno("Unable to open block '%s'", hash->hex);

    if ((len = read_bytes(fd, out, outlen)) < 0)
        die_errno("Unable to read block '%s'", hash->hex);

    if (try_close(fd) < 0)
        die_errno("Unable to close block '%s'", hash->hex);

    return len;
}
