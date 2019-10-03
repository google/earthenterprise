|Google logo|

=========================================
Example Search plug-in Python code sample
=========================================

.. container::

   .. container:: content

      The ``ExampleSearch`` search plug-in for GEE Server provides the
      interface to query a SQL database of San Francisco neighborhoods
      via a search tab in Google Earth EC or a browser. The following
      description captures the overall mechanism of the plug-in but we
      recommend that you take a look at the code directly.

      You can find the two Python files for the ``ExampleSearch``
      plug-in in the following location:

      -  ``/opt/google/gehttpd/wsgi-bin/search/plug-in/example_search_app.py``
      -  ``/opt/google/gehttpd/wsgi-bin/search/plug-in/example_search_handler.py``

      In Google Earth Enterprise, a search plug-in Python-based
      framework relies on the Python Web Server Gateway Interface
      (WSGI), a Python API that acts as an interface between web servers
      and web applications. The web application or framework,
      ``example_search_app.py`` search plug-in, is represented to the
      server as a Python-callable method, ``__call__`` to start the
      application. The callable method must include two arguments:

      -  ``environ`` object—a list that contains the WSGI environment
         information;
      -  ``start_response`` callable, which performs an example search
         and returns the KML/JSONP format output back in the HTTP
         response.

      A request returns a ``response_body``, the search results that are
      sent back to the requesting client wrapped in a list.

      The handler, ``example_search_handler.py``, is the actual plug-in
      code. It defines a class, ``ExampleSearch``, which demonstrates
      how to construct and query a spatial database based on a URL
      search string, extract geometries from the result, associate
      various styles with them, and return the response back to the
      client. The query definition, ``RunExampleSearch``, performs a
      query search on the ``san_francisco_neighborhoods`` SQL table,
      with a response type of KML for Google Earth EC, or JSONP for a
      browser client, and returning the results as a list.

      .. rubric:: JSON object and response structure

      The JSON object defined in ``example_search_handler.py`` and the
      JSON response returned from the query follow the default map
      structure that is bundled with GEE. If you want to create your own
      map application using a custom search response, you can structure
      the JSON to take advantage of Google Maps Javascript API
      functionality.

      For example, if you have ``GeoJSON`` data, a standard for
      geospatial data on the internet, the ``google.maps.Data`` class
      allows you to add GeoJSON data to your map. The ``Data`` class
      follows the structure of ``GeoJSON`` in its data representation
      and makes it easy to import ``GeoJSON`` data and display points,
      linestrings, and polygons. For more information about the Data
      Layer of Google Maps Javascript API, see
      `https://developers.google.com/maps/documentation/javascript/datalayer <https://developers.google.com/maps/documentation/javascript/datalayer>`_.

      .. rubric:: SQL query settings

      The query that ``example_search_handler.py`` runs is hard-coded to
      the database scheme of the ``san_francisco_neighborhoods`` SQL
      table, so if you have different schema, you will need to change the
      query. The search query is standard SQL available at
      ``/opt/google/gehttpd/wsgi-bin/search/common/geconstants.py``. For
      example, the following snippet shows the SQL query used to return
      the polygon perimeters of the neighborhoods listed in the
      “san_francisco_neighborhoods” SQL table:

      .. code-block:: none

         self.example_query = (
           "SELECT ${FUNC}(ST_Force_3DZ(the_geom)) AS the_geom,Area(the_geom),"
           "Perimeter(the_geom),sfar_distr,nbrhood, "
           "GeometryType(the_geom) AS geom_type "
           "FROM san_francisco_neighborhoods "
           "WHERE "
           "lower(nbrhood) like %s"
           )

      As the ``/opt/google/gehttpd/wsgi-bin/search/common/geconstants.py``
      file includes the queries for all the system search plug-ins provided
      with Google Earth Enterprise, you can check out the various
      constructs to help you build the plug-in. We recommend that you
      keep the SQL query in the handler of the plug-in.

      .. rubric:: KML Geometric types supported

      The **ExampleSearch** plug-in returns polygons, but all KML
      geometric types are supported.

      .. tip::

         Only point type query results will fly to first element; with
         geometric types, the query result flies to a bounding box
         within which all search results are visible.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
