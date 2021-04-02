|Google logo|

==============
Databases page
==============

.. container::

   .. container:: content

      The **Databases** page of the :doc:`GEE Server Admin
      console <../geeServerAdmin/signInAdminConsole>` is where you manage Fusion
      databases and portables. From this page you can:

      -  **View and sort your list of databases and portables**. The list is
         populated by the items found in
         ``/opt/google/gehttpd/htdocs/cutter/globes/``. Fusion databases
         must be pushed to this list from the Fusion interface.
         Portables appear in this list after they are registered using
         the **Manage Portables** button on the same page. Click the
         **Refresh** button in the upper right corner to refresh the list.
      -  **Manage databases and portables**. Check the box next to a
         database, then click:

         -  **Publish**: To publish a database. Specify a **Publish**
            **point**, where the database or portable will be accessible
            from. For example, if you specify *sanfrancisco*, it will be
            accessible from *myserver.mydomainname*.com/sanfrancisco.

            .. note::

               When publishing a database, the publish point
               you specify is case-*insensitive*. Upper and lower case
               are not differentiated. Make sure each publish point path
               name you specify is unique.

         -  Choose a :doc:`virtual host <../geeServerAdmin/manageVirtualHosts>`, and,
            depending on the type of item, the following options may
            also be available: :doc:`Search Tabs <../geeServerAdmin/createSearchTabs>`,
            :doc:`Snippet profiles <../geeServerAdmin/manageSnippetProfiles>`, and :doc:`Serve
            WMS <../geeServerAdmin/makeWMSRequests>`.
         -  **Unpublish**: To unpublish a database from its publish
            points.
         -  **Preview**: To preview a published database.
         -  **Remove**: To unpush an unpublished Fusion database, or
            unregister an unpublished portable. This removes the
            database or portable from the list, but does not delete them.
            Portables still appear in the master list of portables,
            which you can view by clicking **Manage Portables**.
      -  **Register or unregister portables**. Click the **Manage
         Portables** button, then click:

         -  **Register**: To add a portable to the database list and
            make it available for publishing.
         -  **Unregister**: To remove a portable from the list of
            databases. If the portable is published, it must be
            unpublished before being unregistered.
         -  The **name** of a portable to download it.
         -  **Refresh**: To refresh the list of portables.
         -  **Close**: To close the Manage Portables dialog.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
