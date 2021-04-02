# Upgrading Earth Enterprise Fusion and Server to version 5.3.9

Upgrading to GEE 5.3.9 is supported from Earth Enterprise 5.2.x and above versions.

Do NOT uninstall GEE 5.2.x. We recommend that you upgrade GEE 5.2.x by simply
installing GEE 5.3.9. Installing GEE 5.3.9 on top of GEE 5.2.x will ensure that
your PostgreSQL databases are backed up and upgraded correctly to the new
PostgreSQL version used by GEE 5.3.9

Upgrade is supported using either scripts or RPMs:

## Upgrading with scripts

1. Build the installer from source by following the instructions here:
   [InstallFusion or Earth Server](https://github.com/google/earthenterprise/wiki/Install-Fusion-or-Earth-Server).

2. After building the installer, run the scripts to install fusion and earth server:

    ```bash
    cd earth_enterprise/src/installer
    sudo ./install_fusion.sh
    sudo ./install_server.sh
    ```

## Upgrading with RPM packages (Red Hat and CentOS only)

Alternatively, instead of using scripts, on Red Hat and CentOS, you can upgrade
GEE 5.2.x to GEE 5.3.9 using [RPM packages](https://github.com/google/earthenterprise/blob/master/earth_enterprise/BUILD_RPMS.md).

Upgrading from any previous version installed via RPM:

1. Install the RPM like normal (e.g. `sudo yum install opengee-*.rpm`) and the
   RPM will take care of the upgrade for you.

2. If your assetroot is not the default value (/gevol/assets), run the commands
   below with your assetroot and publishroot directory paths:

    ```bash
    assetroot=/path/to/assetroot
    publishroot=/path/to/publishroot

    sudo service gefusion stop ; sudo service geserver stop
    sudo /opt/google/bin/geupgradeassetroot --assetroot $assetroot
    sudo /opt/google/bin/geselectassetroot --assetroot $assetroot
    sudo /opt/google/bin/geconfigurepublishroot --path=$publishroot
    sudo service geserver start; sudo service gefusion start
    ```

    NOTE: these commands will not cause your assets to be rebuilt.

## Uninstall notes

Uninstall is NOT recommended before upgrading.

If you decide that you want to uninstall GEE 5.2.0 before installing GEE 5.3.9,
first make sure to backup your PostgreSQL databases. Please keep in mind that
the database backup, made by 5.2.0, would not be compatible with GEE 5.3.9
PostgreSQL databases.

    * Create a backup folder: `mkdir -p /tmp/MyBackupFolder`
    * Make `gepguser` owner of the created folder: `chown gepguser /tmp/MyBackupFolder`
    * Dump PostGreSQL databases using '/opt/google/bin/geresetpgdb' script. This
      script needs to be run under user `gepguser`. This can be achieved by
      switching to user `gepguser` then executing:
      `/opt/google/bin/geresetpgdb backup /tmp/MyBackupFolder`

## Database push compatibility between versions

GEE Server versions 5.2.x are compatible, meaning you can push databases from
any 5.2.x version of GEE Fusion to any 5.2.x version of GEE Server. This
compatibility extends to the disconnected send feature (see gedisconnectedsend
command documentation for more information).
