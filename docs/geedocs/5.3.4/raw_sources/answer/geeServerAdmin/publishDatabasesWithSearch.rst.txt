|Google logo|

=============================
Publish databases with search
=============================

.. container::

   .. container:: content

      Google Earth Enterprise (GEE) provides several different ways that
      you can incorporate search into published databases,
      depending on the complexity of the search you want to perform and
      the data you are searching.

      Search functionality is enabled when you *publish* a database in
      GEE Server. Different search options are available:

      -  Turn on **POI Search** based on search fields you add to a
         vector resource in a vector project or map layer
      -  Add default search tabs, enabling queries of local or non-local
         search data based on **Places**, **Coordinate**, **POI**, and
         **GeocodingFederated** searches
      -  Add **custom search tabs**, created by **custom search
         plug-ins** that incorporate more complex queries and data, such
         as search results based on geometries.

      .. rubric:: Add POI Search

      If you have one or more text fields that you would like to
      query from vector resources in a project, you can simply :doc:`add
      those search fields to a vector
      project <../fusionTutorial/searchFldsForLayers>` in a 3D database; for 2D
      maps, you include a map layer in a map project. Once you have
      added your POI vector project or map layer to the database you
      want to publish, you :doc:`push it to GEE
      Server <../fusionAdministration/pushAndPublishDB>`, then turn on POI Search
      when you publish.

      For example, you may be interested in displaying the US Census
      Bureau population statistics on your map or globe, based on county
      name. You can easily add the search fields to the individual
      population data layer in your vector project. You can also choose
      how you want the labels for your search results to appear on the
      globe, such as search and display a field, or simply display it.

      You can also modify the appearance of the POI search tab in Google
      Earth EC by :doc:`editing the system search tab <../geeServerAdmin/createSearchTabs>`, **POI Search**,
      which sets the query parameters for the search fields you added to
      the vector layer, as well as specifying the labels for the tab, query
      field and suggestion box.

      .. rubric:: Use GEE-provided Search Data

      Google Earth Enterprise provides two searchable databases,
      **Places** and **ExampleSearch**, firstly as data that you can add
      to globes and maps and secondly as examples that demonstrate
      the search plug-in functionality of GEE Server.

      The Places database in ``/opt/google/share/geplaces`` provides
      a searchable database of countries and cities that you can connect
      to using the **POI**, **GeocodingFederated**, **Places** or
      **Coordinate** plug-ins. The Places database is a subset of the
      publicly available `geonames database <http://www.geonames.org/about.html>`_. You might use
      the **Places search tab** to perform queries based on city names,
      for example. You might use the **Coordinate search tab** to
      perform queries based on city, country, or lat/long values.

      The ExampleSearch database in ``/opt/google/share/searchexample`` is a
      table of San Francisco neighborhoods. The **ExampleSearch search tab**
      is provided as a showcase of how a custom search plug-in can be coded
      to extract geometries from a spatial database and then return the results
      so that they display according to the query parameters. The plug-in
      supports query parameters that set **flyToFirstElement** and
      **displayKeys**.

      .. tip::

         Places and ExampleSearch are both PostgreSQL databases so
         you can also access them using the ``psql`` command. See
         `PostgreSQL 9.3.5 Documentation <http://www.postgresql.org/docs/9.3/static/app-psql.html>`_.

      You can examine the Python code of the search plug-ins on which
      the search tabs are built, using it as a template for your own
      plug-in development. To customize your own search plug-ins, see
      :doc:`../geeServerAdmin/addCustomSearchServices`.

      .. rubric:: About system search tabs

      System search tabs are provided with GEE Server. They provide
      hard-coded queries for various search data types available in
      the Places database provided with GEE. You can use them to
      access this database, a subset of the publicly available
      geonames database, or you can edit them for your own purposes.
      The search tabs cannot be deleted, but can be edited and saved
      with a different name.

      The following default search tabs are available:

      -  **POISearch**: Search for POIs.
      -  **GeocodingFederated**: Search for places or coordinates.
      -  **Places**: Search for places using the built-in locations
         database.
      -  **Coordinate**: Search for latitude, longitude pairs. The
         following formats are supported:

         -  Decimal Degrees (e.g. 39.507618\ |deg| -84.168556\ |deg|)
         -  Degrees, Minutes, Seconds (e.g. 20\ |deg|\ 40\ |prime|\ 01.51\ |Prime|\ S 131\ |deg|\ 53\ |prime|\ 51.39\ |Prime|\ E)
         -  Degrees, Decimal Minutes (e.g. 49\ |deg|\ 32.876\ |prime|\ N 110\ |deg|\ 9.193\ |prime|\ E)
         -  Universal Transverse Mercator (e.g. 43 R 637072.95 m E
            2825582.86 m N)
         -  Military Grid Reference System (e.g. 36NTL8040632621)

      -  **ExampleSearch**: Search the example table of San Francisco
         neighborhoods to demonstrate the search plug-in functionality.

      .. rubric:: About search tabs definitions


      Search data is accessed via a query interface in Google Earth
      EC or a browser in the form of search tabs. A search tab
      definition, which you specify in GEE Server, includes the
      **label** you want to apply to the search tab that your users
      will see, the **URL** that points to a local or non-local
      searchable database, supported query parameters, and **fields**
      that you specify for your search queries. You can customize the
      labels and fields in a search tab and use **query parameter
      settings** to control how the results of a query will be
      displayed.

      The database to which you point your search tab definition is
      identified in the query parameters when the globe or map is
      served. The search tab identifies the database by including the
      database ID as a parameter in the query string, an internal
      structure that points to either a searchable database on GEE
      Server or an external server.

      Example query string when serving a local database:

      ``mydb/POISearch?db_id=5&flyToFirstElement;=true&displayKeys;=location&q;=San Francisco``

      Example query string when serving an external database:

      ``http://mysearch_host.com/search_serviceA?db_id=5&flyToFirstElement;=true&displayKeys;=location&q;=San Francisco``

      .. rubric:: Create custom searches

      If you want to access a custom data source, you can :doc:`create your
      own search plug-in <../geeServerAdmin/addCustomSearchServices>` and access it as a
      search tab definition in Google Earth Enterprise Server. A custom
      search plug-in will create requests and responses using the common
      Python-based search framework, letting you adapt your search to
      any data source. Your custom search plug-in will receive search
      requests directly from Google Earth Enterprise Client (EC), or any
      other clients in the form of URL parameters.

      For example, with custom search plug-ins you can use Maps API and
      Places API to leverage the power of their data sets. You can also
      use a proprietary database, e.g., power conversion sites,
      then search for the sites using internal IDs or city names.

      .. rubric:: Summary table of search options

      .. list-table::
         :widths: 20 30 30
         :header-rows: 1

         * - Search options
           - Steps summary
           - Search appearance in Google Earth EC or in a browser
         * - Query vector data layer using a simple text-based search.
           - Add a search field to a vector resource in a vector project or map layer. Turn on **POI Search**
             in the Publish dialog when you publish your database in GEE Server.
           - The **POI Search** appears in the main **Search** tab, labeled Point of interest, including the
             field label you specified in the vector data layer.
         * - Query external database using a default search plugin, for example **POI Search**.
           - Edit the **POI Search tab URL** field to point to your own search database and edit or add query parameters.
             Add the **POI Search tab** in the **Publish** dialog.
           - When you click **Query**, the **POI Search** appears as a new additional tab in Search, displaying the custom
             label and any query field labels and suggestions you added.
         * - Query the Places database provided by GEE.
           - Add the **Places search tab** in the Publish dialog to add location search to your globe or map.
           - The Places Search appears as a new additional tab in Search, displaying the custom label and any query field
             label and suggestion you added.
         * - Query an external Postgres database and return polygons, lines, or geometries.
           - Write a custom search plug-in using the Python-based framework for search tabs. The **ExampleSearch** search
             plug-in is a useful demonstration for this purpose.
           - Your custom search plug-in appears as a new additional tab in Search, displaying the custom label and any query
             field labels and suggestions you added. Alternatively, you can send HTTP requests and receive a response within your
             own custom web application.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px

.. |deg| unicode:: U+00B0 .. DEGREE
.. |prime| unicode:: U+2032 .. PRIME
.. |Prime| unicode:: U+2033 .. DOUBLE PRIME
