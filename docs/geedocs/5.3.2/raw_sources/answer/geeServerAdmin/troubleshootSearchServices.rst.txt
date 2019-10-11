|Google logo|

============================
Troubleshoot search services
============================

.. container::

   .. container:: content

      Once you have added search services to a published database, you
      can query the searchable data from either Google Earth EC or from
      a browser using Google Maps. Querying search data involves sending
      a search request from the client and returning the search service
      results back to the client. If a problem occurs with
      your query, you can most likely find out by checking the log files
      for GEE Server.

      .. rubric:: Did GEE Server receive a search request from the
         client?

      Follow these steps to determine if GEE Server received the search
      request from the client:

      #. Perform a search from the client.
      #. Check the log file ``/opt/google/gehttpd/logs/access_log`` for
         the following type of ``GET`` request:

         ``192.168.1.27 - - [09/Jul/2014:16:13:28 -0700] "GET /3d/POISearch?q=San%20Francisco&flyToFirstElement;=true&displayKeys;=location&DbId;=20&PoiFederated;=1&callback;=handleSearchResults HTTP/1.1" 200 541334``

      .. rubric:: Did the search service return results to the query?

      To establish whether the search service returns results
      successfully to the client's query, you can change the
      ``logger_ge_search`` level to ``DEBUG``.

      #. In the log file, ``/opt/google/gehttpd/conf/ge_logging.conf``,
         change:

         .. code-block:: none

            [logger_ge_search]
            level=INFO

         to:

         .. code-block:: none
         
            [logger_ge_search]
            level=DEBUG

      #. Restart GEE Server:

         ``sudo /etc/init.d/geserver restart``

      #. Perform a search from the client.
      #. Check ``/opt/google/gehttpd/logs/gesearch.out`` for the
         following type of entries:

         .. code-block:: none
         
            [2014-07-09 16:13:38,162]  DEBUG    [MainThread] (poi_search_handler.py:141) - Params: ['%San Francisco%', '%San Francisco%', '%San Francisco%']
            [2014-07-09 16:13:38,479]  DEBUG    [MainThread] (poi_search_handler.py:515) - Parsed search tokens: San Francisco
            [2014-07-09 16:13:38,479]  DEBUG    [MainThread] (poi_search_handler.py:139) - Querying the database gepoi, at port 5432, as user geuser on hostname 127.0.0.1.
            014-07-09 16:13:38,479]  DEBUG    [MainThread] (poi_search_handler.py:140) - Query: SELECT ST_AsGeoJSON(the_geom) AS the_geom, "posn" FROM gepoi_9 WHERE ( lower("posn") LIKE %s )
            [2014-07-09 16:13:38,479]  DEBUG    [MainThread] (poi_search_handler.py:141) - Params: ['%San Francisco%']
            [2014-07-09 16:13:38,500]  DEBUG    [MainThread] (poi_search_handler.py:471) - poi search returned 2516 results
            [2014-07-09 16:13:38,501]  DEBUG    [MainThread] (poi_search_handler.py:472) - results: [[{'field_value': '{"type":"Point","coordinates":[-70.250012045966002,19.29999635719]}', 'field_name': 'geom', 'is_search_display': True, 'is_searchable': True, 'is_displayable': True}, {'field_value': 'San Francisco de Macoris', 'field_name': 'name', 'is_search_display': True, 'is_searchable': True, 'is_displayable': True}],
            ...

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
