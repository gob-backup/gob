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

int bin2hex(char *out, size_t outlen, const unsigned char *in, size_t inlen)
{
    if (outlen < HASH_LEN * 2 + 1)
        return -1;
    sodium_bin2hex(out, outlen, in, inlen);
    return 0;
}

int hex2bin(unsigned char *out, size_t outlen, const char *in, size_t inlen)
{
    size_t parsed_len;

    if (outlen < (inlen / 2))
        return -1;

    if (sodium_hex2bin(out, outlen, in, inlen, NULL, &parsed_len, NULL) < 0)
        return -1;

    if (parsed_len != outlen)
        return -1;

    return 0;
}

int open_block(int storefd, const char *hash, char create)
{
    struct stat st;
    char shard[3];
    int fd, shardfd;

    shard[0] = hash[0];
    shard[1] = hash[1];
    shard[2] = '\0';

    if ((shardfd = openat(storefd, shard, O_RDONLY)) < 0) {
        if (create) {
            if (mkdirat(storefd, shard, 0755) < 0)
                die_errno("Unable to create sharding directory '%s'", shard);
            if ((shardfd = openat(storefd, shard, O_RDONLY)) < 0)
                die_errno("Unable to open sharding directory '%s'", shard);
        } else {
            die_errno("Unable to open sharding directory '%s'", shard);
        }
    }

    if (fstat(storefd, &st) < 0 || !S_ISDIR(st.st_mode))
        die("Storage is not a directory");

    if (create) {
        fd = openat(shardfd, hash + 2, O_CREAT|O_EXCL|O_WRONLY, 0644);
        if (fd < 0 && errno != EEXIST)
            die_errno("Unable to create block '%s'", hash);
    } else if ((fd = openat(shardfd, hash + 2, O_RDONLY)) < 0) {
            die_errno("Unable to open block '%s'", hash);
    }

    close(shardfd);

    return fd;
}

int read_keys(struct nonce_key *nout, struct encrypt_key *cout, const char *file)
{
    unsigned char masterkey[MASTER_KEY_LEN];
    char masterkey_hex[MASTER_KEY_LEN * 2];
    ssize_t bytes;
    int fd;

    if ((fd = open(file, O_RDONLY)) < 0)
        die_errno("Unable to open keyfile '%s'", file);
    if ((bytes = read_bytes(fd, (unsigned char *) masterkey_hex, sizeof(masterkey_hex))) < 0)
        die_errno("Unable to read keyfile '%s'", file);
    if (bytes != MASTER_KEY_LEN * 2)
        die("Invalid key length: expected %"PRIuMAX", got %"PRIuMAX, MASTER_KEY_LEN * 2, bytes);
    if (hex2bin(masterkey, sizeof(masterkey), masterkey_hex, sizeof(masterkey_hex)) < 0)
        die("Unable to convert key to hex");

    if (cout && crypto_kdf_derive_from_key(cout->data, sizeof(cout->data), 1, "gobcrypt", masterkey) < 0)
        die("Unable do derive encryption key");

    if (nout && crypto_kdf_derive_from_key(nout->data, sizeof(nout->data), 2, "gobnonce", masterkey) < 0)
        die("Unable do derive nonce key");

    close(fd);

    return 0;
}
