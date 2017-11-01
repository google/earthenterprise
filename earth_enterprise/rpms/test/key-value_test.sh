#!/bin/bash

# Copyright 2017 the Open GEE Contributors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Unit tests for the key value store used by the RPMs.  The functions that
# start with "test_" are the unit tests.  Other functions are utility
# functions.

SCRIPT_DIR=`dirname "$0"`
RPM_DIR=$(cd "${SCRIPT_DIR}/.."; pwd)

# Load the functions to test
source "${RPM_DIR}/shared/lib/key-value.sh"

test_keyvalue_file_string_starts_with() {
  assertTrue "Positive key value starts with failed" "keyvalue_file_string_starts_with abcde abc"
  assertFalse "Negative key value starts with failed" "keyvalue_file_string_starts_with abcde xyz"
  assertTrue "Empty string starts with failed" "keyvalue_file_string_starts_with abcde ''"
  assertTrue "Same string starts with failed" "keyvalue_file_string_starts_with abcde abcde"
  assertFalse "Prefix too long starts with failed" "keyvalue_file_string_starts_with abcde abcdef"
}

test_keyvalue_file_string_ends_with() {
  assertTrue "Positive key value ends with failed" "keyvalue_file_string_ends_with abcde cde"
  assertFalse "Negative key value ends with failed" "keyvalue_file_string_ends_with abcde xyz"
  assertTrue "Empty string ends with failed" "keyvalue_file_string_ends_with abcde ''"
  assertTrue "Same string ends with failed" "keyvalue_file_string_ends_with abcde abcde"
  assertFalse "Prefix too long ends with failed" "keyvalue_file_string_ends_with abcde abcdef"
}

function_with_output_test() {
  # $1 - function name
  # $2 - message
  # $3 - input value
  # $4 - expected output
  "$1" "$3" "OUTPUT"
  assertEquals "$2" "$4" "$OUTPUT"
}

key_escape_test() {
  # $1 - message
  # $2 - input value
  # $3 - expected output
  function_with_output_test "keyvalue_file_escape_key" "$1" "$2" "$3"
}

test_keyvalue_file_escape_key() {
  key_escape_test "Basic key escape failed" "abcde" "abcde"
  key_escape_test "Space key escape failed" "hello world" "hello\ world"
  key_escape_test "Dollar key escape failed" 'Owed $20' 'Owed\ \$20'
  key_escape_test "Backslash key escape failed" 'back\slash' 'back\\slash'
  key_escape_test "Multiple key escape failed" ' \$' '\ \\\$'
  key_escape_test "Newline at end escape failed" 'abc

' "$'abc\n\n'"
}

value_escape_test() {
  # $1 - message
  # $2 - input value
  # $3 - expected output
  function_with_output_test "keyvalue_file_escape_value" "$1" "$2" "$3"
}

test_keyvalue_file_escape_value() {
  value_escape_test "Basic value escape failed" "aBCde" "aBCde"
  value_escape_test "New line value escape failed" 'abc
def' 'abc\ndef'
  value_escape_test "Carriage return value escape failed" $'abc\rdef' 'abc\rdef'
  value_escape_test "New line at end failed" 'abc

' 'abc\n\n'
  value_escape_test "Backslash value escape failed" 'back\slash' 'back\\slash'
  value_escape_test "Backspace value escape failed" $'abc\bdef' 'abc\bdef'
  value_escape_test "Form feed value escape failed" $'abc\fdef' 'abc\fdef'
  value_escape_test "Vertical tab value escape failed" $'abc\vdef' 'abc\vdef'
  value_escape_test "Multiple value escape failed" $'Yz\r\n\\\b\f\v' 'Yz\r\n\\\b\f\v'
}

value_unescape_test() {
  # $1 - message
  # $2 - input value
  # $3 - expected output
  function_with_output_test "keyvalue_file_unescape_value" "$1" "$2" "$3"
}

test_keyvalue_file_unescape_value() {
  value_unescape_test "Basic value escape failed" "aBCde" "aBCde"
  value_unescape_test "New line value escape failed" 'abc\ndef' 'abc
def'
  value_unescape_test "Carriage return value escape failed" 'abc\rdef' $'abc\rdef'
  value_unescape_test "Backslash value escape failed" 'back\\slash' 'back\slash'
  value_unescape_test "Backspace value escape failed" 'abc\bdef' $'abc\bdef'
  value_unescape_test "Null value escape failed" 'abc\x00def' $'abc\x00def'
  value_unescape_test "Form feed value escape failed" 'abc\fdef' $'abc\fdef'
  value_unescape_test "Vertical tab value escape failed" 'abc\vdef' $'abc\vdef'
  value_unescape_test "Multiple value escape failed" 'Yz\r\n\\\b\f\v' $'Yz\r\n\\\b\f\v'
}

