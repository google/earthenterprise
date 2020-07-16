|Google logo|

======
No SRS
======

.. container::

   .. container:: content

      .. rubric:: Error message

      ``No SRS``
      
      .. rubric:: Description

      This error appears if you try to import a resource that has no
      projection or Spatial Reference System associated with the vector,
      imagery, or terrain data.

      .. rubric:: Resolution

      Make sure you have the appropriate ``.prj`` file, header in the
      imagery (if applicable) and that the projection is valid. If there
      is no projection information, reproject the imagery using
      ``getranslate`` (or, in 2.4, ``khtranlsate``) for imagery or
      terrain, or ``ogr2ogr`` for vector data.

      You can use ``geinfo`` to provide more information about the
      imagery asset, including projection information, the number of
      bands, and the imagery resolution:

      ``geinfo {image file}``
      
      To work with the Fusion server, imagery must be of type ``Byte``,
      have no more than three bands, and must have a projection.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
