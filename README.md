gob - block-based backups
=========================

[![Travis Build Status](https://secure.travis-ci.org/gob-backup/gob.svg?branch=master)](http://travis-ci.org/gob-backup/gob)
[![Coverity Status](https://scan.coverity.com/projects/15683/badge.svg)](https://scan.coverity.com/projects/gob-backup-gob)

`gob` is a block-based backup utility in pure C. It tries to be
as simple as possible while doing everything necessary to create
deduplicated backups. To do so, it strictly follows the Unix
principle of doing one thing per binary.

The main idea of gob is based on the notion of block storages and
indices. The block storage is using a content-adressable file
system inspired by git. gob will read blocks up to a specified
maximum block size, hash this block and then put it into the
block storage using the hash as name. So when reading the same
block twice, it will only get stored once.

The block storage is only the first part, as the restoration
process needs to be able to read blocks in the correct order,
again. To do so, the chunking program will output a list of
hashes of the blocks read, which is called an "index". Using this
list, it becomes trivial to restore the complete data by simply
concatenating all blocks in the order given by the index.

gob provides optional support for encryption by using the
encryption and decryption filters. These can be used to first
encrypt all data previous to it being passed to the chunking
algorithm.

Quick Start
-----------

A typical invocation of gob for the non-encrypted use-case might
look as follows:

    $ cat /dev/sda1 | gob-chunk /var/backups/blocks >index

This will create a complete backup of "/dev/sda1" and store all
blocks at "/var/backups/blocks". Furthermore, the index written
to terminal by `gob-chunk` gets redirected to the "index" file.

Restoring would then look as follows:

    $ cat index | gob-cat /var/backups/blocks >/dev/sda1

This will pipe the index into `gob-cat`, which will read the
necessary blocks in order from "/var/backups/blocks". The restored
data is then written to "/dev/sda1"

For optional encryption, you first need to generate a key. This
is a symmetric key, and as such it is used for encryption and
decryption. Make sure to never store that key alongside with your
backups!

    $ gob-keygen keyfile
    $ cat /dev/sda1 |
        gob-encrypt keyfile |
        gob-chunk /var/backups/blocks >index

As you can see, the only difference is adding the `gob-encrypt`
filter in between to handle encryption of your files. Decryption
looks similar:

    $ cat index |
        gob-decrypt keyfile |
        gob-cat /var/backups/blocks >/dev/sda1

Building
--------

gob is written in strict C90, so it should just work with your
regular C compiler. The only dependency required is libsodium,
which is used for the encryption part. The build process itself
makes use of a simple Makefile and `pkg-config` for locationg
libsodium. In case these prerequisites are available, the
following command should suffice to fully build gob:

    make

Testing
-------

To execute the tests, you can invoke the "test" target, which
will automatically build gob and then execute all tests.

    make test

License
-------

gob is under licensed under GPLv3. This means that you can use
the program as you wish, without any restrictions. However, if
you modify gob itself, you must distribute your modified sources
alongside the program.

See the [COPYING file](COPYING) for the full license text.