test_keyvalue_file_escape_unescape() {
  local VALUE=$'aB\r\n\\\b\f\v\$'
  keyvalue_file_escape_value "$VALUE" "ESCAPED"
  keyvalue_file_unescape_value "$ESCAPED" "UNESCAPED"
  assertEquals "Escape and unescape value failed" "$VALUE" "$UNESCAPED"
}

stdin_test() {
  # $1 - message
  # $2 - variable
  # $3 - expected value
  # $4 - input
  if [ -n "$3" ]; then
    EXPECTED="$3."
  else
    EXPECTED=""
  fi
  OUTPUT=`keyvalue_file_on_stdin_output_value_for_key "$2" <<< "$4"`
  assertEquals "$1" "$EXPECTED" "$OUTPUT"
}

test_keyvalue_file_on_stdin_output_value_for_key() {
  stdin_test "Basic stdin test failed" "abcd" "efgh" $'abcd=efgh\n.'
  stdin_test "Escaped char in key stdin test failed" 'ab$cd' "efgh" 'ab$cd=efgh
.'
  stdin_test "Escaped char in value stdin test failed" 'abcd' $'ef\bgh' 'abcd=ef\bgh
.'
  stdin_test "Missing var stdin test failed" 'xyz' '' $'abcd=efgh\n.'
  stdin_test "Multiple vars stdin test 1 failed" 'ijkl' 'mnop' $'abcd=efgh\nijkl=mnop\nqrst=uvwx\n.'
  stdin_test "Multiple vars stdin test 2 failed" 'abcd' 'efgh' $'abcd=efgh\nijkl=mnop\nqrst=uvwx\n.'
  stdin_test "Multiple vars stdin test 3 failed" 'qrst' 'uvwx' $'abcd=efgh\nijkl=mnop\nqrst=uvwx\n.'
  stdin_test "Comments stdin test failed" 'qrst' 'uvwx' $'# Comment\n# Another comment\nabcd=efgh\nijkl=mnop\n# Comment in the middle\nqrst=uvwx\n.'
}

create_test_file() {
  FILENAME=`mktemp`
  echo "# Header comment" >> "$FILENAME"
  echo "key1=value1" >> "$FILENAME"
  echo "key2=value2" >> "$FILENAME"
  echo "." >> "$FILENAME"
  echo "$FILENAME"
}

test_keyvalue_file_get() {
  # Test a non-existent file
  keyvalue_file_get "notafile.kv" "abc" "OUTPUT"
  assertNull "$OUTPUT"
  
  # Test an existing file
  
  # Create the file
  FILENAME=`create_test_file`
  
  # Read a key from it
  keyvalue_file_get "$FILENAME" "key2" "OUTPUT"
  assertEquals "Get from file failed" "value2" "$OUTPUT"
  
  # Clean up
  rm -f "$FILENAME"
}

# keyvalue_file_set_filter changes the value of an existing key or adds it if
# it doesn't exist.
test_keyvalue_file_set_filter() {
  # Test an existing key
  INPUT=$'key1=value1\n.'
  EXPECTED_OUTPUT=$'key1=value2\n.'
  OUTPUT=`keyvalue_file_set_filter "key1" "value2" <<< "$INPUT"`
  assertEquals "Changing key failed" "$EXPECTED_OUTPUT" "$OUTPUT"

  # Test a non-existent key
  INPUT=$'key1=value1\n.'
  EXPECTED_OUTPUT=$'key1=value1\nkey2=value2\n.'
  OUTPUT=`keyvalue_file_set_filter "key2" "value2" <<< "$INPUT"`
  assertEquals "Adding key failed" "$EXPECTED_OUTPUT" "$OUTPUT"
}

key_set_test() {
  # $1 - file name
  # $2 - key
  # $3 - value
  # $4 - failure message
  # $5 - expected output
  keyvalue_file_set "$1" "$2" "$3"
  FILE_CONTENTS=`cat "$FILENAME"`
  assertEquals "$4" "$5" "$FILE_CONTENTS"
  rm -f "$FILENAME"
}

test_keyvalue_file_set() {
  # Test a non-existent file
  FILENAME=`mktemp --dry-run`
  key_set_test "$FILENAME" "akey" "avalue" "Setting a key in a non-existent file failed" $'akey=avalue\n.'
  
  # Test an existing key
  FILENAME=`create_test_file`
  key_set_test "$FILENAME" "key1" "newvalue" "Setting existing key to new value failed" $'# Header comment\nkey1=newvalue\nkey2=value2\n.'

  # Test a new key
  FILENAME=`create_test_file`
  key_set_test "$FILENAME" "key3" "value3" "Setting existing key to new value failed" $'# Header comment\nkey1=value1\nkey2=value2\nkey3=value3\n.'
}

# Load the unit testing library (this also runs the tests)
source /usr/share/shunit2/shunit2

