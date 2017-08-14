#! /bin/bash

INSTALL_DIR=/opt/google
INSTALL_BIN_DIR="$INSTALL_DIR/bin"

cd earthenterprise/earth_enterprise/src/portableserver/build || exit 1

function unpack_portable_server()
{
    for f in portableserver*.tar.gz; do
        if [ "$f" == "portableserver*.tar.gz" ]; then
            break
        fi
        tar xvz -C "$INSTALL_DIR" -f "$(pwd)/$f" || exit 1

        return 0
    done

    for f in portableserver*.zip; do
        unzip -d "$INSTALL_DIR" "$f" || exit 1

        return 0
    done
}


mkdir -p "$INSTALL_DIR"
unpack_portable_server

mkdir -p "$INSTALL_BIN_DIR"
cat >"$INSTALL_BIN_DIR/portableserver" <<EOF
#! /bin/bash

cd /opt/google/"${f%.tar.gz}"/server/
python portable_server.py
EOF

chmod 0755 "$INSTALL_BIN_DIR/portableserver"
