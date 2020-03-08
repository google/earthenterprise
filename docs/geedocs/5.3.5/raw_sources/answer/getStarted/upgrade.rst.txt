|Google logo|

==================
Upgrade to GEE 5.x
==================

.. container::

   .. container:: content

      .. index:: Upgrade Google Earth Enterprise

      You can upgrade to Google Earth Enterprise 5.x from versions 4.0.0
      through 4.4.1.

      .. rubric:: Before you upgrade

      .. container::

         .. rubric:: We recommend you do the following before upgrading
            to GEE 5.x:
            :name: we-recommend-you-do-the-following-before-upgrading-to-gee-5.x

         #. Confirm your hardware and software meet the :doc:`minimum
            requirements <../installGEE/sysReqGEEServer>`.
         #. Apply the latest Linux distribution software patches.
         #. Create backups of data stored in the
            ``/opt/google/gehttpd/`` folders, including display rule
            templates, snippet profiles, icons, virtual 2D and 3D hosts,
            custom KML data (including SuperOverlays), custom web pages,
            CGI scripts, web applications, and JavaScript or HTML files.

            .. note::

               During the uninstallation/installation,
               contents of ``/opt/google/gehttpd`` and other key
               folders are automatically backed up to:
               ``/var/opt/google/fusion-backups``
               These backups are stored in a ``DATE.TIME`` named
               folder, for example, ``20140310.160027/``.

         #. Remove symbolic links from ``/opt/google/gehttpd/`` to other
            directories.
         #. Create backup copies of any customized Apache Server
            configuration files, SSL/HTTPS certificates, user access
            lists, symbolic links to other file system directories, and
            files served from the Apache document root
            ``/opt/google/gehttpd/htdocs``.
         #. Move or copy portable globe files (.glb or .glm) from the
            default globes folder:
            ``/opt/google/gehttpd/htdocs/cutter/globes``.
         #. Run garbage collect for stream/search on the GEE server
            machine. This will clean all previously deleted databases in
            ``/gevol/published_dbs;``

            ``/opt/google/bin/geserveradmin --garbagecollect --server_type stream``
            ``/opt/google/bin/geserveradmin --garbagecollect --server_type search``

         .. note::

            GEE 5.x does not support 2D maps in the flat
            (Plate Carrée) projection. If you have previously built and
            published a map with a flat projection, these databases will
            be marked as invalid, and you will not be able to perform
            any delete, build, push, or publish actions on them.

      .. rubric:: Upgrading

      .. rubric:: To upgrade to GEE 5.x:
         :name: to-upgrade-to-gee-5.x

      #. Stop ``gefusion`` and ``geserver``.
      #. If you are upgrading from GEE 4.x to 5.x, uninstall previous
         versions of Google Earth Enterprise Fusion and Server.

         .. note::

            When you uninstall Google Earth Enterprise
            Fusion and Server, your asset root remains unchanged;
            none of your resources or assets are removed.

      #. Install GEE Fusion and Server 5.x.
      #. For GEE Server, run the following command:
         ``sudo -u gepguser /opt/google/bin/geresetpgdb``

         .. note::

            ``geresetpgd`` deletes registering information
            about Fusion databases on GEE Server. After installing
            GEE Server/Fusion you will need to:

            -  Rebuild 3D database assets; if **Push** is not
               available, then the database assets need to be
               rebuilt.
            -  Push all 3D and 2D Mercator databases. 2D Plate Carrée
               databases are not valid in GEE 5.0. See :doc:`Web Map
               Service (WMS) <../geeServerAdmin/makeWMSRequests>` for
               projection support for Plate Carrée maps.

      #. Upgrade asset root:

         ``sudo /opt/google/bin/geupgradeassetroot --assetroot /gevol/assets``

      #. Optionally enable cutter:

         ``/opt/google/bin/gecutter enable``

      #. Start ``gefusion`` and ``geserver``.

      .. rubric:: After you upgrade

      .. rubric:: After upgrading:

      #. Restore backups of data stored in the ``/opt/google/gehttpd/``
         folders, either from your own backup copies or from the backup
         folder automatically created during
         uninstallation/installation:
         ``/var/opt/google/fusion-backups``.

         .. tip::

            Only restore data that you have customized, including
            display rule templates, snippet profiles, icons, virtual 2D
            and 3D hosts, custom KML data (including SuperOverlays),
            custom web pages, CGI scripts, web applications, JavaScript
            or HTML files, Apache Server configuration files, SSL/HTTPS
            certificates, user access lists, symbolic links to other
            file system directories, and files served from the Apache
            document root ``/opt/google/gehttpd/htdocs``.

      #. Rebuild your 3D Fusion database assets (a minimal rebuild will be
         forced for terrain and imagery projects). For 2D Mercator
         databases, rebuilding is not required.

         .. note::

            Push is not available if rebuild is required. You
            will need to push each database (3D and 2D Mercator
            databases) to GEE Server (database will be registered on
            server, and updated files will be pushed), where they can
            then be published from the GEE Server Admin console. GEE
            Server supports serving imagery packets of old 2D Mercator
            and 3D Fusion databases.

      .. rubric:: Compatibility with Google Earth EC
         :name: compatibility-with-google-earth-ec

      GEE 5.x is compatible with Google Earth Enterprise Client (EC) and
      Plugin versions 7.0.1 - 7.3.x for Windows, Mac, and Linux.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
