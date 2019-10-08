|Google logo|

===============================================
Publish databases using disconnected publishing
===============================================

.. container::

   .. container:: content

      If you want to publish databases on a server that does not have a
      network connection to your Google Earth Enterprise Fusion
      workstations, you can create a database that can be output to
      portable media, which can then be pushed and published to GEE
      Server.

      A large database can be copied to portable media and loaded onto
      the server using ``geserveradmin`` commands, saving the time and
      bandwidth required to send terabytes of data over the network. In
      addition, you can also create deltas for the same databases, which
      can then be sent over the network.

      .. rubric:: Before you begin
         :name: before-you-begin

      To use the disconnected publishing service, you need to install
      the **GEE Server - Disconnected Add-on**, which you can add by
      running the installation process and choosing **Custom** as the
      installation type.

      .. tip::

         Disconnected production servers require X11 libraries,
         including Mesa. Google Earth Enterprise Server does not require
         an X server to be running or installed, however; only the
         libraries are required.

      .. rubric:: Prepare a disconnected database
         :name: prepare-a-disconnected-database

      Before creating a disconnected database, you can estimate the
      space required to store the disconnected database by running
      ``gedisconnectedsend`` with the ``--report_size_only`` option.

      .. rubric:: To estimate the size of a disconnected database:
         :name: to-estimate-the-size-of-a-disconnected-database

      #. From the command line, specify the database that you want to
         publish with the ``--report_size_only`` flag:

         ``# gedisconnectedsend --report_size_only --sendpath \       /gevol/assets/Databases/test.kdatabase/gedb.kda/ver001/gedb``

      .. rubric:: To create a disconnected database:
         :name: to-create-a-disconnected-database

      #. Create an output directory on your portable media, which you
         will specify when you create the disconnected database.

         ``rm -rf /media/ddb/test_ver001/``

         ``mkdir -p /media/ddb/test_ver001/``

      Create the disconnected database and output the file to your
      portable media:
      ``gedisconnectedsend --output /media/ddb/test_ver001/ \``

      ``--sendpath
      /gevol/assets/Databases/test.kdatabase/gedb.kda/ver001/gedb``

      In this example,
      ``/gevol/assets/Databases/test.kdatabase/gedb.kda/ver001/gedb/``
      is the path to the ``gedb`` directory of the disconnected
      database.

      .. rubric:: Prepare delta disconnected databases

      The disconnected database command includes the option
      ``--havepath`` or ``--havepathfile``, to create a delta
      disconnected database, based on different versions of the same
      database. To create the delta, the assetroot of the GEE Fusion
      machine must store the versions of databases, as reported by
      geserveradmin ``--listdbs`` on the GEE Server machine, from which
      the delta disconnected database is created.

      When publishing from a GEE Fusion machine (in this example,
      ``machine_one``) to a disconnected GEE Server machine (in this
      example, ``machine_two``), verify the list of versions of the
      disconnected database for which you want to create the delta:

      For example, on ``machine_two``:

      ``geserveradmin --fusion_host machine_one --listdbs > /home/user_name/ddb/dblist_server_host``

      On ``machine_one``, create the delta disconnected database using
      the ``--havepathfile`` option:

      ``gedisconnectedsend --output /media/ddb/test_ver001/ \``

      ``--havepathfile /home/user_name/ddb/dblist_server_host \``

      ``--sendpath /gevol/assets/Databases/test.kdatabase/gedb.kda/ver001/gedb``

      .. rubric:: Publish on the remote server host
         :name: publish-on-the-remote-server-host

      Connect the portable media to the Server host and mount the drive.
      In this example,
      ``/media/ddb/test_ver001/gevol/assets/Databases/test.kdatabase/gedb.kda/ver001/gedb/``
      is the path to the ``gedb`` directory of the disconnected
      database.

      .. rubric:: To publish on the remote server host:
         :name: to-publish-on-the-remote-server-host

      #. Set ownership and permissions for the ``gedb`` directory:

         ``$ sudo chown -R gefusionuser:gegroup ddb/``

         ``$ sudo chmod -R 755 ddb/``

      #. Register the database on the Server. Specifying the
         --stream_server_url is optional.

         ``geserveradmin --fusion_host fusion_host.company.com \``

         ``--stream_server_url http://your_stream_server  \``

         ``--adddb \``

         ``/media/ddb/test_ver001/gevol/assets/Databases/test.kdatabase/gedb.kda/ver001/gedb/``

      #. Push the files to the Server:

         ``geserveradmin --fusion_host fusion_host.company.com \``

         ``--stream_server_url http://your_stream_server \``

         ``--pushdb \``

         ``/media/ddb/test_ver001/gevol/assets/Databases/test.kdatabase/gedb.kda/ver001/gedb/``

      #. Publish the database on the Server either using the :doc:`GEE Server
         Admin console <3497763>` or on the command
         line. The GEE Server Admin console Publish dialog includes
         options to add :doc:`search <3497832>`, :doc:`snippet
         profiles <6004748>`, :doc:'specify a virtual
         host <6013604>`, and :doc:`turn on
         WMS <4441137>`.

         **To publish on the command line:**

         ``geserveradmin --fusion_host fusion_host.company.com \``

         ``--stream_server_url http://your_stream_server \``

         ``--publishdb \``

         ``/media/ddb/test_ver001/gevol/assets/Databases/test.kdatabase/gedb.kda/ver001/gedb/``

      #. Verify that the database manifests:

         ``geserveradmin --stream_server_url http://your_stream_server \``

         ``--listdbs``

         ``geserveradmin --stream_server_url http://your_stream_server \``

         ``--dbdetails \``

         ``/media/ddb/test_ver001/gevol/assets/Databases/test.kdatabase/gedb.kda/ver001/gedb``

      #. Verify the integrity of files in the published database:

         ``ssh your_server``

         ``/opt/google/bin/geindexcheck --database \``

         ``/gevol/published_dbs/stream_space/your_fusion_host/gevol/assets/``

         ``Databases/test.kdatabase/gedb.kda/ver001/gedb``

         ``/opt/google/bin/geindexcheck --mode all --database \``

         ``/gevol/published_dbs/stream_space/your_fusion_host/gevol/assets/``

         ``Databases/test.kdatabase/gedb.kda/ver001/gedb``

         .. tip::

            The paths in the examples above were split onto multiple
            lines for documentation using backslashes. They should be
            entered on one line when completing your commands.

      .. rubric:: Symbolic links
         :name: symbolic-links

      Symbolic links are turned on by default in the published databases
      configuration file, ``AllowSymLinks=Y`` in the
      ``/gevol/published_dbs/.config``, resulting in hard or soft
      symbolic links being created, depending on the logical volumes of
      the relevant directories:

      -  **hard-link**: the disconnected database folder and
         published_dbs folder are on the same logical volume.
      -  **soft-link**: the disconnected database folder and
         published_dbs folder are on different logical volumes.
      -  If the disconnected database folder is in ``/tmp``, the
         directory files will be copied.

      .. rubric:: Rewrite KML Layer URLs

      If the disconnected database contains KML layers, the KML layers's
      URLs may require adjustments in order to resolve the KML content
      in a disconnected environment. (See :doc:`2760457`). Use the ``gerewritedbroot``
      command to update the disconnected database KML links with new
      hostname, ports, paths, or filenames.

      #. Publish KML content

         Copy, publish, and modify the KML files on the disconnected
         GEE Server machine's network as needed. Validate that the
         referenced KML files are accessible on the disconnected network
         via HTTP or HTTPS using a web browser or command line tools.
         Note the differences in hostnames, paths, and ports between
         their origninal locations and their locations on the
         disconnected network environment.

         KML files can be served from the
         ``/opt/google/gehttpd/htdocs/`` folder or a subfolder, or on a
         separate web server.

      #. Identify which dbroot file needs to be updated, and store in an
         environment variable for later use:

         ``export TARGET_DBROOT='/media/ddb/test_ver001/gevol/assets/Databases/test.kdatabase/gedb.kda/ver001/gedb/targets/new_target/dbroots/dbroot.v5p.DEFAULT'``

      #. Rewrite the KML layer links in the database on the Server.

         ``/opt/google/bin/gerewritedbroot --source http://your_stream_server/new_target \``

         ``--dbroot_file $TARGET_DBROOT\``

         ``--kml_server your_stream_server --kml_url_path  your_htdoc_path/ \``

         ``--preserve_kml_filenames``

      #. Changes are not refected until the server is restarted.

         ``sudo service geserver restart``

      #. Validate that the server has the correct database information
         by dumping the dbroot as text:

         ``wget -O dbroot.v5p.DEFAULT http://you_stream_server/new_target/dbRoot.v5``

         ``gedumpdbroot dbroot.v5p.DEFAULT``

      .. rubric:: Delete disconnected databases

      To delete disconnected databases at their end of life use
      ``geserveradmin --deletedb``, the same procedure used for
      databases that are published normally.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
