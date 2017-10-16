# Source this file to get functions defined below.

# Parse a shell command's first argument.
#
# Example usage:
#   COMMAND_STRING="RUN -v a:b -c 'echo Yo'"
#   local ARGUMENT
#
#   while [ -n "$COMMAND_STRING" ]; do
#       cmd_parse_first_arg "$COMMAND_STRING" yes ARGUMENT COMMAND_STRING ||
#           return 1
#       echo "ARGUMENT: $ARGUMENT"
#   done
#
# Output:
#   ARGUMENT: RUN
#   ARGUMENT: -v
#   ARGUMENT: a:b
#   ARGUMENT: -c
#   ARGUMENT: echo Yo
cmd_parse_first_arg()
{
	# Use long variable prefixes to avoid collisions with output variable names:
	local CMD_PARSE_FIRST_ARG_COMMAND_STRING="$1"
	local CMD_PARSE_FIRST_ARG_STRIP_LEADING_SPACE="$2"
	local CMD_PARSE_FIRST_ARG_ARGUMENT_OUTPUT_VARIABLE_NAME="$3"
	local CMD_PARSE_FIRST_ARG_REMAINING_STRING_OUTPUT_VARIABLE_NAME="$4"
	
	local CMD_PARSE_FIRST_ARG_RESULT=""
	local CMD_PARSE_FIRST_ARG_LF
	local CMD_PARSE_FIRST_ARG_I
	local CMD_PARSE_FIRST_ARG_CH

	printf -v CMD_PARSE_FIRST_ARG_LF '\n'

	(( CMD_PARSE_FIRST_ARG_I=0 ))
	if [ -n "$CMD_PARSE_FIRST_ARG_STRIP_LEADING_SPACE" ]; then
		while [ "$CMD_PARSE_FIRST_ARG_I" -lt "${#CMD_PARSE_FIRST_ARG_COMMAND_STRING}" ]; do
			CMD_PARSE_FIRST_ARG_CH="${CMD_PARSE_FIRST_ARG_COMMAND_STRING:$CMD_PARSE_FIRST_ARG_I:1}"
			case "$CMD_PARSE_FIRST_ARG_CH" in
				[[:space:]])
					# Skip the space
					;;
				\\)
					CMD_PARSE_FIRST_ARG_CH="${CMD_PARSE_FIRST_ARG_COMMAND_STRING:$[ $CMD_PARSE_FIRST_ARG_I + 1 ]:1}"
					if [ "$CMD_PARSE_FIRST_ARG_CH" != "\\" ]; then
						break
					fi
					;;
				*)
					break
					;;
			esac
			(( CMD_PARSE_FIRST_ARG_I++ ))
		done
	fi

	while [ "$CMD_PARSE_FIRST_ARG_I" -lt "${#CMD_PARSE_FIRST_ARG_COMMAND_STRING}" ]; do
		CMD_PARSE_FIRST_ARG_CH="${CMD_PARSE_FIRST_ARG_COMMAND_STRING:$CMD_PARSE_FIRST_ARG_I:1}"
		case "$CMD_PARSE_FIRST_ARG_CH" in
			\\)
				# Esaped character:
				(( CMD_PARSE_FIRST_ARG_I++ ))
				CMD_PARSE_FIRST_ARG_CH="${CMD_PARSE_FIRST_ARG_COMMAND_STRING:$CMD_PARSE_FIRST_ARG_I:1}"
				if [ "$CMD_PARSE_FIRST_ARG_CH" != "$CMD_PARSE_FIRST_ARG_LF" ]; then
						CMD_PARSE_FIRST_ARG_RESULT="${CMD_PARSE_FIRST_ARG_RESULT}${CMD_PARSE_FIRST_ARG_CH}"
				fi
				;;
			\')
				# Singly-quoted string:
				while true; do
					(( CMD_PARSE_FIRST_ARG_I++ ))
					if [ "$CMD_PARSE_FIRST_ARG_I" -ge "${#CMD_PARSE_FIRST_ARG_COMMAND_STRING}" ]
					then
						echo "Unterminated singly-quoted string!" >&2
						return 1
					fi
					CMD_PARSE_FIRST_ARG_CH="${CMD_PARSE_FIRST_ARG_COMMAND_STRING:$CMD_PARSE_FIRST_ARG_I:1}"
					if [ "$CMD_PARSE_FIRST_ARG_CH" = "'" ]; then
						break
					else
						CMD_PARSE_FIRST_ARG_RESULT="${CMD_PARSE_FIRST_ARG_RESULT}${CMD_PARSE_FIRST_ARG_CH}"
					fi
				done
				;;
			\")
				# Doubly-quoted string:
				while true; do
					(( CMD_PARSE_FIRST_ARG_I++ ))
					if [ "$CMD_PARSE_FIRST_ARG_I" -ge "${#CMD_PARSE_FIRST_ARG_COMMAND_STRING}" ]
					then
						echo "Unterminated doubly-quoted string!" >&2
						return 1
					fi
					CMD_PARSE_FIRST_ARG_CH="${CMD_PARSE_FIRST_ARG_COMMAND_STRING:$CMD_PARSE_FIRST_ARG_I:1}"
					case "$CMD_PARSE_FIRST_ARG_CH" in
						\\)
							# Esaped character:
							(( CMD_PARSE_FIRST_ARG_I++ ))
							CMD_PARSE_FIRST_ARG_CH="${CMD_PARSE_FIRST_ARG_COMMAND_STRING:$CMD_PARSE_FIRST_ARG_I:1}"
							if [ "$CMD_PARSE_FIRST_ARG_CH" != "$CMD_PARSE_FIRST_ARG_LF" ]; then
								CMD_PARSE_FIRST_ARG_RESULT="${CMD_PARSE_FIRST_ARG_RESULT}${CMD_PARSE_FIRST_ARG_CH}"
							fi
							;;
						\")
							break
							;;
						*)
							CMD_PARSE_FIRST_ARG_RESULT="${CMD_PARSE_FIRST_ARG_RESULT}${CMD_PARSE_FIRST_ARG_CH}"
							;;
					esac
				done
				;;
			[[:space:]])
				# End of argument:
				break
				;;
			*)
				CMD_PARSE_FIRST_ARG_RESULT="${CMD_PARSE_FIRST_ARG_RESULT}${CMD_PARSE_FIRST_ARG_CH}"
				;;
		esac
		(( CMD_PARSE_FIRST_ARG_I++ ))
	done

	printf -v "$CMD_PARSE_FIRST_ARG_ARGUMENT_OUTPUT_VARIABLE_NAME" "%s" \
		"$CMD_PARSE_FIRST_ARG_RESULT"
	printf -v "$CMD_PARSE_FIRST_ARG_REMAINING_STRING_OUTPUT_VARIABLE_NAME" \
		"%s" "${CMD_PARSE_FIRST_ARG_COMMAND_STRING:$CMD_PARSE_FIRST_ARG_I}"
}

