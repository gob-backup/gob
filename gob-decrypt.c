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

#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <sodium.h>

#include "config.h"
#include "common.h"

int main(int argc, char *argv[])
{
    unsigned char *cipher = malloc(CIPHER_BLOCK_LEN);
    unsigned char *plain = malloc(PLAIN_BLOCK_LEN);
    struct encrypt_key enckey;
    ssize_t bytes;

    if (argc != 2)
        die("USAGE: %s ( --version | <KEYFILE> )", argv[0]);

    if (!strcmp(argv[1], "--version"))
        version("gob-decrypt");

    if (sodium_init() < 0)
        die("Unable to initialize libsodium");

    if (read_keys(NULL, &enckey, argv[1]) < 0)
        die("Unable to read keyfile '%s'", argv[1]);

    while ((bytes = read_bytes(STDIN_FILENO, cipher, CIPHER_BLOCK_LEN)) == CIPHER_BLOCK_LEN) {
        uint32_t plainlen;

        if (crypto_aead_chacha20poly1305_decrypt(plain, NULL, NULL,
                cipher + NONCE_LEN, CIPHER_DATA_LEN, NULL, 0, cipher, enckey.data) < 0)
            die("Unable to decrypt plaintext");

        plainlen = ntohl(*(uint32_t *) plain);
        if (plainlen > PLAIN_DATA_LEN)
            die("Invalid encoded length of %"PRIu32, plainlen);

        if (write_bytes(STDOUT_FILENO, plain + sizeof(uint32_t), plainlen) < 0)
            die_errno("Unable to write ciphertext to stdout");
    }

    if (bytes > 0)
        die("Block too short");
    if (bytes < 0)
        die_errno("Unable to read from stdin");

    return 0;
}
