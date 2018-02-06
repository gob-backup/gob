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

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <arpa/inet.h>

#include <sodium.h>

#include "config.h"
#include "common.h"

static int store_block(int storefd, unsigned char *block, size_t blocklen)
{
    unsigned char hash[HASH_LEN];
    char hex[HASH_LEN * 2 + 1];
    int fd;

    if (crypto_generichash(hash, sizeof(hash), block, blocklen, NULL, 0) < 0)
        die("Unable to hash block");
    if (bin2hex(hex, sizeof(hex), hash, sizeof(hash)) < 0)
        die("Unable to convert binary to hex");

    if ((fd = open_block(storefd, hex, 1)) >= 0) {
        if (write_bytes(fd, block, blocklen) < 0)
            die_errno("Unable to write block '%s'", hex);
        close(fd);
    }

    puts(hex);

    return 0;
}

int main(int argc, char *argv[])
{
    crypto_generichash_state *state = malloc(crypto_generichash_statebytes());
    unsigned char block[BLOCK_LEN];
    unsigned char hash[HASH_LEN];
    char hex[HASH_LEN * 2 + 1];
    size_t total = 0;
    ssize_t bytes;
    int storefd;

    if (argc != 2)
        die("USAGE: %s <DIR>", argv[0]);

    if ((storefd = open(argv[1], O_DIRECTORY)) < 0)
        die_errno("Unable to open storage '%s'", argv[1]);

    if (crypto_generichash_init(state, NULL, 0, HASH_LEN) < 0)
        die("Unable to initialize hashing state");

    while ((bytes = read_bytes(STDIN_FILENO, block, BLOCK_LEN)) > 0) {
        total += bytes;

        if (crypto_generichash_update(state, block, BLOCK_LEN) < 0)
            die("Unable to update hash");
        if (store_block(storefd, block, BLOCK_LEN) < 0)
            die("Unable to store block");
    }

    if (bytes < 0)
        die_errno("Unable to read block");

    if (crypto_generichash_final(state, hash, sizeof(hash)) < 0)
        die("Unable to finalize hash");

    if (bin2hex(hex, sizeof(hex), hash, sizeof(hash)) < 0)
        die("Unable to convert binary to hex");

    printf(">%s %"PRIuMAX"\n", hex, total);
    free(state);
    close(storefd);

    return 0;
}
