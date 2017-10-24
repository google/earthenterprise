#! /bin/bash

# An environment variable server that listens on a named pipe.
#
# Use a file lock when making requests to avoid concurrency problems.  E.g.:
#
# (
# 	flock -e 9
# 	echo "GET /var/USER" > /var/run/env-server.socket
# 	# Add a "." at the end to prevent final new-line stripping:
# 	user=$(cat /var/run/env-server.socket && echo .)
# 	# Remove the added "." suffix:
#	user=${user%.}
# 	# Use $user . . .
# ) 9>/var/run/env-server.lock

: ${ENVSERVE_PIPE_PATH:=/var/run/env-server.socket}

ENVSERVE_DELETE_PIPE_ON_EXIT=""

envserve_log()
{
	local MESSAGE_TYPE="$1"
	local MESSAGE="$2"

	case "$MESSAGE_TYPE" in
		debug)
			echo "$MESSAGE"
			;;
		info)
			echo "$MESSAGE"
			;;
		error)
			echo "$MESSAGE" >&2
			;;
		*)
			echo "Unknown message type: $MESSAGE_TYPE" >&2
			echo "Message: $MESSAGE" >&2
			return 1
			;;
	esac
}

envserve_exit()
{
	local EXIT_STATUS="$1"

	if [ -n "$ENVSERVE_DELETE_PIPE_ON_EXIT" ]; then
		envserve_log info "Deleting named pipe: $ENVSERVE_PIPE_PATH"
		rm -f "$ENVSERVE_PIPE_PATH"
	fi

	exit "$EXIT_STATUS"
}

envserve_send_reply()
{
	local STATUS_CODE="$1"
	local MESSAGE="$2"

	echo "$STATUS_CODE $MESSAGE" >"$ENVSERVE_PIPE_PATH"
}

envserve_raw_reply()
{
	local REPLY="$1"

	echo -n "$REPLY" >"$ENVSERVE_PIPE_PATH"
}

envserve_handle_get()
{
	local ENVSERVE_LOCATION="$1"

	case "$ENVSERVE_LOCATION" in
		/var/ENVSERVE_*|var/envserve_*|var/[0-9]*|var/@*|var/\#*)
			envserve_log error "Forbidden variable requested: $ENVSERVE_LOCATION"
			envserve_send_reply 403 "Forbidden resource requested: $ENVSERVE_LOCATION"
			;;
		/var/*)
			local ENVSERVE_VAR_NAME="${ENVSERVE_LOCATION:5}"
			# WARNING: Don't uncomment the following when serving secrets.
			# Use only for debugging.
			# envserve_log debug "Getting $ENVSERVE_VAR_NAME: ${!ENVSERVE_VAR_NAME}"
			envserve_raw_reply "${!ENVSERVE_VAR_NAME}"
			;;
		*)
			envserve_log error "Location not found: $ENVSERVE_LOCATION!"
			envserve_send_reply 404 "Resource not found: $ENVSERVE_LOCATION"
			return 1
			;;
	esac
}

envserve_handle_put()
{
	local ENVSERVE_LOCATION="$1"

	case "$ENVSERVE_LOCATION" in
		/shut-down)
			envserve_log info "Shutdown request received.  Terminating."
			envserve_exit 0
			;;
		*)
			envserve_log error "Location not found: $ENVSERVE_LOCATION!"
			envserve_send_reply 404 "Resource not found: $ENVSERVE_LOCATION"
			return 1
			;;
	esac
}


if [ ! -f "$ENVSERVE_PIPE_PATH" ]; then
	envserve_log info "Creating named pipe: $ENVSERVE_PIPE_PATH"
	: ${ENVSERVE_SOCKET_MODE:=0600}
	mkfifo --mode="$ENVSERVE_SOCKET_MODE" "$ENVSERVE_PIPE_PATH" || exit 1
	ENVSERVE_DELETE_PIPE_ON_EXIT=yes
fi

envserve_log info "Env server listening on: $ENVSERVE_PIPE_PATH"
while true; do
	read ENVSERVE_METHOD ENVSERVE_LOCATION < "$ENVSERVE_PIPE_PATH" || {
		envserve_log error "Failed to read request: $ENVSERVE_METHOD $ENVSERVE_LOCATION"
		envserve_send_reply 501 $(printf "Internal Server Error\\n\\nFailed to read request.\\n")
		continue
	}
	envserve_log info "$ENVSERVE_METHOD $ENVSERVE_LOCATION"
	
	case "$ENVSERVE_METHOD" in
		GET)
			envserve_handle_get "$ENVSERVE_LOCATION"
			;;
		PUT)
			envserve_handle_put "$ENVSERVE_LOCATION"
			;;
		*)
			envserve_log error "Unsupported method: $ENVSERVE_METHOD!"
			envserve_send_reply 405 "Unsupported method: $ENVSERVE_METHOD!"
			;;
	esac
done
