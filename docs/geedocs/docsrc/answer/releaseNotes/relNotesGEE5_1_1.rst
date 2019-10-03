|Google logo|

========================
Release notes: GEE 5.1.1
========================

.. container::

   .. container:: content

      .. rubric:: Overview

      GEE 5.1.1 is an incremental release of :doc:`GEE
      5.1 <../releaseNotes/relNotesGEE5_1_0>`. It contains several bug fixes in
      Fusion and GEE Server, and updates to third-party libraries.

      .. rubric:: Supported Platforms

      The Google Earth Enterprise 5.1.1 release is supported on 64-bit
      versions of the following operating systems:

      -  Red Hat Enterprise Linux versions 6.0 to 6.5, including the
         most recent security patches
      -  CentOS 6.5
      -  Ubuntu 10.04 and 12.04 LTS

      Google Earth Enterprise 5.1.1 is compatible with Google Earth
      Enterprise Client (EC) version 7.1.4 and Google Earth Plug-in
      versions 7.0.1 - 7.1.1.1888 for Windows, Mac, and Linux.

      .. rubric:: Library Updates

      .. list-table::
         :widths: 30 30
         :header-rows: 1

         * - Library
           - Updated Version
         * - GDAL
           - 1.11.1 (with libcurl support)
         * - libjpeg
           - v8d1
         * - OpenSSL
           - 0.9.8ze
         * - libcurl
           - 7.39.0
         * - MesaLib
           - 7.11
         * - NumPy
           - 1.9.1
         * - OpenLDAP
           - 2.4.40
         * - mod_wsgi
           - 3.5
         * - libgeotiff
           - 1.4.0
         * - PostgreSQL
           - 8.4.22
         * - PostGIS
           - 1.5.8
         * - GEOS
           - 3.4.2
         * - Libxml2
           - 2.9.1
         * - Google Maps JS API V3
           - 3.19

      .. rubric:: Resolved Issues

      .. list-table::
         :widths: 25 25 50
         :header-rows: 1

         * - Number
           - Description
           - Resolution
         * - 12239387, 18719211
           - Under certain conditions, historical imagery is not displayed correctly.
           - Fixed. To display historical imagery correctly, take the following steps:

              #. Upgrade Google Earth EC to v7.1.4. Older versions of Google Earth EC do not
                 show the most recent imagery when the time slider is activated.
              #. Rebuild the 3D database after upgrading to GEE 5.1.1. The rebuild step is
                 required before a database can be pushed to GEE Server again, and is usually rapid.
              #. Publish with GEE Server and view the 3D database in Google Earth EC
                 with the :doc:`historical imagery <../fusionTutorial/buildHistImageryProj>` option turned on.

         * - 15212134
           - Cuts of 2D maps fail if no polygon is specified.
           - Fixed in GEE Server. See :doc:`../geeServerAdmin/createPortableGlobesMaps`.
         * - 15290004
           - When attempting to cancel a cut using the **Cutter** tool, the Cancel button
             does not properly kill any running task on GEE Server.
           - Fixed. The **Cutter** cancel process now kills any running task on GEE Server.
         * - 16981234
           - Portable Server fails to run on RHEL 6.4, returning ``Error Loading python lib libpython2.7.so.1.0``.
           - Fixed. See :doc:`Portable Release Notes <../portable/portableReleaseNotes5_0>`.
         * - 18476901
           - When attempting to publish databases using disconnected publishing, ``gedisconnectedsend``
             reports incorrect suffix for bytes.
           - Fixed. See :doc:`../fusionAdministration/publishDBWithDiscPublishing`.
         * - 18506290
           - Serving 2D Database with *Mercator on the Fly* imagery fails when imagery has no worldwide coverage.
           - Serving 2D Database with *Mercator on the Fly* imagery fails when imagery has no worldwide coverage.
         * - 18509863
           - HTTPS protocol not respected in URL-based search tab.
           - Fixed. See :doc:`../geeServerAdmin/createSearchTabs`.
         * - 18572866
           - GEE 4.x virtual servers (``default_ge``, ``default_map``) are not deleted when upgrading to GEE 5.1.x.
           - Fixed.
         * - 18668563
           - During the installation process, the installer returns ``java.lang.NullPointerException`` error.
           - Fixed.
         * - 18791567
           - Building a 2D database without an imagery project crashes ``gesystemmanager``.
           - Fixed. See :doc:`../fusionTutorial/createMapDB`.
         * - 19003972
           - Improve `Mercator on the Fly` reprojection performance.
           - Fixed. See :doc:`Add flat imagery to Mercator map databases
             <../fusionResAndProj/addFlatImageryToMercatorMapDBInGEE5.1.0>`.
         * - 19020117
           - :doc:`WMS <../geeServerAdmin/makeWMSRequests>`
             GetCapabilities returns inaccurate bounding box information; transposed coordinates on GetMap 1.3.0 requests.
           - Fixed.
         * - 18980809
           - When publishing, the Delete button may not be available for a search service that has been added to a database.
           - Fixed.
         * - 18724718
           - Additional support in the UI for new configuration options for search service deployment:

              * Specify search services deployment lists in the publish dialog
              * Override text when creating Search tabs and Supplemental search using the Supplemental UI button

           - Fixed.
         * - 18935285
           - When new resources have been added, Fusion GUI produces unnecessary ``gepackgen``
             tasks during project builds. This occurs when: a new resource is added using the
             Fusion GUI, which has previously been created with console tool; the new resource is
             created without specifying an acquisition date and also has the same base
             resolution as other resources in the existing raster project.
           - Fixed.
         * - 19286893
           - Uncaught TypeError: Cannot read property ``value`` of ``null`` when publishing 2D databases.
           - Fixed.
         * - 19276438
           - During push, Fusion GUI processes at 100%.
           - Fixed.
         * - 18724718
           - Default search tab cannot be hidden, and custom ones are not displayed by default.
           - Fixed. Modified management of search services deployment for 3D Databases:
             you can configure a search service to deploy to an EC search tab (services with one
             search field) or to a Supplemental Search.
         * - 11333524
           - When pushing a database: improve provision of database push status.
           - Fixed.

              * Improved logging and fixed progress bar updating when pushing
              * Cleanup in ``PublisherClient`` and profiles parser

         * - 19412572
           - Publishing an updated map at the same target point causes ``gehttpd`` to crash
             when serving Databases that use Mercator on the Fly, caused by a request to a
             non-existent version of imagery layer. Returns a 404 error code.
           - Fixed.
         * - 19280022
           - When using URL-based search tabs, erroneous search requests may occur.
           - Fixed.
         * - 19338090
           - Support added for Google Geocoder responses for 2D searches.
           - Fixed.
         * - 5447870
           - Segmentation fault when using the ``geraster2kml`` tool.
           - Fixed.
         * - 19709212
           - Icon file cannot be read.
           - Fixed.
         * - 20185775
           - Missing bundle.hdr file in published database, resulting in blurry imagery in some areas.
           - Fixed.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
