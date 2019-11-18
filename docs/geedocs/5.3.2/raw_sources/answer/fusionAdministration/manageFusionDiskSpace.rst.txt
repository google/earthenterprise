|Google logo|

========================
Manage Fusion disk space
========================

.. container::

   .. container:: content

      Fusion writes all assets, projects, and databases to the asset
      root directory (typically named ``/gevol/assets``). As a result,
      the asset root can fill up quickly.

      You can put source data, such as raw images, in a different
      directory like ``/gevol/src``, but the asset root can still fill
      up from processing source files. Cleaning assets, projects, and
      databases helps reduce the disk use in the asset root, but
      eventually the volume will fill up completely.

      A good way to create free space in your asset root is to move your
      pyramid files to auxiliary storage. When you create and build a
      new imagery asset, pyramid (``.pyr``) files are also created, and
      are saved to the ``raster.kip`` and ``mask.kmp`` directories.
      Google delivers most of its data to customers in pyramid format.
      The pyramid files must always be available to the Fusion server,
      but after they are built, they do not change size. That makes them
      good candidates for moving to a separate storage location.

      The result of moving the ``.pyr`` files is an asset that stores
      configuration files in the asset root and large pyramid files in
      auxiliary storage. Besides creating space in your asset root,
      there are other advantages to moving your pyramid files:

      -  You can share assets across Fusion installations, or copy
         assets from one system to another.
      -  You can create assets on stand-alone computers and copy them to
         the grid later.
      -  You can manage disk space more easily. Because pyramid files do
         not grow or shrink, you can keep filling a volume with the
         pyramid files until it is full, then add another volume.
      -  You can store metadata (like the security level, the date the
         source file was acquired, the bounding box, the resolution, the
         sensor type, and any other relevant data) with each pyramid
         directory. If you want to use the metadata in a new imagery
         project, it is more easily accessible than if it is buried in
         the asset tree.

      .. rubric:: Moving pyramid files

      There are a few different ways to move pyramid files out of an
      asset root that is filling up:

      -  Move files out of the asset root and create symbolic links to
         the files. To do this, copy the entire ``raster.kip``
         directory to a different location and then symbolically link
         ``raster.kip`` to the new location with the ``ln -s``
         command:

         .. code-block:: bash

            ln -s /path/to/new/raster.kip

         This option is expedient and can provide significant short-term
         relief to a full asset root. However, extensive use of symbolic
         links can increase the potential risk of Fusion or publish
         errors, and is not good administrative practice.

      -  Set up KRP and KRMP :ref:`task rules <Configure_Task_Rules>` to
         direct Fusion to write the pyramid files to a different storage
         location instead of to the asset root.

      -  Copy the pyramid files from the asset root and to other
         storage, then recreate the asset. See the instructions below.

      .. rubric:: To copy the pyramid files and recreate the asset:

      After the ``raster.kip`` and ``mask.kmp`` directories are created
      and the asset is finished building, all of the information needed
      to copy and reuse the asset is inside these two directories.
      There are other files in the asset directory, but they are
      auxiliary and not needed. The basic strategy for managing disk
      space is:

      #. Locate the asset to be moved. It will be in a directory with a
         ``.kiasset`` extension.
      #. Locate the ``raster.kip`` and the ``mask.kmp`` directories
         under the asset directory:

         .. code-block:: bash

            find $asset.kiasset -type d -name raster.kip
            find $asset.kiasset -type d -name mask.kmp

      #. Copy the ``raster.kip`` and ``mask.kmp`` directories to the
         auxiliary storage volume:

         .. code-block:: bash

            cp -av $kip $new_location/$asset.kip
            cp -av $kmp $new_location/$asset.kmp

         **Note**: The auxiliary storage volume must be a defined Fusion
         volume. You can define the new volume with the
         ``geconfigurefusionvolume`` command. Volumes are presented to
         the server as NFS file systems, and those cannot be nested. For
         example, NFS does not allow ``Volume1`` and ``Volume2`` to be
         mounted as ``/Volume1/Volume2``.

      #. Rename the ``raster.kip`` and ``mask.kmp`` files so that they
         have the same name. The name should be descriptive of the
         asset. For example, if the asset is ``EastChicago.kiasset``,
         the directories should be called ``EastChicago.kip`` and
         ``EastChicago.kmp``.
      #. Modify the imagery asset with the ``gemodifyimageryasset``
         command, using the same name that the asset was originally
         created with:

         .. code-block:: bash

            gemodifyimageryasset -o imagery/EastChicago -havemask /gevol/newvolume/imagery/EastChicago.kip

         For more details about the commands to recreate the assets,
         see *Importing Preprocessed Resources* in the Google Earth
         Enterprise Reference Guide.

      #. Rebuild the imagery project and any database that contains the
         imagery project:

         .. code-block:: bash

            gebuild imagery/EastChicago

         The ``gemodifyimageryasset`` and ``gebuild`` commands will
         complete in seconds, because the heavy processing took place
         when the pyramid files were generated.

      #. Clean the imagery projects and databases using the Fusion UI:

         a. From the **Asset Manager**, right-click the project or
            database and select **Asset Versions**.
         b. Right-click the previous version and select **Clean**.
            Cleaning the project and database also cleans all the
            assets, removing the pyramid files from the asset root and
            freeing up quite a bit of space.
         c. Verify that each of the assets that were modified were
            cleaned, and that the pyramid files were removed from the
            asset root.

      **Note**: The asset expects the pyramid files to remain in the
      same location you specified in the ``gemodifyimageryasset``
      command. Do not move the pyramid files to a new location after
      you have copied them and then modified, built, and cleaned the
      asset.

      .. rubric:: Example script
         :name: example-script

      The example below copies all the pyramid files from
      ``/gevol/assets/imagery`` to ``/gevol/volume1``.

      .. code-block:: bash

         # The commands are echoed to the terminal so you can review them before executing.
         # To enable the commands, uncomment the following line:
         # do_command=true

         asset_root=/gevol/assets
         asset_directory=Resources/Imagery
         new_location=/gevol/volume1

         cd $asset_root/$asset_directory
         for asset in `ls | grep kiasset | sed 's/\.kiasset//'`
         do
            # Find the raster.kip and mask.kmp under the asset directory
            kip=`find $asset.kiasset -type d -name raster.kip`
            kmp=`find $asset.kiasset -type d -name mask.kmp`

            # Copy the raster and mask directories to the new volume
            echo "cp -av $kip $new_location/$asset.kip"
            if [ "$do_command" == "true" ]; then cp -av $kip $new_location/$asset.kip; fi
            echo "cp -av $kmp $new_location/$asset.kmp"
            if [ "$do_command" == "true" ]; then cp -av $kmp $new_location/$asset.kmp; fi

            # modify and build each of the imagery assets
            echo "gemodifyimageryresource --havemask -o $asset_directory/$asset $new_location/$asset.kip"
            #if [ "$do_command" == "true" ]; then gemodifyimageryresource --havemask -o $asset_directory/$asset $new_location/$asset.kip; fi
            echo "gebuild $asset_directory/$asset"
            if [ "$do_command" == "true" ]; then gebuild $asset_directory/$asset; fi
         done

         # Rebuild, then clean the imagery project and the database,
         # then verify that all the assets were cleaned.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
