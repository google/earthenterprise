#! /bin/bash

# Run this script to start Earth Server, and Fusion in the Docker container.

# Set these variables before running this script to override the
# default values below:
: ${FIX_ASSET_HOSTNAME:=true}


while [[ "$#" -gt 1 ]]; do
    case "$1" in
        -t|--temporary)
            TEMPORARY_SERVERS="true"
            ;;
        *)
            echo "Unrecognized command-line argument: $1" >&2
            exit 1
            ;;
    esac
    shift
done

if [ -n "$TEMPORARY_SERVERS" ]; then
    echo "*******************************************************************************"
    echo "* Warning: This is a temporary server!                                        *"
    echo "*          Any globes you build and publish, and files modifications you make *"
    echo "*          will be lost when the Docker continer is stopped!                  *"
    echo "*******************************************************************************"
    echo

    echo "Re-creating the Fusion asset root."
    (
        echo y  # create a new asset root
        echo y  # create source volume
        echo    # directory for source volume: [/gevol/src]
        echo n  # add more volumes
    ) | \
        /opt/google/bin/geconfigureassetroot --new --assetroot /gevol/assets/
fi

if [ -n "$FIX_ASSET_HOSTNAME" ]; then
    /opt/google/bin/geconfigureassetroot --fixmasterhost
fi

# Start services:
/etc/init.d/geserver start && \
/etc/init.d/gefusion start && \
/bin/bash
