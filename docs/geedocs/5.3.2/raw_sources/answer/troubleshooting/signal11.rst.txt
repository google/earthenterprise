|Google logo|

=========
Signal 11
=========

.. container::

   .. container:: content

      .. rubric:: Error message

      ``Process terminated with signal 11``
      
      .. rubric:: Description

      The error appears in the project Asset log and the terminal window
      when your server has run out of memory. The vector project that
      cannot build is using all the memory on the server and the build
      cannot finish.

      .. rubric:: Resolution

      Check and correct for any of these issues:

      -  A vector data set that has polygons inside of polygons.
      -  A default simplification method set to a low number, such as
         .5.
      -  A large vector data set or a data set with lots of vertices.
      -  A vector data set with the LOD set for a large range, such as
         4-18. Typically, you should set the LOD to:

         -  Levels 4-10 for items such as country borders.
         -  Levels 10-12 for regional data, such as large rivers and
            lakes.
         -  Above level 12 for city-wide data, such as small local
            roads, parcel data, census block groups, and zip codes.
         -  Above level 18 is only only for very detailed data. Levels
            above 18 exponentially increase your processing times.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
