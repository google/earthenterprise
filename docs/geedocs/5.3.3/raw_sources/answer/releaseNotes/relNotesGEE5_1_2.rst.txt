|Google logo|

========================
Release notes: GEE 5.1.2
========================

.. container::

   .. container:: content

      .. rubric:: Overview

      GEE 5.1.2 is an incremental release of
      :doc:`GEE 5.1 <../releaseNotes/relNotesGEE5_1_0>`.
      It contains several bug fixes in Fusion and GEE Server,
      and updates to third-party libraries.

      .. rubric:: Supported Platforms

      The Google Earth Enterprise 5.1.2 release is supported on 64-bit
      versions of the following operating systems:

      -  Red Hat Enterprise Linux versions 6.0 to 7.1, including the
         most recent security patches
      -  CentOS 6.0 to 7.1
      -  Ubuntu 10.04 and 12.04 LTS

      Google Earth Enterprise 5.1.2 is compatible with Google Earth
      Enterprise Client (EC) version 7.1.5 and Google Earth Plug-in
      versions 7.0.1 - 7.1.5.1557 for Windows and Mac.

      .. rubric:: Library Updates

      .. list-table::
         :widths: 30 30
         :header-rows: 1

         * - Library
           - Updated Version
         * - Google Maps JS API V3
           - 3.20
         * - Apache httpd
           - 2.2.31
         * - OpenSSL
           - 0.9.8zg
         * - libcurl
           - 7.45

      .. rubric:: Resolved Issues

      .. list-table::
         :widths: 25 25 50
         :header-rows: 1

         * - Number
           - Description
           - Resolution
         * - 19946027
           - Support additional search option.
           - A new **SearchGoogle** search tab is available for use with 3D databases which
             queries Google's location database. Search sugestions are also offered as you type.
         * - 20225923, 19871110, 11179114
           - gesystemmanager crashes when processing very large projects
           - Fixed in Fusion: Optimized management of listeners of CombinedRPAsset versions in asset root.
             Also reduced the size of asset configuration files.
         * - 4431033
           - Extra data added in imagery projects for imagery that is not visible
           - Fixed in Fusion: skip identical assets with the same bounds.
         * - 21126307
           - Remove option for Disconnected Add-on from GEE Server installer to simplify installation.
           - Fixed in GEE Server Installer: Disconnected Add-on is always installed.
         * - 20185775
           - Add a tool to check the consistency of pushed databases in the publish root.
           - Run ``gecheckpublishroot.py``.
         * - 10280645
           - ``gemodifyimageryresource`` incorrectly reports bad images.
           - Fixed in Fusion/gevirtualraster.
         * - 16135553
           - ``gemaskgen`` reports 'Process terminated by signal 8'.
           - Fixed in Fusion: check for invalid image sizes; improve logging.
         * - 22007798
           - Missing Apache directives when adding a SSL virtual host with ``geserveradmin --ssl``\ code> .
           - Fixed in GEE Server: properly configure virtual host when 'vh_url' parameter is not specified.
         * - 21852939
           - Support opacity on 2D map layers.
           - Fixed in GEE Server: 2D Map Viewer includes sliders for controlling layer opacity
         * - 19499802
           - ``gebuild`` does not check whether source file has changed.
           - Fixed in Fusion: 'Refresh' functionality added to ``gemodifyvectorresource``,
             ``gemodifyimageryresource``, and ``gemodifyterrainresource`` to detect changes
             to source files, and update assets if necessary.
         * - 4171577
           - Disconnected delta publishes not possible if previous database versions are cleaned.
           - Fixed in GEE Fusion: generate and store database manifest files within versions of a database.
         * - 19962321
           - Report the number of cached Assets and Asset Versions in ``getop``
           - Fixed in Fusion.
         * - 20859774
           - GLC Assembly fails with missing file: ``/opt/google/gehttpd/htdocs/cutter/template/earth``.
           - Fixed in GEE Server/Cutter.
         * - 19995336, 21301170, 21301504
           - Selecting "Save As" creates duplicate IDs in Fusion.
           - Fixed in Fusion for Map Layer, Raster projects and Vector assets.
         * - 18280076
           - Implement dbroot conversion
           - Fixed in GEE Server: enable support for disconnected publishing of GEE 4.x database to GEE 5.x Server.
         * - 6820671
           - Fatal error reported while registering disconnected database: unifiedindex files not found
           - Fixed in Fusion.
         * - 17303394
           - Implement automatic detection of old glbs for globe cutting or assembly
           - Fixed in GEE Server.
         * - 19991743
           - Add support for database short names when pushing
           - Fixed in GEE Server: e.g. ``geserveradmin --adddb Databases/BlueMarble``.
         * - 18917848
           - Include layer icons in 2D Maps
           - Fixed in GEE Server.
         * - 18922625
           - Coordinate Search triggers Internal Server error (HTTP 500) for bad query strings
           - Fixed in GEE Server: return HTTP 400 for incorrect search requests.
         * - 22097637
           - Support search history in client
           - Fixed in GEE Server: clicking on 'History' in EC shows previous successful search requests.
         * - 22318180
           - Security vulnerabilities in Cutter.
           - Fixed in GEE Server/Cutter: patched ``gecutter`` and validated inputs
             to minimize risk of remote-code execution and SQL-injection when Cutter is enabled.
         * - 24132440
           - Uncaught exception when serving 3D glbs: GET /query?request=Json&var;=geeServerDefs&is2d;=t
           - Fixed in Portable Server.
         * - 24103836
           - Support 'Satellite' mapTypeControl for 2D databases built with Google Basemap
           - Fixed in GEE Server.
         * - 23937667
           - Search bar is not available when viewing 3D portable globe
           - Fixed in Portable Server: POI and Places search available.
         * - 23557041
           - Missing icons in search results
           - Fixed in Portable Server.
         * - 22097637
           - Provide search tabs for 2D Map portable
           - Fixed in GEE Server/Cutter.
         * - 23569399
           - 'db_id' parameter missing when POI search is not present in a published database's search services
           - Fixed in GEE Server.
         * - 23496088
           - Exception thrown when Places search returns no results
           - Fixed in Portable Server.
         * - 20068112
           - ``geserveradmin --addvh --ssl`` creates a virtual host with an invalid port number
           - Fixed in GEE Server: use the '--vhurl' option for non-default SSL ports.
             See ``geserveradmin`` help for usage & syntax.
         * - 22958187
           - Include Google Geocoder in default search services
           - Fixed in GEE Server: 'SearchGoogle' tab is available as a default search
             service for both 2D and 3D databases; queries Google's geocoders and requires Internet access (client-side)
         * - 23399349
           - Incorrect handling of POI search queries like "Paris, France"
           - Fixed in GEE Server: search queries like "Paris, France" are parsed as a single search token.
         * - 1826725
           - ``gepackgen`` fails with 'Specified data products have different coverage'.
           - Fixed in Fusion: implement Cluster Analyzer for virtual rasters ``*.khvr files``.
             It analyzes inset clustering and area ratios to suggest optimal splits of a virtual raster.
             Check ``gerasterimport`` log, and see ``gevirtualraster`` for usage and syntax.
         * - 22414308
           - Support snippet for 'View in Google Maps' in EC
           - Fixed in GEE Server: enable 'View in Google Maps' in EC, publishing 3D database i
             with 'google_maps_url' snippet set to 'http://maps.google.com/'.
         * - 22958590
           - Places queries can makes server unresponsive for large number of search results
           - Fixed in GEE Server.
         * - 22879773
           - Federated Search returns HTTP 500 error
           - Fixed in GEE Server: if Coordinate search fails, proceed with Places search.
         * - 22954617
           - Viewport for displaying multiple POI search results is incorrectly calculated.
           - Fixed in GEE Server (2D Map Viewer).
         * - 21165472
           - GLC assembly fails to copy final glc to globes directory, for large glc files
           - Fixed in GEE Server/Cutter.
         * - 25422176
           - Fusion fails to push databases with very large POI files
           - Fixed in Fusion: updated internal data structures to support POI files > 4 GB; i
             improved logging in POI parser to report exceptions when ingesting POI data into postgres.
         * - 11254639
           - EC makes calls to google.com when rendering search results
           - Fixed in GEE Server: localized all KML rendering; expose dbroot snippets in
             'search_config' group: ``kml_render_url, kml_search_url, error_page_url``.
         * - 25430798
           - ``SearchGoogle`` search tab returns Server Error
           - Fixed in GEE Server: updated User-Agent header in search handler.
         * - 24407861
           - Support database pushes over HTTPS/SSL
           - Fixed in Fusion: Server Association Manager includes 'CA certificate' path and
             'Insecure SSL connection' checkbox for self-signed certificates.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
