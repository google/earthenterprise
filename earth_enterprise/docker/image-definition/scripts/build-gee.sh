#! /bin/bash

# Whether to build packages (such as RPMs and Debs):
: ${BUILD_PACKAGES:=""}

# Whether to build RPM packages:
: ${BUILD_RPMS:=""}

# Whether to build Deb packages:
: ${BUILD_DEBS:=""}

# Whether to copy build packages outside the build directory:
: ${COPY_PACKAGES:=""}


while [ "$#" -ge 1 ]; do
    case "$1" in
        --packages)
            BUILD_PACKAGES="yes"
            ;;
        --no-packages)
            BUILD_PACKAGES=""
            ;;
        --rpms)
            BUILD_RPMS="yes"
            ;;
        --no-rpms)
            BUILD_RPMS=""
            ;;
        --debs)
            BUILD_DEBS="yes"
            ;;
        --no-debs)
            BUILD_DEBS=""
            ;;
        --copy-packages)
            COPY_PACKAGES="yes"
            ;;
        --no-copy-packages)
            COPY_PACKAGES=""
            ;;
        *)
            echo "$(basename "$0"): Unrecognized command line argument: $1"
            exit 1
            ;;
    esac
    shift
done

# How many jobs to run in parallel when building:
: ${JOB_COUNT:=$[ 2 * $(nproc) ]}


if [ -n "$BUILD_PACKAGES" ]; then
    if [ -z "$BUILD_RPMS" ] && type yum >/dev/null 2>&1; then
        BUILD_RPMS="yes"
    fi

    if [ -z "$BUILD_DEBS" ] && type apt-get >/dev/null 2>&1; then
        BUILD_DEBS="yes"
    fi
fi


cd earthenterprise/earth_enterprise || exit 1
scons "-j${JOB_COUNT}" release=1 build || exit 1

# Copy binaries to install package location (required for packaging):
scons "-j${JOB_COUNT}" release=1 stage_install || exit 1


if [ "$BUILD_RPMS" ]; then
    (
        cd rpms && ./gradlew openGeeRpms || exit 1
        if [ -n "$COPY_PACKAGES" ]; then
            mkdir -p /root/opengee/packages
            cp build/distributions/*.rpm /root/opengee/packages
        fi
    ) || exit 1
fi

if [ "$BUILD_DEBS" ]; then
    (
        cd rpms && ./gradlew openGeeDebs || exit 1
        if [ -n "$COPY_PACKAGES" ]; then
            mkdir -p /root/opengee/packages
            cp build/distributions/*.deb build/distributions/*.changes \
                /root/opengee/packages
        fi
    ) || exit 1
fi
