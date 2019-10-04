|Google logo|

=======================================
Select, add, or change Earth EC servers
=======================================

.. container::

   .. container:: content

      You can use Google Earth EC to view globes and maps from your own
      database servers.

      Earth EC lets you do any of the following:

      -  Add as many database servers as you want.
      -  Set a default server that is automatically signed in to
         whenever you start Earth EC.
      -  Connect to as many as eight servers at the same time.
      -  Change servers, or sign in and out of servers.
      -  Remove servers that you no longer use.

      .. rubric:: Sign in to an Earth EC server
         :name: sign-in-to-an-earth-ec-server

      .. rubric:: To sign in to a server:
         :name: to-sign-in-to-a-server

      #. From Google Earth EC, select **File** > **Server Sign In**.

         The **Select Server** window appears. When you start Google
         Earth EC the first time, **Select Server** appears
         automatically.

      #. Enter your server settings as shown in the table below.

      .. rubric:: Select Server options
         :name: select-server-options

      .. list-table::
         :widths: 35 50
         :header-rows: 1

         * - Setting
           - Description
         * - Server
           - Select or enter the address for the database server that hosts the globes or maps you want to view. If you do not know the server address, ask your administrator.
         * - Port
           - Select or enter the port for the server. If you don’t know the port number, ask your administrator.
         * - Always sign in to this server
           - Select to skip the Select Server window and automatically sign in to this server whenever you start Earth EC.
             To display Select Server again, select **File > Disable Automatic Sign In**.
         * - Disable caching
           - Select this if you want to make sure that you are viewing the latest content from the server or if your cached files become corrupted. **Note:** Disabling the cache slows down performance.

      .. rubric:: Sign out of an Earth EC server
         :name: sign-out-of-an-earth-ec-server

      To sign out of a server database, go to **File** > **Server Sign
      Out**.

      .. rubric:: Add servers to Earth EC
         :name: add-servers-to-earth-ec

      To add a server database, go to **File** > **Add Database**.

      The **Select Server** window appears so that you can enter your
      server settings.

      .. rubric:: Connect to multiple Earth EC servers at the same time
         :name: connect-to-multiple-earth-ec-servers-at-the-same-time

      Whenever you add another server database, Earth EC signs into the
      new database and stays connected to your current database. You can
      view data from up to eight server databases at the same time.

      .. rubric:: Change your Earth EC server
         :name: change-your-earth-ec-server

      .. rubric:: To change your Earth EC server:
         :name: to-change-your-earth-ec-server

      #. Select **File** > **Server Sign Out**.
         Earth EC signs out of your current server.
      #. Select **File** > **Server Sign In**.
      #. Select a server from the list that appears.

      .. rubric:: Useful resources

      -  :doc:`What is Google Earth EC? <../googleEarthEnterpriseClient/whatisEC>`
      -  :doc:`../googleEarthEnterpriseClient/fixEarthECIssues`

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
