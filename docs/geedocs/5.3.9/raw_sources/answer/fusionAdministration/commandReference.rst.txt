|Google logo|

=================
Command reference
=================

.. container::

   .. container:: content

      This article describes all of the command line tools used for
      system administration in alphabetical order. If you prefer, you
      can find each tool’s syntax by entering the name of the tool on
      the command line with the ``--help`` option, for example:

      ``geserveradmin --help``

      This article uses the following typographic conventions:

      =========================================================================================== ==============================================================
      *Italic*                                                                                    Information that the user must supply
      **Bold**                                                                                    Text that the user must type exactly as shown
      Ellipsis **...**                                                                            Argument that can be repeated several times in a command
      Square brackets **[ ]**                                                                     Optional commands or arguments
      Curly braces **{ }** with options separated by pipes **\|**; for example: **{even \| odd}** Lists a set of choices from which the user can select only one
      Parentheses **( )**                                                                         Grouped items that function together
      =========================================================================================== ==============================================================

      When using ``insetresource`` in imagery and terrain project
      commands below, the passed inset resource needs to be a relative
      path to GEE assets folder.

      .. rubric:: geaddtoimageryproject
         :name: geaddtoimageryproject

      **geaddtoimageryproject** [-\\-**mercator** | -\\-**flat**] [-\\-**historical_imagery** | -\\-**no_historical_imagery**] -o *projectname* {[-\\-**maxlevel** *level* | -\\-**maxleveloverride** *level*] *insetresource*}...

      .. rubric:: Purpose
         :name: purpose

      Adds additional resources to an existing imagery project. This
      tool is capable of building Mercator imagery projects for 2D
      databases, or Flat (Plate Carrée) imagery projects with or without
      Historic Imagery Support for 3D databases.

      .. rubric:: Commands
         :name: commands

      -\\-**mercator**

      *Optional*. Uses Mercator map projection for the imagery project.

      -\\-**flat**

      *Default*. Uses Flat map (Plate Carrée) projection.

      -\\-**historical_imagery**

      *Optional*. Uses historical imagery for the project..

      -\\-**no_historical_imagery**

      *Default*. Uses normal imagery for the project.

      .. rubric:: Options
         :name: options

      -\\-**maxlevel** *level*

      .. warning::

         Deprecated in release GEE 5.2.5 and higher. Use
         ``--maxleveloverride`` instead.

      *Optional*. Sets the maximum level for the imagery. Uses internal
      Fusion scale. Deprecated because it does not match the levels specified
      in the Fusion UI.

      -\\-**maxleveloverride** *level*

      *Optional*. Sets the maximum level for the imagery. Uses the
      imagery scale in Fusion. Matches the levels in the Fusion UI.

      .. rubric:: geaddtoterrainproject
         :name: geaddtoterrainproject

      **geaddtoterrainproject** [-\\-**mercator** | -\\-**flat**] -o *projectname* {--**maxlevel** *level* | -\\-**maxleveloverride** *level*]} [-\\-**no_terrain_overlay** | {-\\-**terrain_overlay** -\\-**start_level** *level* -\\-**resource_min_level** *level* }] *insetresource*} ...

      .. rubric:: Purpose

      Adds additional resources to an existing terrain project. This
      tool is capable of building Mercator terrain projects for 2D
      databases or Flat (Plate Carrée) terrain projects.

      .. rubric:: Commands

      -\\-**mercator**

      *Optional*. Uses Mercator map projection for the terrain project.

      -\\-**flat**

      *Default*. Uses Flat map (Plate Carrée) projection.

      -\\-**maxlevel** *level*

      .. warning::

         Deprecated in release GEE 5.2.5 and higher. Use
         ``--maxleveloverride`` instead.

      *Optional*. Sets the maximum level for the terrain. Uses internal
      Fusion scale. Deprecated because it does not match the levels specified
      in the Fusion UI.

      -\\-**maxleveloverride** *level*

      *Optional*. Sets the maximum level for the terrain. Uses the
      terrain scale in Fusion. Matches the levels in the Fusion UI.

      -\\-**no_terrain_overlay**

      *Default*. make this terrain project a normal project.

      -\\-**terrain_overlay**

      *Optional*. make this terrain project an overlay project.

      -\\-**start_level** *level*

      *Optional*. the level from which to start building the terrain
      overlay project. start_level is an even integer between 4 and 24
      inclusive.

      -\\-**resource_min_level** *level*

      *Optional*. the threshold level that separates fill terrain from
      overlay terrain. resource_min_level is any integer between 4 and
      24 inclusive.

      .. _geconfigassetroot:
      .. rubric:: geconfigureassetroot

      **geconfigureassetroot** {-\\-**new** -\\-**assetroot path**  [-\\-**srcvol** *path*] | -\\-**repair** | -\\-**editvolumes** | -\\-**listvolumes** | -\\-**addvolume** | -\\-**fixmasterhost** | -\\-**noprompt**}  [-\\-*chown*]

      .. rubric:: Purpose

      To add volume definitions or edit existing volume definitions.

      .. tip::

         You must run this command as root. Except for the
         **-\\-listvolumes** command, you must stop the fusion service
         before using this command and then start it again after you are
         done.

      .. rubric:: Example

      .. code-block:: none

         geconfigureassetroot --new --assetroot /gevol/assets
         geconfigureassetroot --new --assetroot /gevol/assets --srcvol /data1/src
         geconfigureassetroot --repair
         geconfigureassetroot --editvolumes

      .. rubric:: Options

      -\\-**assetroot** *path*

      Path to asset root. This option is mandatory or optional in the
      ``geconfigureassetroot`` commands. If optional, then the current
      asset root is used if it is not specified.

      -\\-**noprompt**

      *Optional*. Perform the command without prompting the user for any
      input. This option requires that some commands have arguments
      specified on the command line.

      -\\-**chown**

      *Optional*. Attempt to change file owner and permissions.  This is
      only needed when the asset root has been created or modified by users
      other than the default Fusion user.

      .. rubric:: Commands

      -\\-**new** -\\-**assetroot** *path*

      *Optional*. Creates a new asset root. Specify the path to the new
      asset root.

      .. note::

         If you omit the path, the system creates a new asset
         root in ``/gevol/assets``.

      -\\-**srcvol** *path*

      *Optional*. Specify the path to the source volume.

      -\\-**repair** [-\\-assetroot *path*]

      *Optional*. Repairs various inconsistencies in the asset root
      (such as permissions, ownership, missing ID files, etc.).
      When you run this command, the tool auto-detects the problems that
      need to be repaired and fixes them.

      .. warning::

         Do not use this command unless you see a system message
         instructing you to do so.

      -\\-**editvolumes** [-\\-assetroot *path*]

      *Optional*. Follow the prompts to add a volume to the selected
      asset root or, modify the ``localpath`` definition for an existing
      volume, or to add a volume definition.

      -\\-**listvolumes** [-\\-assetroot *path*]

      *Optional*. List the available (configured) volumes for the
      selected asset root.

      -\\-**fixmasterhost** [-\\-assetroot *path*]

      *Optional*. Change the *assetroot host* entry to match the current
      host name. (This command corrects cases where a host name is
      changed after installing and configuring Google Earth Enterprise
      Fusion.)

      -\\-**addvolume** *volume_name:path*]

      *Optional*. Change the *assetroot host* entry to match the current
      host name. (This command corrects cases where a host name is
      changed after installing and configuring Google Earth Enterprise
      Fusion.)

      .. _Configure_Publish_Root_Different:
      .. rubric:: geconfigurepublishroot
         :name: geconfigurepublishroot

      geconfigurepublishroot [-\\-path=*path*] [-\\-allow_symlinks] [-\\-noprompt] [-\\-chown]

      .. rubric:: Purpose

      To specify the path where you want to push databases for
      publishing and serving with the current Google Earth Enterprise
      Server. Follow the prompts.

      .. note::

         You must run this command as root.

      .. rubric:: Example

      geconfigurepublishroot -\\-path /gevol/published_dbs -\\-allow_symlinks

      .. rubric:: Commands

      -\\-path=*path*

      *Optional*. The path to the publish root. Default value is
      ``/gevol/published_dbs``.

      -\\-allow_symlinks

      *Optional*. Configures the publisher to accept symbolic links.
      Useful when the publish root is on a separate logical volume from
      the asset root. Default is no.

      -\\-noprompt

      *Optional*. Perform the command without prompting the user for any
      input. This option requires that some commands have arguments
      specified on the command line. If the arguments are insufficient
      or the configuration fails, the program will return -1 (0 is
      returned on success).

      -\\-chown

      *Optional*. Correct permissions and ownership of the publish root.
      This is needed when the publish root was created or modified by a user
      other than the default Apache user, such as when re-using a publish root
      from a previously uninstalled version of GEE.

      .. warning::

         For large publish roots the -\\-chown option can significantly increase
         the running time of geconfigurepublishroot.
         
      .. warning::

         Do not create more than one publish root for a single asset
         root. That configuration produces unpredictable or undesirable
         results, including the inability to push at all from that asset
         root. You cannot push the same database multiple times to
         different publish roots on the same server.

      .. rubric:: gecutter
         :name: gecutter

      **gecutter** {**enable** | **disable**}

      .. rubric:: Purpose

      To enable and disable the Cutter tool. Once you have enabled the
      Cutter, you launch it from the Settings menu in the GEE Server
      admin console. You can also launch the Cutter directly from
      ``http://myserver.com/cutter``.

      .. note::

         The default admin security does not apply to the
         Cutter, so although it provides security if you try to launch
         the Cutter from the Admin console Settings menu, it does not
         block direct access to the Cutter via the URL. If you need
         Cutter security, you must add it separately. See
         :doc:`../geeServerConfigAndSecurity/ports`.

      See :doc:`../geeServerAdmin/createPortableGlobesMaps`.

      .. rubric:: Example

      .. code-block:: none

         gecutter enable

         gecutter disable

      .. rubric:: gedisconnectedclean
         :name: gedisconnectedclean

      .. warning::

         Deprecated in release GEE 4.4 and higher.

      **gedisconnectedclean** [-\\-**dbpath** *dbpath*] [-\\-**list** *assetroot*]

      .. rubric:: Purpose

      To clean a disconnected database from a disconnected mock asset
      root.

      .. rubric:: Example

      gedisconnectedclean -\\-dbpath /gevol/assets/Databases/MyPOIs.kdatabase

      .. rubric:: Commands

      -\\-**dbpath** *dbpath*

      *Required*. Specify the database path to clean. This must be a
      low-level path to a database directory (one of the entries in the
      ``assetroot/dbpaths.list`` file). See ``--list`` command option to
      find databases stored within the mock asset root.

      -\\-**list** *assetroot*

      *Optional*. List all dbpaths currently in disconnected asset root

      .. rubric:: gedisconnectedpublish
         :name: gedisconnectedpublish

      .. warning::

         Deprecated in release GEE 4.4 and higher. Use
         :ref:`geserveradmin --publishdb <Publish_DB_Disconnected>` instead.

      **gedisconnectedpublish** [*db_alias*] *db_name*

      .. rubric:: Purpose

      To publish a database on a disconnected server.

      .. rubric:: Example

      gedisconnectedpublish MyPOIs

      .. rubric:: Commands

      *db_alias*

      *Optional*. Since *db_name* is the “low-level” name of the
      database, *db_alias* allows you to enter a name that is easier to
      remember, for example, ``Databases/SF Highways.kdabase?ver=1``.

      *db_name*

      *Required*. The full, “low-level” name of the database you want to
      publish.

      .. rubric:: gedisconnectedreceive
         :name: gedisconnectedreceive

      .. warning::

         Deprecated in version 4.0. ``gedisconnectedreceive`` is
         required only when the disconnected database was sent with an
         older (pre 4.0) version of Fusion.

      **gedisconnectedreceive** -\\-**input** *dirname*

      .. rubric:: Purpose

      To copy a disconnected database from either detachable media or
      local storage into the mock asset root.

      .. rubric:: Example

      For detachable media:

      gedisconnectedreceive -\\-input /mnt/usbdrive/SFHighways_3dDatabase_v20

      For local storage:

      .. code-block:: none

         gedisconnectedreceive --input
         /gevol/src/disconnected_databases/SFHighways_3dDatabase_v20

      .. rubric:: Commands

      -\\-**input** *dirname*

      *Required*. Specify the directory that contains the files to be
      copied. This is typically the mount point of a hard drive.

      .. note::

         The ``gedisconnectedreceive`` command will create an asset tree
         that mirrors the asset tree of the Fusion system that built the
         database.

         The ``gedisconnectedreceive`` command will copy data to the mock
         asset root if the input folder is on a separate volume than the
         mock asset root. Links to the input folder to the mock asset
         root will be created if both the input and mock asset root
         folders on the same volume.

      .. rubric:: gedisconnectedsend
         :name: gedisconnectedsend

      **gedisconnectedsend** [-\\-**extra** *filename*] [-\\-**havepath** *dbpath*] [-\\-**havepathfile** *file*] -\\-**output** *dirname* [-\\-**sendpath** *dbpath*] [-\\-**sendver** *dbver*]

      .. rubric:: Purpose

      To gather all the files from a Fusion asset root necessary for a
      disconnected push/publish, for either publishing new databases or
      publishing "delta" updates.

      .. rubric:: Example

      .. code-block:: none

         gedisconnectedsend --sendver Databases/SFHighways.kdatabase?version=2 --output /gevol/src/disconnected_databases/SFHighways_3dDatabase_v2

      .. rubric:: Commands

      -\\-extra filename

      *Optional*. Specify an extra file to package. This is typically
      used to repair broken files.

      -\\-havepath dbpath

      *Optional*. Specify which database path already exists on the
      target server. This must be a low-level path to a database
      directory and may be specified more than once.

      -\\-havepathfile file

      *Optional*. Specify the file that contains the list of existing
      database paths (copy of *assetroot*\ ``/dbpaths.list`` from the
      remote server).

      -\\-output dirname

      *Required*. Specify where to gather the files. The directory must
      already exist and be empty. This is typically the mount point of
      a hard drive.

      -\\-sendpath dbpath

      *Optional*. Specify which database path to send. This must be a
      low-level path to a database directory. You can determine this
      path by entering ``gequery --outfiles``\ *dbver* on the source
      server.

      -\\-sendver dbver

      *Optional*. Specify which database version to send. Use the
      ``?version=...`` syntax. Available database versions may be found
      with the ``gequery --versions`` command.

      .. rubric:: genewmapdatabase

      **genewmapdatabase** [-\\-**mercator** | -\\-**flat**] ] -o *databasename* [-\\-**imagery** *imagery project*] [-\\-**map** *imap project*]...

      .. rubric:: Purpose

      Creates a new 2D map database. If an imagery or map project is
      specified, it is added to the database. Flat or mercator databases
      can be created. Mercator databases can use either mercator or flat
      imagery projects, with mercator projects given priority if there
      is a naming collision. Flat databases can only use flat imagery
      projects.

      .. rubric:: Commands

      -\\-mercator

      *Optional*. Uses Mercator map projection.

      -\\-flat

      *Default*. Uses Flat map (Plate Carrée) projection.

      -\\-imagery imagery project

      *Optional*. The imagery project to be added to the database. If
      the database is mercator, the imagery project can be flat or
      mercator, with mercator being given priority during naming
      collisions. If the database is flat, the imagery project must be
      flat.

      -\\-**map** map project

      *Optional*. The map project to be added to the database.

      .. rubric:: gemodifyimageryproject
         :name: gemodifyimageryproject

      **gemodifyimageryproject** [-\\-**mercator** | -\\-**flat**] [-\\-**historical_imagery** | -\\-**no_historical_imagery**] -o *projectname* {[-\\-**maxlevel** *level* | -\\-**maxleveloverride** *level*] *insetresource*}...

      .. rubric:: Purpose

      Modifies an existing imagery project.

      .. rubric:: Commands

      -\\-**mercator**

      *Optional*. Uses Mercator map projection for the imagery project.

      -\\-**flat**

      *Default*. Uses Flat map (Plate Carrée) projection.

      -\\-**historical_imagery**

      *Optional*. Uses historical imagery for the project.

      -\\-**no_historical_imagery**

      *Default*. Uses normal imagery for the project.

      .. rubric:: Options

      -\\-**maxlevel** *level*

      .. warning::

         Deprecated in release GEE 5.2.5 and higher. Use
         ``--maxleveloverride`` instead.

      *Optional*. Sets the maximum level for the imagery. Uses internal
      Fusion scale. Deprecated because it does not match the levels specified
      in the Fusion UI.

      -\\-**maxleveloverride** *level*

      *Optional*. Sets the maximum level for the imagery. Uses the
      imagery scale in Fusion. Matches the levels in the Fusion UI.

      .. rubric:: gemodifyterrainproject
         :name: gemodifyterrainproject

      **gemodifyterrainproject** [-\\-**mercator** | -\\-**flat**] -o *projectname* {-\\-**maxlevel** *level* | -\\-**maxleveloverride** *level*]} [-\-**no_terrain_overlay** | {-\\-**terrain_overlay** -\\-**start_level** *level* -\\-**resource_min_level** *level* }] *insetresource*} ...

      .. rubric:: Purpose

      Modifies an existing terrain project.

      .. rubric:: Commands

      -\\-**mercator**

      *Optional*. Uses Mercator map projection for the terrain project.

      -\\-**flat**

      *Default*. Uses Flat map (Plate Carrée) projection.

      -\\-**maxlevel** *level*

      .. warning::

         Deprecated in release GEE 5.2.5 and higher. Use
         ``--maxleveloverride`` instead.

      *Optional*. Sets the maximum level for the terrain. Uses internal
      Fusion scale. Deprecated because it does not match the levels specified
      in the Fusion UI.

      -\\-**maxleveloverride** *level*

      *Optional*. Sets the maximum level for the terrain. Uses the
      terrain scale in Fusion. Matches the levels in the Fusion UI.

      *Optional*.

      -\\-**no_terrain_overlay**

      *Default*. make this terrain project a normal project.

      -\\-**terrain_overlay**

      *Optional*. make this terrain project an overlay project.

      -\\-**start_level** *level*

      *Optional*. the level from which to start building the terrain
      overlay project. start_level is an even integer between 4 and 24
      inclusive.

      -\\-**resource_min_level** *level*

      *Optional*. the threshold level that separates fill terrain from
      overlay terrain. resource_min_level is any integer between 4 and
      24 inclusive.

      .. rubric:: genewimageryproject
         :name: genewimageryproject

      **genewimageryproject** [-\\-**mercator** | -\\-**flat**] [-\\-**historical_imagery** | -\\-**no_historical_imagery**] -o *projectname* {[-\\-**maxlevel** *level* | -\\-**maxleveloverride** *level*] *insetresource*}...

      .. rubric:: Purpose

      Creates a new imagery project.

      .. rubric:: Commands

      -\\-**mercator**

      *Optional*. Uses Mercator map projection for the imagery project.

      -\\-**flat**

      *Default*. Uses Flat map (Plate Carrée) projection.

      -\\-**historical_imagery**

      *Optional*. Uses historical imagery for the project.

      -\\-**no_historical_imagery**

      *Default*. Uses normal imagery for the project.

      .. rubric:: Options

      -\\-**maxlevel** *level*

      .. warning::

         Deprecated in release GEE 5.2.5 and higher. Use
         ``--maxleveloverride`` instead.

      *Optional*. Sets the maximum level for the imagery. Uses internal
      Fusion scale. Deprecated because it not match the levels specified
      in the Fusion UI.

      -\\-**maxleveloverride** *level*

      *Optional*. Sets the maximum level for the imagery. Uses the
      imagery scale in Fusion. Matches the levels in the Fusion UI.

      .. rubric:: genewterrainproject
         :name: genewterrainproject

      **genewterrainproject** [-\\-**mercator** | -\\-**flat**] -o *projectname* {-\\-**maxlevel** *level* | -\\-**maxleveloverride** *level*]} [-\\-**no_terrain_overlay** | {-\\-**terrain_overlay** -\\-**start_level** *level* -\\-**resource_min_level** *level* }] *insetresource*} ...

      .. rubric:: Purpose

      Creates a new terrain project.

      .. rubric:: Commands

      -\\-**mercator**

      *Optional*. Uses Mercator map projection for the terrain project.

      -\\-**flat**

      *Default*. Uses Flat map (Plate Carrée) projection.

      -\\-**maxlevel** *level*

      .. warning::

         Deprecated in release GEE 5.2.5 and higher. Use
         ``--maxleveloverride`` instead.

      *Optional*. Sets the maximum level for the terrain. Uses internal
      Fusion scale. Deprecated because it does not match the levels specified
      in the Fusion UI.

      -\\-**maxleveloverride** *level*

      *Optional*. Sets the maximum level for the terrain. Uses the
      terrain scale in Fusion. Matches the levels in the Fusion UI.

      *Optional*.

      -\\-**no_terrain_overlay**

      *Default*. make this terrain project a normal project.

      -\\-**terrain_overlay**

      *Optional*. make this terrain project an overlay project.

      -\\-**start_level** *level*

      *Optional*. the level from which to start building the terrain
      overlay project. start_level is an even integer between 4 and 24
      inclusive.

      -\\-**resource_min_level** *level*

      *Optional*. the threshold level that separates fill terrain from
      overlay terrain. resource_min_level is any integer between 4 and
      24 inclusive.

      .. rubric:: gepublishdatabase
         :name: gepublishdatabase

      .. warning::

         Deprecated in GEE 4.0.

      Use ``geserveradmin`` to push and publish databases or use the
      Fusion GUI and :doc:`GEE Server <../geeServerAdmin/publishDatabasesPortables>`.

      .. rubric:: geselectassetroot

      **geselectassetroot** [-\\-**lock**] [-\\-**noprompt**] [-\\-**unlock**] ( [-\\-**assetroot** *path* [-\\-**role** {**master** | **slave**}] [-\\-**numcpus** *num*]] )

      .. rubric:: Purpose

      To perform a variety of operations related to existing asset roots
      on the current machine.

      .. tip::

         You must stop the system manager before using this command and
         then start it again after you are done. You must also run this
         command as root.

      .. rubric:: Example

      .. code-block:: none

         geselectassetroot --list
         geselectassetroot --lock
         geselectassetroot --unlock
         geselectassetroot --assetroot /gevol/assets
         geselectassetroot --assetroot /gevol/assets --role slave --numcpus 3

      .. rubric:: Options

      -\\-**assetroot <dir>**

      Path to the asset root. ``--assetroot`` is shown in the commands
      below as mandatory or optional. If optional, then the current
      asset root is used if it is not specified.

      -\\-**noprompt**

      Do not prompt for more information, returns -1 to indicate an error
      if command fails or has insufficient arguments.

      .. rubric:: Commands

      -\\-**list**

      *Optional.* Displays a list of the known asset roots on this
      machine.

      -\\-**lock**

      *Optional.* Disables the ability to select a different asset root
      on this machine.

      -\\-**noprompt**

      *Optional*. Perform the command without prompting the user for any
      input. This option requires that some commands have arguments
      specified on the command line.

      -\\-**unlock**

      *Optional.* Enables the ability to select a different asset root
      on this machine. (Use only if ``--lock`` is enabled.)

      -\\-**assetroot** *path*

      *Optional.* Specify the path to the asset root for this machine.

      -\\-**role** {**master** | **slave**}

      *Optional.* Specify this machine's role in the asset root (master
      or slave). The default role is master. This command is available
      only in combination with ``--assetroot``.

      -\\-**numcpus** *num*

      *Optional.* Specify the number of CPUs on this machine to use for
      processing. The default will be the maximum number of CPUs
      detected on the machine during installation. This command is
      available only in combination with ``--assetroot``.

      .. rubric:: geselectpublishroot
         :name: geselectpublishroot

      geselectpublishroot path

      .. rubric:: Purpose

      To specify a different publish root. The specified path must
      exist. If you want to create a publish root, see
      :ref:`geconfigurepublishroot <Configure_Publish_Root_Different>`.

      .. rubric:: Example

      ``geselectpublishroot /gevol/published_dbs``

      .. rubric:: Arguments
         :name: arguments

      *path*

      *Required*. Specify the path to the desired publish root.

      .. rubric:: geserveradmin
         :name: geserveradmin

      **geserveradmin** [*options*] *commands*

      .. rubric:: Purpose

      To configure your Google Earth Enterprise Server. This section
      breaks down the ``geserveradmin`` commands into the following
      categories:

      -  Options
      -  Database
      -  Virtual host
      -  Admin

      All of the commands of each type are described below. At least one
      command is required.

      .. rubric:: Examples
         :name: examples

      .. code-block:: none

         geserveradmin --listdbs
         geserveradmin --server_type stream --dbdetails “/gevol/assets/Databases/SF Neighborhoods.kdatabase/gedb.kda/ver001/gedb”
         geserveradmin --addvh my_public_vh --vhurl http://myserver.com/public_vh
         geserveradmin --deletevh my_public_vh
         geserveradmin --deletedb
         geserveradmin --garbagecollect

      .. rubric:: geserveradmin command options
         :name: geserveradmin-command-options

      .. rubric:: Fusion host name
         :name: fusion-host-name

      -\\-**fusion_host**

      *Optional*. Fusion host name. Defaults to the current host name.

      .. rubric:: Stream server URL
         :name: stream-server-url

      -\\-**stream_server_url** *url*

      *Optional*. Specify a stream server other than the default.
      Defaults to the current server.

      -\\-**search_server_url** *url*

      .. warning::

         Deprecated. Always specify a stream server.

      .. rubric:: Server type
         :name: server-type

      -\\-**server_type** {**stream** | **search**}

      *Optional*. Specify whether the server(s) in question are
      ``stream`` or ``search`` server(s). The default is ``stream``.
      This option is required with the ``listdbs``, ``dbdetails``, and
      ``garbagecollect`` commands.

      .. rubric:: geserveradmin Database Commands
         :name: geserveradmin-database-commands

      Each of the database commands is listed below, along with its
      syntax, description, and options. If the name of the database
      contains one or more spaces, double quote the entire path. (See
      the examples above.)

      .. rubric:: List registered databases
         :name: list-registered-databases

      -\\-**listdbs**  [-\\-**portable**]

      Lists all databases registered on the server. If ``--portable`` is
      specified, only portable databases are listed.

      .. rubric:: Database file list
         :name: database-file-list

      -\\-**dbdetails** *db_name*

      Provides a list of all of the files required by the specified
      database. If omitted, the server type defaults to ``stream``.

      .. rubric:: List published databases
         :name: list-published-databases

      -\\-**publisheddbs** [-\\-**portable**]

      Lists the database(s) currently published on the server. If
      ``--portable`` is specified, only portable databases are listed.

      .. rubric:: List target paths
         :name: list-target-paths

      -\\-**listtgs**

      Lists all the target paths currently serving databases on the
      server.

      .. rubric:: Add database
         :name: add-database

      -\\-**adddb** *db_name* [-\\-dbalias *alias*]

      Registers a new database with the specified name.

      .. list-table:: adddb option
         :widths: 30 30 30
         :header-rows: 1

         * - -\\-adddb option
           - Required/Optional
           - Description
         * - -\\-dbalias alias
           - Optional
           - Specifies a user-friendly name for the database.

      .. rubric:: Delete database

      -\\-**deletedb** *db_name*

      Deletes the specified database entry from the server. Does not
      delete the actual files. (This command is similar to putting files
      in the trash on a Windows or Mac desktop. See also
      ``--garbagecollect``.)

      .. note::

         If you want to delete a currently published database, you
         first need to unpublish. (See also ``--unpublish``.) To list
         the currently published databases, use the ``--publisheddbs``
         option. (See also ``--deletevh``.)

      .. rubric:: Push databases
         :name: push-databases

      -\\-**pushdb** *db_name*... [-\\-**force_copy**]

      Pushes one or more databases to the server. For example,
      ``--pushdb db1 --pushdb db2``

      .. list-table:: pushdb option
         :widths: 30 15 55
         :header-rows: 1

         * - -\\-pushdb option
           - Required/Optional
           - Description
         * - -\\-**force_copy**
           - Optional
           - Copies database files while pushing/publishing;
             otherwise creates a hard/symbolic link when server
             settings allow. To allow symbolic links, specify
             using ``geconfigurepublishroot``: ``sudo /opt/google/bin/geconfigurepublishroot
             -path=/gevol/published_dbs -allow_symlinks``.

      .. _Publish_DB_Disconnected:
      .. rubric:: Publish database
         :name: publish-database

      -\\-**publishdb** *db_name* -\\-**targetpath** *target_path* [-\\-**vhname** *vh_name*] [-\\-**setecdefault**] [-\\-**enable_poisearch** [-\\-**enable_enhancedsearch**]]

      Publish the specified database on the specified target path. If
      the virtual host name is omitted, it publishes to the default
      virtual host: "public".

      .. list-table:: publishdb option
         :widths: 30 15 55
         :header-rows: 1

         * - -\\-publishdb option
           - Required/Optional
           - Description
         * - -\\-**targetpath** *target_path*
           - Required
           - Specifies the target path on which to publish.
         * - -\\-**vhname** *vh_name*
           - Optional
           - Specify the name of the virtual host. If the virtual host name is omitted,
             it publishes to the default virtual host, “public”.
         * - -\\-**setecdefault**
           - Optional
           - Publish this database as the default for the Earth Client to connect
             to if no database or virtual host is specified upon initial connection.
         * - -\\-**enable_poisearch**
           - Optional
           - Enable Point of Interest search if database contains POI data.
         * - -\\-**enable_enhancedsearch**
           - Optional
           - If POI search is enabled, enable enhanced search.

      .. rubric:: Unpublish database
         :name: unpublish-database

      -\\-**unpublish** *target_path*

      Unpublish database served from specified target path. For example,
      to unpublih a target path ``/test``:

      ``geserveradmin --unpublish /test``

      .. rubric:: geserveradmin Virtual Host Commands
         :name: geserveradmin-virtual-host-commands

      Each of the virtual host (VH) commands is listed below, along with
      its syntax, description, and options.

      .. tip::

         With GEE 5.x, you can now set up a virtual host that provides a
         secure publishing point for as many databases as you associate
         with it.

      .. caution::

         Publishing to virtual hosts other than the default server is
         supported only in version 4.2 or later of Google Earth EC. If you
         are using version 4.0 or earlier, only databases that you publish
         to the default server can be accessed by Google Earth EC.

      .. rubric:: List virtual hosts
         :name: list-virtual-hosts

      -\\-**listvhs**

      Provides a list of all registered virtual hosts configured for the
      current machine.

      .. rubric:: List virtual host information
         :name: list-virtual-host-information

      -\\-**vhdetails** *vh_name*

      Displays the name, URL, and cache level of the specified virtual
      host.

      .. rubric:: Add virtual hosts
         :name: add-virtual-hosts

      -\\-**addvh** *vh_name* [-\\-**vhurl** *url*] [-\\-**vhcachelevel** *level*] [-\\-**ssl**]

      Registers a new virtual host with the specified name. Spaces are
      not allowed in the virtual host name. For example:

      geserveradmin \\addvh public_vh -\\-vhurl http://mysite.com/public_vh

      .. list-table:: addvh options
         :widths: 30 15 55
         :header-rows: 1

         * - -\\-addvh option
           - Required/Optional
           - Description
         * - -\\-**vhurl** *url*
           - Optional
           - The ``vhurl``specifies the location of the virtual host. It must
             match the corresponding server-side virtual host configuration.
             If ``vhurl`` is omitted, it will be set to
             ``http://yourserver.domain/vh_name``.
             There are three ways to specify the ``vhurl``:

             -  Location-based URL, such as ``/private_ge``.
                For example, if the entire URL is
                ``http://www.company.com/private_ge``,
                you enter ``/private_ge``.
                **Note:** Google recommends that you use the
                ``_ge`` and ``_map`` naming convention to make
                it easier to distinguish between virtual host types.
             -  Port-based URL, such as: :: http://www.company.com:1234
                The entire URL, including protocol, servername,
                path (if applicable), and port are required.
             -  Name-based URL, such as: :: http://corp.company.com

             For this type of specification, you must modify
             your DNS appropriately for the virtual host.
             After you use this command, you must create a              |
             configuration file for the new virtual host.
         * - -\\-**vhcachelevel** *num*
           - Optional
           - Specify a cache level (``1``, ``2``, or ``3``)
             for the virtual host. The default is ``2``. This cache
             is different than the client cache. This option caches
             only the index nodes at display levels 4, 8, and 12
             (not data packets). If you increase this setting,
             Google Earth Enterprise Fusion caches more of the
             index in RAM, thereby decreasing server latency
             at the cost of server RAM. Level 3 uses approximately
             1 GB of RAM. Level 2 uses approximately 4 MB of RAM.
             Level 1 uses approximately 16 KB of RAM. Each
             additional cache level consumes 256 times the RAM
             as the previous level and saves one disk read per packet served.

             The server makes no checks that the RAM needed for caching
             does not exceed the total RAM on the machine. For example,i
             if you have three virtual hosts set to cache at level 3 on
             a machine that has only 2 GB of RAM, the machine will thrash
             memory. The default is Level 2, so you should be able to create
             as many virtual hosts as you want at the default cache level
             without worrying about running out of RAM.

             Typically, users increase only a small number of virtual hosts
             to cache level 3 on production servers and leave the rest of themi
             at level 2. On servers that share a machine with Google Earth
             Enterprise Fusion, do not increase the level to 3.
             Google Earth Enterprise Fusion needs more RAM than the server does.

         * - -\\-**ssl**
           - Optional
           - Create a location-based virtual host with SSL configuration
             with the naming convention ``_host.location_ssl`` located in
             the path ``/conf.d/virtual_servers/`` . For detailed information
             about ensuring your Apache HTTP server configuration files are
             set up correctly, see
             :doc:`../geeServerConfigAndSecurity/configureGeeServer_SSL_HTTPS`

      .. rubric:: Delete virtual hosts

      -\\-**deletevh** *vh_name*

      Permanently deletes the specified virtual host.

      .. note::

         If you want to delete a virtual host, you must first
         unpublish all currently published databases associated with it.
         To list the currently published databases for the virtual host
         you want to delete, use the ``--publisheddbs`` option. (See
         also ``--unpublish``.)

      .. rubric:: geserveradmin Admin Commands
         :name: geserveradmin-admin-commands

      Each of the admin commands is listed below, along with its syntax
      and description.

      .. rubric:: Delete database files
         :name: delete-database-files

      -\\-**garbagecollect**

      Permanently deletes the files for databases that have been
      selected for deletion. Generally, you run this command nightly to
      remove the files for databases that users have deleted to free up
      space on the storage device. (This command is similar to emptying
      the trash on a Windows or Mac operating system. See also
      ``--deletedb``.)

      .. note::

         Deletes only those files that are not used by other
         databases on that server.

      .. rubric:: Clean up portable globes and maps registration
         :name: clean-up-portable-globes-and-maps-registration

      -\\-**portable_cleanup**

      Clean up portable globes registration information. The cleanup
      unregisters/unpublishes portable globes or maps that have been
      removed from your ``/globes`` directory. You should run
      ``--portable_cleanup`` to clean portable registration information
      when portable files, which are currently published/registered,
      have been removed from your ``/globes`` directory.

      .. note::

         The cleanup is not implemented when there are no
         portable globes or maps in the globes directory:
         ``/opt/google/gehttpd/htdocs/cutter/globes``.

      .. _getop:
      .. rubric:: getop

      **getop** [-\\-**delay** *seconds*]

      .. rubric:: Purpose
         :name: purpose-17

      To display a list of what Google Earth Enterprise Fusion is
      currently working on and whether ``gesystemmanager`` and
      ``geresourceprovider``\ are currently running.

      Enter **Ctrl+C** to exit and return to the prompt.

      .. rubric:: Example
         :name: example-9

      getop -\\-delay 30

      .. rubric:: Commands
         :name: commands-14

      -\\-**delay** *seconds*

      *Optional*. Specify the number of seconds' delay between refreshes.
      For example, if you specify ``30``, ``getop`` runs every 30
      seconds. If you do not specify the delay, the display updates
      every five seconds.

      .. rubric:: geupgradeassetroot
         :name: geupgradeassetroot

      **geupgradeassetroot** -\\-**assetroot** *path* [-\\-**noprompt**]

      .. rubric:: Purpose
         :name: purpose-18

      To upgrade an existing asset root after installing a later version
      of the software.

      .. note::

         You must run this command as root.

         You must stop the system manager before using this
         command and then start it again after you are done.

      .. rubric:: Example
         :name: example-10

      geupgradeassetroot -\\-assetroot /data1/assets

      .. rubric:: Commands
         :name: commands-15

      -\\-**assetroot** *path*

      *Required*. Specify the path to the asset root. If omitted, the
      asset root defaults to ``/gevol/assets``.


      -\\-**noprompt**

      *Optional*. Perform the upgrade without prompting the user for any
      input. This option requires that some commands have arguments
      specified on the command line.


.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
