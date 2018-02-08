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
#include <sys/types.h>

#include "config.h"

#define MASTER_KEY_LEN     (crypto_kdf_KEYBYTES)
#define ENCRYPTION_KEY_LEN (crypto_aead_chacha20poly1305_KEYBYTES)
#define NONCE_KEY_LEN      (crypto_generichash_KEYBYTES)

#define NONCE_LEN     (crypto_aead_chacha20poly1305_NPUBBYTES)

#define PLAIN_BLOCK_LEN (BLOCK_LEN - crypto_aead_chacha20poly1305_ABYTES - NONCE_LEN)
#define PLAIN_META_LEN  (sizeof(uint32_t))
#define PLAIN_DATA_LEN  (PLAIN_BLOCK_LEN - PLAIN_META_LEN)

#define CIPHER_BLOCK_LEN (BLOCK_LEN)
#define CIPHER_DATA_LEN  (CIPHER_BLOCK_LEN - NONCE_LEN)

void die(const char *fmt, ...);
void die_errno(const char *fmt, ...);

ssize_t read_bytes(int fd, unsigned char *buf, size_t buflen);
int write_bytes(int fd, const unsigned char *buf, size_t buflen);

int bin2hex(char *out, size_t outlen, const unsigned char *in, size_t inlen);
int hex2bin(unsigned char *out, size_t outlen, const char *in, size_t inlen);

int open_block(int storefd, const char *hash, char create);

int read_key(unsigned char *key, size_t keysize, const char *file);
