# Upgrading Earth Enterprise Fusion and Server to version 5.2.1

Upgrading to GEE 5.2.1 is supported from Earth Enterprise 5.2.0 version using the following steps:

1. Do NOT uninstall GEE 5.2.0. We recommend that you upgrade GEE 5.2.0 by simply installing GEE 5.2.1. Installing GEE 5.2.1 on top of GEE 5.2.0 will ensure that your postgreSQL databases are backed up and upgraded correctly to the new postgreSQL version used by GEE 5.2.1. [Install Fusion or Earth Server](https://github.com/google/earthenterprise/wiki/Install-Fusion-or-Earth-Server)

Alternatively on Redhat and Centos, you can use upgrade GEE 5.2.0 to GEE 5.2.1 using [RPM] (https://github.com/google/earthenterprise/blob/master/earth_enterprise/BUILD_RPMS.md)

1. If you decide that you want to uninstall GEE 5.2.0 before installing GEE 5.2.1, first make sure to backup your postgreSQL databases. Please keep in mind that those backup, made by 5.2.0, would not be compatible with GEE 5.2.1 postgreSQL databases.

    * Create a backup folder: mkdir -p /tmp/MyBackupFolder
    * Make gepguser owner of the created folder: chown gepguser /tmp/MyBackupFolder
    * Dump postGreSQL databases using '/opt/google/bin/geresetpgdb' script. This script needs to be run under user 'gepguser'. 
    This can be achieved by switching to user 'gepguser' then executing: "/opt/google/bin/geresetpgdb backup /tmp/MyBackupFolder
