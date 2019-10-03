|Google logo|

==========================
Add custom search services
==========================

.. container::

   .. container:: content

      If you want to access a custom data source, you can create your
      own search plug-in and access it as a search tab definition in
      Google Earth Enterprise Server. A custom search plug-in will
      create requests and responses using the common Python-based search
      framework, letting you adapt your search to any data source. Your
      custom search plug-in will receive search requests directly from
      Google Earth Enterprise Client (EC), or any other clients in the
      form of URL parameters.

      For example, with custom search plug-ins you can use Maps API and
      Places API to leverage the power of their data sets. You can also
      use a proprietary database, e.g., power conversion sites,
      then search for the sites using internal IDs or city names.

      A custom search plug-in may also provide a solution if you want to
      search data that is not on your Google Earth Enterprise Server or
      you want to combine Google Earth Enterprise databases in new ways.
      For example, you may want to combine a Places search with your own
      vector point data set, enabling your users to search by city name
      or facility name. The custom search plug-in would
      forward your request to both the **POI Search** plug-in and the
      **Places** plug-in and return a merged set.

      .. note::

         Google Maps API and Places API access services over
         the internet, and require licenses to use.

      .. note::

         You will need some familiarity with Python to work
         with custom search plug-ins.

      -  :ref:`Configure search queries for Google Earth EC and Google
         Maps <Configure_Search_Queries_GEEC_Maps>`
      -  :ref:`Access Search Plug-ins <Access_Search_Plugins>`
      -  :ref:`Implement your custom search plug-in <Implement_Custom_Search_Plugin>`

      .. _Configure_Search_Queries_GEEC_Maps:
      .. rubric:: Configure search queries for Google Earth EC and
         Google Maps

      Any web service, servlet, or web application you configure the
      search tabs to query:

      -  Must return valid KML to Google Earth EC.
      -  Must return valid JSONP in the specified structure.

      .. warning::

         Because search tabs are returned as JSONP, they must
         originate from the same server as the HTML page containing
         the 2D map.

      .. _Access_Search_Plugins:
      .. rubric:: Access Search Plug-ins

      The Google Earth Enterprise search plug-ins are stored in a
      default directory that you can use to add or delete your custom
      plug-ins as required. You can also modify the configuration
      file \ ``/opt/google/gehttpd/conf.d/mod_wsgi-ge.conf`` to
      register new search plug-ins with Google Earth Enterprise Server
      instead of creating search tabs. The configuration file provides a
      convenient method for managing search plug-ins with scripts, for
      example, to register newly created plug-in endpoints so that they
      can be accessible when you view your database in Google Earth EC.

      You can access the plug-ins that are installed with Google Earth
      Enterprise Server at:

      ``/opt/google/gehttpd/wsgi-bin/search/plugin``

      For example, access ``/gesearch/ExampleSearch`` in your browser as
      follows:

      ``http://your.gee.server/gesearch/ExampleSearch``

      The plug-in end points are for Google Earth Enterprise search
      plug-ins are defined in
      ``/opt/google/gehttpd/conf.d/mod_wsgi-ge.conf``.

      .. code-block:: none

         # Handles Places search requests.
           WSGIScriptAlias /gesearch/PlacesSearch /opt/google/gehttpd/wsgi-bin/search/plugin/geplaces_search_app.py
           # Handles Coordinate search requests.
           WSGIScriptAlias /gesearch/CoordinateSearch /opt/google/gehttpd/wsgi-bin/search/plugin/coordinate_search_app.py
           # Handles Federated search requests.
           WSGIScriptAlias /gesearch/FederatedSearch /opt/google/gehttpd/wsgi-bin/search/plugin/federated_search_app.py
           # Handles Example search requests.
           WSGIScriptAlias /gesearch/ExampleSearch /opt/google/gehttpd/wsgi-bin/search/plugin/example_search_app.py
           # Handles POI search requests.
           WSGIScriptAlias /POISearch /opt/google/gehttpd/wsgi-bin/search/plugin/poi_search_app.py

      .. _Implement_Custom_Search_Plugin:
      .. rubric:: Implement a custom search plug-in

      You implement a custom search plug-in by creating your own
      Python search application using WSGI, then registering the new
      search plug-in so that it can be added as you publish databases.

      .. rubric:: To implement a custom search plug-in:

      #. Create your search application, using the WSGI framework. See
         the :doc:`Example Search Python module <../geeServerAdmin/exampleSearchPluginSample>`
         for example code.
      #. Locate your search plug-in, for example ``custom_search.py``,
         in the ``/opt/google/gehttpd/wsgi-bin/search/plugin/``
         directory.
      #. Register your plug-in by adding its definition in the Python
         module configuration file:
         ``/opt/google/gehttpd/conf.d/mod_wsgi-ge.conf``. Your
         registration in the configuration file should look something
         like the following:

         .. code-block:: none

            # Handles My Custom search requests.
            WSGIScriptAlias /gesearch/CustomSearch
            /opt/google/gehttpd/wsgi-bin/search/plugin/mycustom_search_app.py

      #. Restart Google Earth Enterprise Server:

         ``# sudo /etc/init.d/geserver restart``

      #. Open the Google Earth Enterprise Server Admin console and click
         **Search tabs**. The list of Search tabs definitions appears.
      #. Click **Create New** to create a new search definition.
      #. Add a name and label.
      #. In the URL field, add the ``WSGIScriptAlias`` that you
         specified in the
         ``/opt/google/gehttpd/conf.d/mod_wsgi-ge.conf`` file. In this
         example, you would enter ``gesearch/CustomSearch``.

      #. If you want to add query parameters that are hidden from Google
         Earth EC, such as the way the results will be sorted or the
         number of results displayed at a time, enter them in the
         **Additional Query Parameters** field. The syntax is
         **key1=value1&key2;=value2**.
         For example, **sortby=name&numresults;=10** sorts by the name field and
         displays 10 results at a time. The search application must be
         able to understand and respond to these key/value pairs, so you
         must be very familiar with the search application to use this
         field.
      #. Set the fields and other options as needed and click **Save**.
         Your new custom search definition is added to the list on the
         **Search tabs** page.
      #. When you publish a database, add your search definition from
         the **Search tabs** drop-down list in the Publish dialog.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
