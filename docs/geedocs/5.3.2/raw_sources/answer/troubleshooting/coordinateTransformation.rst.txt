|Google logo|

========================
coordinatetransformation
========================

.. container::

   .. container:: content

      .. rubric:: Error message

      ``No PROJ.4 translation for source SRS, coordinatetransformation initialization has failed. Fusion Warning: Invalid extents for raster product: nsew``

      .. rubric:: Description

      The data that you are attempting to process does not contain valid
      Projection information, or the extents of the data extend beyond
      180 (or -180) degrees.

      .. rubric:: Resolution

      To verify that data has a projection, run:

      ``geinfo {sourcedatafile}``
      
      Use ``getranslate`` to assign projection information to the data,
      or ``gereproject`` to give the data a projection that is readable
      by GEE.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
