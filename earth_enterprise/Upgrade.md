# Upgrading Earth Enterprise Fusion and Server to 5.2.1

Upgrading to GEE 5.2.1 is supported from Earth Enterprise 5.1.3 and 5.2.0 versions.

## Upgrading GEE 5.2.0 to 5.2.1

1. Do not uninstall GEE 5.2.0. We recommend that you upgrade GEE 5.2.0 by simply installing GEE 5.2.1. Installing GEE 5.2.1 on top of GEE 5.2.0 will ensure that your postgreSQL databases are backed up and upgraded correctly to the new postgreSQL version used by GEE 5.2.1. [Install Fusion or Earth Server](https://github.com/google/earthenterprise/wiki/Install-Fusion-or-Earth-Server)

1. If you decide that you want to uninstall GEE 5.2.0 before installing GEE 5.2.1, first make sure to backup your postgreSQL databases. Please keep in mind that those backup, made by 5.2.0, would not be compatible with GEE 5.2.1 postgreSQL databases.

    * Create a backup folder: mkdir -p /tmp/MyBackupFolder
    * Make gepguser owner of the created folder: chown gepguser /tmp/MyBackupFolder
    * Dump postGreSQL databases using '/opt/google/bin/geresetpgdb' script. This script needs to be run under user 'gepguser'. 
    This can be achieved by switching to user 'gepguser' then executing: "/opt/google/bin/geresetpgdb backup /tmp/MyBackupFolder
