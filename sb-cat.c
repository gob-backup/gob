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

#include <sodium.h>

#include "config.h"

static void die(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    exit(1);
}

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

    for (i = 0; i <= (HASH_LEN * 2); i++) {
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

int main(int argc, char *argv[])
{
    unsigned char hash[HASH_LEN];
    char *chain = NULL;
    size_t total = 0, data_len;

    if (argc != 2)
        die("USAGE: %s <DIR>\n", argv[0]);

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

    if (parse_trailer(hash, &data_len, chain) < 0)
        die("Unable to parse trailer\n");

    return 0;
}
