|Google logo|

===============
Map projections
===============

.. container::

   .. container:: content

      Google Maps use Mercator projection for browser-based maps. When
      Fusion creates a map that uses **maps.google.com** imagery as the
      source, it uses Google Maps projection for the rasterized vectors
      so that they match the underlying Google Map tiles. This EPSG code
      is **EPSG:900913** or **EPSG:3785**.

      However, GEE builds 3D data using the
      `WGS84 <http://en.wikipedia.org/wiki/World_Geodetic_System>`_
      datum (**EPSG:4326**). If you want to reuse 3D database
      resources, you can create a flat map using the Fusion Flat (Plate
      Carrée) projection.

      If you build a Mercator-based database, you must build resources
      specifically for use with Mercator-based projection.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
