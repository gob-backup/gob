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

int gob_init(int argc, const char *argv[])
{
    if (argc != 2)
        die("USAGE: %s init <DIR>", argv[0]);

    atexit(close_stdout);

    if (store_init(argv[1]) < 0)
        die("Unable to initialize store");

    return 0;
}
