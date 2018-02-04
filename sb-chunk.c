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

#include <sys/stat.h>

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <arpa/inet.h>

#include <sodium.h>

#define BLOCK_LEN 4096
#define HASH_LEN  16

struct block {
    unsigned char data[BLOCK_LEN];
};

struct block_storage {
    const char *path;
    int pathfd;
};

static void die(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    exit(1);
}

static ssize_t read_block(struct block *out, int fd)
{
    size_t total = 0;

    while (total < sizeof(out->data)) {
        ssize_t bytes = read(fd, out->data + total, sizeof(out->data) - total);
        if (bytes < 0)
            die("Failed reading block: %s", strerror(errno));
        if (bytes == 0)
            break;

        total += bytes;
    }

    memset(out->data + total, 0, sizeof(out->data) - total);

    return total;
}

static int store_block(const struct block_storage *storage, const struct block *block)
{
    unsigned char hash[HASH_LEN];
    char hex[HASH_LEN * 2 + 1], shard[3];
    int dirfd, fd;

    if (crypto_generichash(hash, sizeof(hash), block->data, sizeof(block->data), NULL, 0) < 0)
        die("Unable to hash block");
    sodium_bin2hex(hex, sizeof(hex), hash, sizeof(hash));

    shard[0] = hex[0];
    shard[1] = hex[1];
    shard[2] = '\0';

    if ((dirfd = openat(storage->pathfd, shard, O_DIRECTORY)) < 0) {
        if (mkdirat(storage->pathfd, shard, 0755) < 0)
            die("Unable to create sharding directory '%s': %s\n", shard, strerror(errno));
        if ((dirfd = openat(storage->pathfd, shard, O_DIRECTORY)) < 0)
            die("Unable to open sharding directory '%s': %s\n", shard, strerror(errno));
    }

    if ((fd = openat(dirfd, hex + 2, O_CREAT|O_EXCL|O_WRONLY, 0644)) < 0 && errno != EEXIST)
        die("Unable to create block '%s': %s\n", hex, strerror(errno));

    if (fd >= 0) {
        size_t written = 0;
        ssize_t bytes;

        while (written != sizeof(block->data)) {
            if ((bytes = write(fd, block->data + written, sizeof(block->data) - written)) < 0)
                die("Unable to write block '%s': %s\n", hex, strerror(errno));
            written += bytes;
        }
    }

    puts(hex);

    close(fd);
    close(dirfd);

    return 0;
}

int main(int argc, char *argv[])
{
    struct block_storage storage;
    crypto_generichash_state *state = malloc(crypto_generichash_statebytes());
    unsigned char hash[HASH_LEN];
    char hex[HASH_LEN * 2 + 1];
    size_t total = 0;

    if (argc != 2)
        die("USAGE: %s <DIR>\n", argv[0]);

    storage.path = argv[1];
    if ((storage.pathfd = open(storage.path, O_DIRECTORY)) < 0)
        die("Unable to open storage '%s': %s\n", storage.path, strerror(errno));

    if (crypto_generichash_init(state, NULL, 0, HASH_LEN) < 0)
        die("Unable to initialize hashing state");

    while (1) {
        struct block block;
        ssize_t bytes;

        if ((bytes = read_block(&block, STDIN_FILENO)) < 0)
            return -1;
        if (bytes == 0)
            break;
        total += bytes;

        if (crypto_generichash_update(state, block.data, sizeof(block.data)) < 0)
            die("Unable to update hash");

        if (store_block(&storage, &block) < 0)
            die("Unable to store block");
    }

    if (crypto_generichash_final(state, hash, sizeof(hash)) < 0)
        die("Unable to finalize hash");

    sodium_bin2hex(hex, sizeof(hex), hash, sizeof(hash));
    printf(">%s %"PRIuMAX"\n", hex, total);
    free(state);

    return 0;
}
