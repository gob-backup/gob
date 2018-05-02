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
#include <sys/types.h>

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <arpa/inet.h>

#include "config.h"
#include "common.h"

int main(int argc, char *argv[])
{
    unsigned char *block = malloc(BLOCK_LEN);
    struct hash_state state;
    struct hash hash;
    size_t total = 0;
    ssize_t bytes;
    int storefd;

    if (argc != 2)
        die("USAGE: %s ( --version | <DIR> )", argv[0]);

    if (!strcmp(argv[1], "--version"))
        version("gob-chunk");

    if ((storefd = open_store(argv[1])) < 0)
        die("Unable to open store");

    if (hash_state_init(&state) < 0)
        die("Unable to initialize hashing state");

    while ((bytes = read_bytes(STDIN_FILENO, block, BLOCK_LEN)) > 0) {
        struct hash hash;

        total += bytes;

        if (hash_state_update(&state, block, bytes) < 0)
            die("Unable to update hash");
        if (write_block(&hash, storefd, block, bytes) < 0)
            die("Unable to store block");
        puts(hash.hex);
    }

    if (bytes < 0)
        die_errno("Unable to read block");

    if (hash_state_final(&hash, &state) < 0)
        die("Unable to finalize hash");

    printf(">%s %"PRIuMAX"\n", hash.hex, total);

    free(block);
    close(storefd);

    return 0;
}
