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
TEST_DIR=$(mktemp -d /tmp/sb-tests-XXXXXXXX)

assert_files_equal() {
	cmp "$1" "$2"
	return $?
}

assert_equal() {
	test "$1" = "$2"
	return $?
}

assert_not_equal() {
	test "$1" != "$2"
	return $?
}

assert_success() {
	eval "$@"
	return $?
}

assert_failure() {
	eval "$@"
	test $? -ne 0
	return $?
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
	assert_failure sb-keygen
'

test_expect_success 'keygen generates key' '
	assert_success sb-keygen key &&
	assert_success test -f key
'

test_expect_success 'keygen avoids overwriting existing key' '
	assert_failure sb-keygen key
'

test_expect_success 'encryption generates fixed blocksize' '
	echo test | sb-encrypt key | wc -c >actual &&
	echo $((4096 * 1024)) >expected &&
	assert_files_equal actual expected
'

test_expect_success 'encryption generates multiples of blocksize' '
	dd if=/dev/zero bs=$((4096 * 1025)) count=1 | sb-encrypt key | wc -c >actual &&
	echo $((4096 * 1024 * 2)) >expected &&
	assert_files_equal actual expected
'

test_expect_success 'key generates deterministic sequence' '
	dd if=/dev/zero bs=$((4096 * 1025)) count=1 | sb-encrypt key >expected &&
	dd if=/dev/zero bs=$((4096 * 1025)) count=1 | sb-encrypt key >actual &&
	assert_files_equal actual expected
'

test_expect_success 'encryption and decryption roundtrips' '
	assert_equal $(echo test | sb-encrypt key | sb-decrypt key) test
'

test_expect_success 'decryption with different key fails' '
	assert_success sb-keygen other &&
	assert_not_equal $(echo test | sb-encrypt key | sb-decrypt other) test
'

test_expect_success 'chunking without block directory fails' '
	assert_failure sb-chunk
'

test_expect_success 'chunking with block directory succeeds' '
	assert_success mkdir blocks &&
	assert_success echo test | sb-chunk blocks >actual &&
	assert_success test -e blocks/21/ebd7636fdde0f4929e0ed3c0beaf55 &&
	cat >expected <<-EOF &&
		21ebd7636fdde0f4929e0ed3c0beaf55
		>21ebd7636fdde0f4929e0ed3c0beaf55 5
	EOF
	assert_files_equal actual expected
'

test_expect_success 'multiple equal chunks generate same hash' '
	assert_success dd if=/dev/zero bs=4M count=2 | sb-chunk blocks >actual &&
	assert_success test -e blocks/a1/45668a0b23bf1551f17838cf35e30e &&
	cat >expected <<-EOF &&
		a145668a0b23bf1551f17838cf35e30e
		a145668a0b23bf1551f17838cf35e30e
		>223d6f95048605b982a4d09ec2083405 8388608
	EOF
	assert_files_equal actual expected
'

rm -rf "$TEST_DIR"

# vim: noexpandtab
