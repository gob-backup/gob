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

#include "common.h"

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define HEXCHARS "0123456789abcdef"

static unsigned char *block;

static int scan_shard(int storefd, const char *shard)
{
    struct hash computed_hash, expected_hash;
    struct dirent *ent;
    char filehash[HASH_LEN * 2 + 1];
    DIR *sharddir = NULL;
    int shardfd = -1, err = 0;

    if ((shardfd = openat(storefd, shard, O_RDONLY)) < 0) {
        warn("Unable to open shard");
        err = -1;
        goto out;
    }

    if ((sharddir = fdopendir(shardfd)) == NULL) {
        warn("Unable to open shard directory %s", shard);
        err = -1;
        goto out;
    }

    while ((ent = readdir(sharddir)) != NULL) {
        int bytes, blockfd = -1;
        struct stat stat;

        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
            continue;

        if (fstatat(shardfd, ent->d_name, &stat, 0) < 0) {
            warn("unable to stat '%s/%s'", shard, ent->d_name);
            err = -1;
            goto next;
        }

        if (!S_ISREG(stat.st_mode)) {
            warn("invalid entry '%s/%s'", shard, ent->d_name);
            err = -1;
            goto next;
        }

        if (strlen(ent->d_name) != (HASH_LEN * 2 - 2) ||
                strspn(ent->d_name, HEXCHARS) != (HASH_LEN * 2 - 2)) {
            warn("invalid entry name '%s/%s' %lu %d", shard, ent->d_name, strspn(ent->d_name, HEXCHARS), HASH_LEN * 2 - 2);
            err = -1;
            goto next;
        }

        if ((blockfd = openat(shardfd, ent->d_name, O_RDONLY)) < 0) {
            warn("unable to open block");
            err = -1;
            goto next;
        }

        if ((bytes = read_bytes(blockfd, block, BLOCK_LEN)) < 0) {
            warn("unable to read block");
            err = -1;
            goto next;
        }

        if (hash_compute(&computed_hash, block, bytes) < 0) {
            warn("Unable to hash block");
            err = -1;
            goto next;
        }

        if (snprintf(filehash, sizeof(filehash), "%s%s",
                    shard, ent->d_name) != HASH_LEN * 2 ||
            hash_from_str(&expected_hash, filehash, sizeof(filehash) - 1) < 0)
        {
            warn("File name is not a valid hash");
            err = -1;
            goto next;
        }

        if (!hash_eq(&computed_hash, &expected_hash)) {
            warn("Hash mismatch for block %s%s", shard, ent->d_name);
            err = -1;
            goto next;
        }

next:
        if (blockfd >= 0)
            close(blockfd);
    }

out:
    if (sharddir) {
        closedir(sharddir);
    } else if (shardfd >= 0) {
        close(shardfd);
    }
    return err;
}

int main(int argc, char *argv[])
{
    struct store store;
    struct dirent *ent;
    DIR *storedir;
    int err = 0;

    if (argc != 2)
        die("USAGE: %s ( --version | <DIR> )", argv[0]);

    atexit(close_stdout);

    if (!strcmp(argv[1], "--version"))
        version("gob-fsck");

    if ((block = malloc(BLOCK_LEN)) == NULL)
        die_errno("Unable to allocate block");

    if (store_open(&store, argv[1]) < 0)
        die_errno("Unable to open store");

    if ((storedir = fdopendir(store.fd)) == NULL)
        die_errno("Unable to open store directory");

    while ((ent = readdir(storedir)) != NULL) {
        struct stat stat;

        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..") || !strcmp(ent->d_name, "version"))
            continue;

        if (fstatat(store.fd, ent->d_name, &stat, 0) < 0) {
            warn("unable to stat shard '%s'", ent->d_name);
            err = -1;
            continue;
        }

        if (!S_ISDIR(stat.st_mode)) {
            warn("invalid shard '%s/%s'", argv[1], ent->d_name);
            err = -1;
            continue;
        }

        if (strlen(ent->d_name) != 2 ||
            !strchr(HEXCHARS, ent->d_name[0]) ||
            !strchr(HEXCHARS, ent->d_name[1]))
        {
            warn("invalid sharding directory '%s/%s'", argv[1], ent->d_name);
            err = -1;
            continue;
        }

        if (scan_shard(store.fd, ent->d_name) < 0) {
            warn("invalid sharding directory '%s/%s'", argv[1], ent->d_name);
            err = -1;
            continue;
        }
    }

    free(block);
    closedir(storedir);
    store_close(&store);

    return err;
}
