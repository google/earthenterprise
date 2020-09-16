|Google logo|

================
Search tabs page
================

.. container::

   .. container:: content

      The **Search tabs** page of the :doc:`GEE Server Admin
      console <../geeServerAdmin/signInAdminConsole>` is where you can customize
      search services. From this page you can:

      -  **View the default search tabs**:

         -  **POISearch**: Search from select vector and map layer
            fields.
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

      -  **Create new search tabs**: Click **Create New** and provide a
         search name, label, and parameters.
      -  **Edit search tabs**: Check the box next to a search tab and click
         **Edit**. Editing a default search tab creates a new search tab
         with your changes.
      -  **Delete custom search tabs**: Check the box next to a custom
         search tab and click **Delete**.

      .. rubric:: Learn more

      -  :doc:`../geeServerAdmin/createSearchTabs`
      -  :doc:`Specifying search fields for individual
         layers <../fusionTutorial/searchFldsForLayers>`
      -  :doc:`Configuring a searchable database <../fusionTutorial/confSearchableDB>`

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px

.. |deg| unicode:: U+00B0 .. DEGREE
.. |prime| unicode:: U+2032 .. PRIME
.. |Prime| unicode:: U+2033 .. DOUBLE PRIME
