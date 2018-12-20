/*
 * Copyright (C) 2018 Patrick Steinhardt
 *
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

#include "blake2/blake2.h"

#include "config.h"

#define BLOCK_STORE_VERSION 1
#define BLOCK_STORE_VERSION_FILE "version"

struct hash {
    unsigned char bin[HASH_LEN];
    char hex[HASH_LEN * 2 + 1];
};

struct hash_state {
    blake2b_state state;
};

struct store {
    int fd;
    int shardfds[256];
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

int store_open(struct store *out, const char *path);
void store_close(struct store *store);
int store_write(struct hash *out, struct store *store, const unsigned char *data, size_t datalen);
int store_read(unsigned char *out, size_t outlen, struct store *store, const struct hash *hash);