# Source this file to get the functions defined below, or call with
# command-line arguments.

envclient_get_variable()
{
    local ENVCLIENT_SOCKET_PATH="$1"
    local ENVCLIENT_LOCK_PATH="$2"
    local ENVCLIENT_VARIABLE_NAME="$3"
    # WARNING: Don't use an output variable name starting with "ENVCLIENT_",
    # it may clash with a local, or global variable:
    local ENVCLIENT_OUTPUT_VARIABLE_NAME="$4"

    local ENVCLIENT_VALUE=$(
        (
            flock -x 9
            sleep 0.25
            echo "GET /var/$ENVCLIENT_VARIABLE_NAME" > "$ENVCLIENT_SOCKET_PATH"
            # Add a "." at the end to prevent final new-line stripping:
            cat "$ENVCLIENT_SOCKET_PATH" && echo .
            rm -f "$ENVCLIENT_LOCK_PATH"
        ) 9>"$ENVCLIENT_LOCK_PATH"
    )

    # Remove the added "." suffix:
    ENVCLIENT_VALUE=${ENVCLIENT_VALUE%.}
    # Put the value in a variable:
    printf -v "$ENVCLIENT_OUTPUT_VARIABLE_NAME" "%s" "$ENVCLIENT_VALUE"
}

envclient_main()
{
    local ENVCLIENT_SOCKET_PATH
    local ENVCLIENT_LOCK_PATH
    local ENVCLIENT_VARIABLE_NAME

    while [ "$#" -gt 0 ]; do
        case "$1" in
            -s|--socket-path)
                ENVCLIENT_SOCKET_PATH="$2"
                shift
                ;;
            -l|--lock-path)
                ENVCLIENT_LOCK_PATH="$2"
                shift
                ;;
            -n|--variable-name)
                ENVCLIENT_VARIABLE_NAME="$2"
                shift
                ;;
            *)
                ENVCLIENT_VARIABLE_NAME="$1"
                ;;
        esac
        shift
    done

    : ${ENVCLIENT_LOCK_PATH:="$ENVCLIENT_SOCKET_PATH.lock"}

    if [ -n "$ENVCLIENT_SOCKET_PATH" -a -n "$ENVCLIENT_VARIABLE_NAME" ]
    then
        envclient_get_variable \
            "$ENVCLIENT_SOCKET_PATH" "$ENVCLIENT_LOCK_PATH" \
            "$ENVCLIENT_VARIABLE_NAME" ENVCLIENT_VARIABLE_VALUE
        echo "$ENVCLIENT_VARIABLE_VALUE"
    else
        echo "Both socket-path and variable-name need to be set to fetch a variable!" >&2
    fi
}

if [ "$#" -gt 1 ]; then
    envclient_main $@
fi
