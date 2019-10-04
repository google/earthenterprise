|Google logo|

============
Set up HTTPS
============

.. container::

   .. container:: content

      If you have set up SSL/HTTPS configurations on your GEE server, you
      can also set up a secure Fusion server for your browser-based
      maps.

      .. warning::

         The following procedure is applicable only to release 4.x and
         previous versions of GEE. For **GEE release 5.x**, see
         :doc:`../geeServerConfigAndSecurity/configureGeeServer5.1.0_SSL_HTTPS`.

      .. rubric:: To set up an HTTPS virtual server for Fusion:
         :name: to-set-up-an-https-virtual-server-for-fusion

      #. Verify that you have already set up SSL/HTTPS configurations on
         your GEE server.

      #. Create an HTTPS-compatible virtual server:

         ``geserveradmin --stream_server_url http://myserver.org --server_type stream --addvs https_2d --vstype map --vscachelevel 1 --vsurl https://myserver.org/mymap``

      #. Copy and save the ``Include`` statement from ``geserveradmin``.
         Later, you will add that to the configuration file. It will look
         something like:

         ``Include conf.d/virtual_servers/runtime/mymap_runtime``

      #. | Copy a configuration file from the ``examples`` folder:

         ``cp /opt/google/share/gehttpd/examples/  /opt/google/gehttpd/conf.d/virtual_servers/https_mymap.location``

      #. | Edit the ``https_mymap.location`` file, insert the
           ``Include`` statement you copied and saved, and update the
           LOCATION tags to the real virtual server:

         .. code-block:: none
         
            r...@fusion.localhost.org:/opt/google/gehttpd/conf.d/virtual_servers 
            # cat https_2d.location # This is an example of a location-based map virtual server #
            Substitute appropriate values in the following variables
            # 1. <LOCATION> : The new location name.
            # 2. <VS_NAME> : The virtual server name used to create the virtual
            # server with geserveradmin

            RewriteRule ^/mymap$ /mymap/ [R] RewriteRule ^/mymap/$ /maps/fusionmaps_local.html [PT] RewriteRule ^/mymap/mapfiles/(.*)$ /maps/mapfiles/$1 [PT]

            <Location "/mymap/*">
            SetHandler gedb-handler
            Include conf.d/virtual_servers/runtime/https_2d_runtime
            SSLRequireSSL
            SSLVerifyClient none
            </Location>

      #. | Modify the ``/opt/google/gehttpd/conf/extra/httpd-ssl.conf``
           file so that the
           ``/opt/google/gehttpd/conf.d/virtual_server/https_2d.location``
           file and the virtual server are loaded by the HTTPS/443
           virtual host and not the HTTP/80 virtual host:

         .. code-block:: none
         
            (vi /opt/google/gehttpd/conf/extra/httpd-ssl.conf...)
            <snip>
            ... <VirtualHost _default_:443>
            # General setup for the virtual host
            DocumentRoot "/opt/google/gehttpd/htdocs"
            ServerName myserver.org:443

            ServerAdmin administra...@myserver.org
            ErrorLog "/opt/google/gehttpd/logs/error_log"
            TransferLog "/opt/google/gehttpd/logs/access_log"
            Include conf.d/virtual_servers/https_2d.location
            ...
            <snip>

      #. | Edit the HTTP/port 80 virtual host to load only the
           HTTP-available virtual servers:

         ``(vi /opt/google/gehttpd/conf.d/gemodules.conf)``

      #. | Comment out the ``Include .. *.location`` line and manually
           list the included location files so that the
           ``https2d.location`` file is excluded:

         .. code-block:: none
         
            LoadModule gedb_module /opt/google/gehttpd/modules/mod_gedb.so

            NameVirtualHost *:80
            <VirtualHost *:80>
            # You should specify a ServerName in each VirtualHost declaration
            # to avoid unnecessary DNS lookups.
            # For example, ServerName www.mycompany.com

            # Redirect the home page to display the GE logo
            Include conf.d/index_rewrite
            Include conf.d/jkmount

            # Include all location-based virtual servers
            # Include conf.d/virtual_servers/*.location (Comment out this line.)
            Include conf.d/virtual_servers/default_ge.location (Add this line.)
            Include conf.d/virtual_servers/default_map.location (Add this line.)
            Include conf.d/virtual_servers/default_search.location (Add this line.)
            </VirtualHost>

      #. | Save the file and restart the GEE Server software:

         ``/etc/init.d/ geserver restart``

         This separates the HTTP and HTTPS virtual servers from the
         Apache software so that unencrypted and encrypted data can be
         hosted from both.

         .. note::
         
            The firewall blocks external port 80 / HTTP
            connections, but the Publisher tool must use the HTTP port to
            upload information, even if your system only allows this
            internally.

      #. Create a Fusion server association for the new **https2d**
         virtual server. Use ``http://myserver.org`` for the URL for
         both Stream and Search URLs, then press the **Query** button
         and select the correct https2d virtual server from the
         drop-down list.

      #. Save the new server association, then publish a 2D database to
         the virtual server.

      .. rubric:: Listing registered virtual stream servers
         :name: listing-registered-virtual-stream-servers

      To avoid confusion or conflict between http:// and https://
      addresses, you can use ``geserveradmin`` parameters like the
      ``-- stream_server_url http://myserver.org`` option that lists
      registered virtual stream servers. For example, instead of using
      the ``geserveradmin --listvss`` command alone, you can use
      ``geserveradmin --stream_server_url http://myserver.org --listvss``.

      .. rubric:: Binding Apache to port 80
         :name: binding-apache-to-port-80

      You need HTTP to facilitate all ``geserveradmin`` work, including
      publishes, so make sure that your ``gehttpd.conf`` configuration
      file lets Apache bind to port 80. You can allow internal access to
      HTTP even if you block external access to HTTP ports. This lets
      the Publisher tool maintain the GEE Server software while you
      disallow external unencrypted data communications.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
