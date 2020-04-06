|Google logo|

=========================================
Portable User Guide for Windows and Linux
=========================================

.. container::

   .. container:: content

      .. rubric:: Contents

      -  :ref:`Introduction to Google Earth Enterprise Portable <Introduction_Portable>`
      -  :ref:`Useful resources <Useful_Resources>`
      -  :ref:`Install GEE Portable <Install_GEE_Portable>`
      -  :ref:`Serve a globe or map from GEE Portable <Serve_Globe_Map_GEE_Portable>`
      -  :ref:`Serve a globe or map from the GEE Server <Serve_Globe_Map_GEE_Server>`
      -  :ref:`Broadcast a globe or map <Broadcast_Globe_Map>`
      -  :ref:`Connect using GEE Portable <Connect_GEE_Portable>`
      -  :ref:`Connect using the Google Earth EC <Connect_Using_GEEC>`
      -  :ref:`Connect using the Google Earth Plug-in <Connect_Using_Google_earth_Plugin>`
      -  :ref:`Get info about your map <Get_Info_About_Globe_Map>`
      -  :ref:`Change your configuration options <Change_Configuration_Options>`

      .. _Introduction_Portable:
      .. rubric:: Introduction to Google Earth Enterprise Portable

      Google Earth Enterprise Portable (GEE Portable) lets you view
      portable globes and maps on your laptop or desktop without
      requiring network access. This is useful for emergency responses
      to disasters like earthquakes or floods, or for maps that contain
      private information that you do not want to share on the internet.

      You can install GEE Portable on a Windows or Linux machine. It
      starts within seconds, and then you can launch a web browser to
      display one of the portable globes or maps that has been saved to
      the GEE Portable ``data`` directory.

      .. note::

         You can also connect to a portable globe or map with
         the Google Earth Enterprise Client (Google Earth EC). The
         Google Earth Plug-in for viewing a globe in the browser is now
         obsolete, but if you still have a copy of the plug-in and a browser that
         supports it, you can use it to connect to a Portable Server.

      .. rubric:: Portable globe and maps

      A portable globe or map is a single file that stores all the
      geospatial data available within your specified area of interest —
      including all high-resolution imagery, terrain, vector data, KML
      files, and searchable point of interest (POI) locations.

      Outside the specified area of interest, the globe or map stores
      only low-resolution imagery and terrain. You specify the levels of
      resolution when you cut the globe or map.

      The following table describes the portable file types and their
      compatibility with GEE Portable.


      .. list-table::
         :widths: 20 50 50
         :header-rows: 1

         * - Portable File Type
           - Description
           - Compatibility with Portable
         * - .glb
           - Portable 3D globe.
           - All versions.
         * - .glm
           - Portable 2D map.
           - All versions.
         * - .glc
           - Composite map or globe file *assembled* from 2D or 3D layers of other portable files.
           - All versions of GEE Open Source Portable, discontinued GEE versions 4.4 and higher.

      .. rubric:: Creating portable globes and maps

      You can :doc:`../geeServerAdmin/createPortableGlobesMaps` with the cutter tool feature of
      the Google Earth Enterprise (GEE) Server, or you can obtain them
      from third-party vendors. Depending on your area of coverage, it
      can take only a few minutes to specify and generate a globe or map
      and then save it to the GEE Portable ``data`` directory.

      .. _Useful_Resources:
      .. rubric:: Useful resources

      -  :doc:`Creating portable globes and maps <../geeServerAdmin/createPortableGlobesMaps>`.
         Google Earth Enterprise users can learn how to cut globes or
         maps to serve from Portable.
      -  :doc:`Portable Developer Guide <../portable/portableDeveloperGuide>`. Software
         developers can create or customize applications for Portable.

      .. _Install_GEE_Portable:
      .. rubric:: Install GEE Portable

      GEE Portable is supported on:

      -  Windows 7 and 10
      -  Red Hat Enterprise Linux version 7, including the most recent
         security patches
      -  Ubuntu 14.04 LTS and 16.04 LTS

      .. rubric:: To install GEE Portable:
         :name: to-install-gee-portable

      #. Refer to the `Portable Server on Earth Enterprise
         Wiki <https://github.com/google/earthenterprise/wiki/Portable-Server>`_
         for build, install, and run instructions.

      .. note::

         Before you install an upgrade, back up the contents
         of your ``data`` directory.

      Ther are currently no OS packages or installers and uninstallers
      for Portable server. The installation procedure is to build it on
      the OS distribution you want to use it on as a file archive. You
      can extract the contents of the archive in whatever location you
      like, then execute the server from that location.

      .. note::

         The Google Earth Plug-in is not supported any longer,
         so you cannot view globes (``.glb`` or ``.glc`` files) in your
         browser (unless you happen to have a copy of the old plug-in
         and a browser that can run it on Windows or Mac OS). You can,
         however, view maps (``.glm`` files). You can also use your
         Linux machine to serve 3D globes that you can view from Linux,
         Mac, or Windows machines that are connected to the Linux server.

      .. _Serve_Globe_Map_GEE_Portable:
      .. rubric:: Serve a globe or map from GEE Portable

      Unpacking GEE Portable creates a directory with a name similar to
      ``portableserver-<os>-<version>``, with ``<os>`` and ``<version>``
      having values depending on the operating system, Portable version,
      and build date. This directory contains a folder named ``data``
      (unless you renamed it in ``portable.cfg``). Copy your globe or
      map to the ``data`` folder. The GEE Portable interface lists all
      the globes and maps placed in this folder. If you no
      longer want a globe or map to appear in the list, simply remove it
      from the ``data`` folder.

      .. _Serve_Globe_Map_GEE_Server:
      .. rubric:: Serve a globe or map from the Google Earth Enterprise
         Server

      If you want to serve a globe or map to a large number of users,
      you can use a Google Earth Enterprise Server (GEE Server) on a
      Linux machine instead of a GEE Portable Server on a user’s
      machine. GEE Server is capable of storing very large globes or
      maps, and it also lets you serve globes and maps on your own
      private network so that only authorized users can connect.

      .. rubric:: To serve a map or globe from GEE Server:

      #. Enable the GEE Server Cutter tool on the command line:

         -  In GEE Open Source: ``gecutter enable``

         By default, the cut globes are stored in the
         ``/opt/google/gehttpd/htdocs/cutter/globes`` directory.

         To change the directory, create a symlink to point to another
         directory.

      #. Access the Google Earth Enterprise Server Admin console in a
         browser window by going to ``myserver.mydomainname.com/admin``,
         replacing *myserver* and *mydomainname* with your server and
         domain.
      #. Sign in with the default credentials or the username and
         password assigned to you:

         -  Default username: ``geapacheuser``
         -  Default password: ``geeadmin``

         .. note::

            If you do not know your username and password,
            contact your Google Earth Enterprise Server System
            Administrator.

      #. Click **Manage Portable** to display the list of portable files
         in the ``/opt/google/gehttpd/htdocs/cutter/globes`` directory
         (by default).
      #. Click **Register** next to the portable file you want to
         connect to. A message appears to indicate that your portable
         map or globe has been registered to GEE Server. Close the
         Manage portable globes window.

         If you want to download the file, click the file name.

         The registered portable map or globe now appears in the
         **Databases** list of the GEE Server Admin console.

      #. Check the box next to the portable file name, then click
         **Publish**. The Publish dialog appears.
      #. Enter a **Publish point** or accept the default. For example, the
         Publish point **MyCutGlobe** would result in a serving URL
         ``myserver.mydomainname.com/MyCutGlobe``, where *myserver* and
         *mydomainname* are specific to your server.
      #. Specify a virtual host and optionally turn on WMS.
      #. Click the **Publish** button to publish the portable file.

         A message is displayed to indicate that your portable map or globe
         has been published and the Publish point is updated in the
         Databases list.

      #. Click the Publish point link to view the portable map or globe
         in a new browser tab.

      .. _Broadcast_Globe_Map:
      .. rubric:: Broadcast a globe or map

      .. rubric:: To share a globe or map with others on your network:

      #. When ``disable_broadcasting`` is set to ``True`` in
         ``portable.cfg``, the default setting, you can enable
         broadcasting using either of the following methods:

         -  Add ``accept_all_requests True`` to your ``portable.cfg``
            file.
         -  Add ``disable_broadcasting False`` to your ``portable.cfg``
            file, and follow the next option.

      #. When ``disable_broadcasting`` is set to ``False`` in
         ``portable.cfg``:

         -  Add ``accept_all_requests True`` to your ``portable.cfg``
            file.
         -  Visit the Portable Server administration page, open a globe
            or map, and click on the broadcast icon (|Broadcast globe
            icon|).

      .. note::

         By default, broadcasting is off and cannot be turned
         on via an http call to the API. This feature is controlled by
         the ``disable_broadcasting`` flag, which is set to ``True`` in
         ``portable.cfg``. However, if you set ``accept_all_requests``
         to ``True`` in ``portable.cfg``, then broadcasting is enabled,
         regardless of the ``disable_broadcasting`` state.

      .. _Connect_GEE_Portable:
      .. rubric:: Connect using GEE Portable

      Start Portable Server by executing ``python portable_server.py``
      from the folder you unpacked the Portable Server archive in.
      After it launches, open a browser and navigate to
      ``http://localhost:9335`` (using any
      custom port or hostname you may have configured). Click the
      |Portable folder icon| **Folder** (outlined in red), then select the
      map you want to view. You can view only one map at a time.
      Portable globes cannot be viewed with a web browser. Use
      Google Earth Enterprise Client to view globes.

      Globes and maps that are broadcast on your local network might
      require an access key. If prompted, enter the key to view the
      globe or map. You can obtain the key from the person who is
      broadcasting the globe or map.

      .. _Connect_Using_GEEC:
      .. rubric:: Connect using the Google Earth Enterprise Client

      Launch the Google Earth Enterprise Client (Google Earth EC). When
      prompted for a server address, enter ``http://localhost:9335``. If
      you changed the default port in ``portable.cfg``, use the new
      port value instead.

      .. _Connect_Using_Google_earth_Plugin:
      .. rubric:: Connect using the Google Earth Plug-in

      GEE Portable comes with a preconfigured HTML page called
      ``hello_maps.html`` that displays your map using the ``Google Maps API``.

      If you wish to make your own custom application, start by making
      a copy of either of these files and then add your own edits.

      To access either of the files, enter the URL in your browser:

      | ``http://localhost:9335/local/preview/developers/hello_maps.html``

      .. note::

         Although it is possible to configure GEE Portable to
         display multiple maps at the same time, this is not
         recommended or supported. The additional configuration requires
         you to build the map to reference a specific port
         number, which means you would have to rebuild it if you wanted
         to re-use it for any other ports.

      .. _Get_Info_About_Globe_Map:
      .. rubric:: Get info about your globe or map

      Click the |Portable folder icon| **Folder** (outlined in red) to view a
      list of the globes and maps you can access. Each globe or map is
      listed with its file name, description, creation date, and size.

      Select a map, then click the |Portable menu icon| **Menu**
      (outlined in red) then select **Show layer list** to see
      all the layers for that map. You can use the list to
      select the layers you want the map to display. To hide
      the layer list, click the **Menu**, then select **Hide layer
      list**.
      |

      .. _Change_Configuration_Options:
      .. rubric:: Change your configuration options

      The Google Earth Portable directory contains the
      ``server/portable.cfg`` configuration file. This is the directory
      you unpacked the Portable Server archive in.

      The Portable configuration file defines the editable options
      listed below.

      -  ``port``. The port on which to serve the globe or map. The
         default is 9335.
      -  ``globes_directory``. The directory that contains the globe and
         map files.
      -  ``map_name``. The default globe or map to serve when GEE
         Portable launches.
      -  ``fill_missing_map_tiles``. If set to ``True``, enables
         pixel-filling from the ancestor map tile when there are no more
         tile descendants. Set this to ``False`` if you want to clearly
         indicate areas that are beyond their natural resolution, or if
         you just want to improve performance. You can also improve
         performance by lowering the value of the
         ``max_missing_maps_tile_ancestor`` option below.
      -  ``max_missing_maps_tile_ancestor``. If the
         ``fill_missing_map_tiles`` option (above) is set to ``True``
         and no tiles exist at your current display level, this option
         specifies the maximum number of tiles to create from samples of
         the ancestor tiles. Using a lower value can improve performance
         because the server creates fewer tiles. The value is written as
         2\ :sup:`x` by 2\ :sup:`x` sized pixels. By default, :sup:`x` =
         3. (Or 2\ :sup:`3` by 2\ :sup:`3` pixels, which equals 8 x 8
         pixels. This is Display Level 6, or 24 tiles). To lower the
         value, replace :sup:`x` with a number lower than 3.
      -  ``local_override``. If set to ``True``, GEE Portable looks for
         all the files on the server first before looking for them on
         your machine.
      -  ``disable_broadcasting``. By default, this flag is set to ``True``,
         preventing broadcasting from being turned on via an HTTP call
         to the API. However, if you set ``accept_all_requests`` to
         ``True`` in ``portable.cfg``, then broadcasting is enabled,
         regardless of the ``disable_broadcasting`` state.
      -  ``accept_all_requests``. If set to ``True``, GEE Portable accepts
         all requests to the server, and thus enables broadcasting,
         regardless of the state of the ``disable_broadcasting`` flag.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
.. |Broadcast globe icon| image:: ../../art/fusion/portable/broadcast_false.gif
   :width: 38px
   :height: 31px
.. |Portable folder icon| image:: ../../art/fusion/portable/portable_folder_icon.png
.. |Portable menu icon| image:: ../../art/fusion/portable/portable_menu_icon.png
