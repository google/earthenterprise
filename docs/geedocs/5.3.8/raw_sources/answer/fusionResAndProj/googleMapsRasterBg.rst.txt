|Google logo|

=============================
Google Maps raster background
=============================

.. container::

   .. container:: content

      Creating a Google Maps raster background for your vector data is
      useful to speed up processing times for large globes. Trying to
      create a vector-only globe can result in very long processing
      times. For example, building an ocean or a large continent using
      only vector data can take weeks or months. Instead, you can use
      Google Maps raster data to create a basic 2D background in either
      Mercator or Flat projection, then you can add vector layers like
      roads, cities, and borders.

      .. rubric:: Overview of steps

      These are the main steps to create a Google Maps raster
      background:

      #. Set up your computer.
      #. Obtain the data.
      #. Create the blue background tiles.
      #. Add the vector data.
      #. Create the resource and project.

      .. rubric:: To create a Google Maps raster background:

      The instructions below show how to create the raster background
      starting with the NASA 500-meter base. At this resolution,
      shorelines pixelate slightly at higher zoom levels. If you want a
      finer shoreline, you can start with a higher resolution base or
      re-sample the 500-meter base with increased resolution.

      #. Set up your computer:

         -  Make sure you have at least 20 GB of hard disk space.
         -  Make sure you have all the GDAL binaries. Fusion installs
            ``gdal_translate`` and ``gdal_rasterize``, but Fusion
            does not include the ``gdal_merge`` script. To obtain
            ``gdal_merge``, download the latest GDAL binaries from the
            GDAL site or download FWTools and install it in the
            ``/usr/local`` directory. FWTools includes all the GDAL
            binaries and a few other useful tools.
         -  If you want to view the resulting tiles before you fuse them
            with Fusion, install GIMP on your Linux machine.

      #. Obtain the data:

         -  Download the A1 file in the Nasa Blue Marble July
            data set from
            `https://visibleearth.nasa.gov/view_cat.php?categoryID=1484 <https://visibleearth.nasa.gov/view_cat.php?categoryID=1484>`_.
            You can also download the BlueMarble A1 file from an HTTP
            mirror of the BlueMarble site at the Arctic Region
            Supercomputing Center at
            `http://mirrors.arsc.edu/nasa/world_500m/ <http://mirrors.arsc.edu/nasa/world_500m/>`_.
            
            .. note::
               There are eight files, named A1, A2, B1, B2, C1,
               C2, D1, and D2. Download the A1 file only. The A1 tile
               represents the top left corner of the globe, D2 is the
               bottom right, and so on. You will generate the remaining seven
               tiles later.

         -  Georeference the A1 file data:

            .. code-block:: none
            
               gdal_translate -a_srs WGS84 -a_ullr -180 90 -90 0
               world.topo.bathy.200407 .3x21600x21600.A1.png
               world.topo.bathy.200407.3x21600x21600.A1.tif

         -  Download the shoreline dataset from:
            `http://www.ngdc.noaa.gov/mgg/shorelines/gshhs.html <http://www.ngdc.noaa.gov/mgg/shorelines/gshhs.html>`_. This
            dataset is in a single ``.zip`` file that expands into
            several directories named ``c``, ``l``, ``i``, ``h``, and
            ``f``. The lowest resolution is in the ``c`` directory and
            the highest is in the ``f`` directory. There are four
            shapefiles in each directory:

            L1 = shorelines
            L2 = lakes and rivers
            L3 = islands in lakes
            L4 = ponds within islands in lakes

      #. Create the blue background tiles:

         -  Create the A1 tile for the ocean in the right size and in
            the Google Maps shade of blue. The NASA ``.tif`` geospatial
            image file is already the right size, so you can just edit
            its color layers to create a tile with the Google Maps RGB
            value of 153, 179, 204:

            .. code-block:: none

               gdal_translate -b 1 -scale 0 255 153 153 world-topo-bathy-200407-3x21600x21600-A1.tif red.tif
               gdal_translate -b 1 -scale 0 255 179 179 world-topo-bathy-200407-3x21600x21600-A1.tif green.tif
               gdal_translate -b 1 -scale 0 255 204 204 world-topo-bathy-200407-3x21600x21600-A1.tif blue.tif

         -  Merge the layers to form a 3-band ``.tif`` file called
            ``GoogleBlueA1.tif:``

            .. code-block:: none

               /usr/local/FWTools-2.0.6/bin_safe/gdal_merge.py -o
               GoogleBlueA1.tif -separate red.tif green.tif blue.tif

         -  Make a copy of this blue tile to use later:

            ``cp GoogleBlueA1.tif GoogleBlue.tif``

         -  Create the remaining seven blue tiles by using
            ``gdal_translate`` and inserting the appropriate corner
            coordinates:

            .. code-block:: none

               gdal_translate -a_ullr -180  0 -90 -90 GoogleBlue.tif GoogleBlueA2.tif
               gdal_translate -a_ullr  -90 90   0   0 GoogleBlue.tif GoogleBlueB1.tif
               gdal_translate -a_ullr  -90  0   0 -90 GoogleBlue.tif GoogleBlueB2.tif
               gdal_translate -a_ullr    0 90  90   0 GoogleBlue.tif GoogleBlueC1.tif
               gdal_translate -a_ullr    0  0  90 -90 GoogleBlue.tif GoogleBlueC2.tif
               gdal_translate -a_ullr   90 90 180   0 GoogleBlue.tif GoogleBlueD1.tif
               gdal_translate -a_ullr   90  0 180 -90 GoogleBlue.tif GoogleBlueD2.tif

      #. Add the vector data:

         -  The shoreline data creates the continents, and the lake data
            creates the lakes within the continents. These instructions
            use the ``h`` level of resolution from the shoreline data set
            because the blue marble is only in 500-meter resolution. If
            you re-sample the blue marble to a higher resolution, you
            might want to use one of the higher-resolution shorelines.

            The L1 layer for shorelines uses the RGB value for Google
            Maps land areas, which is 242, 239, 233. The L2 layer for the
            lakes uses the same RGB value as the ocean (153, 179, 204).
            
            Burn the shorelines and lakes into the blue tiles one at a
            time:

            .. code-block:: none

               gdal_rasterize -b 1 -burn 242 -b 2 -burn 239 -b 3 -burn 233 -l GSHHS_h_L1 GSHHS_h_L1.shp GoogleBlueA1.tif
               gdal_rasterize -b 1 -burn 153 -b 2 -burn 179 -b 3 -burn 204 -l GSHHS_h_L2 GSHHS_h_L2.shp GoogleBlueA1.tif
               
               gdal_rasterize -b 1 -burn 242 -b 2 -burn 239 -b 3 -burn 233 -l GSHHS_h_L1 GSHHS_h_L1.shp GoogleBlueB1.tif
               gdal_rasterize -b 1 -burn 153 -b 2 -burn 179 -b 3 -burn 204 -l GSHHS_h_L2 GSHHS_h_L2.shp GoogleBlueB1.tif

               gdal_rasterize -b 1 -burn 242 -b 2 -burn 239 -b 3 -burn 233 -l GSHHS_h_L1 GSHHS_h_L1.shp GoogleBlueC1.tif
               gdal_rasterize -b 1 -burn 153 -b 2 -burn 179 -b 3 -burn 204 -l GSHHS_h_L2 GSHHS_h_L2.shp GoogleBlueC1.tif
               
               gdal_rasterize -b 1 -burn 242 -b 2 -burn 239 -b 3 -burn 233 -l GSHHS_h_L1 GSHHS_h_L1.shp GoogleBlueD1.tif
               gdal_rasterize -b 1 -burn 153 -b 2 -burn 179 -b 3 -burn 204 -l GSHHS_h_L2 GSHHS_h_L2.shp GoogleBlueD1.tif
               
               gdal_rasterize -b 1 -burn 242 -b 2 -burn 239 -b 3 -burn 233 -l GSHHS_h_L1 GSHHS_h_L1.shp GoogleBlueA2.tif
               gdal_rasterize -b 1 -burn 153 -b 2 -burn 179 -b 3 -burn 204 -l GSHHS_h_L2 GSHHS_h_L2.shp GoogleBlueA2.tif
               
               gdal_rasterize -b 1 -burn 242 -b 2 -burn 239 -b 3 -burn 233 -l GSHHS_h_L1 GSHHS_h_L1.shp GoogleBlueB2.tif
               gdal_rasterize -b 1 -burn 153 -b 2 -burn 179 -b 3 -burn 204 -l GSHHS_h_L2 GSHHS_h_L2.shp GoogleBlueB2.tif
               
               gdal_rasterize -b 1 -burn 242 -b 2 -burn 239 -b 3 -burn 233 -l GSHHS_h_L1 GSHHS_h_L1.shp GoogleBlueC2.tif
               gdal_rasterize -b 1 -burn 153 -b 2 -burn 179 -b 3 -burn 204 -l GSHHS_h_L2 GSHHS_h_L2.shp GoogleBlueC2.tif
               
               gdal_rasterize -b 1 -burn 242 -b 2 -burn 239 -b 3 -burn 233 -l GSHHS_h_L1 GSHHS_h_L1.shp GoogleBlueD2.tif
               gdal_rasterize -b 1 -burn 153 -b 2 -burn 179 -b 3 -burn 204 -l GSHHS_h_L2 GSHHS_h_L2.shp GoogleBlueD2.tif

      #. Create the resource and project:

         -  Create a single imagery resource from these eight tiles,
            then create an imagery project that contains this resource
            and nothing else.
         -  Use this imagery project for the 2D map. You can also
            create a Mercator resource and project out of these same
            tiles.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
