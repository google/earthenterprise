# Upgrading Earth Enterprise Fusion and Server to version 5.2.1

Upgrading to GEE 5.2.1 is supported from Earth Enterprise 5.2.0 version using the following steps:

1. Do NOT uninstall GEE 5.2.0. We recommend that you upgrade GEE 5.2.0 by simply installing GEE 5.2.1. Installing GEE 5.2.1 on top of GEE 5.2.0 will ensure that your PostgreSQL databases are backed up and upgraded correctly to the new PostgreSQL version used by GEE 5.2.1. [Install Fusion or Earth Server](https://github.com/google/earthenterprise/wiki/Install-Fusion-or-Earth-Server). This can be achieved by running the scripts to install fusion and earth server:

```bash
cd earth_enterprise/src/installer
sudo ./install_fusion.sh 
sudo ./install_server.sh
```

Alternatively, instead of using scripts, on Redhat and Centos, you can upgrade GEE 5.2.0 to GEE 5.2.1 using [RPM packages] (https://github.com/google/earthenterprise/blob/master/earth_enterprise/BUILD_RPMS.md)

1. If you decide that you want to uninstall GEE 5.2.0 before installing GEE 5.2.1, first make sure to backup your PostgreSQL databases. Please keep in mind that the database backup, made by 5.2.0, would not be compatible with GEE 5.2.1 PostgreSQL databases.

    * Create a backup folder: `mkdir -p /tmp/MyBackupFolder`
    * Make gepguser owner of the created folder: `chown gepguser /tmp/MyBackupFolder`
    * Dump PostGreSQL databases using '/opt/google/bin/geresetpgdb' script. This script needs to be run under user 'gepguser'. 
    This can be achieved by switching to user 'gepguser' then executing: `"/opt/google/bin/geresetpgdb backup /tmp/MyBackupFolder`
