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


if [ -n "$BUILD_PACKAGES" ]; then
    if [ -z "$BUILD_RPMS" ] && type yum >/dev/null 2>&1; then
        BUILD_RPMS="yes"
    fi

    if [ -z "$BUILD_DEBS" ] && type apt-get >/dev/null 2>&1; then
        BUILD_DEBS="yes"
    fi
fi


cd earthenterprise/earth_enterprise && \
scons -j8 release=1 build

if [ "$BUILD_RPMS" ]; then
    (
        cd rpms && ./gradlew openGeeRpms
        if [ -n "$COPY_PACKAGES" ]; then
            mkdir -p /root/opengee/packages
            cp *.rpm /root/opengee/packages
        fi
    )
fi

if [ "$BUILD_DEBS" ]; then
    (
        cd rpms && ./gradlew openGeeDebs
        if [ -n "$COPY_PACKAGES" ]; then
            mkdir -p /root/opengee/packages
            cp *.deb *.changes /root/opengee/packages
        fi
    )
fi
