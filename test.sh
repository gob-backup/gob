#!/bin/sh
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

PATH="$(pwd):${PATH}"
TEST_NUM=0
TEST_DIR=$(mktemp -d /tmp/gob-tests-XXXXXXXX)

assert_equal() {
	cmp "$1" "$2"
}

assert_success() {
	eval "$@"
}

assert_failure() {
	eval "$@"
	test $? -eq 1
}

test_expect_success() {
	(
		cd "$TEST_DIR"
		eval "$2"
	) >/dev/null 2>&1

	if test $? -eq 0
	then
		echo "ok $TEST_NUM $1"
	else
		echo "failed $TEST_NUM $1"
	fi

	TEST_NUM=$(($TEST_NUM + 1))
}

test_expect_success 'keygen without keyfile fails' '
	assert_failure gob-keygen
'

test_expect_success 'keygen generates key' '
	assert_success gob-keygen key &&
	assert_success test -f key
'

test_expect_success 'keygen avoids overwriting existing key' '
	assert_failure gob-keygen key
'

test_expect_success 'generate a deterministic key' '
	echo -n 8f1cce617550ebb227730bbf5d67a56a0692df2002dd29ea0629604400ebeb77 >key
'

test_expect_success 'encryption generates fixed blocksize' '
	assert_success "echo test | gob-encrypt key | wc -c >actual" &&
	echo $((4096 * 1024)) >expected &&
	assert_equal actual expected
'

test_expect_success 'encryption generates multiples of blocksize' '
	assert_success "dd if=/dev/zero bs=$((4096 * 1025)) count=1 | gob-encrypt key | wc -c >actual" &&
	echo $((4096 * 1024 * 2)) >expected &&
	assert_equal actual expected
'

test_expect_success 'key generates deterministic sequence' '
	assert_success "dd if=/dev/zero bs=$((4096 * 1025)) count=1 | gob-encrypt key >expected" &&
	assert_success "dd if=/dev/zero bs=$((4096 * 1025)) count=1 | gob-encrypt key >actual" &&
	assert_equal actual expected
'

test_expect_success 'encryption and decryption roundtrips' '
	assert_success "echo test | gob-encrypt key | gob-decrypt key" >actual &&
	echo test >expected &&
	assert_equal actual expected
'

test_expect_success 'decryption with different key fails' '
	assert_success gob-keygen other &&
	assert_failure "echo test | gob-encrypt key | gob-decrypt other"
'

test_expect_success 'chunking with invalid block store version fails' '
	assert_success mkdir block-invalid-version &&
	assert_success "echo 0 >block-invalid-version/version" &&
	assert_failure "echo foobar | gob-chunk block-invalid-version"
'

test_expect_success 'chunking without block directory fails' '
	assert_failure gob-chunk
'

test_expect_success 'chunking with block directory succeeds' '
	assert_success mkdir blocks &&
	assert_success echo test | gob-chunk blocks >actual &&
	assert_success test -e blocks/21/ebd7636fdde0f4929e0ed3c0beaf55 &&
	cat >expected <<-EOF &&
		21ebd7636fdde0f4929e0ed3c0beaf55
		>21ebd7636fdde0f4929e0ed3c0beaf55 5
	EOF
	assert_equal actual expected
'

test_expect_success 'multiple equal chunks generate same hash' '
	assert_success dd if=/dev/zero bs=4M count=2 | gob-chunk blocks >actual &&
	assert_success test -e blocks/a1/45668a0b23bf1551f17838cf35e30e &&
	cat >expected <<-EOF &&
		a145668a0b23bf1551f17838cf35e30e
		a145668a0b23bf1551f17838cf35e30e
		>223d6f95048605b982a4d09ec2083405 8388608
	EOF
	assert_equal actual expected
'

test_expect_success 'chunk and cat roundtrip' '
	assert_success "echo foobar | gob-chunk blocks | gob-cat blocks >actual" &&
	echo foobar >expected &&
	assert_equal actual expected
'

test_expect_success 'cat with multiple blocks succeeds' '
	assert_success "dd if=/dev/zero bs=5M count=1 >expected" &&
	assert_success "cat expected | gob-chunk blocks | gob-cat blocks >actual" &&
	assert_equal actual expected
'

test_expect_success 'cat with only trailer fails' '
	assert_success "echo foobar | gob-chunk blocks | head -n1 >index" &&
	assert_failure "cat index | gob-cat blocks"
'

test_expect_success 'cat with too short trailer length fails' '
	assert_success "echo foobar | gob-chunk blocks | sed s/7$/4/ >index" &&
	assert_failure "cat index | gob-cat blocks"
'

test_expect_success 'cat with too long trailer length fails' '
	assert_success "echo foobar | gob-chunk blocks | sed s/7$/20/ >index" &&
	assert_failure "cat index | gob-cat blocks"
'

test_expect_success 'cat with invalid trailer hash fails' '
	assert_success "echo foobar | gob-chunk blocks | sed \"s/>..../>0000/\" >index" &&
	assert_failure "cat index | gob-cat blocks"
'

test_expect_success 'cat with missing trailer fails' '
	assert_success "echo foobar | gob-chunk blocks | head -n-1 >index" &&
	assert_failure "cat index | gob-cat blocks"
'

test_expect_success 'cat with non-existing blocks fails' '
	cat >index <<-EOF &&
		00000000000000000000000000000000
		>00000000000000000000000000000000 7
	EOF
	assert_failure "cat index | gob-cat blocks"
'

test_expect_success 'fsck with valid block store succeeds' '
	assert_success mkdir fsck &&
	assert_success echo test | gob-chunk fsck &&
	assert_success gob-fsck fsck
'

test_expect_success 'fsck with invalid store file fails' '
	assert_success gob-fsck fsck &&
	assert_success touch fsck/bogus &&
	assert_failure gob-fsck fsck &&
	assert_success rm fsck/bogus
'

test_expect_success 'fsck with invalid store file fails' '
	assert_success gob-fsck fsck &&
	assert_success mkdir fsck/ab &&
	assert_success touch fsck/ab/invalid &&
	assert_failure gob-fsck fsck &&
	assert_success rm fsck/ab/invalid
'

test_expect_success 'fsck with corrupted store file fails' '
	assert_success gob-fsck fsck &&
	assert_success echo "foobar" > fsck/21/ebd7636fdde0f4929e0ed3c0beaf55 &&
	assert_failure gob-fsck fsck
'

rm -rf "$TEST_DIR"

# vim: noexpandtab
