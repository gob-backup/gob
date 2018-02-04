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

#include <stddef.h>

struct block {
    unsigned char data[BLOCK_LEN];
};

void die(const char *fmt, ...);
int read_bytes(int fd, char *buf, size_t buflen);
int write_bytes(int fd, const char *buf, size_t buflen);

int bin2hex(char *out, size_t outlen, const unsigned char *in, size_t inlen);
int hex2bin(unsigned char *out, size_t outlen, const char *in, size_t inlen);
