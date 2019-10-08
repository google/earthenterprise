|Google logo|

========================
Release notes: GEE 5.1.1
========================

.. container::

   .. container:: content

      .. rubric:: Overview
      
      GEE 5.1.1 is an incremental release of :doc:`GEE
      5.1 <6078762`. It contains several bug fixes in
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

      .. rubric:: Library updates

      ===================== =============================
      Library               Updated version
      ===================== =============================
      GDAL                  1.11.1 (with libcurl support)
      libjpeg               v8d1
      OpenSSL               0.9.8ze
      libcurl               7.39.0
      MesaLib               7.11
      NumPy                 1.9.1
      OpenLDAP              2.4.40
      mod_wsgi              3.5
      libgeotiff            1.4.0
      PostgreSQL            8.4.22
      PostGIS               1.5.8
      GEOS                  3.4.2
      Libxml2               2.9.1
      Google Maps JS API V3 3.19
      ===================== =============================

      .. rubric:: Resolved Issues

      +-----------------------+-----------------------+-----------------------+
      | Number                | Description           | Resolution            |
      +=======================+=======================+=======================+
      | 12239387, 18719211    | Under certain         | Fixed. To display     |
      |                       | conditions,           | historical imagery    |
      |                       | historical imagery is | correctly, take the   |
      |                       | not displayed         | following steps:      |
      |                       | correctly.            |                       |
      |                       |                       | -  Upgrade Google     |
      |                       |                       |    Earth EC to        |
      |                       |                       |    v7.1.4. Older      |
      |                       |                       |    versions of Google |
      |                       |                       |    Earth EC do not    |
      |                       |                       |    show the most      |
      |                       |                       |    recent imagery     |
      |                       |                       |    when the time      |
      |                       |                       |    slider is          |
      |                       |                       |    activated.         |
      |                       |                       | -  Rebuild the 3D     |
      |                       |                       |    database after     |
      |                       |                       |    upgrading to GEE   |
      |                       |                       |    5.1.1. The rebuild |
      |                       |                       |    step is required   |
      |                       |                       |    before a database  |
      |                       |                       |    can be pushed to   |
      |                       |                       |    GEE Server again,  |
      |                       |                       |    and is usually     |
      |                       |                       |    rapid.             |
      |                       |                       | -  Publish with GEE   |
      |                       |                       |    Server and view    |
      |                       |                       |    the 3D database in |
      |                       |                       |    Google Earth EC    |
      |                       |                       |    with the           |
      |                       |                       |    `historical        |
      |                       |                       |    imagery <../answer |
      |                       |                       | /4412458.html>`__     |
      |                       |                       |    option turned on.  |
      +-----------------------+-----------------------+-----------------------+
      | 15212134              | Cuts of 2D maps fail  | Fixed in GEE Server.  |
      |                       | if no polygon is      | See `Create portable  |
      |                       | specified.            | globes and            |
      |                       |                       | maps <../answer/32307 |
      |                       |                       | 77.html>`__.          |
      +-----------------------+-----------------------+-----------------------+
      | 15290004              | When attempting to    | Fixed. The **Cutter** |
      |                       | cancel a cut using    | cancel process now    |
      |                       | the **Cutter** tool,  | kills any running     |
      |                       | the Cancel button     | task on GEE Server.   |
      |                       | does not properly     |                       |
      |                       | kill any running task |                       |
      |                       | on GEE Server.        |                       |
      +-----------------------+-----------------------+-----------------------+
      | 16981234              | Portable Server fails | Fixed. See `Portable  |
      |                       | to run on RHEL 6.4,   | release               |
      |                       | returning             | notes <../answer/4594 |
      |                       | ``Error Loading pytho | 620.html>`__.         |
      |                       | n lib libpython2.7.so |                       |
      |                       | .1.0``.               |                       |
      +-----------------------+-----------------------+-----------------------+
      | 18476901              | When attempting to    | Fixed. See            |
      |                       | publish databases     | `Publishing databases |
      |                       | using disconnected    | using disconnected    |
      |                       | publishing,           | publishing <../answer |
      |                       | ``gedisconnectedsend` | /6051700.html>`__.    |
      |                       | `                     |                       |
      |                       | reports incorrect     |                       |
      |                       | suffix for bytes.     |                       |
      +-----------------------+-----------------------+-----------------------+
      | 18506290              | Serving 2D Database   | Fixed. See `Add flat  |
      |                       | with *Mercator on the | imagery to Mercator   |
      |                       | Fly* imagery fails    | map                   |
      |                       | when imagery has no   | databases <../answer/ |
      |                       | worldwide coverage.   | 6081069.html>`__.     |
      +-----------------------+-----------------------+-----------------------+
      | 18509863              | HTTPS protocol not    | Fixed. See `Create    |
      |                       | respected in          | search                |
      |                       | URL-based search tab. | tabs <../answer/34978 |
      |                       |                       | 32.html>`__.          |
      +-----------------------+-----------------------+-----------------------+
      | 18572866              | GEE 4.x virtual       | Fixed.                |
      |                       | servers               |                       |
      |                       | (``default_ge``,      |                       |
      |                       | ``default_map``) are  |                       |
      |                       | not deleted when      |                       |
      |                       | upgrading to GEE      |                       |
      |                       | 5.1.x.                |                       |
      +-----------------------+-----------------------+-----------------------+
      | 18668563              | During the            | Fixed.                |
      |                       | installation process, |                       |
      |                       | the installer returns |                       |
      |                       | ``java.lang.NullPoint |                       |
      |                       | erException``         |                       |
      |                       | error.                |                       |
      +-----------------------+-----------------------+-----------------------+
      | 18791567              | Building a 2D         | Fixed. See `Creating  |
      |                       | database without an   | a map                 |
      |                       | imagery project       | database <../answer/4 |
      |                       | crashes               | 412455.html>`__.      |
      |                       | ``gesystemmanager``.  |                       |
      +-----------------------+-----------------------+-----------------------+
      | 19003972              | Improve *Mercator on  | Fixed. See `Add flat  |
      |                       | the Fly* reprojection | imagery to Mercator   |
      |                       | performance.          | map                   |
      |                       |                       | databases <../answer/ |
      |                       |                       | 6081069.html>`__.     |
      +-----------------------+-----------------------+-----------------------+
      | 19020117              | `WMS <../answer/44411 | Fixed.                |
      |                       | 37.html>`__           |                       |
      |                       | GetCapabilities       |                       |
      |                       | returns inaccurate    |                       |
      |                       | bounding box          |                       |
      |                       | information;          |                       |
      |                       | transposed            |                       |
      |                       | coordinates on GetMap |                       |
      |                       | 1.3.0 requests.       |                       |
      +-----------------------+-----------------------+-----------------------+
      | 18980809              | When publishing, the  | Fixed.                |
      |                       | Delete button may not |                       |
      |                       | be available for a    |                       |
      |                       | search service that   |                       |
      |                       | has been added to a   |                       |
      |                       | database.             |                       |
      +-----------------------+-----------------------+-----------------------+
      | 18724718              | Additional support in | Fixed.                |
      |                       | the UI for new        |                       |
      |                       | configuration options |                       |
      |                       | for search service    |                       |
      |                       | deployment:           |                       |
      |                       |                       |                       |
      |                       | -  Specify search     |                       |
      |                       |    services           |                       |
      |                       |    deployment lists   |                       |
      |                       |    in the publish     |                       |
      |                       |    dialog             |                       |
      |                       | -  Override text when |                       |
      |                       |    creating Search    |                       |
      |                       |    tabs and           |                       |
      |                       |    Supplemental       |                       |
      |                       |    search using the   |                       |
      |                       |    Supplemental UI    |                       |
      |                       |    button             |                       |
      +-----------------------+-----------------------+-----------------------+
      | 18935285              | When new resources    | Fixed.                |
      |                       | have been added,      |                       |
      |                       | Fusion GUI produces   |                       |
      |                       | unnecessary           |                       |
      |                       | ``gepackgen`` tasks   |                       |
      |                       | during project        |                       |
      |                       | builds. This occurs   |                       |
      |                       | when: a new resource  |                       |
      |                       | is added using the    |                       |
      |                       | Fusion GUI, which has |                       |
      |                       | previously been       |                       |
      |                       | created with console  |                       |
      |                       | tool; the new         |                       |
      |                       | resource is created   |                       |
      |                       | without specifying an |                       |
      |                       | acquisition date and  |                       |
      |                       | also has the same     |                       |
      |                       | base resolution as    |                       |
      |                       | other resources in    |                       |
      |                       | the existing raster   |                       |
      |                       | project.              |                       |
      +-----------------------+-----------------------+-----------------------+
      | 19286893              | Uncaught TypeError:   | Fixed.                |
      |                       | Cannot read property  |                       |
      |                       | ``value`` of ``null`` |                       |
      |                       | when publishing 2D    |                       |
      |                       | databases.            |                       |
      +-----------------------+-----------------------+-----------------------+
      | 19276438              | During push, Fusion   | Fixed.                |
      |                       | GUI processes at      |                       |
      |                       | 100%.                 |                       |
      +-----------------------+-----------------------+-----------------------+
      | 18724718              | Default search tab    | Fixed. Modified       |
      |                       | cannot be hidden, and | management of search  |
      |                       | custom ones are not   | services deployment   |
      |                       | displayed by default. | for 3D Databases: you |
      |                       |                       | can configure a       |
      |                       |                       | search service to     |
      |                       |                       | deploy to an EC       |
      |                       |                       | search tab (services  |
      |                       |                       | with one search       |
      |                       |                       | field) or to a        |
      |                       |                       | Supplemental Search.  |
      +-----------------------+-----------------------+-----------------------+
      | 11333524              | When pushing a        | Fixed.                |
      |                       | database: improve     |                       |
      |                       | provision of database | -  Improved logging   |
      |                       | push status.          |    and fixed progress |
      |                       |                       |    bar updating when  |
      |                       |                       |    pushing            |
      |                       |                       | -  Cleanup in         |
      |                       |                       |    ``PublisherClient` |
      |                       |                       | `                     |
      |                       |                       |    and profiles       |
      |                       |                       |    parser             |
      +-----------------------+-----------------------+-----------------------+
      | 19412572              | Publishing an updated | Fixed.                |
      |                       | map at the same       |                       |
      |                       | target point causes   |                       |
      |                       | ``gehttpd`` to crash  |                       |
      |                       | when serving          |                       |
      |                       | Databases that use    |                       |
      |                       | Mercator on the Fly,  |                       |
      |                       | caused by a request   |                       |
      |                       | to a non-existent     |                       |
      |                       | version of imagery    |                       |
      |                       | layer. Returns a 404  |                       |
      |                       | error code.           |                       |
      +-----------------------+-----------------------+-----------------------+
      | 19280022              | When using URL-based  | Fixed.                |
      |                       | search tabs,          |                       |
      |                       | erroneous search      |                       |
      |                       | requests may occur.   |                       |
      +-----------------------+-----------------------+-----------------------+
      | 19338090              | Support added for     | Fixed.                |
      |                       | Google Geocoder       |                       |
      |                       | responses for 2D      |                       |
      |                       | searches.             |                       |
      +-----------------------+-----------------------+-----------------------+
      | 5447870               | Segmentation fault    | Fixed.                |
      |                       | when using the        |                       |
      |                       | ``geraster2kml``      |                       |
      |                       | tool.                 |                       |
      +-----------------------+-----------------------+-----------------------+
      | 19709212              | Icon file cannot be   | Fixed.                |
      |                       | read.                 |                       |
      +-----------------------+-----------------------+-----------------------+
      | 20185775              | Missing bundle.hdr    | Fixed.                |
      |                       | file in published     |                       |
      |                       | database, resulting   |                       |
      |                       | in blurry imagery in  |                       |
      |                       | some areas.           |                       |
      +-----------------------+-----------------------+-----------------------+

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
