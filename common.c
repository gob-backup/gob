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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sodium.h>

#include "common.h"
#include "config.h"

void die(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    exit(1);
}

int read_bytes(int fd, char *buf, size_t buflen)
{
    size_t total = 0;

    while (total != buflen) {
        ssize_t bytes = read(fd, buf + total, buflen - total);
        if (bytes < 0)
            return -1;
        if (bytes == 0)
            return total;
        total += bytes;
    }

    return buflen;
}

int write_bytes(int fd, const char *buf, size_t buflen)
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
    if (inlen != HASH_LEN)
        return -1;
    if (outlen < HASH_LEN * 2 + 1)
        return -1;
    sodium_bin2hex(out, outlen, in, inlen);
    return 0;
}

int hex2bin(unsigned char *out, size_t outlen, const char *in, size_t inlen)
{
    size_t parsed_len;

    if (outlen != HASH_LEN)
        return -1;
    if (inlen != HASH_LEN * 2)
        return -1;

    if (sodium_hex2bin(out, outlen, in, inlen, NULL, &parsed_len, NULL) < 0)
        return -1;

    if (parsed_len != HASH_LEN)
        return -1;

    return 0;
}
