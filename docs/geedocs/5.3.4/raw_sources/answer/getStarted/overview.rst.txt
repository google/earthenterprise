|Google logo|

========
Overview
========

.. container::

   .. container:: content

      .. index:: About Google Earth Enterprise

      Google Earth Enterprise (GEE) enables you to store and process
      terabytes of imagery, terrain, and vector data on your own server
      infrastructure, and publish globes and maps securely for your
      users to view using the Google Earth Enterprise Client (EC), or
      through your own application using the Google Maps and Earth APIs.

      Google Earth Enterprise is comprised of two distinct software
      packages, Fusion and Server, that work together to build and host
      your private Google Earth and Google Maps layers. Your users can
      view these 3D globes and 2D maps using the Google Earth Enterprise
      Client or web browser, respectively. You can also create
      ‘portable’ globes and maps that your users can view using Portable
      Server.

      .. rubric:: 10 Things to Know About Building and Publishing Globes
         & Maps
         :name: things-to-know-about-building-and-publishing-globes-maps

      .. tip::

         Here are some common tasks and key tips when working with
         Google Earth Enterprise. To learn more, click on the references
         at the end of each section.

      .. rubric:: 1. Build databases with Fusion
         :name: build-databases-with-fusion

      Google Earth Enterprise Fusion combines all of your imagery,
      terrain, and vector data into a single, flyable Google Earth globe
      (3D) or a Google Map (2D). After you import source data into your
      asset root and begin working with it, it becomes part of three
      fundamental components of Fusion: Resources, Projects, and
      Databases. The relationship among these three components is well-
      defined. Resources comprise projects, and projects comprise
      databases. A given resource can be used in more than one project,
      and a given project can be used in more than one database.

      If you are working with Mercator map data, you also specify which
      resource(s) comprise each map layer, which map layer(s) comprise
      each Maps vector project, and which Maps vector project and
      imagery project comprise each Maps database.

      To build databases and manage all the resources and projects they
      comprise, you use the Asset Manager UI (or its command-line
      equivalents), from which you can also modify and push your
      databases.

      .. rubric:: Learn More

      - :doc:`../fusionTutorial/buildDatabase`
      - :doc:`../fusionTutorial/createMapDB`

      .. rubric:: 2. View your build logs
         :name: view-your-build-logs

      During the course of creating resources, projects, and databases,
      you can check on the various processes that control the builds,
      helping you determine how your system is performing as your assets
      are being built. Error messages and warnings are also displayed
      here. To view the asset logs in the **Fusion Asset Manager**,
      right-click on an Asset to open its Current Version Properties.
      The System Manager also lets you follow the progress of your
      builds, letting you review waiting and active builds, as well as a
      complete activity log.

      .. rubric:: Learn More
         :name: learn-more-1

      -  :doc:`../installGEE/verifyTroubleshootGEE`
      -  :doc:`Fusion Administration Guide <../fusionAdministration/beforeYouConfigure>`

      .. rubric:: 3. Push Databases to GEE Server
         :name: push-databases-to-gee-server

      Google Earth Enterprise Fusion prepares 2D and 3D databases for
      hosting by first pushing them to GEE server. Pushing a database
      registers it with an associated server and transfers database
      files to the **publish root**, which contains all the files needed
      to serve that database version from GEE Server.

      | Once it is registered, you “publish” a database on GEE server by
        specifying a “publish point”, virtual host, and optionally,
        search tabs and snippets. Once published, your database is ready
        to be accessed by users through Google Earth EC (3D) or a web
        browser (2D).

      | If you want to publish databases on a server that does not have
        a network connection to your Fusion workstation, you can create
        a “disconnected database” which can be placed on portable media,
        and then pushed and published on a GEE Server.

      .. rubric:: Learn More
         :name: learn-more-2

      -  :doc:`../fusionAdministration/pushAndPublishDB`
      -  :doc:`../fusionAdministration/publishDBWithDiscPublishing`

      .. rubric:: 4. Publish databases securely with GEE Server
         :name: publish-databases-securely-with-gee-server

      Does your organization host multiple servers with different
      authentication protocols? If you need to specify unique options to
      publish a map or globe at a given URL, you create individual
      **publish points** so that the map or globe can then be easily
      published multiple times at different URLs. And if you need to
      apply different security protocols to that map or globe, you can
      create new **virtual hosts**.

      Associating **search services** and **snippet profiles** at the
      time of publishing allows you to present different versions on the
      same underlying Fusion database, which can then be made available
      at different URLs and under different security protocols. One
      example is to protect one set of searchable data but display it on
      the same globe as you do an unprotected set of searchable data.
      You might then publish the database at one URL with the sensitive
      search and a secured **virtual host**, then publish the same
      database at a different URL with a non-sensitive search and public
      virtual host.

      .. rubric:: Learn More
         :name: learn-more-3

      -  :doc:`../geeServerAdmin/manageVirtualHosts`
      -  :doc:`../geeServerConfigAndSecurity/configureGeeVirtualHostForLDAP`
      -  :doc:`../geeServerConfigAndSecurity/configureGeeServer5.1.0_SSL_HTTPS`

      .. rubric:: 5. Make Web Map Service (WMS) requests
         :name: make-web-map-service-wms-requests

      GEE Server supports the OpenGIS Web Map Service Interface Standard
      (WMS), which provides a standard HTTP interface to request map
      images from one or more published geospatial databases. One of the
      benefits of using the WMS standard is that supported clients can
      request images from multiple WMS servers and then combine those
      mapping images into a single view. Because the WMS standard is
      used to fetch the images, they can easily be overlaid on one
      another. Supported clients include
      :doc:`QGIS <../geeServerAdmin/makeWMSRequests>`, ArcGIS/ArcGIS
      Explorer Desktop, and `Google Earth Enterprise Client(EC)
      <https://github.com/google/earthenterprise/wiki/Google-Earth-Enterprise-Client-(EC)>`_.

      .. rubric:: Learn More
         :name: learn-more-4

      -  :doc:`../geeServerAdmin/makeWMSRequests`

      .. rubric:: 6. View databases
         :name: view-databases

      You can enable your users to access your private or public 3D
      globe via Google Earth Enterprise Client (EC), or they can access
      your 2D maps through a browser using the Google Maps API.

      Google Earth EC is similar to the familiar Google Earth client and
      offers your users an easy way to view geospatial data compared to
      traditional desktop GIS software. Search data is made accessible
      by **search tabs** in the EC client window, and each search tab
      can appear with customizable query fields and parameters for your
      specific users’ needs.

      .. rubric:: Learn More
         :name: learn-more-5

      -  :doc:`../fusionTutorial/pushPublishView`

      .. rubric:: 7. Google Maps JavaScript API
         :name: google-maps-javascript-api

      Web-based maps mashups can be easily built with your data through
      Google Earth Enterprise and data is securely viewable and
      accessible through a browser using the Google Earth or Google Maps
      API. Use the GEE-specific ``geeCreateFusionMap`` class to
      instantiate and interact with map layers to create a container
      within an HTML page and then apply the same options as you would
      use with ``google.maps.Map``.

      .. rubric:: Learn More
         :name: learn-more-6

      -  :doc:`../developers/googleMapsAPIV3ForGEE`

      .. rubric:: 8. Search Services
         :name: search-services

      GEE provides **search tabs** that you can incorporate into your
      published databases. You can access search services in different
      ways: by specifying search fields (POI) in one or more of your
      vector layers, by adding one of the default search tabs, or by
      writing a custom application that uses “search plug-ins” to query
      external databases.

      Search data is accessed via a query interface in Google Earth EC
      or a browser in the form of search tabs. You can customize the
      labels and fields in a search tab and use query parameter settings
      to control how the results of a query will be displayed.

      .. rubric:: Learn More
         :name: learn-more-7

      -  :doc:`../geeServerAdmin/publishDatabasesWithSearch`
      -  :doc:`../geeServerAdmin/searchPOIVectorMapLayerData`
      -  :doc:`../geeServerAdmin/createSearchTabs`
      -  :doc:`../geeServerAdmin/addCustomSearchServices`

      .. rubric:: 9. Portable
         :name: portable

      With the **Google Earth Enterprise Portable Server**, your users
      can take Google Earth and Maps with them into the field for
      completely disconnected offline use. A portable globe or map is a
      single self-contained file that stores all the geospatial data
      available within your specified area of interest — including all
      high-resolution imagery, terrain, vector data, KML files, and
      searchable point of interest (POI) locations.

      To create a portable map or globe, you launch the **cutter tool**
      from GEE Server and “cut” a polygon or provide a KML to define the
      area of interest. Google Earth Enterprise Portable launches a web
      browser to display your portable globes or maps that have been
      saved to the GEE Portable maps directory. You can also assemble
      different cuts into **composite portable files**. You might want
      to apply layers that use different cuts or regions of interest,
      then assemble them to create one single portable map or globe. You
      can serve these portable globes from Portable Server or GEE
      Server, and then view the portable 3D globes (.glb) from EC, or 2D
      portable maps (.glm) from a web browser.

      .. rubric:: Learn More
         :name: learn-more-8

      -  :doc:`../portable/portableUserGuideWinLinux`
      -  :doc:`../portable/portableDeveloperGuide`
      -  :doc:`../geeServerAdmin/createPortableGlobesMaps`

      .. rubric:: 10. Manage your GEE system
         :name: manage-your-gee-system

      Before you even install, you will need to make sure you have planned
      for the location of your resources and your published databases,
      your **asset** and **publish roots**. And you will need to consider
      administrative privileges, keeping in mind that you will need to
      accept the default user and group access privileges for GEE Server
      or customize them for your organization. As you are likely to be
      processing large amounts of data, we recommend having a maintenance
      plan in place for your GEE system to include backup & restore
      strategies, and periodic clean-up of unwanted asset versions.

      .. rubric:: Learn More
         :name: learn-more-9

      -  :doc:`Fusion Administration Guide <../fusionAdministration>`
      -  :doc:`../fusionAdministration/manageFusionDiskSpace`

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
