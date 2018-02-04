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

#include <unistd.h>

#include "common.h"

int read_bytes(int fd, char *buf, size_t buflen)
{
    size_t total = 0;

    while (total != buflen) {
        ssize_t bytes = read(fd, buf + total, buflen - total);
        if (bytes < 0)
            return -1;
        if (bytes == 0)
            return -1;
        total += bytes;
    }

    return 0;
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
