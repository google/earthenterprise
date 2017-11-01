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

# These file contains an XML parser in BASH, with XML node content extraction
# by basic XPath expressions.
#   * CData content is not supported.
#   * XPath expressions like `last()`, `position()`, `<`, `>`, `=` are not
#     supported.

# This code is based on
# <https://stackoverflow.com/questions/893585/how-to-parse-xml-in-bash>.


xml_string_starts_with()
{
    local STRING="$1"
    local PREFIX="$2"

    [ "${STRING:0:${#PREFIX}}" == "$PREFIX" ]
}

xml_string_ends_with()
{
    local STRING="$1"
    local SUFFIX="$2"

    [ "${STRING:$[ ${#STRING} - ${#SUFFIX} ]}" == "$SUFFIX" ]
}

xml_string_trim_left()
{
    local _XSTL_RESULT="$1"
    local _XSTL_OUTPUT_VARIABLE_NAME="$2"

    _XSTL_RESULT="${_XSTL_RESULT#"${_XSTL_RESULT%%[![:space:]]*}"}"

    printf -v "$_XSTL_OUTPUT_VARIABLE_NAME" "%s" "$_XSTL_RESULT"
}

xml_string_trim_right()
{
    local _XSTR_RESULT="$1"
    local _XSTR_OUTPUT_VARIABLE_NAME="$2"

    _XSTR_RESULT="${_XSTR_RESULT%"${_XSTR_RESULT##*[![:space:]]}"}"

    printf -v "$_XSTR_OUTPUT_VARIABLE_NAME" "%s" "$_XSTR_RESULT"
}

xml_string_trim()
{
    local _XST_RESULT="$1"
    local _XST_OUTPUT_VARIABLE_NAME="$2"

    xml_string_trim_left "$_XST_RESULT" "$_XST_OUTPUT_VARIABLE_NAME"
    xml_string_trim_right "${!_XST_OUTPUT_VARIABLE_NAME}" \
        "$_XST_OUTPUT_VARIABLE_NAME"
}

# Reads the next XML tag from standard input, and returns the
# <tag expression>, and <tag>tag content</tag>.
xml_read_tag()
{
    local _RT_TAG_EXPRESSION_OUTPUT_VARIABLE_NAME="$1"
    local _RT_TAG_CONTENT_OUTPUT_VARIABLE_NAME="$2"

    local _RT_OLD_IFS="$IFS"
    local _RT_RESULT_CODE
    local IFS=\>

    read -d \< "$_RT_TAG_EXPRESSION_OUTPUT_VARIABLE_NAME" \
        "$_RT_TAG_CONTENT_OUTPUT_VARIABLE_NAME"
    _RT_RESULT_CODE="$?"

    IFS="$_RT_OLD_IFS"

    xml_string_trim "${!_RT_TAG_EXPRESSION_OUTPUT_VARIABLE_NAME}" \
        "$_RT_TAG_EXPRESSION_OUTPUT_VARIABLE_NAME"

    return "$_RT_RESULT_CODE"
}

# Calls a call-back function for each child of the current XML tag (read from
# standard input).
#
# The call-back prototype is:
#
# callback()
# {
#     local NODE_XPATH="$1"
#     local ELEMENT_1_INDEX="$2"
#     local ATTRIBUTE_1_INDEX="$3"
#     local NODE_CONTENT="$4"   
# }
#
# Returns 1 if the end of standard input is reached.
xml_visit_tag_children()
{
    # Callback used for visiting XML nodes (like element content and
    # attributes):
    local _VTC_VISIT_FUNCTION_NAME="$1"

    local _VTC_END_TAG_OUTPUT_VARIABLE_NAME="$2"

    # Absolute XPath to the current node's parent:
    local _VTC_CURRENT_PATH="$3"

    # 1-based index of the current node in the parent's array of child
    # elements:
    local _VTC_CURRENT_CHILD_1_INDEX="$4"

    xml_read_tag _VTC_TAG_EXPRESSION_OUTPUT _VTC_TAG_CONTENT_OUTPUT
    local _VTC_RETURN_STATUS="$?"

    # We need local variables so we can do recursion:
    local _VTC_TAG_EXPRESSION="$_VTC_TAG_EXPRESSION_OUTPUT"
    local _VTC_TAG_NO_CHILDREN=""
    local _VTC_TAG_CONTENT="$_VTC_TAG_CONTENT_OUTPUT"

    if xml_string_starts_with "$_VTC_TAG_EXPRESSION" '?'; then
        _VTC_TAG_EXPRESSION="${_VTC_TAG_EXPRESSION:1}"
    fi
    if xml_string_ends_with "$_VTC_TAG_EXPRESSION" '?'; then
        _VTC_TAG_EXPRESSION="${_VTC_TAG_EXPRESSION%?}"
        _VTC_TAG_NO_CHILDREN=true
    fi

    if xml_string_ends_with "$_VTC_TAG_EXPRESSION" '/'; then
        _VTC_TAG_EXPRESSION="${_VTC_TAG_EXPRESSION%/}"
        _VTC_TAG_NO_CHILDREN=true
    fi

    local _VTC_ELEMENT_NAME

    local _VTC_TERM_INDEX=0
    local _VTC_ATTRIBUTE_NAME
    local _VTC_ATTRIBUTE_VALUE

    local _VTC_TERM

    for _VTC_TERM in $_VTC_TAG_EXPRESSION; do
        if [ "$_VTC_TERM_INDEX" -lt 1 ]; then
            if xml_string_starts_with "$_VTC_TERM" "/"; then
                # This is closing </tag>:
                printf -v "$_VTC_END_TAG_OUTPUT_VARIABLE_NAME" \
                    "%s" "$_VTC_TERM"
                return "$_VTC_RETURN_STATUS"
            fi

            _VTC_ELEMENT_NAME="$_VTC_TERM"
            _VTC_CURRENT_PATH="${_VTC_CURRENT_PATH}/${_VTC_TERM}[${_VTC_CURRENT_CHILD_1_INDEX}]"
            "$_VTC_VISIT_FUNCTION_NAME" "$_VTC_CURRENT_PATH" \
                "$_VTC_CURRENT_CHILD_1_INDEX" "$_VTC_TERM_INDEX" \
                "$_VTC_TAG_CONTENT"
        else
            _VTC_ATTRIBUTE_NAME="${_VTC_TERM%%=*}"
            _VTC_ATTRIBUTE_VALUE="${_VTC_TERM:$[ ${#_VTC_ATTRIBUTE_NAME} + 1]}"
            
            if xml_string_starts_with "$_VTC_ATTRIBUTE_VALUE" '"' && \
            xml_string_ends_with "$_VTC_ATTRIBUTE_VALUE" '"'; then
                _VTC_ATTRIBUTE_VALUE="${_VTC_ATTRIBUTE_VALUE:1:$[ ${#_VTC_ATTRIBUTE_VALUE} - 2 ]}"
            fi
            
            "$_VTC_VISIT_FUNCTION_NAME" \
                "${_VTC_CURRENT_PATH}@${_VTC_ATTRIBUTE_NAME}" \
                "$_VTC_CURRENT_CHILD_1_INDEX" "$_VTC_TERM_INDEX" \
                "$_VTC_ATTRIBUTE_VALUE"
        fi
        (( _VTC_TERM_INDEX++ ))
    done
    
    # Visit child elements:
    local _VTC_CHILD_1_INDEX

    if [ -n "$_VTC_TAG_NO_CHILDREN" ]; then
        printf -v "$_VTC_END_TAG_OUTPUT_VARIABLE_NAME" \
            "%s" "/$_VTC_ELEMENT_NAME"        
    else
        _VTC_CHILD_1_INDEX=1
        while [ "$_VTC_RETURN_STATUS" -eq 0 ]; do
            xml_visit_tag_children "$_VTC_VISIT_FUNCTION_NAME" \
                _VTC_END_TAG "$_VTC_CURRENT_PATH" "$_VTC_CHILD_1_INDEX"
            _VTC_RETURN_STATUS="$?"
        
            if [ -n "$_VTC_END_TAG" ]; then
                _VTC_END_TAG=""
                return "$_VTC_RETURN_STATUS"
            fi
            (( _VTC_CHILD_1_INDEX++ ))
        done
    fi

    return "$_VTC_RETURN_STATUS"
}

# Invokes a given function for each XML node parsed from the standard input.
#   Entities in node content are not decoded.
#
# E.g.:
#
#    xml_node_visit()
#    {
#        local NODE_XPATH="$1"
#        local ELEMENT_1_INDEX="$2"
#        local ATTRIBUTE_1_INDEX="$3"
#        local NODE_CONTENT="$4"   
#    
#        echo "$NODE_XPATH @[$ATTRIBUTE_1_INDEX] => $NODE_CONTENT"
#    }
#    
#    xml_visit_nodes xml_node_visit < volumes.xml
xml_visit_nodes()
{
    # Callback used for visiting XML nodes (like element content and
    # attributes):
    local _XVN_VISIT_FUNCTION_NAME="$1"

    local _XVN_CHILD_1_INDEX=1

    while xml_visit_tag_children "$_XVN_VISIT_FUNCTION_NAME" _XVN_END_TAG \
        "" "$_XVN_CHILD_1_INDEX"
    do
        (( _XVN_CHILD_1_INDEX++ ))
    done
}


# Echoes the node text, if an XML node with a matching XPath is visited, and,
# optionally, terminates the shell script.
_XNETAP_XPATH_REGEX=""
_XNETAP_EXIT_AT_1ST_RESULT=""

xml_node_extract_text_at_path()
{
    local NODE_XPATH="$1"
    local ELEMENT_1_INDEX="$2"
    local ATTRIBUTE_1_INDEX="$3"
    local NODE_CONTENT="$4"

    # echo "NODE_XPATH: $NODE_XPATH ~= $_XNETAP_XPATH_REGEX"

    if echo "$NODE_XPATH" | grep -q -x -E -e "$_XNETAP_XPATH_REGEX"; then
        echo "$NODE_CONTENT"
        if [ -n "$_XNETAP_EXIT_AT_1ST_RESULT" ]; then
            exit 0
        fi
    fi
}

# Converts an XPath to an XPath regular expression that matches the XPaths
# used during `xml_node_visit`.
#   The regular expression is output to standard output.
xml_xpath_to_regex()
{
    local XPATH="$1"
    local TERM
    local ELEMENT_NAME
    local CHILD_INDEX

    while [ -n "$XPATH" ]; do
        if xml_string_starts_with "$XPATH" "//"; then
            printf ".*"
            XPATH="${XPATH#/}"
            continue
        fi

        # String leading '/':
        XPATH="${XPATH#/}"
        # Extract path term between slashes:
        TERM="${XPATH%%/*}"
        if [ -n "$TERM" ]; then
            # Strip the current term from the remaining XPATH:
            XPATH="${XPATH:${#TERM}}"
        else
            # We're at the last path term:
            TERM="$XPATH"
            XPATH=""
        fi

        # Test for a child index in $TERM:
        ELEMENT_NAME="${TERM%\[*\]}"
        CHILD_INDEX="${TERM:${#ELEMENT_NAME}}"
        CHILD_INDEX="${CHILD_INDEX#\[}"
        CHILD_INDEX="${CHILD_INDEX%\]}"

        if [ -n "$CHILD_INDEX" ]; then
            # $TERM contains a [child index]:
            printf "%s" "/${ELEMENT_NAME}\\[${CHILD_INDEX}\\]"
        else
            # $TERM doesn't contain a [child index]:
            printf "%s" "/${TERM}\\[[0-9]+\\]"
        fi
    done
}

# Extracts the node text at a given XPath from the XML on the standard input.
#   Only basic XPath expressions are supported.  Expressions containing
# consttructs, such as `last()`, `position()`, `>`, `<`, are not currently
# supported.
xml_get_node_text()
{
    local XPATH="$1"
    _XNETAP_EXIT_AT_1ST_RESULT="$2"

    _XNETAP_XPATH_REGEX=$(xml_xpath_to_regex "$XPATH")

    xml_visit_nodes xml_node_extract_text_at_path
}

# Extracts the node text at a given XPath from an XML file.
#   XML entities are not decoded.  You could pipe the output to do so.
#
# E.g.:
#
#    xml_file_get_node_text \
#        "$SELF_DIR/volumes.xml" \
#        "//VolumeDefList/volumedefs/item[1]/host" \
#        yes |
#    recode html..utf-8
xml_file_get_node_text()
{
    local FILE_PATH="$1"
    local XPATH="$2"
    local EXIT_AT_1ST_RESULT="$3"

    xml_get_node_text "$XPATH" "$EXIT_AT_1ST_RESULT" < "$FILE_PATH"
}
