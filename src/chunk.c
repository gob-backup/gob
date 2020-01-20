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

#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>

int gob_chunk(int argc, const char *argv[])
{
    unsigned char *block = malloc(BLOCK_LEN);
    struct hash_state state;
    struct hash hash;
    struct store store;
    size_t total = 0;
    ssize_t bytes;

    if (argc != 2)
        die("USAGE: %s chunk <DIR>", argv[0]);

    atexit(close_stdout);

    if (store_open(&store, argv[1]) < 0)
        die("Unable to open store");

    if (hash_state_init(&state) < 0)
        die("Unable to initialize hashing state");

    while ((bytes = read_bytes(STDIN_FILENO, block, BLOCK_LEN)) > 0) {
        total += (size_t) bytes;

        if (hash_state_update(&state, block, (size_t) bytes) < 0)
            die("Unable to update hash");
        if (store_write(&hash, &store, block, (size_t) bytes) < 0)
            die("Unable to store block");
        puts(hash.hex);
    }

    if (bytes < 0)
        die_errno("Unable to read block");

    if (hash_state_final(&hash, &state) < 0)
        die("Unable to finalize hash");

    printf(">%s %"PRIuMAX"\n", hash.hex, total);

    if (store_close(&store) < 0)
        die("Unable to close store");

    free(block);

    return 0;
}
