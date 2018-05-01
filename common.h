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

#include <sodium.h>

#include "config.h"

#define BLOCK_STORE_VERSION 1
#define BLOCK_STORE_VERSION_FILE "version"

#define MASTER_KEY_LEN     (crypto_kdf_KEYBYTES)
#define ENCRYPTION_KEY_LEN (crypto_aead_chacha20poly1305_KEYBYTES)
#define NONCE_KEY_LEN      (crypto_generichash_KEYBYTES)

#define NONCE_LEN     (crypto_aead_chacha20poly1305_NPUBBYTES)

#define PLAIN_BLOCK_LEN (BLOCK_LEN - crypto_aead_chacha20poly1305_ABYTES - NONCE_LEN)
#define PLAIN_META_LEN  (sizeof(uint32_t))
#define PLAIN_DATA_LEN  (PLAIN_BLOCK_LEN - PLAIN_META_LEN)

#define CIPHER_BLOCK_LEN (BLOCK_LEN)
#define CIPHER_DATA_LEN  (CIPHER_BLOCK_LEN - NONCE_LEN)

struct nonce_key {
    unsigned char data[NONCE_KEY_LEN];
};

struct encrypt_key {
    unsigned char data[ENCRYPTION_KEY_LEN];
};

struct hash {
    unsigned char bin[HASH_LEN];
    char hex[HASH_LEN * 2 + 1];
};

struct hash_state {
    crypto_generichash_state state;
};

void die(const char *fmt, ...);
void die_errno(const char *fmt, ...);
void warn(const char *fmt, ...);
void version(const char *executable);

ssize_t read_bytes(int fd, unsigned char *buf, size_t buflen);
int write_bytes(int fd, const unsigned char *buf, size_t buflen);

int hash_from_bin(struct hash *out, const unsigned char *data, size_t len);
int hash_from_str(struct hash *out, const char *str, size_t len);
int hash_eq(const struct hash *a, const struct hash *b);

int hash_compute(struct hash *out, const unsigned char *data, size_t len);
int hash_state_init(struct hash_state *state);
int hash_state_update(struct hash_state *state, const unsigned char *data, size_t len);
int hash_state_final(struct hash *out, struct hash_state *state);

int open_store(const char *path);
int open_block(int storefd, const char *hash, char create);

int read_keys(struct nonce_key *nout, struct encrypt_key *cout, const char *file);
