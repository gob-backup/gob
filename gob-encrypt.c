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
    unsigned char *plain = malloc(PLAIN_BLOCK_LEN);
    unsigned char *cipher = malloc(CIPHER_BLOCK_LEN);
    struct encrypt_key enckey;
    struct nonce_key noncekey;
    uint32_t cnt = 0;
    ssize_t bytes;

    if (argc < 2)
        die("USAGE: %s <KEYFILE>", argv[0]);

    if (sodium_init() < 0)
        die("Unable to initialize libsodium");

    if (read_keys(&noncekey, &enckey, argv[1]) < 0)
        die("Unable to read keyfile '%s'", argv[1]);

    while ((bytes = read_bytes(STDIN_FILENO, plain + sizeof(uint32_t),
                PLAIN_DATA_LEN)) > 0)
    {
        crypto_generichash_state state;
        size_t cipherlen;
        uint32_t ncnt = htonl(cnt);

        *(uint32_t *) plain = htonl(bytes);
        memset(plain + bytes + PLAIN_META_LEN, 0, PLAIN_DATA_LEN - bytes);

        if (crypto_generichash_init(&state, noncekey.data, sizeof(noncekey.data), NONCE_LEN) < 0 ||
                crypto_generichash_update(&state, (unsigned char *) &ncnt, sizeof(ncnt)) < 0 ||
                crypto_generichash_update(&state, plain, bytes) < 0 ||
                crypto_generichash_final(&state, cipher, NONCE_LEN))
            die("Unable to derive nonce");

        if (crypto_aead_chacha20poly1305_encrypt(cipher + NONCE_LEN, (void *) &cipherlen,
                plain, PLAIN_BLOCK_LEN, NULL, 0, NULL, cipher, enckey.data) < 0)
            die("Unable to encrypt plaintext");

        if (cipherlen + NONCE_LEN != CIPHER_BLOCK_LEN)
            die("Encryption resulted in invalid block length: expected %"PRIuMAX", got %"PRIuMAX,
                    CIPHER_BLOCK_LEN - NONCE_LEN, cipherlen);

        if (write_bytes(STDOUT_FILENO, cipher, CIPHER_BLOCK_LEN) < 0)
            die_errno("Unable to write ciphertext to stdout");

        if ((++cnt) == 0)
            die("Overflow in counter");
    }

    if (bytes < 0)
        die_errno("Unable to read from stdin");

    free(plain);
    free(cipher);

    return 0;
}
