|Google logo|

=========================
Import pre-processed data
=========================

.. container::

   .. container:: content

      You can import pre-processed data into Fusion. The data comes in
      folders that have Fusion-specific extension names. For example,
      raster data is inside a pair of Imagery and Terrain folders that
      have the same file name with different extensions. Vector data is
      in a single folder with a .kvp extension. There is a diferent
      Fusion import command or command option for each extension. The
      extensions and their import commands are described below:

      +-----------------------+-----------------------+--------------------------+
      | File Extension        | Data Type             | Import Command           |
      +=======================+=======================+==========================+
      | .kip                  | Imagery               | ``genewimageryresource`` |
      |                       |                       |                          |
      +-----------------------+-----------------------+--------------------------+
      | .ktp                  | Terrain               | ``genewterrainresource`` |
      |                       |                       |                          |
      +-----------------------+-----------------------+--------------------------+
      | .kvp                  | Vector                | ``genewvectorresource``  |
      |                       |                       |                          |
      +-----------------------+-----------------------+--------------------------+
      | .kmp                  | Mask                  | ``--havemask`` command   |
      |                       |                       | option                   |
      |                       |                       | Use the ``--nomask``     |
      |                       |                       | option to import         |
      |                       |                       | files without a mask.    |
      +-----------------------+-----------------------+--------------------------+

      .. rubric:: To import pre-processed data to Fusion:

      #. Copy the data to an appropriate directory on your Fusion
         machine. For example, for imagery: ``/gevol/src/imagery/...``
      #. Verify that ``read`` and ``execute`` permissions are set on the
         folder and subfiles so that the Fusion user (``gefusionuser``
         by default) can access and read the files.
      #. Open a terminal window on your processing machine, then change
         directory to your asset root:
         ``cd /gevol/assets``
      #. Use the appropriate command for the type of data you have:
         imagery, terrain, or vector. Your command must contain either
         the ``--havemask`` or ``--nomask`` option. To import imagery
         that includes pre-processed mask files, use ``-havemask``. If
         your raster data does not have a mask, use ``-nomask``.
         For example, the NASA Blue Marble imagery does not have a mask.
         If you import it without using ``-nomask``, it displays a black
         line and circular gaps at each pole because the map edges are
         masked and the center of the earth is exposed.
         Commands are in the format:
         ``[commandname] [options] [--meta <key>=<value>]... -o <assetname> { --filelist <file> | <sourcefile> ...}``
         For all ``genew{imagery,terrain,vector}resource`` tools, the
         output path (``-o path``) must be relative to the asset root
         directory.
         You can specify source file names on the command line or in a
         file list. To view supported command options, enter
         ``[commandname] --help | -?``

         +-----------------------+-----------------------+-----------------------+
         | File Type             | Command               | Example               |
         +=======================+=======================+=======================+
         | .kip (Imagery)        | ``genewimageryresourc | To add files named    |
         |                       | e --provider <PROVIDE |   ``SanFrancisco.kip` |
         |                       | R_KEY> --havemask -o  | `                     |
         |                       | \path/to/resource/dir |   and                 |
         |                       | ectory/resourcename \ |   ``SanFrancisco.kmp` |
         |                       |  /path/to/imagery.kip | `                     |
         |                       | ``                    |   to the              |
         |                       |                       |   ``/gevol/assets/ima |
         |                       |                       | gery/noam/usa/ca/SanF |
         |                       |                       | rancisco``            |
         |                       |                       |   directory:          |
         |                       |                       | ``genewimageryresou   |
         |                       |                       | rce --havemask -o \ i |
         |                       |                       | magery/noam/usa/ca/Sa |
         |                       |                       | nFrancisco \ /gevol/s |
         |                       |                       | rc/imagery/noam/usa/c |
         |                       |                       | a/SanFrancisco.kip``  |
         +-----------------------+-----------------------+-----------------------+
         | .ktp (Terrain)        | ``genewterrainresourc | To add files named    |
         |                       | e --provider <PROVIDE | ``GTOPO30.ktp`` and   |
         |                       | R_KEY> --havemask -o  | ``GTOPO30.kmp`` to    |
         |                       | \ path/to/resource/di | the                   |
         |                       | rectory/resourcename  | ``/gevol/assets/terra |
         |                       | \ /path/to/terrain.kt | in/world/``           |
         |                       | p``                   | directory:            |
         |                       |                       | ``genewterrainresourc |
         |                       |                       | e --havemask -o \ ter |
         |                       |                       | rain/world/GTOPO30 \  |
         |                       |                       | /gevol/src/terrain/wo |
         |                       |                       | rld/GTOPO30.ktp``     |
         +-----------------------+-----------------------+-----------------------+
         | .kvp (Vector)         | ``genewvectorresource | To add a file named   |
         |                       |  --provider <PROVIDER |   ``income.kvp`` to   |
         |                       | _KEY> --encoding <ENC |   the                 |
         |                       | ODING_SCHEME> -o \ pa |   ``/gevol/assets/vec |
         |                       | th/to/resource/direct | tor/noam/usa/demograp |
         |                       | ory/resourcename \ /p | hic/``                |
         |                       | ath/to/vector.kvp``   |   directory:          |
         |                       |                       | ``genewvectorresour   |
         |                       |                       | ce -o \ vector/noam/u |
         |                       |                       | sa/demographic/income |
         |                       |                       |  \ /gevol/src/vector/ |
         |                       |                       | processed/noncommerci |
         |                       |                       | al/nationalatlas/inco |
         |                       |                       | me.kvp``              |
         +-----------------------+-----------------------+-----------------------+

      #. Use the ``gebuild`` command to build the asset:
         ``gebuild path/to/resource/directory/resourcename``

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
