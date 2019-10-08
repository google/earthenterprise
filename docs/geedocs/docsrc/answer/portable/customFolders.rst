|Google logo|

==============
Custom folders
==============

.. container::

   .. container:: content

      By default, GEE Portable stores cut globes in the Apache Document
      root of the GEE Server at
      ``/opt/google/gehttpd/htdocs/cutter/globes``. However, you can
      move the portable globes folder to a custom location. You might
      want to do this, for example, if this path is within a volume with
      limited storage space and you have space available in a separate
      volume.

      .. rubric:: Before you begin

      -  Install and configure GEE Server on the machine that will
         support users who create portable globes.
      -  Identify the path to your globes folder. This can be the same
         mount point for external storage of source data or the GEE
         Server Publish Root.
      -  Gain access to an account with root-level privileges.

      .. rubric:: To create a custom globes folder:

      #. Make sure that no globe cutting operations are in progress.
         Creating a custom folder makes modifications directly to the
         folder where portable globes are hosted and can interfere with
         any ongoing globe cutting operations.
      #. Stop the GEE Server service:
         ``sudo /etc/init.d/geserver stop``
      #. Use either of the methods below to set up your custom folder:

         -  Create a new folder:
            ``mkdir /gevol/src/globes``
         -  Move the original folder to a storage volume:
            ``mv /opt/google/gehttpd/htdocs/cutter/globes /gevol/src/globes``

      #. Create a symbolic link from the custom folder to the original
         folder location:
         ``ln -sf /gevol/src/globes /opt/google/gehttpd/htdocs/cutter/globes``
      #. Change ownership of the custom folder to
         ``geapacheuser:gegroup`` and change permissions to 755
         (rwxr-wr-w):
         ``chown -R geapacheuser:gegroup /gevol/src/globes``
         ``chmod -R 755 /gevol/src/globes``
      #. Start the GEE Server service.
      #. To test that globe cutting works, load the GEE Globe Cutting
         web page and either create a new portable globe or test with an
         existing globe. If you have an existing globe, click **List
         globes and maps directory** in the GEE Globe Cutting web page
         and make sure that your globe appears there.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
