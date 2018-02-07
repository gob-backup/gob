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
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <sodium.h>

#include "config.h"
#include "common.h"

#define PLAIN_LEN (BLOCK_LEN - crypto_aead_chacha20poly1305_ABYTES)

int main(int argc, char *argv[])
{
    unsigned char key[crypto_aead_chacha20poly1305_KEYBYTES];
    unsigned char nonce[crypto_aead_chacha20poly1305_NPUBBYTES];
    unsigned char *plain = malloc(PLAIN_LEN);
    unsigned char *cipher = malloc(BLOCK_LEN);
    ssize_t plainlen;

    if (argc < 2)
        die("USAGE: %s <KEYFILE>", argv[0]);

    if (sodium_init() < 0)
        die("Unable to initialize libsodium");

    if (read_key(key, sizeof(key), argv[1]) < 0)
        die("Unable to read keyfile '%s'", argv[1]);

    memset(nonce, 0, sizeof(nonce));

    while ((plainlen = read_bytes(STDIN_FILENO, plain + sizeof(uint32_t),
                    PLAIN_LEN - sizeof(uint32_t))) > 0)
    {
        *(uint32_t *) plain = htonl(plainlen);
        memset(plain + sizeof(uint32_t) + plainlen, 0, PLAIN_LEN - sizeof(uint32_t) - plainlen);

        if (crypto_aead_chacha20poly1305_encrypt(cipher, NULL,
                plain, PLAIN_LEN, NULL, 0, NULL, nonce, key) < 0)
            die("Unable to encrypt plaintext");

        if (write_bytes(STDOUT_FILENO, cipher, BLOCK_LEN) < 0)
            die_errno("Unable to write ciphertext to stdout");

        increment(nonce, sizeof(nonce));
    }

    if (plainlen < 0)
        die_errno("Unable to read from stdin");

    free(plain);
    free(cipher);

    return 0;
}
