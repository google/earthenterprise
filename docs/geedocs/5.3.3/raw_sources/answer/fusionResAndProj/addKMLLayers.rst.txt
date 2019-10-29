|Google logo|

==============
Add KML layers
==============

.. container::

   .. container:: content

      Fusion lets you add **KML layers** in addition to fused vector
      layers. KML layers consist of a wrapper layer (typically a small,
      simple resource) that contains a link to a KML or KMZ file. The
      layer displays the linked content, not the wrapper layer content.

      KML layers are useful if you want to give users access to data
      that is not hosted on the Earth server, or to data that changes
      frequently. In fact, the free edition of Google Earth includes
      many KML layers that stream data from Google servers to the
      client.

      .. warning::

         Each vector project must include at least one resource that is
         not a KML layer. If you try to build a project consisting only
         of KML layers, the build will fail with the error "No vector
         packets for indexing."

      .. rubric:: To add a KML layer:

      #. Open your Fusion vector project and click the blank-page icon
         in the top right corner of the dialog box.

         The **Add a layer** dialog box appears.

      #. Add a resource to use as a wrapper layer for the KML or KMZ
         link.

         Users will not see the content of the wrapper layer, so add any
         small, simple resource that contains lines or polygons, such as the "CAHighways" resource in the Fusion tutorial
         dataset. If you want to use a resource that is already in your
         project, add it a second time.

      #. Right-click the new layer and select **Layer Properties**.

         The **Layer Properties** dialog box appears.

      #. In the **KML URL** field, enter the link to your KML or KMZ
         file.

         The file must be on a server that is accessible to users at all
         times. If the file is missing, the client may fail. To prevent
         this, place the file on the Google Earth server or place a
         small KML file on the Earth server that contains a NetworkLink
         reference to your target KML or KMZ file on another server.

      #. Enter a name and select an icon for the layer.
      #. Click **OK** to save your changes and exit the **Layer
         Properties** dialog box.

         The vector project lists your new KML layer.

      #. Save the vector project, then rebuild and publish your
         database.

         After the client connects to the published database, users can
         view the KML layer.

      .. rubric:: To add multiple KML layers:

      If you want to add multiple KML or KMZ files that are not hosted on
      the Google Earth server, the best practice is to place one KML
      file on the Earth server that links to all your files on other
      servers. Then you can create a layer that loads the single KML
      file. That way, even if the server that hosts your KML or KMZ
      files stops responding, the client is unlikely to fail.

      #. Place the KML or KMZ files on the server or servers that will
         host them.
      #. Create a new parent KML file that contains NetworkLink
         references to each of the KML or KMZ files.
      #. Place the parent KML file on the Earth server.
      #. In Fusion, create a KML layer that links to the parent KML file
         on the Earth server.

         In the Earth client, users can expand the parent KML layer to
         select a child KML layer.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
