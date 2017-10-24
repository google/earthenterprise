#! /bin/bash

# This script sets up package repositories, reads secrets from a server,
# and obtains the Open GEE build enviroment.

# Make a build directory:
mkdir -p /root/opengee/build

cd /root/opengee/build


# Install the build environment:
if type subscription-manager >/dev/null 2>&1; then
    # We're on RHEL, we need to get secrets and register a Red Hat Yum
    # subscription.

    # Read variables from the secrets server:
    . /opt/tst-docker/bin/lib/env-client.sh
    for VAR in REDHAT_USERNAME REDHAT_PASSWORD; do
        envclient_get_variable \
            /var/opt/run/tst-docker/secrets/socket \
            /var/opt/run/tst-docker/secrets/socket.lock \
            "$VAR" "$VAR" || exit 1
    done

    # Shut down the secrets server:
    echo "PUT /shut-down" > /var/opt/run/tst-docker/secrets/socket

    #   We register the Docker image with the Red Hat repository provider (you
    # need to have a username and password) so we can install Yum packages.
    # We unregister when we finish, so we're not taking up a machine license.
    subscription-manager register \
            --username="${REDHAT_USERNAME}" --password="${REDHAT_PASSWORD}" \
            --auto-attach || exit 1
    /root/opengee/bin/get-gee-build-environment.sh -y --clone-repository=no ||
        {
            subscription-manager unregister
            exit 1
        }
    subscription-manager unregister
else
    # We don't need screts, shut down the secrets server:
    echo "PUT /shut-down" > /var/opt/run/tst-docker/secrets/socket

    /root/opengee/bin/get-gee-build-environment.sh -y --clone-repository=no
fi
