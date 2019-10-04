|Google logo|

========================
Release notes: GEE 5.0.1
========================

.. container::

   .. container:: content

      This document includes the following information about the Google
      Earth Enterprise 5.0.1 release:

      .. rubric:: Overview

      GEE 5.0.1 is an incremental release of :doc:`GEE
      5.0 <../releaseNotes/relNotesGEE5_0>`. It includes library updates
      and fixes to several bugs.

      .. rubric:: Supported Platforms

      The Google Earth Enterprise 5.0.1 release is supported on
      64-bit versions of the following operating systems:

      -  Red Hat Enterprise Linux versions 6.0 to 6.5, including
         the most recent security patches
      -  CentOS 6.5
      -  Ubuntu 10.04 and 12.04 LTS

      Google Earth Enterprise 5.0.1 is compatible with Google
      Earth Enterprise Client (EC) and Plugin versions 7.0.1 -
      7.1.1.1888 for Windows, Mac, and Linux.

      .. rubric:: Library Updates

      **Google Maps JavaScript Maps API V3:**

         -  Updated to version 3.14.
            See `change release <https://code.google.com/p/gmaps-api-issues/wiki/JavascriptMapsAPIv3Changelog>`_
            for information.

      **GDAL:**

         -  Updated to version 1.10.1. See `GDAL Release 1.10.1 News <http://trac.osgeo.org/gdal/wiki/Release/1.10.1-News>`_
            for information.

      **Apache HTTP Server:**

         -  Updated to version 2.2.27. See `Apache HTTP Server Project <http://www.apache.org/dist/httpd/Announcement2.2.html>`_
            for information.

      **Libattr extended file attributes:**

         -  Updated from libattr 2.4.46 to version 2.4.47.

      **Openssl update to address** \`vulnerabilities: <http://www.openssl.org/news/vulnerabilities.html>`_

         -  Updated openssl from v0.9.8r to v0.9.8y.

      .. rubric:: Known Issues

      .. list-table::
         :widths: 25 25 50
         :header-rows: 1

         * - Number
           - Description
           - Workaround
         * - 12239387
           - For databases that include historical imagery, if you turn on **Historical imagery** in Google Earth EC and
             move the time slider to the current time, the most recent database in your historical imagery does not
             display correctly.
           - When you move the time slider to view the current database, turn off **Historical imagery**
             to display the database correctly.
         * - 14463855
           - In GEE Server, when publishing a database or portable, the publish point you specify is now case-*insensitive*.
             Upper and lower case are not differentiated.
           - Specify unique publish point path names. If you have already specified publish points that are
             distinguished only by case, such as "Google" and "google," **Unpublish**, specify unique publish points,
             then **Publish** again.
         * - 15221527
           - In GEE Server, when cutting a globe or map, the **Cancel** operation does not stop all associated processes.
           - Before you begin the cutting process, create a *kill* script as follows:

             > sudo vi /opt/google/bin/kill_uid

             ps -eF | grep $1 | grep -v grep | awk
             '{print $2}' | xargs kill -9

             > sudo chmod 755 /opt/google/bin/kill_uid

             Start a cut. If you need to cancel it, do the following steps:

              - Copy the uid from the script window. It will be in the paths used in the
                commands and will look something like ``14845_1401320450.614644``.
              - Click the **Cancel** button.
              - Kill any cutting processes on the server. For example:
                > sudo  /opt/google/bin/kill_uid 14845_1401320450.614644

                **Note** Because you are stopping the cutting process in the JavaScript, your script
                should only need to kill one process, i.e., the last process that was started.
         * - 11736928
           - Terrain copyright information listed in the Provider Manager does not display on the
             published globe in Google Earth EC.
           - No workaround.
         * - 14291339
           - Hebrew label characters that read right to left, for example, "מִבְחָן" get rendered in reverse,
             for example, "ןחָבְמִ". This issue only applies to 2D databases.
           - This example can be resolved as follows: "מִבְחָן".split("").reverse().join("")

      .. rubric:: Resolved Issues

      .. list-table::
         :widths: 25 25 50
         :header-rows: 1

         * - Number
           - Description
           - Resolution
         * - 13010755
           - You can add the Google Base Map as imagery for map databases but viewing a map database
             in a browser currently requires creating and editing a copy of
             ``/opt/google/gehttpd/htdocs/maps_google.html``, one HTML file for each map published.
           - Fixed. No editing of ``maps_google.html`` required. Note that resolution allows using
             only Google Base Map or the Google Base Map *and* an imagery layer. See i
             :doc:`Using Google Base Map. <../fusionTutorial/createMapDB>`.
         * - 14357181
           - When running the Cutter command, the Apache server connection may not close correctly
             after the background process has completely executed, leading to a build failure of the portable globe or map.
           - Fixed. The Apache server now closes the connection appropriately after the cutting process has completed.
         * - 14105700
           - Portable globes and maps may become unregistered and unpublished from GEE Server when the i
             ``/globes`` directory becomes temporarily unavailable.
           - Fixed. If you have registered and/or published portable files that are no longer available in
             your ``/globes`` directory, use the
             `geserveradmin --portable_cleanup <../fusionAdministration/commandReference>` command
             to clean up portable globes registration information. The cleanup unregisters/unpublishes
             portable globes or maps that have been removed from your ``/globes`` directory.
         * - 11315730
           - Uninstalling GEE Fusion 5.x  prevents GEE Server 5.x  from running; likewise uninstalling
             GEE Server 5.x prevents GEE Fusion 5.x from running.
           - Fixed. Uninstalling either Fusion or GEE Server does not impact the running of the remaining
             installed component. However, do make sure that you continue to use the same release versions
             of Fusion and GEE Server to avoid any compatibility issues.
         * - 13232808
           - Portable globes (``*.glc``) without a timestamp in the Manage portable dialog may not be registered with GEE Server.
           - Fixed.
         * - 15274582
           - In rare circumstances, tiles may be missing from vector data in 2D portable files.
           - Fixed.
         * - 14463855
           - GEE Server fails when publishing to different publish points that are only distinguished by case,
             such as "Google" and "google."
           - Fixed. In GEE Server, when publishing a database or portable, the publish point you
             specify is now case-*insensitive*. Upper and lower case are not differentiated. Make
             sure you specify unique publish point path names.
         * - 3941714
           - The GLC assembly tool doesn't clean up files correctly after a composite globe or map is created.
           - Fixed.
         * - 13941482
           - The GLC assembly tool uses the <code>/tmp/</code> directory for composite globe or map processing,
             which is frequently on a limited partition space.
           - Fixed. GLC assembly now uses the ``/opt/google/gehttpd/htdocs/cutter/globes/.globe_builder``
             directory for composite file processing, i.e., the same volume as your Portable globes directory.
         * - 13931811
           - When deleting a layer and rebuilding a ``.glc``, sometimes the GLC assembly tool does not appear to remove the layer.
           - Fixed.
         * - 13916427
           - The display of the build progress window of the GLC assembly tool is delayed when clicking **Assemble Glc**.
           - Fixed.
         * - 13916422
           - The pan and zoom controls in the Cutter tool window are partially obscured by the **Create new offline map** panel.
           - Fixed. The pan and zoom controls are now hidden.
         * - 12239387
           - Historical imagery from 5.x-generated imagery projects display incorrectly.
           - Fixed. Support added for displaying historical imagery from 5.x-generated imagery projects.
             There is still one remaining issue which requires turning off historical imagery in order
             to see the most current database in the current version of Google Earth EC.
             See <a href="#KnownIssues">Known Issues</a>.
         * - 13889571
           - When entering subsequent search parameters in a search tab in Google Earth EC, search results from
             previous query parameters persist in subsequent searches.
           - Fixed.
         * - 13584831
           - Search parameters that include quotes are not supported.
           - Fixed. Note that the comma delimiter cannot be quoted, so the workaround
             is "38","-122.2", not "38, -122.2". Just quoting either the latitude or the longitude will also work.
         * - 13498453
           - When clicking on an item in search results in Google Earth EC that include the FlyToSpot query parameter,
             a javascript error may occur.
           - Fixed.
         * - 13609551
           - The **Push** option available in the context menu when clicking on a database asset that has no valid versions.
           - Fixed.
         * - 13680266
           - When clicking on a search result that returns a single item in Google Earth EC, a javaScript error may occur.
           - Fixed. Limits the search bounds when only one search result is returned and prevents the
             zoom level from exceeding level 17 for 2D and 3D databases.
         * - 11352561
           - There is no option to set a suggestion for a top-level search, typically POI search,
             in the <strong>Publish</strong> dialog.
           - Fixed. Added a text field to set "suggestion" for top-level search to enable override of default
             suggestion of "Point of interest", e.g., and option to set it to an empty string.
         * - 1115030
           - Uninstalling Fusion or Server from ``/opt/google/install`` fails with java.lang.OutOfMemoryError.
           - Fixed.
         * - 13459510
           - Publishing to the same target in GEE Server may unpublish the database.
           - Fixed.
         * - 12362796
           - Databases may disappear from GEE Server after non-use over a period of several days.
           - Fixed
         * - 12891539
           - Search service incorrectly handles UTF-8 encoding query for 2D maps.
           - Fixed.
         * - 12995368
           - Missing interface control for setting <em>polygon resolution</em> in the GEE Server Cutter tool.
           - Fixed by adding the **Advanced** option in the GEE Server Cutter tool to set the polygon resolution.
             See :doc:`Creating portable globes and maps <../geeServerAdmin/createPortableGlobesMaps>`.
         * - 12981516
           - Limited support of UTF-8 encoded queries for ``geplaces`` search.
           - Fixed. Improved the support of UTF-8 encoded queries for ``geplaces`` search, e.g., ``cities``.
         * - 11715339
           - The Preview option in the GEE Server Admin console Databases window fails to display
             any preview for Fusion 3D databases.
           - Fixed ``/opt/google/gehttp/htdocs/earth/earth_local.html`` to point to the correct publish point.
         * - 12671863
           - Support **flyToFirstElement** as additional query parameter for the search services.
             See :doc:`../geeServerAdmin/createSearchTabs`.
           - Fixed.
         * - 8115171
           - Document :doc:`Unable to make reservation <../troubleshooting/unableMakeReservation>` error msg.
           - Fixed.
         * - 8114492
           - Document :doc:`:Unable to find a suitable resource provider <../troubleshooting/unableSuitableResProv>` error msg.
           - Fixed.
         * - 11051993
           - In certain cases, if a globe is corrupt or permissions are incorrectly set, the GEE Server will crash.
           - Fix returns more information about damaged globes.
         * - 12371493
           - Some databases that were not successfully published still appear in the GEE Server admin page.
           - Fix introduces better error checking.
         * - 11435585
           - Changing the port of Earth Server breaks GEE Server admin commands.
           - Fix adds port check to match Apache server.
         * - 9765322
           - Vector polygons are rendered in the wrong location in certain cases.
           - Fixed in the polygon pipeline.
         * - 7737928
           - Cannot build the tutorial *SF Terrain* project without including **WorldTopography** terrain resource.
           - Fixed.
         * - 11254639
           - KML template should have no references to www.google.com.
           - Fixed.
         * - 11134962
           - Horizontal black lines appear at tile boundaries.
           - Fix in the imagery processing pipeline.
         * - 11022364
           - Issues Installing 5.x on top of 4.4.
           - Fixed installer to improve upgrade process.
         * - 7442639
           - GEE supports only one field per custom Search tab in EC 6.2 and later.
           - Fixed. The 5.0.1 release supports multiple fields in EC 6.2 and all later versions.
             To include multiple Search tab fields for EC 6.2 or later

              - Go to GEE Server admin console at ``http://localhost/admin``.
              - Click ``Search tabs``.
              - Click ``Create new``.
              - Enter your first field definition, then click ``Add field``. You can add as many fields as you want.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
