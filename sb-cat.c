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
#include <stdio.h>
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
    size_t bin_len;
    int i;

    if ((trailer = find_last_line(chain)) == NULL)
        die("Invalid input without trailer\n");

    if (*trailer != '>')
        die("Last line is not a trailer line\n");
    trailer++;

    for (i = 0; i < (HASH_LEN * 2); i++) {
        char h = *trailer++;
        if (!strchr("0123456789abcdef", h))
            die("Invalid trailer hash\n");
        hash[i] = h;
    }

    if (*trailer != ' ')
        die("No separator between trailer hash and length\n");

    if ((*datalen_out = strtol(trailer, NULL, 10)) == 0)
        die("Invalid data length in trailer\n");

    if (sodium_hex2bin(hash_out, HASH_LEN, hash, sizeof(hash), NULL, &bin_len, NULL) < 0)
        die("Unable to decode trailer hash\n");
    if (bin_len != HASH_LEN)
        die("Trailer hash is too short\n");

    return 0;
}

static int read_block(int dirfd, char *hash, size_t len)
{
    char shard[3];
    int fd, shardfd;

    shard[0] = hash[0];
    shard[1] = hash[1];
    shard[2] = '\0';

    if ((shardfd = openat(dirfd, shard, O_DIRECTORY)) < 0)
        die("Unable to open sharding directory '%s': %s\n", shard, strerror(errno));

    if ((fd = openat(shardfd, hash + 2, O_RDONLY)) < 0)
        die("Unable to open block '%s': %s\n", hash, strerror(errno));

    while (len) {
        char block[BLOCK_LEN];
        ssize_t blocklen = MIN(BLOCK_LEN, len);

        if (read_bytes(fd, block, blocklen) != blocklen)
            die("Unable to read block '%s': %s\n", hash, strerror(errno));
        if (write_bytes(STDOUT_FILENO, block, MIN(BLOCK_LEN, len)) < 0)
            die("Unable to write block '%s': %s\n", hash, strerror(errno));

        len -= blocklen;
    }

    close(shardfd);
    close(fd);
    return 0;
}

int main(int argc, char *argv[])
{
    unsigned char trailer_hash[HASH_LEN];
    char *chain = NULL, *haystack, *line;
    size_t total = 0, data_len;
    int dirfd;

    if (argc != 2)
        die("USAGE: %s <DIR>\n", argv[0]);

    if ((dirfd = open(argv[1], O_DIRECTORY)) < 0)
        die("Unable to open storage '%s': %s\n", argv[1], strerror(errno));

    while (1) {
        char buf[1024];
        ssize_t bytes;

        bytes = read(STDIN_FILENO, buf, sizeof(buf));
        if (bytes < 0)
            die("Unable to read from stdin: %s", strerror(errno));
        if (bytes == 0)
            break;

        chain = realloc(chain, total + bytes + 1);
        memcpy(chain + total, buf, bytes);
        total += bytes;
        chain[total] = '\0';
    }

    if (parse_trailer(trailer_hash, &data_len, chain) < 0)
        die("Unable to parse trailer\n");

    haystack = chain;
    while ((line = strtok(haystack, "\n")) != NULL) {
        unsigned char line_hash[HASH_LEN];
        size_t line_hash_len;

        if (*line == '>')
            break;

        if (data_len == 0)
            die("More lines, but all data read\n");

        if (sodium_hex2bin(line_hash, HASH_LEN, line, strlen(line), NULL, &line_hash_len, NULL) < 0)
            die("Unable to decode line hash\n");
        if (line_hash_len != HASH_LEN)
            die("Trailer hash is too short\n");

        if (read_block(dirfd, line, MIN(data_len, BLOCK_LEN)) < 0)
            die("Unable to read block '%s': %s", line, strerror(errno));
        data_len -= data_len % BLOCK_LEN;
        haystack = NULL;
    }

    return 0;
}
