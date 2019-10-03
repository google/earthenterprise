|Google logo|

=============================
Release notes: Open GEE 5.2.3
=============================

.. container::

   .. container:: content

      .. rubric:: New Features

      **C++11 support**. With release 5.2.3 Open GEE C++ code now
      supports and defaults to compiling using the ``-std=gnu++11``
      compile flag. This default can be overridden and the C++ code can
      be compiled using the ``-std=gnu++98`` flag instead. However, Open
      GEE 5.2.3 will be the last release to support compiling C++ code
      using the ``-std=gnu++98`` flag. The subsequent releases after
      Open GEE 5.2.3 will no longer support the ``-std=gnu++98`` flag.
      All C++ code will need to conform to the gnu++11 standard.

      **Remove TCP/IP connections to PostgreSQL**. With release 5.2.3
      Open GEE code will now, by default, connect to PostgreSQL
      exclusively using Unix domain sockets. This will make it easier to
      secure PostgreSQL for security sensitive installations. For those
      upgrading from a previous release the following files will need to
      be changed before disabling PostgreSQL tcp listener:

      -  /opt/google/search/conf/ExampleSearch.conf
      -  /opt/google/search/conf/GEPlacesSearch.conf
      -  /opt/google/search/conf/PoiSearch.conf

      You will need to change the host from '127.0.0.1' to '/tmp'
      **Volume deletion**. ``geconfigureassetroot`` now supports the
      option ``--removevolume`` for volume deletion.

      **Performance tuning for gecombineterrain**. ``gecombineterrain``
      now supports the option ``--numCompressThreads`` which specifies
      the number of maximum worker threads used to compress packets in
      this operation. If not specified ``gecombineterrain`` defaults
      this option to match the number of available CPUs.

      **Package name customization at build time**. Scons build now
      accepts a custom label argument which is appended to the version
      in RPM file names.

      **Rewrite KML URLs**. Users can rewrite the URLs for KML resources
      included in databases at publish time.

      **Maps API JavaScript Files**. Maps API Javascript files are now
      available and installed at
      ``/opt/google/gehttpd/htdocs/maps/mapfiles``.

      .. rubric:: Supported Platforms

      The Open GEE 5.2.3 release is supported on 64-bit versions of the
      following operating systems:

      -  Red Hat Enterprise Linux version 6.x and 7.x, including the
         most recent security patches
      -  CentOS 6.x and 7.x
      -  Ubuntu 14.04 LTS and 16.04 LTS

      Google Earth Enterprise 5.2.3 is compatible with Google Earth
      Enterprise Client (EC) version 7.1.5 and above.

      To upgrade from Open GEE 5.2.0, do NOT uninstall it. We recommend
      that you upgrade Open GEE 5.2.0 by simply installing Open GEE
      5.2.3. Installing Open GEE 5.2.3 on top of Open GEE 5.2.0 will
      ensure that your PostgreSQL databases are backed up and upgraded
      correctly to the new PostgreSQL version used by Open GEE 5.2.3.

      .. rubric:: Resolved Issues

      .. list-table::
         :widths: 25 25 50
         :header-rows: 1

         * - Number
           - Description
           - Resolution
         * - 225
           - Fusion lets you create folder with space in the name
           - AssetManager UI only allows letters, numbers, and underscores for directory name creation
         * - 226
           - Verify the wrongly spelled "aquisition" is not appearing anywhere in the product
           - Fixed these spelling errors by replacing with Acquisition in multiple files.
         * - 337
           - Add a compiler version check to scons scripts
           - Scons scripts ensure gcc version 4.8+ is installed before continuing with builds
         * - 342
           - Fusion UI crashes when opening non supported type of file
           - Added check to kiasset, ktasset, xml file load to handle invalid files
         * - 535
           - DownloadTutorial.sh often is not staged properly for install
           - Tutorial download script is now included in the Fusion RPM
         * - 682
           - Update geconfigurepublishroot to fully correct file permissions
           - geconfigurepublishroot recursively sets the owner and permissions for publish root
         * - 731
           - Error in publish of SSL-enabled database
           - Temporary fix from 5.2.2 was found to be the best solution
         * - 762
           - Package libraries and utilities common to Fusion and Server for Ubuntu 16
           - opengee-common deb packages can be created for Ubuntu 16
         * - 806
           - gecombineterrain defaulted to only 1 cpu to process data and ignored arguments
           - Added --numCompressThreads option to accept the number of threads to use.
             Default value is the number of CPUs available. Deprecated --numcpus option
         * - 821
           - Calling geeInitMap results in an infinite recursive loop
           - Code to display non-existent polygon removed
         * - 835
           - GE Server gets its URL from the client instead of itself
           - Fixed code so that when you publish DB, GEServer's URL will be used and not the
             URL which we get from the Publish request message.
         * - 861
           - Running diagnostics on a server only installation will result in a failure for testAdqeuateDiskSpace
           - This test is skipped if Fusion is not present.
         * - 877
           - Fusion RPM install scripts to only ``chown`` the asset root when necessary
           - Recursively ``chown`` only when the asset root directory has incorrect ownership on a
             Fusion master host. This allows upgrading Fusion on machines where changing asset root
             ownership recursively would be very expensive.
         * - 912
           - TestDiskSpace diagnostics test fails on CentOS7
           - Changed a function to return the percentage of available disk space instead of the percentage of used disk space.

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
           - Do not use the buttons on the Portable Server web interface for adding
             remote servers or broadcasting to remote servers as these features are no longer supported.
         * - 326
           - Libraries may be loaded from the wrong directory
           - Delete any library versions that should not be loaded or use LD_LIBRARY_PATH to
             load libraries from ``/opt/google/lib``.
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
         * - 686
           - Scons fails to detect libpng library on CentOS 6
           - Ensure that a default ``g++`` compiler is installed.
         * - 700
           - Add EL6/EL7 check to RPMs
           - Make sure that RPMS are installed on same EL version that they were produced for.
         * - 788
           - Search fails after transferring and publishing a database using disconnected send from the command line
           - Re-publish the database from the web interface.
         * - 825
           - Geserver fails to startup fully due to conflicting protobuf library
           - Run ``pip uninstall protobuf`` to uninstall the protobuf library installed by pip.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
