|Google logo|

=============================
Release notes: Open GEE 5.2.4
=============================

.. container::

   .. container:: content

      .. rubric:: New Features

      **C++98 no longer supported**. As of 5.2.4 C++11 syntax can be
      used and building open GEE using C++98 standard will no longer be
      supported

      **Added "Save and Build" file menu option**. As of 5.2.4 the
      Fusion UI will include an additional file menu option that allows
      users to save and build assets with one click, instead of clicking
      the save option and then the build option immediately after.

      **Added build parameter to redirect the build output**. Build
      output can now be optionally directed to a folder of your choosing
      instead of the current hard coded defaults. See
      earth_enterprise/BUILD.md in source code for more details.

      **Added experimental build cache folder parameter to the build**.
      Currently this was added as an experimental option to potentially
      speed up builds. Some more work remains to be done before using
      this option in production builds. Use of this option is not
      recommended except for giving feedback and/or helping to address
      issues with Open GEE's build process using scons cache. See
      earth_enterprise/BUILD.md in source code for more details.

      **Added progress indicators during gesystemmanager processing** .
      Add progress log messages to ``gesystemmanager`` while assets are
      being built.

      .. rubric:: Supported Platforms

      The Open GEE 5.2.4 release is supported on 64-bit versions of the
      following operating systems:

      -  Red Hat Enterprise Linux version 6.x and 7.x, including the
         most recent security patches
      -  CentOS 6.x and 7.x
      -  Ubuntu 14.04 LTS and 16.04 LTS

      Google Earth Enterprise 5.2.4 is compatible with Google Earth
      Enterprise Client (EC) version 7.1.5 and above.

      To upgrade from Open GEE 5.2.0, do NOT uninstall it. We recommend
      that you upgrade Open GEE 5.2.0 by simply installing Open GEE
      5.2.4. Installing Open GEE 5.2.4 on top of Open GEE 5.2.0 will
      ensure that your PostgreSQL databases are backed up and upgraded
      correctly to the new PostgreSQL version used by Open GEE 5.2.4.

      .. rubric:: Resolved Issues

      .. list-table::
         :widths: 25 25 50
         :header-rows: 1

         * - Number
           - Description
           - Resolution
         * - 881
           - Error Saving Map Layer with Icon
           - Saving map layers with icons now functions correctly.
         * - 882
           - GEE Portable build script fails on CentOS 6
           - Added code to use g++ from the devtoolset2 installation on CentOS 6 if the default g++ compiler version is too low.
         * - 934
           - Polygon displayed at the wrong location when viewing GLC assemly
           - The Cutter WSGI script now looks at the projection of the GLM files going into the assembly,
             rather than using a hardcoded Mercator projection for the assembly. Also, if a user tries
             to assemble GLMs with differing projections, a warning will be issued.
         * - 946
           - Panoramic Streetview JavaScript error
           - Fixed bug in Google Maps API that prevented panoramic streetview from loading.
         * - 991
           - DNS diagnostics test fails when there is more than one hostname present
           - Test correctly reads host name entries.

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
           - No known workaround.
         * - 9
           - Improve FileUnpacker Handling of Invalid Files
           - No known workaround.
         * - 20
           - Simplify build process for portable builds on MacOS
           - Building and running Portable Server on MacOS should be possible with minimal changes.
         * - 34
           - Scons build creates temporary directories named “0”
           - No known workaround.
         * - 126
           - The Fusion installer creates a backup on the first run
           - No known workaround. The created backup can be deleted.
         * - 127
           - Incorrect error messages from Fusion installer
           - No known workaround.
         * - 190
           - Hostname mismatch check in installers doesn't work as expected
           - No known workaround.
         * - 193
           - Updated docs are not copied if the ``/tmp/fusion_os_install`` directory already exists
           - Delete ``/tmp/fusion_os_install`` at the beginning of the stage_install build process.
         * - 200
           - stage_install fails on the tutorial files when ``/home`` and ``/tmp`` are on different file systems
           - Ensure that ``/home`` and ``/tmp`` are on the same file system or download the tutorial
             files to ``/opt/google/share/tutorials/fusion/`` after installing Fusion.
         * - 201
           - Some tiles are displayed incorrectly in the Enterprise Client when terrain is enabled
           - No known workaround.
         * - 203
           - Some vector layer options are not saved
           - No known workaround.
         * - 221
           - The asset manager may display that a job is "Queued" when in fact the job is "Blocked"
           - No known workaround.
         * - 234
           - Geserver raises error executing apache_logs.pyc
           - No known workaround.
         * - 254
           - Automasking fails for images stored with UTM projection
           - Use GDAL to convert the images to a different projection before ingesting them into Fusion.
         * - 269
           - gevectorimport doesn't crop features
           - Use GDAL/OGR to crop vector dataset before importing them using Fusion.
         * - 295
           - Fix buffer overflows and leaks in unit tests
           - No known workaround.
         * - 309
           - Check for the FusionConnection before new asset is populated
           - Make sure that gefusion service is started.
         * - 320
           - The Portable Server web page uses obsolete REST calls
           - Do not use the buttons on the Portable Server web interface for adding remote servers
             or broadcasting to remote servers as these features are no longer supported.
         * - 326
           - Libraries may be loaded from the wrong directory
           - Delete any library versions that should not be loaded or use LD_LIBRARY_PATH to load
             libraries from ``/opt/google/lib``.
         * - 340
           - GE Fusion Terrain is black
           - No known workaround.
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
           - No known workaround.
         * - 403
           - Missing Close button on system manager window in RHEL 7
           - Right-click the title bar and select Close.
         * - 404
           - Opaque polygons in preview.
           - No known workaround.
         * - 405
           - Vector layer preview not cleared in some situations
           - Reset the preview window to the correct state by either clicking on it or previewing another vector layer.
         * - 407
           - Corrupt data warning when starting Fusion
           - No known workaround but Fusion loads and runs correctly.
         * - 419
           - Fix Fusion Graphics Acceleration in Ubuntu 14 Docker Container Hosted on Ubuntu 16
           - No known workaround.
         * - 437
           - Rebooting VM while it is building resources results in a corrupted XML
           - No known workaround.
         * - 439
           - Uninstalling Fusion without stopping it results in unexpected error message
           - Ignore that error message.
         * - 440
           - Fuzzy imagery in historical imagery tests.
           - No known workaround.
         * - 442
           - Multiple database pushes after upgrade don't report a warning
           - No known workaround.
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
           - No known workaround.
         * - 456
           - Inconsistent behavior of vector layers after upgrade
           - No known workaround.
         * - 460
           - Possibility of seg fault in QDateWrapper
           - No known workaround.
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
           - No known workaround.
         * - 569
           - geserver service installation and uninstallation issues
           - Before uninstalling geserver verify if it's running or not.
         * - 590
           - Maps API JavaScript Files Not Found
           - No known workaround.
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
         * - 1026
           - High resolution imagery sometimes doesn't appear when paired with low-resolution imagery and terrain
           - No known workaround.
         * - 1032
           - Cannot build on CentOS7 ('error: unknown option \first-parent')
           - Upgrade git to 1.84+.
         * - 1037
           - Projects use different level numbers on CLI than displayed in Fusion UI
           - For Terrain projects add 5 to the desired level, for Imagery projects add 7.
         * - 1038
           - genewmapdatabase doesn't accept flat imagery project when --mercator specified
           - Configure the new database using the Fusion UI instead of CLI.
         * - 1041
           - In-place upgrade from previous release fails on Ubuntu 14
           - Edit ``earth_enterprise/src/installer/common.sh``. In the xml_file_get_xpath() function,
             change the tail command to from "+2" to "+3"

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
