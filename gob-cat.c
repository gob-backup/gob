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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sodium.h>

#include "config.h"
#include "common.h"

static int parse_trailer(struct hash *hash_out, size_t *datalen_out, const char *trailer)
{
    if (*trailer != '>')
        die("Last line is not a trailer line");
    trailer++;

    if (strlen(trailer) < HASH_LEN * 2)
        die("Trailer is too short");

    if (hash_from_str(hash_out, trailer, HASH_LEN * 2) < 0)
        die("Unable to decode trailer hash");
    trailer += HASH_LEN * 2;

    if (*trailer != ' ')
        die("No separator between trailer hash and length");
    trailer++;

    if ((*datalen_out = strtol(trailer, NULL, 10)) == 0)
        die("Invalid data length in trailer");

    return 0;
}

int main(int argc, char *argv[])
{
    struct hash_state state;
    struct hash expected_hash, computed_hash;
    struct store store;
    unsigned char *block = malloc(BLOCK_LEN);
    char *line = NULL;
    ssize_t linelen;
    size_t total = 0, n = 0, expected_len;

    if (argc != 2)
        die("USAGE: %s ( --version | <DIR> )", argv[0]);

    if (!strcmp(argv[1], "--version"))
        version("gob-cat");

    if (store_open(&store, argv[1]) < 0)
        die("Unable to open store");

    if (hash_state_init(&state) < 0)
        die("Unable to initialize hashing state");

    while ((linelen = getline(&line, &n, stdin)) > 0) {
        struct hash hash;
        ssize_t blocklen;

        if (*line == '>')
            break;

        if (line[linelen - 1] == '\n')
            line[--linelen] = '\0';

        if (hash_from_str(&hash, line, 0) < 0)
            die("Invalid index hash '%s'", line);

        if ((blocklen = store_read(block, BLOCK_LEN, &store, &hash)) < 0)
            die_errno("Unable to open block '%s'", line);

        if (hash_state_update(&state, block, blocklen) < 0)
            die("Unable to update hash");

        if (write_bytes(STDOUT_FILENO, block, blocklen) < 0)
            die_errno("Unable to write block '%s'", line);

        total += blocklen;
    }

    if (linelen < 0 && !feof(stdin))
        die_errno("Unable to read index");

    if ((parse_trailer(&expected_hash, &expected_len, line)) < 0)
        die("Unable to read index");

    if (hash_state_final(&computed_hash, &state) < 0)
        die("Unable to finalize hash");

    if (total != expected_len)
        die("Size mismatch");

    if (!hash_eq(&computed_hash, &expected_hash))
        die("Hash mismatch");

    free(line);
    free(block);
    store_close(&store);

    return 0;
}
