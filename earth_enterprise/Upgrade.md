# Upgrading Earth Enterprise Fusion and Server to 5.2.1

Upgrading is currently supported from Earth Enterprise 5.1.3 and 5.2.0 versions.

## Upgrading GEE 5.2.0 to 5.2.1

1. Do not uninstall GEE 5.2.0. We recommend that you upgrade your GEE 5.2.0 by simply installing 5.2.1. If you decide
that you want to uninstall GEE 5.2.0, first make sure to backup your postgreSQL databases:

    Backup postGreSQL databases
        * Create a backup folder: mkdir -p SomeBackupFolder
        * Create a backup folder and make gepguser owner: chown gepguser SomeBackupFolder
        * Dump postGreSQL databases using '/opt/google/bin/geresetpgdb' script: run_as_user gepguser "/opt/google/bin/geresetpgdb backup     
          SomeBackupFolder

1. Installing GEE 5.2.1 on top of GEE 5.2.0 will ensure that your postgreSQL databases are backedup and upgraded to
the new postgreSQL version used by GEE 5.2.1.

1. [Install Fusion or Earth Server](https://github.com/google/earthenterprise/wiki/Install-Fusion-or-Earth-Server)

