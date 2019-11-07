|Google logo|

=============================
Release notes: Open GEE 5.2.2
=============================

.. container::

   .. container:: content

      .. rubric:: New Features

      **Ability to load a default globe, using Google Earth Enterprise
      Client, when connecting to the base URL path**. This can be
      achieved through enhanced ``geserveradmin`` command when
      publishing a database using the option ``--setecdefault``.
      Example:
      ``geserveradmin --publishdb /gevol/assets/Databases/SFMapDatabase.kmmdatabase/mapdb.kda/ver001/mapdb/ --targetpath http://myserver.org --setecdefault``

      **geserver and gefusion services support status**. geserver and
      gefusion now support returning the status of the services
      indicating whether they are running or not.

      **Detailed profiling data for terrain processing**.
      ``gecombineterrain`` operation can produce performance details if
      profiling is enabled. To turn on profiling use
      ``log_performance=1`` when building Open GEE. Ex.
      ``scons -j8 release=1 log_performance=1 build``. Executing
      ``gecombineterrain`` will, then, create a log file detailing
      timing of the execution. The file is named
      ``time_stats.date_time_stamp.csv`` (ex.
      ``time_stats.03-26-2018-11:23:46.csv``) and stored in working
      directory of ``gecombineterrain`` if run from terminal. If run
      from Fusion UI the .csv log file will be stored in
      ``/gevol/published_dbs/stream_space/host-name`` and
      ``/gevol/assets/gee-database-name/Db.kdatabase/gedb.kda/``

      .. rubric:: Supported Platforms

      The Open GEE 5.2.2 release is supported on 64-bit versions of the
      following operating systems:

      -  Red Hat Enterprise Linux version 6.x and 7.x, including the
         most recent security patches
      -  CentOS 6.x and 7.x
      -  Ubuntu 14.04 LTS and 16.04 LTS

      Google Earth Enterprise 5.2.2 is compatible with Google Earth
      Enterprise Client (EC) version 7.1.5 and above.

      To upgrade from Open GEE 5.2.0, do NOT uninstall it. We recommend
      that you upgrade Open GEE 5.2.0 by simply installing Open GEE
      5.2.2. Installing Open GEE 5.2.2 on top of Open GEE 5.2.0 will
      ensure that your PostgreSQL databases are backed up and upgraded
      correctly to the new PostgreSQL version used by Open GEE 5.2.2.

      .. rubric:: Resolved Issues

      .. list-table::
         :widths: 25 25 50
         :header-rows: 1

         * - Number
           - Description
           - Resolution
         * - 810
           - When GEE Server listens to specific IP address the returned default local virtual
             host is an IP address instead of the hostname
           - Resolved host name from ServerName in gehttpd.conf or FQDN.
         * - 252
           - Google Earth Server fails to load Admin console when there is an invalid link to a published portable file
           - Added check to ensure files actually exist before continuing on to database object creation.
         * - 660
           - Building Open GEE on Ubuntu 16.04 throws an error 'ImportError: No module named git:'
           - gitpython introduced dependency was documented in build instructions.
         * - 670
           - Diagnostics fails in current release_5.2.1
           - Fixed and temporary removed diagnostic link was restored.

      .. rubric:: Known Issues

      .. list-table::
         :widths: 25 25 50
         :header-rows: 1

         * - Number
           - Description
           - Workaround
         * - 4
           - Google basemap fails to load in 2D Mercator Maps
           - Obtain a valid Google Maps API key and include it in ``/opt/google/gehttpd/htdocs/maps/maps_google.html``.
         * - 8
           - Ensure GEE Portable Cutter Job Completes
           - No current work around.
         * - 9
           - Improve FileUnpacker Handling of Invalid Files
           - No current work around.
         * - 20
           - Simplify build process for portable builds on MacOS
           - Building and running Portable Server on MacOS should be possible with minimal changes.
         * - 34
           - Scons build creates temporary directories named “0”
           - No current work around.
         * - 126
           - The Fusion installer creates a backup on the first run
           - No current work around. The created backup can be deleted.
         * - 127
           - Incorrect error messages from Fusion installer
           - No current work around.
         * - 190
           - Hostname mismatch check in installers doesn't work as expected
           - No current work around.
         * - 193
           - Updated docs are not copied if the ``/tmp/fusion_os_install`` directory already exists
           - Delete ``/tmp/fusion_os_install`` at the beginning of the stage_install build process.
         * - 200
           - stage_install fails on the tutorial files when ``/home`` and ``/tmp`` are on different file systems
           - Ensure that ``/home`` and ``/tmp`` are on the same file system or download the
             tutorial files to ``/opt/google/share/tutorials/fusion/`` after installing Fusion.
         * - 201
           - Some tiles are displayed incorrectly in the Enterprise Client when terrain is enabled
           - No current work around.
         * - 202
           - Icons are not displayed on vector layers in the Enterprise Client
           - No current work around. It is not clear if this is an error in GEE or in the Enterprise Client.
         * - 203
           - Some vector layer options are not saved
           - No current work around.
         * - 221
           - The asset manager may display that a job is "Queued" when in fact the job is "Blocked"
           - No current work around.
         * - 225
           - Fusion lets you create folder with space in the name
           - Avoid creating folder with space in their name.
         * - 234
           - Geserver raises error executing apache_logs.pyc
           - No current work around.
         * - 254
           - Automasking fails for images stored with UTM projection
           - Use GDAL to convert the images to a different projection before ingesting them into Fusion.
         * - 269
           - gevectorimport doesn't crop features
           - Use GDAL/OGR to crop vector dataset before importing them using Fusion.
         * - 295
           - Fix buffer overflows and leaks in unit tests
           - No current work around.
         * - 309
           - Check for the FusionConnection before new asset is populated
           - Make sure that gefusion service is started.
         * - 320
           - The Portable Server web page uses obsolete REST calls
           - Do not use the buttons on the Portable Server web interface for adding remote
             servers or broadcasting to remote servers as these features are no longer supported.
         * - 326
           - Libraries may be loaded from the wrong directory
           - Delete any library versions that should not be loaded or use LD_LIBRARY_PATH
               to load libraries from ``/opt/google/lib``.
         * - 340
           - GE Fusion Terrain is black
           - No current work around.
         * - 342
           - Fusion crashes when opening an unsupported file type
           - Re-open Fusion and avoid opening unsupported file types.
         * - 343
           - gefusion: File ->open->*.kiasset*,*.ktasset*,*.kip does not work
           - kip is not a supported format. Void opening files with .kip extension.
         * - 380
           - Provider field in resource-view is blank
           - Open the individual resource to see the provider.
         * - 401
           - GEE commands are not in the path for sudo.
           - Specify the full path when running commands or add ``/opt/google/bin``
             to the path for all users, including the super user.
         * - 402
           - Provider manager window locked to main window.
           - No current work around.
         * - 403
           - Missing Close button on system manager window in RHEL 7
           - Right-click the title bar and select Close.
         * - 404
           - Opaque polygons in preview.
           - No current work around.
         * - 405
           - Vector layer preview not cleared in some situations
           - Reset the preview window to the correct state by either clicking on it or previewing another vector layer.
         * - 407
           - Corrupt data warning when starting Fusion
           - No current work around but Fusion loads and runs correctly.
         * - 419
           - Fix Fusion Graphics Acceleration in Ubuntu 14 Docker Container Hosted on Ubuntu 16
           - No current work around.
         * - 437
           - Rebooting VM while it is building resources results in a corrupted XML
           - No current work around.
         * - 439
           - Uninstalling Fusion without stopping it results in unexpected error message
           - Ignore that error message.
         * - 440
           - Fuzzy imagery in historical imagery tests.
           - No current work around.
         * - 442
           - Multiple database pushes after upgrade don't report a warning
           - No current work around.
         * - 444
           - Fusion installer does not upgrade the asset root on RHEL 7
           - Upgrade the asset root manually by running the command that is printed when you try to start the Fusion service.
         * - 445
           - Path to tutorial source volume in gee_test instructions is different from path used in installers
           - Use ``/opt/google/share/tutorials``.
         * - 448
           - Out of Memory issues
           - Use a system that has more than 4GB RAM.
         * - 453
           - Improve \`check_server_processes_running\` detection for uninstall
           - No current work around.
         * - 456
           - Inconsistent behavior of vector layers after upgrade
           - No current work around.
         * - 460
           - Possibility of seg fault in QDateWrapper
           - No current work around.
         * - 474
           - Running gee_check on some supported platforms reports that the platform is not supported
           - You can ignore the failed test if using a supported platform (Ubuntu 14.04, Ubuntu 16.04, RHEL 7, and CentOS 7).
         * - 477
           - 'service geserver stop/start/restart' doesn't work on Ubuntu 16.04 without a reboot
           - Reboot and try again.
         * - 487
           - gdal - python utilities do not recognize osgeo module
           - Install ``python-gdal``.
         * - 507
           - Volume host is reported unavailable if \`hostname\` doesn't match volume host
           - Set the host values in ``/gevol/assets/.config/volumes.xml`` to the FQDN and restart the Fusion service.
         * - 535
           - DownloadTutorial.sh often is not staged properly for install
           - Copy ``DownloadTutorial.sh`` to ``/tmp/fusion_os_install``.
         * - 557
           - WMS service problem with 'width' & 'height' & 'bbox'
           - No current work around.
         * - 569
           - geserver service installation and uninstallation issues
           - Before uninstalling geserver verify if it's running or not.
         * - 590
           - Maps API JavaScript Files Not Found
           - No current work around.
         * - 594
           - Save errors only reported for the first image
           - Close the form in question and try again.
         * - 640
           - Save button disabled in 'Map Layer' creation dialog when an error encountered
           - Close the resource form and open it again to make the save option available again.
         * - 651
           - Release executables and libraries depend on gtest
           - Follow current build instructions that requires ``gtest`` to be installed.
         * - 669
           - Missing repo in RHEL 7 build instructions
           - Enable ``rhel-7-server-optional-rpms`` and ``rhel-7-server-optional-source-rpms`` repos.
         * - 682
           - Update geconfigurepublishroot to fully correct file permissions
           - Manually correct the file permissions.
         * - 686
           - Scons fails to detect libpng library on CentOS 6
           - Ensure that a default ``g++`` compiler is installed.
         * - 694
           - Search fails after transferring and publishing a database using disconnected send from the command line
           - Re-publish the database from the web interface.
         * - 700
           - Add EL6/EL7 check to RPMs
           - Make sure that RPMS are installed on same EL version that they were produced for.
         * - 731
           - Error in publish of SSL-enabled database
           - A temporary fix was added in this release. A more permanent fix will be done in Open GEE 5.2.3.
         * - 825
           - Geserver fails to startup fully due to conflicting protobuf library
           - Run ``pip uninstall protobuf`` to uninstall the protobuf library installed by pip.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
