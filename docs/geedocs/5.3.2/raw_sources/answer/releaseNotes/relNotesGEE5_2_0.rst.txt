|Google logo|

=============================
Release notes: Open GEE 5.2.0
=============================

.. container::

   .. container:: content

      .. rubric:: Overview

      Open GEE 5.2.0 is the first open source release of Google Earth
      Enterprise. It is designed to be compatible with GEE 5.1.3 with
      the exception of MrSID support. Open GEE 5.2.0 includes a number
      of code and library changes to support the open source effort.

      .. rubric:: New Features

      **Open Sourcing**. Open GEE 5.2.0 includes a number of changes
      that were necessary for the open sourcing effort. This includes
      several library replacements, library unbundling, removal of
      obsolete code and libraries, changes to support new compilers,
      updates to support git and Github, and several documentation
      changes.

      **New Installers**. Previous GEE installers used proprietary tools
      that are not appropriate for open source projects. Open GEE 5.2.0
      includes new installers that run as bash scripts. There are also
      new build and install scripts for the Portable Server.

      **New Tutorial Files**. Images used in the Fusion tutorial are now
      provided in both tif and jp2 format. In addition, the tutorial
      files are stored and downloaded separately from the rest of the
      Open GEE code.

      **OpenGEE.org Web Site**. The Open GEE repository now includes the
      source code for the Open GEE website, which can be viewed at
      `http://www.opengee.org <http://www.opengee.org>`_.

      **New --listvolumes Flag in geconfigureassetroot**. The
      geconfigureassetroot utility now supports a --listvolumes flag
      that allows users to display the configured volumes for the
      current asset root without stopping Fusion.

      .. rubric:: Supported Platforms

      The Open GEE 5.2.0 release is supported on 64-bit versions of the
      following operating systems:

      -  Red Hat Enterprise Linux version 7.x, including the most recent
         security patches
      -  CentOS 7.x
      -  Ubuntu 14.04 LTS and 16.04 LTS

      Google Earth Enterprise 5.2.0 is compatible with Google Earth
      Enterprise Client (EC) version 7.1.5 and above.

      .. rubric:: New and Updated Libraries

      Open GEE 5.2.0 includes extensive library changes. This list may
      be incomplete.

      .. list-table::
         :widths: 25 25 50
         :header-rows: 1

         * - Library
           - Version
           - New or Updated
         * - Google Maps
           - API V3 3.29
           - Updated
         * - mod_wsgi
           - 4.5.14
           - Updated
         * - OpenJPEG
           - 2.1.2
           - New
         * - QT
           - 3.3.8b-free
           - Updated
         * - GDAL
           - 2.1.2
           - Updated

      If you are upgrading from GEE 5.1.3, you may have to reinstall the
      prerequisite libraries after uninstalling 5.1.3 but before
      installing 5.2.0.

      .. rubric:: Known Issues

      .. list-table::
         :widths: 25 25 50
         :header-rows: 1

         * - Number
           - Description
           - Workaround
         * - 2
           - MrSID imagery is not supported
           - MrSID support can be added by purchasing a proprietary GDAL plugin that
             supports MrSID and modifying the build process to include the plugin.
         * - 4
           - Google basemap fails to load in 2D Mercator Maps
           - Obtain a valid Google Maps API key and include it in ``/opt/google/gehttpd/htdocs/maps/maps_google.html``.
         * - 6
           - The Portable UI reports an error any time a cut is canceled, even if the cancel was successful
           - Ignore the misleading error message.
         * - 200
           - stage_install fails on the tutorial files when ``/home`` and ``/tmp`` are on different file systems
           - Ensure that ``/home`` and ``/tmp`` are on the same file system or download the
             tutorial files to ``/opt/google/share/tutorials/fusion/`` after installing Fusion.
         * - 202
           - Icons are not displayed on vector layers in the Enterprise Client
           - No current work around. It is not clear if this is an error in GEE or in the Enterprise Client.
         * - 203
           - Some vector layer options are not saved
           - No current work around
         * - 254
           - Automasking fails for images stored with UTM projection
           - Use GDAL to convert the images to a different projection before ingesting them into Fusion.
         * - 320
           - The Portable Server web page uses obsolete REST calls
           - Do not use the buttons on the Portable Server web interface for adding
             remote servers or broadcasting to remote servers as these features are no longer supported.
         * - 326
           - Libraries may be loaded from the wrong directory
           - Delete any library versions that should not be loaded or use
             `LD_LIBRARY_PATH to load libraries from ``/opt/google/lib``.
         * - 333
           - Portable Server is not supported on MacOS
           - Building and running Portable Server on MacOS should be possible with minimal changes.
         * - 335, 359
           - If there is an error while saving a resource, the resource cannot be saved again even if the error is resolved
           - Close the resource form and open it again to make the save option available again.
         * - 340
           - GE Fusion Terrain is black
           - No current work around
         * - 342
           - Fusion crashes when opening an unsupported file type
           - Re-open fusion and avoid opening unsupported file types.
         * - 375
           - Invalid version of psycopg2 on Ubuntu 16.04
           - On Ubuntu 16.04 switch to 'python-psycopg2' instead of 'python2.7-psycopg2'.
         * - 380
           - Provider field in resource-view is blank
           - Open the individual resource to see the provider
         * - 401
           - GEE commands are not in the path for sudo
           - Specify the full path when running commands or add /opt/google/bin to the path for all users, including the super user
         * - 402
           - Provider manager window locked to main window.
           - No current work around
         * - 403
           - Missing close button on system manager window in RHEL 7
           - Right click the title bar and select close
         * - 404
           - Opaque polygons in preview.
           - No current work around
         * - 405
           - Vector layer preview not cleared in some situations
           - Reset the preview window to the correct state by either clicking on it or previewing another vector layer
         * - 407
           - Corrupt data warning when starting fusion
           - No current work around but Fusion loads and runs correctly.
         * - 423
           - Slower JPEG2000 performance than 5.1.3
           - Use Geotiff or other image formats.
         * - 437
           - Rebooting VM while it is building resources results in a corrupted XML
           - No current work around
         * - 440
           - Fuzzy imagery in historical imagery tests.
           - No current work around
         * - 444
           - Fusion installer does not upgrade the asset root on RHEL 7
           - Upgrade the asset root manually by running the command that is printed when you try to start the fusion service
         * - 453
           - Improve \`check_server_processes_running\` detection for uninstall
           - No current work around
         * - 456
           - Inconsistent behavior of vector layers after upgrade
           - No current work around
         * - 474
           - Running gee_check on some supported platforms reports that the platform is not supported
           - You can ignore the failed test if using a supported platform (Ubuntu 14.04, Ubuntu 16.04, RHEL 7, and CentOS 7).
         * - 476
           - Support building on CentOS6 with Python2.6
           - No current work around
         * - 477
           - 'service geserver stop/start/restart' doesn't work on Ubuntu 16.04 without a reboot
           - Reboot and try again

      .. rubric:: Resolved Issues

      .. list-table::
         :widths: 25 25 50
         :header-rows: 1

         * - Number
           - Description
           - Resolution
         * - (none)
           - Error when fusing a mosaic in GDAL 2.x
           - Implemented ``IReadBlock()`` API for ``khVRRasterBand``
         * - 16
           - gefusionuser must have write access to vector files
           - Removed the requirement for write access
         * - 26
           - Fusion segmentation faults when trying to push and the server is not available
           - Passed the correct data type to cURL function
         * - 167
           - If save fails because the Fusion server is not running, the user cannot save again even if the problem is fixed
           - Updated state management in resource creation form
         * - 179
           - Update date parsing to fix default date handling
           - Added code to handle default dates in Fusion tools
         * - 196
           - Simplify SSL settings and enable TLS 1.2 by default
           - Consolidated SSL settings and updated the defaults to include TLS 1.2
         * - 239
           - ``cachedreadaccessor_unittest`` fails on ``free()`` call on Ubuntu 16.04
           - Increased the size of a buffer that was overflowing
         * - 243
           - Fix broken line in cutter script
           - Fixed the relevant line
         * - 351
           - Portable Globe Cutter fails with a Python error
           - Fixed import errors in the cutter scripts
         * - 381
           - Error running POI Search
           - Needed to declare the coding in headers of python script:# -- coding: utf-8 -
         * - 431
           - portable build failure - CentOS 7 - old version of pexpect.
           - Try loading fdpexpect for version 3.\* and above. If not successful, fallback to fdpexpect from older pexpect package.
         * - 435
           - Specify C++ version for portable build
           - Updated the build_lib scripts to include the flag "-std=gnu++98"
         * - 436
           - Seg fault in fusion when opening vector resource created in old version
           - Eliminated NULL pointer error in date/time function

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
