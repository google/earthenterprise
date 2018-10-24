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

# Source this file to get the functions defined below.

# This file contains functions for storing and retrieving values in a
# key-value file.  There is no indexing implemented, and the key-value file
# may be temporarily stored in memory, so this library is only useful for
# small-size keys and values, and a low number of keys in the store.


keyvalue_file_string_starts_with()
{
    local STRING="$1"
    local PREFIX="$2"

    [ "${STRING:0:${#PREFIX}}" == "$PREFIX" ]
}

keyvalue_file_string_ends_with()
{
    local STRING="$1"
    local SUFFIX="$2"

    [ "${STRING:$[ ${#STRING} - ${#SUFFIX} ]}" == "$SUFFIX" ]
}

# Escapes a key to a string that can be stored in the key-value store.
keyvalue_file_escape_key()
{
    # local KEY="$1"
    # local OUTPUT_VARIABLE_NAME="$2"

    printf -v "$2" "%q" "$1"
}

# Escapes a value to a string that can be stored in the key-value store.
# Note: this function does not escape null characters because current versions
# of bash do not support null characters in variable values.
keyvalue_file_escape_value()
{
    local _KFEV_VALUE="$1"
    local _KFEV_OUTPUT_VARIABLE_NAME="$2"

    local _KFEV_RESULT=""
    local _KFEV_CHAR_CR=""
    local _KFEV_CHAR_LF=""
    local _KFEV_i=0

    printf -v _KFEV_CHAR_CR "%b" '\r'
    printf -v _KFEV_CHAR_LF "%b" '\n'

    while [ "$_KFEV_i" -lt "${#_KFEV_VALUE}" ]; do
        case "${_KFEV_VALUE:$_KFEV_i:1}" in
            $_KFEV_CHAR_CR)
                _KFEV_RESULT="${_KFEV_RESULT}\\r"
                ;;
            $_KFEV_CHAR_LF)
                _KFEV_RESULT="${_KFEV_RESULT}\\n"
                ;;
            \\)
                _KFEV_RESULT="${_KFEV_RESULT}\\\\"
                ;;
            $(printf "%b" '\b'))
                _KFEV_RESULT="${_KFEV_RESULT}\\b"
                ;;
            $(printf "%b" '\f'))
                _KFEV_RESULT="${_KFEV_RESULT}\\f"
                ;;
            $(printf "%b" '\v'))
                _KFEV_RESULT="${_KFEV_RESULT}\\v"
                ;;
            *)
                _KFEV_RESULT="${_KFEV_RESULT}${_KFEV_VALUE:$_KFEV_i:1}"
        esac
        (( _KFEV_i++ ))
    done

    printf -v "$_KFEV_OUTPUT_VARIABLE_NAME" "%s" "$_KFEV_RESULT"
}

# Converts an escaped value from the form that it's stored as in the key-value
# store to the original value that was sent to the store.
keyvalue_file_unescape_value()
{
    # local ESCAPED="$1"
    # local OUTPUT_VARIABLE_NAME="$2"

    printf -v "$2" "%b" "$1"
}

# Gets a value with a given key from a key-value store being read from the
# standard input.
#
# If the key is found, the value is output to standard output, followed by a 
# '.' character (to allow for values that end in new lines).  If the key is
# not found, nothing is written to the standard output, and the function
# returns  with return code of 1.
keyvalue_file_on_stdin_output_value_for_key()
{
    local _KFGFS_KEY="${1}="
    local _KFGFS_VALUE
    local REPLY

    while read -r; do
        if keyvalue_file_string_starts_with "$REPLY" "${_KFGFS_KEY}"
        then
            _KFGFS_VALUE="${REPLY:${#_KFGFS_KEY}}"
            keyvalue_file_unescape_value \
                "$_KFGFS_VALUE" \
                "REPLY"
            echo "${REPLY}."
            return 0
        fi
    done

    # Key not found:
    return 1
}

# Gets a value with a given key from a key-value store in a file.
#
# If the key is found, the value is stored in the variable whose name is in
# the _KFGFS_OUTPUT_VARIABLE_NAME variable.  If the key is not found, the
# variable whose name is in _KFGFS_OUTPUT_VARIABLE_NAME is unset, and the
# function returns with return code of 1.
#
# Warning: Some values don't work as OUTPUT_VARIABLE_NAME (e.g., "REPLY").
keyvalue_file_get()
{
    local _KFG_STORE_PATH="$1"
    local _KFG_KEY="$2"
    local _KFG_OUTPUT_VARIABLE_NAME="$3"
    local _KFG_VALUE

    if [ -f "$_KFG_STORE_PATH" ]; then
        _KFG_VALUE=$(
            exec 9> "${_KFG_STORE_PATH}.lock"
            flock --shared 9 || exit 1
            keyvalue_file_on_stdin_output_value_for_key \
                "$_KFG_KEY" "$_KFG_OUTPUT_VARIABLE_NAME" \
                < "$_KFG_STORE_PATH"
        )
        if [ -z "$_KFG_VALUE" ]; then
            # Key not found:
            unset "$_KFG_OUTPUT_VARIABLE_NAME"
            return 1
        else
            # Strip the final '.':
            _KFG_VALUE="${_KFG_VALUE%.}"
            printf -v "$_KFG_OUTPUT_VARIABLE_NAME" "%s" "$_KFG_VALUE"
        fi
    else
        # Store file not found, so key not found:
        unset "$_KFG_OUTPUT_VARIABLE_NAME"
        return 1
    fi
}

# Sets the value for a given key in a key-value store.  The current store is
# read from the standard input, and the updated store is output to the
# standard output.
keyvalue_file_set_filter()
{
    local KEY
    local VALUE

    keyvalue_file_escape_key "$1" KEY
    KEY="${KEY}="

    keyvalue_file_escape_value "$2" VALUE

    local REPLY
    local KEY_WAS_SET=""

    while read -r; do
        # Skip end-of-file marks:
        if [ "$REPLY" == "." ]; then
            continue
        fi
        if keyvalue_file_string_starts_with "$REPLY" "$KEY"; then
            # Replace value for an existing key:
            if [ -z "$KEY_WAS_SET" ]; then
                echo "${KEY}${VALUE}"
                KEY_WAS_SET=yes
            fi
        else
            echo "$REPLY"
        fi
    done

    if [ -z "$KEY_WAS_SET" ]; then
        # Add value for a new key:
        echo "${KEY}${VALUE}"
    fi

    # Add an end-of-file mark (to help reading the file in memory in BASH):
    echo "."
}

# Sets the value for a given key in a key-value file.
keyvalue_file_set()
{
    local STORE_PATH="$1"
    local KEY="$2"
    local VALUE="$3"

    (
        flock --exclusive 9 || exit 1
        if [ -f "$STORE_PATH" ]; then
            local UPDATED_STORE=$(keyvalue_file_set_filter "$KEY" "$VALUE" < "$STORE_PATH")
        else
            local UPDATED_STORE=$(keyvalue_file_set_filter "$KEY" "$VALUE" < /dev/null)
        fi
        echo "$UPDATED_STORE" > "$STORE_PATH"
    ) 9> "$STORE_PATH.lock"
}
