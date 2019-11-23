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

PATH="$(pwd)/src:${PATH}"
TEST_NUM=0
TEST_DIR=$(mktemp -d /tmp/gob-tests-XXXXXXXX)
FAILED=0

assert_equal() {
	cmp "$1" "$2"
}

assert_success() {
	eval "$@"
}

assert_failure() {
	eval "$@"
	test $? -ne 0
}

test_expect_success() {
	TEST_NUM=$(($TEST_NUM + 1))

	if test $# -eq 3
	then
		if ! eval "$1"
		then
			echo "ok $TEST_NUM # SKIP $2"
			return
		fi

		shift
	fi

	(
		cd "$TEST_DIR" || exit -1

		OUTPUT="$(eval "$2" 2>&1)"
		STATUS="$?"

		if test $STATUS -eq 0
		then
			echo "ok $TEST_NUM $1"
		else
			echo "failed $TEST_NUM $1"
			test -n "$OUTPUT" && echo "$OUTPUT" | sed 's/^/  > /' >&2
		fi

		exit "$STATUS"
	)

	if test "$?" -ne 0
	then
		FAILED=$(($FAILED + 1))
	fi
}

test_expect_success 'chunking with invalid block store version fails' '
	assert_success mkdir block-invalid-version &&
	assert_success echo 0 >block-invalid-version/version &&
	assert_success echo foobar >input &&
	assert_failure gob chunk block-invalid-version <input
'

test_expect_success 'chunking without block directory fails' '
	assert_failure gob chunk
'

test_expect_success 'test -w /dev/full' 'chunking with invalid stdout fails' '
	assert_success mkdir store &&
	assert_success echo test >input &&
	assert_failure gob chunk store <input >/dev/full
'

test_expect_success 'chunking with block directory succeeds' '
	assert_success mkdir blocks &&
	assert_success echo test >input &&
	assert_success gob chunk blocks <input >actual &&
	assert_success test -e blocks/21/ebd7636fdde0f4929e0ed3c0beaf55 &&
	cat >expected <<-EOF &&
		21ebd7636fdde0f4929e0ed3c0beaf55
		>21ebd7636fdde0f4929e0ed3c0beaf55 5
	EOF
	assert_equal actual expected
'

test_expect_success 'multiple equal chunks generate same hash' '
	assert_success "dd if=/dev/zero bs=4194304 count=2 >zeroes" &&
	assert_success "gob chunk blocks <zeroes >actual" &&
	assert_success test -e blocks/a1/45668a0b23bf1551f17838cf35e30e &&
	cat >expected <<-EOF &&
		a145668a0b23bf1551f17838cf35e30e
		a145668a0b23bf1551f17838cf35e30e
		>223d6f95048605b982a4d09ec2083405 8388608
	EOF
	assert_equal actual expected
'

test_expect_success 'chunk and cat roundtrip' '
	assert_success "echo foobar >input" &&
	assert_success "gob chunk blocks <input >index" &&
	assert_success "gob cat blocks <index >actual" &&
	echo foobar >expected &&
	assert_equal actual expected
'

test_expect_success 'cat with multiple blocks succeeds' '
	assert_success dd if=/dev/zero bs=5242880 count=1 >expected &&
	assert_success gob chunk blocks <expected >index &&
	assert_success gob cat blocks <index >actual &&
	assert_equal actual expected
'

test_expect_success 'cat with only trailer fails' '
	assert_success echo foobar >input &&
	assert_success gob chunk blocks <input >index &&
	assert_success head -n1 <index >truncated &&
	assert_failure gob cat blocks <truncated
'

test_expect_success 'cat with too short trailer length fails' '
	assert_success echo foobar >input &&
	assert_success gob chunk blocks <input >index &&
	assert_success sed s/7$/4/ <index >invalid &&
	assert_failure gob cat blocks <invalid
'

test_expect_success 'cat with too long trailer length fails' '
	assert_success echo foobar >input &&
	assert_success gob chunk blocks <input >index &&
	assert_success sed s/7$/20/ <index >invalid &&
	assert_failure gob cat blocks <invalid
'

test_expect_success 'cat with invalid trailer hash fails' '
	assert_success echo foobar >input &&
	assert_success gob chunk blocks <input >index &&
	assert_success sed s|>....|>0000| <index >invalid &&
	assert_failure gob cat blocks <invalid
'

test_expect_success 'cat with missing trailer fails' '
	assert_success echo foobar >input &&
	assert_success gob chunk blocks <input >index &&
	assert_success head -n1 <index >truncated &&
	assert_failure gob cat blocks <truncated
'

test_expect_success 'cat with non-existing blocks fails' '
	cat >index <<-EOF &&
		00000000000000000000000000000000
		>00000000000000000000000000000000 7
	EOF
	assert_failure gob cat blocks <index
'

test_expect_success 'fsck with valid block store succeeds' '
	assert_success mkdir fsck &&
	assert_success echo test >input &&
	assert_success gob chunk fsck <input &&
	assert_success gob fsck fsck
'

test_expect_success 'fsck with invalid shard fails' '
	assert_success gob fsck fsck &&
	assert_success touch fsck/bogus &&
	assert_failure gob fsck fsck &&
	assert_success rm fsck/bogus
'

test_expect_success 'fsck with invalid store file fails' '
	assert_success gob fsck fsck &&
	assert_success mkdir fsck/ab &&
	assert_success touch fsck/ab/invalid &&
	assert_failure gob fsck fsck &&
	assert_success rm fsck/ab/invalid
'

test_expect_success 'fsck with corrupted store file fails' '
	assert_success gob fsck fsck &&
	assert_success echo foobar > fsck/21/ebd7636fdde0f4929e0ed3c0beaf55 &&
	assert_failure gob fsck fsck
'

echo "1..$TEST_NUM"

rm -rf "$TEST_DIR"

test "$FAILED" -eq 0

# vim: noexpandtab
