|Google logo|

============================================
Custom POI Search plug-in Python code sample
============================================

.. container::

   .. container:: content

      The ``Custom POI Search`` search plug-in sample code uses the GEE
      Python search module to query the **Google Places database
      (www.google.com)** via the `Google Places
      API <https://developers.google.com/places/>`_. It demonstrates
      how to construct and query an external database based on a URL
      search string, extract geometries from the result, associate
      various styles with them, and return the response back to the
      client.

      You can find the two Python files for the ``POI Search`` plug-in
      in the following location:

      -  ``/opt/google/gehttpd/wsgi-bin/search/plugin/custom_POI_search_app.py``
      -  ``/opt/google/gehttpd/wsgi-bin/search/plugin/custom_POI_search_handler.py``

      In this Python module, an implementation has been provided to
      access the `Google Places
      database <https://developers.google.com/places/>`_, format the
      results, and send the response in XML or JSON as per the
      requirement. Valid inputs are:

      -  ``location=lat,lng``
      -  ``radius = 10(in miles)``
      -  ``key = <server key required to access the Google Places database>``

      Further information about obtaining a server key is documented at:

      `https://developers.google.com/places/documentation/ <https://developers.google.com/places/documentation/>`_

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
