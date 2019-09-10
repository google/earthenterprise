|Google logo|

=============
Settings page
=============

.. container::

   .. container:: content

      Click the |gear icon| **Settings** icon in the :doc:`GEE Server Admin
      console <3470759>` to:

      .. rubric:: Perform .glc assembly

      Create new .glc globes from existing globes (.glm/.glb):

      #. Enter a name and description for the new .glc.
      #. Paste **Polygon** KML for use with the new globe.
      #. Select either **2D** layers to build a .glc using selected
         layers of existing .glm files, or **3D** layers to build a
         .glc using .glb files at the layers.
      #. Click **Assemble .glc**.

      .glc files are added to the default globes directory when
      they are created.

      See :doc:`4643041`.

      .. rubric::  Perform .glc disassembly

      Extract .glm/.glb files:

      #. Select a .glc to disassemble.
      #. Enter a **New output directory** where the files will be
         placed.
      #. Click **Disassemble glc**.

      See :doc:`4643041`.

      .. rubric:: Launch Cutter

      Create a new .glm/.glb from published Fusion databases or
      Portable globes (.glm/.glb, not .glc):

      #. Select a source map.
      #. Enter a name and description for the offline map.
      #. Choose to **Overwrite** existing globes with the same
         name.
      #. Choose a **Region Selection** method:

         #. **Manual**: Manually draw the boundaries of your
            region using the polygon button in the upper-right of
            the map.
         #. **Paste KML**: Define your region by pasting a KML
            file.

      #. Specify a **World level**; this represents the highest
         resolution at which the entire world will be available.
      #. Optionally use the zoom buttons in the lower-right of the
         map to change your zoom level. This represents the
         highest resolution at which the data within the region of
         interest will be available.
      #. Click **Cut map**.

      The cut progress will be displayed at the bottom of the
      screen. Upon successful completion, the globe is saved to
      the default globes directory.

      See :doc:`3230777`.

      .. rubric:: View Apache logs

      View Apache logs for troubleshooting errors or unexpected
      behavior with GEE Server.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
.. |gear icon| image:: ../../art/server/admin/accounts_icon_gear_padded.gif
