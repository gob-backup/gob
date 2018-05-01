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

#include <string.h>
#include <fcntl.h>

#include <sodium.h>

#include "config.h"
#include "common.h"

int main(int argc, char *argv[])
{
    unsigned char key[crypto_kdf_KEYBYTES];
    char hex[sizeof(key) * 2 + 1];
    int fd;

    if (argc != 2)
        die("USAGE: %s ( --version | <KEYFILE> )", argv[0]);

    if (!strcmp(argv[1], "--version"))
        version("gob-keygen");

    if (sodium_init() < 0)
        die("Unable to initialize libsodium");

    crypto_kdf_keygen(key);
    sodium_bin2hex(hex, sizeof(hex), key, sizeof(key));

    if ((fd = open(argv[1], O_EXCL|O_CREAT|O_RDWR, 0600)) < 0)
        die_errno("Unable to create keyfile '%s'", argv[1]);

    if (write_bytes(fd, (unsigned char *) hex, strlen(hex)) < 0)
        die_errno("Unable to write keyfile '%s'", argv[1]);

    return 0;
}
