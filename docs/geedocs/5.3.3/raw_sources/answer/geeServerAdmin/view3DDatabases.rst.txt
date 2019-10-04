|Google logo|

=================
View 3D databases
=================

.. container::

   .. container:: content

      You can view your globes using Google Earth Enterprise Client (EC)
      or in a browser.

      -  :ref:`View a globe in Google Earth EC <View_Globe_GEEC>`
      -  :ref:`View a globe using Google Earth Plugin <View_Globe_Browser>`

      .. _View_Globe_GEEC:
      .. rubric:: View a globe in Google Earth EC

      Google Earth Enterprise Client lets you connect directly with
      your organization’s private globes. You can sign in to as many
      databases as you want.

      .. rubric:: To view a globe in Google Earth EC:

      #. Launch Google Earth EC. The Select Server dialog appears.
      #. Specify the **Publish point** by entering or selecting the URL
         or IP address of your server and database in the **Server** field.

         For example, a Publish point of **BayAreaHighways** would be
         hosted at *http://myserver.mydomainname.com*/BayAreaHighways,
         where *myserver* and *mydomainname* are specific to your
         server.

      #. Click the **Sign In** button.

         .. warning::

            If you have logged in to this server with Google Earth EC
            previously, log out, clear your cache, and log back in. For
            help with clearing your cache, see :doc:`../googleEarthEnterpriseClient/clearGoogleEarthEC`.

      #. Google Earth EC displays your database. The Layers panel shows
         the terrain, imagery, and vector layers in the published
         database.

      .. rubric:: Learn more

      -  :doc:`What is Google Earth EC? <../googleEarthEnterpriseClient/whatisEC>`
      -  :doc:`../googleEarthEnterpriseClient/clearGoogleEarthEC`

      .. _View_Globe_Browser:
      .. rubric:: Viewing 3D databases in a browser

      You can view 3D globes from a browser using **Preview** if you
      have access to Google Earth Plugin and a supported browser.
      However, Google Earth Plugin is now deprecated and does not work
      in modern browsers that do not implement NPAPI.

      .. rubric:: To view your map database from Google Earth Enterprise
         Server:

      #. Log in to the Admin console of Google Earth Enterprise Server.
         The Admin console opens to the **Databases** page.
      #. Check the box next to the 3D database that you want to view.
      #. Click **Preview** or the **Publish point** of the database you
         selected. A new browser tab opens displaying your 3D database.

      .. rubric:: Learn more

      -  :doc:`../geeServerAdmin/implementEarthPlugin`
      -  :doc:`../geeServerAdmin/provideEarthPlugin`

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
