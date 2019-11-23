/*
 * Copyright (C) 2019 Patrick Steinhardt
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

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(*x))

static struct {
    int (*fn)(int argc, const char *argv[]);
    const char *name;
    const char *description;
} commands[] = {
    { gob_cat,   "cat",   "Concatenate chunks" },
    { gob_chunk, "chunk", "Chunk and store data" },
    { gob_fsck,  "fsck",  "Check consistency of a store"  },
};

int main(int argc, const char *argv[])
{
    size_t i;

    if (argc <= 1) {
        fprintf(stderr, "USAGE: %s [--version] <COMMAND>\n\n", argv[0]);
        fprintf(stderr, "The following commands are available:\n\n");

        for (i = 0; i < ARRAY_SIZE(commands); i++) {
            fprintf(stderr, "\t%s\t%s\n", commands[i].name, commands[i].description);
        }

        return 1;
    } else if (!strcmp(argv[1], "--version")) {
        printf("%s version "GOB_VERSION"\n\n"
               "block size: %d\n"
               "hash size:  %d\n", argv[0], BLOCK_LEN, HASH_LEN);
        return 0;
    }

    for (i = 0; argc > 1 && i < ARRAY_SIZE(commands); i++) {
        if (strcmp(argv[1], commands[i].name))
            continue;
        memmove(argv + 1, argv + 2, sizeof(char *) * (unsigned) argc - 2);
        return commands[i].fn(argc - 1, argv);
    }

    if (i == ARRAY_SIZE(commands))
        fprintf(stderr, "%s: '%s' is not a known gob command\n", argv[0], argv[1]);

    return 1;
}
