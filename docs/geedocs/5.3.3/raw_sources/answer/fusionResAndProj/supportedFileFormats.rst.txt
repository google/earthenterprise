|Google logo|

=============================
Supported file formats in GEE
=============================

.. container::

   .. container:: content

      The following tables list the raster data (imagery and terrain)
      file formats and the vector file formats that are supported by
      **Google Earth Enterprise (GEE) Fusion**.

      Imagery data consists of satellite and overhead photographs.
      Terrain data provides topographical information about a geographic
      area.

      Both imagery and terrain data are types of raster data, that is,
      characterized by a grid of cells covering an area of interest,
      where each pixel, the smallest unit of information in the grid,
      displays a unique attribute. All raster data must have geographic
      coordinates and projection information included in the file
      headers or contained in external text-based world files and
      projection files. All imagery data must be in 8-bit format with
      either one band for panchromatic or three bands for color. All
      terrain data can be in 16-bit or 32-bit format with a single band.

      In Google Earth EC, the imagery data is draped over the terrain
      data, giving the imagery a topographical appearance.

      Google Earth Enterprise Fusion imports common imagery and terrain
      data formats shown below, as a subset of the `raster formats
      supported by GDAL <http://www.gdal.org/formats_list.html>`_.

      .. rubric:: Supported imagery and terrain data formats

      .. list-table::
         :widths: 20 60
         :header-rows: 1

         * - File Format
           - Notes
         * - DTED
           -
         * - Erdas Imagine (IMG)
           -
         * - GeoTIFF
           -
         * - GIF
           - Geographic coordinates and projection information must be accompanied
             by external world and projection files.
         * - JPEG
           - Geographic coordinates and projection information must be accompanied
             by external world and projection files.
         * - JPEG2000
           -
         * - Microstation (DGN)
           - Microstation DGN files from prior to version 8 are supported.
             Versions 8 and later are not supported.
         * - MrSID
           -
         * - NITF
           -
         * - PNG
           - Geographic coordinates and projection information must be accompanied
             by external world and projection files.
         * - TAB
           -
         * - TIF
           - Geographic coordinates and projection information must be accompanied
             by external world and projection files.
         * - USGS ASCII DEM
           -
         * - USGS SDTS DEM
           -

      .. tip::

         **Importing large imagery files?** Although Fusion does not
         allow you to import raw imagery source files larger than 80 GB,
         you can split large resource files into smaller ones using the
         ``gesplitkhvr`` tool. See the tutorial :doc:`../fusionTutorial/segmentLargeImageryFiles`.

      .. rubric:: Supported vector data formats

      Vector data consists of geographic features that are either
      geographic coordinates (points), sequences of connected geographic
      coordinates (lines), or closed sequences of connected geographic
      coordinates (polygons). Each feature typically has attribute
      fields, such as name, street address, or a web site URL.

      Google Earth Enterprise Fusion imports data in the common vector
      and point file formats, as a subset of the `OGR vector
      formats <http://www.gdal.org/ogr_formats.html>`_.

      .. list-table::
         :widths: 20 65
         :header-rows: 1

         * - File Format
           - Notes
         * - ESRI Shape File (.shp)
           - For each ESRI shape file you import into Google Earth Enterprise
             Fusion, one DBF and one SHX configuration file, each with the same
             name as the original and the appropriate extension (.dbf and .shx)
             must be located in the same folder. In addition, if there is any
             projection in the image, a PRJ file with the same name as the
             original file and the appropriate extension (.prj) must be located
             in the same folder. Other associated files you can include with
             each SHP file are SBN, SBX, CPG, and LYR.
         * - Generic ASCII
           - Point data only in comma-separated values or tab-delimited text
             format.
         * - KML/KMZ
           -
         * - MapInfo File (.tab)
           -
         * - US Census Tiger Line Files
           -

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
