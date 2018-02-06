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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sodium.h>

#include "config.h"
#include "common.h"

#define MIN(x, y) ((x) <= (y) ? (x) : (y))

static const char *find_last_line(const char *string)
{
    const char *end = string + strlen(string) - 1;

    while (*end == '\n')
        end--;

    while (end[-1] != '\n' && end > string)
        end--;

    if (end == string)
        return NULL;

    return end;
}

static int parse_trailer(unsigned char *hash_out, size_t *datalen_out, const char *chain)
{
    char hash[HASH_LEN * 2];
    const char *trailer;
    int i;

    if ((trailer = find_last_line(chain)) == NULL)
        die("Invalid input without trailer");

    if (*trailer != '>')
        die("Last line is not a trailer line");
    trailer++;

    for (i = 0; i < (HASH_LEN * 2); i++) {
        char h = *trailer++;
        if (!strchr("0123456789abcdef", h))
            die("Invalid trailer hash");
        hash[i] = h;
    }

    if (*trailer != ' ')
        die("No separator between trailer hash and length");

    if ((*datalen_out = strtol(trailer, NULL, 10)) == 0)
        die("Invalid data length in trailer");

    if (hex2bin(hash_out, HASH_LEN, hash, sizeof(hash)) < 0)
        die("Unable to decode trailer hash");

    return 0;
}

static char *read_index(unsigned char *hash_out, size_t *size_out)
{
    unsigned char buf[1024];
    char *chain = NULL;
    ssize_t bytes;
    size_t total = 0;

    while ((bytes = read_bytes(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        chain = realloc(chain, total + bytes + 1);
        memcpy(chain + total, buf, bytes);
        total += bytes;
        chain[total] = '\0';
    }

    if (bytes < 0)
        die_errno("Unable to read from stdin");

    if (parse_trailer(hash_out, size_out, chain) < 0)
        die("Unable to parse trailer");

    return chain;
}

static int read_block(unsigned char *out, size_t outlen, int storefd, char *hash)
{
    int fd;

    if ((fd = open_block(storefd, hash, 0)) < 0)
        die_errno("Unable to open block '%s'", hash);

    if (read_bytes(fd, out, outlen) != (ssize_t) outlen)
        die_errno("Unable to read block '%s'", hash);

    close(fd);
    return 0;
}

int main(int argc, char *argv[])
{
    crypto_generichash_state *state = malloc(crypto_generichash_statebytes());
    unsigned char trailer_hash[HASH_LEN], computed_hash[HASH_LEN];
    char *chain = NULL, *haystack, *hash;
    size_t data_len;
    int storefd;

    if (argc != 2)
        die("USAGE: %s <DIR>", argv[0]);

    if ((storefd = open(argv[1], O_DIRECTORY)) < 0)
        die_errno("Unable to open storage '%s'", argv[1]);

    if ((chain = read_index(trailer_hash, &data_len)) == NULL)
        die("Unable to read index");

    if (crypto_generichash_init(state, NULL, 0, HASH_LEN) < 0)
        die("Unable to initialize hashing state");

    haystack = chain;
    while ((hash = strtok(haystack, "\n")) != NULL) {
        unsigned char line_hash[HASH_LEN];
        unsigned char block[BLOCK_LEN];
        size_t blocklen = MIN(data_len, BLOCK_LEN);

        if (*hash == '>')
            break;

        if (data_len == 0)
            die("More lines, but all data read");

        if (hex2bin(line_hash, sizeof(line_hash), hash, strlen(hash)) < 0)
            die("Unable to decode hash");

        if (read_block(block, BLOCK_LEN, storefd, hash) < 0)
            die_errno("Unable to read block '%s'", hash);

        if (crypto_generichash_update(state, block, BLOCK_LEN) < 0)
            die("Unable to update hash");

        if (write_bytes(STDOUT_FILENO, block, blocklen) < 0)
            die_errno("Unable to write block '%s'", hash);

        data_len -= blocklen;
        haystack = NULL;
    }

    if (data_len)
        die("Premature end of chain");

    if (crypto_generichash_final(state, computed_hash, sizeof(computed_hash)) < 0)
        die("Unable to finalize hash");

    if (memcmp(computed_hash, trailer_hash, sizeof(computed_hash)))
        die("Trailer hash does not match computed hash");

    free(state);

    close(storefd);

    return 0;
}
