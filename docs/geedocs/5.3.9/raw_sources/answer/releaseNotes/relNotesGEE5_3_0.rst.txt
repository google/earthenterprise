|Google logo|

=============================
Release notes: Open GEE 5.3.0
=============================

.. container::

   .. container:: content

      .. rubric:: New Features

      **Added the ability to control the XML cache size.** The size can
      be modified by configuring the ``INIT_HEAP_SIZE``,
      ``MAX_HEAP_SIZE``, and ``BLOCK_SIZE`` settings in
      ``/etc/opt/google/XMLparams``. This file must be created manually.
      View the :doc:`../fusionAdministration/confTaskRulesForFusionPerf`
      document for more details.

      **Users can now force the XML cache to purge periodically.** This
      can be done by configuring the ``PURGE`` and ``PURGE_LEVEL``
      settings in ``/etc/opt/google/XMLparams``. This file must be
      created manually. View the :doc:`../fusionAdministration/confTaskRulesForFusionPerf` document for more details.

      **Added a ``long_version`` parameter to the SCons build.** This
      will allow the user to override the long version string.

      **Added options to the Server install script.** The server install
      script now checks the size of the publish root and allows the user
      to set custom user and group names.

      **Improved system manager performance and memory consumption for
      large builds.** This release eliminates some duplicate processing
      and storage in system manager, which reduces the amount of time
      and memory it takes to run operations on large builds.

      **Upgraded to OpenJPEG 2000 2.3.1.** This upgrade brings bugs
      fixes and improvements.

      .. rubric:: Supported Platforms

      The Open GEE 5.3.0 release is supported on 64-bit versions of the
      following operating systems:

      -  Red Hat Enterprise Linux version 6.x and 7.x, including the
         most recent security patches
      -  CentOS 6.x and 7.x
      -  Ubuntu 14.04 LTS and 16.04 LTS

      Google Earth Enterprise 5.3.0 is compatible with Google Earth
      Enterprise Client (EC) version 7.1.5 and above.

      To upgrade from Open GEE 5.2.0, do NOT uninstall it. We recommend
      that you upgrade Open GEE 5.2.0 by simply installing Open GEE
      5.3.0. Installing Open GEE 5.3.0 on top of Open GEE 5.2.0 will
      ensure that your PostgreSQL databases are backed up and upgraded
      correctly to the new PostgreSQL version used by Open GEE 5.3.0.

      .. rubric:: Resolved Issues

      .. list-table::
         :widths: 25 25 50
         :header-rows: 1

         * - Number
           - Description
           - Resolution
         * - 127
           - Incorrect error messages from Fusion installer.
           - Stop incorrect error from showing when re-installing with\ ``-u``\ flag.
         * - 200
           - ``stage_install`` fails on the tutorial files when ``/home`` and ``/tmp`` are on different file systems.
           - Files are linked in ``/tmp`` using symbolic links back to original locations.
         * - 279
           - Server upgrade resets the Admin Console password.
           - When the GEE server is upgraded, either through the ``install_server.sh``
             script or the RPMs, the Admin Console password will be preserved if the
             password file is found at the expected location.
         * - 799
           - The GEE Portable Server does not return valid JSON when queried with ``http://<server>/<map>/query?request=Json``.
           - Added a ``v`` parameter to the query; when its value is 2, the GEE Portable Server will
             return proper JSON. Currently, it defaults to 1 if not specified.
         * - 1093
           - The ``geselectpublishroot`` server command line tool offers no help, and appears to take
             some action when the ``--help`` flag is specified.
           - Help has been added through the ``--help`` option, or if any invalid parameter is given.
         * - 1098
           - TravisCI can pass when compilation fails.
           - Add logic to cause compilation failures to be detected and reported.
         * - 1104
           - CentOS 7 (and maybe others) fails to build Open GEE. Build seems to fail when
             building third party parts of Qt. This appears to be an issue with libpng.
           - Fixed by including ``libpng12`` in the build script.
         * - 1122
           - Check for PIL (Python Imaging Library) returns an error.
           - Change how PIL is included.
         * - 1158
           - ``geeServerDefs`` are not returned by Portable Server after being requested,
             once the globe has been viewed by a client. It is not possible to request
             ``geeServerDefs`` for globes other than the current ``selectedGlobe``.
           - ``geeServerDefs`` are returned correctly and can be requested for any valid globe at any time.
         * - 1166
           - Imagery resources can be created with invalid dates.
           - Added a validity check that ensures a valid date is entered.
             A date of 0000-00-00 can also be entered as a default value. The user will not
             be allowed to save with an invalid date entered. This applies only to the GUI.
         * - 1169
           - Cutting a globe with historical imagery embeds a link to the original
             Google Earth Server that served the historical imagery.
           - There is an advanced option to include historical imagery links or not. The default
             option will be to NOT include links to historical imagery, and the links will be stripped from the
             cut globe. When the historical imagery links are enabled, the existing behavior will be preserved.
         * - 1187
           - RPM upgrades change owner and permissions on symlinked portable globe directories.
           - Symlinked portable globe directories are not modified during geserver upgrades.
         * - 1190
           - Some 32-bit platforms are unable to open files of size >2GB.
           - Use POSIX file API to support large files on some 32-bit platforms.
         * - 1192
           - There appears to be a regression issue in OpenJPEG 2000 lib version 2.3.0
             (that Fusion uses), causing an error when fusing jp2 files.
           - We have upgraded to OpenJPEG 2.3.1 fixing this issue.
         * - 1194
           - Sockets build up during large project builds.
           - gesystemmanager now has a timeout when trying to respond to a request for status.
             This prevents a backlog of requests that could build up while it is busy handling
             a long-running request. The timeout is configurable.
         * - 1201
           - Fusion uses incorrect or no date if using .kip files as source.
           - Fusion now uses acquisition data correctly.
         * - 1210
           - Fix Xerces-related memory leaks.
           - Added an experimental option that fixes several memory leaks related to our usage of the Xerces library.
         * - 1228
           - Assemble button is not fully visible, making it difficult to use the GLC assembly tool.
           - Added a scroll-bar to glc_assemble webpage to allow scrolling to 'Assemble' button.
         * - 1251
           - Build number was incorrectly being appended to the version in RPM packaging.
           - RPM packaging should put the build number in the ``release`` field of the
             RPM properties and not append it to the version number in the ``version`` field.
             This was preventing point releases from properly upgrading.
         * - 1275
           - Search results from KML layers do not display properly in Earth Client.
           - Updated KML rendering libraries with new versions provided by Google.
         * - 1276
           - System manager purges the asset and asset version caches unnecessarily during builds.
             For large builds this can evict most of the items from the cache.
           - Remove the calls to these purge functions.
         * - 1270
           - Fusion uninstaller uses incorrect username " ------"
           - Fixed XML parsing in common.sh that could return incorrect values.

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
           - Ensure GEE Portable Cutter job completes
           - No current work around.
         * - 9
           - Improve FileUnpacker handling of invalid files
           - No current work around.
         * - 20
           - Simplify build process for portable builds on MacOS
           - Building and running Portable Server on MacOS should be possible with minimal changes.
         * - 34
           - SCons build creates temporary directories named “0”
           - No current work around.
         * - 126
           - The Fusion installer creates a backup on the first run
           - The created backup can be deleted.
         * - 190
           - Hostname mismatch check in installers doesn't work as expected
           - No current work around.
         * - 193
           - Updated docs are not copied if the ``/tmp/fusion_os_install`` directory already exists
           - Delete ``/tmp/fusion_os_install`` at the beginning of the stage_install build process.
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
           - Right-click the title bar and select **Close**.
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
           - Fix Fusion graphics acceleration in Ubuntu 14 Docker container hosted on Ubuntu 16
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
           - You can ignore the failed test if using a supported platform
             (Ubuntu 14.04, Ubuntu 16.04, RHEL 6 and 7, and CentOS 6 and 7).
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
           - SCons fails to detect libpng library on CentOS 6
           - Ensure that a default ``g++`` compiler is installed.
         * - 700
           - Add EL6/EL7 check to RPMs
           - Make sure that RPMS are installed on same EL version that they were produced for.
         * - 788
           - Search fails after transferring and publishing a database using disconnected send from the command line
           - Re-publish the database from the web interface.
         * - 825
           - Geserver fails to start up fully due to conflicting protobuf library
           - Run ``pip uninstall protobuf`` to uninstall the protobuf library installed by pip.
         * - 1202
           - Can't select .kip, .ktp, or .kvp as source for resource using Fusion UI.
           - Add the resource from the command line.
         * - 1257
           - Fusion CLI allows users to enter an invalid date/time for Imagery Resources.
           - Ensure you use a valid date when creating resources from the command line.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
