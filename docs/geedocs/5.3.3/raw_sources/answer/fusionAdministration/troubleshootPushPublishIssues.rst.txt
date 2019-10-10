|Google logo|

===================================
Troubleshoot push/publishing issues
===================================

.. container::

   .. container:: content

      If you are having problems pushing/publishing a database, follow
      the troubleshooting steps below to debug the issue.

      .. rubric:: A note about pushing and publishing databases

      The *push* operation copies all the necessary files associated
      with a given 2D/3D Fusion database version from the Fusion host to
      Google Earth Enterprise Server and *registers* the database.
      Pushing is performed from Fusion.

      The *publish* operation makes a previously pushed database
      available for serving at a specified publish point. Publishing is
      performed on Google Earth Enterprise Server.

      WIth *disconnected publishing*, you can create a database that can
      be output to portable media, which can then be pushed and
      published to GEE Server.

      -  :doc:`../fusionAdministration/pushAndPublishDB`
      -  :doc:`../geeServerAdmin/publishDatabasesPortables`
      -  :doc:`../fusionAdministration/publishDBWithDiscPublishing`

      .. index:: Troubleshoot Push Issues
      .. rubric:: Troubleshoot Push Issues

      Errors in the push operation may occur when Fusion fails to copy
      and register the database files to GEE Server. Use the following
      debugging steps, presented in a logical order, to uncover and
      solve any push issues. The error you encounter may depend on the
      push method you use. For example, a local push may fail simply
      because of a shortage of disk space. A remote push may fail
      because of an incorrect server association setting.

      .. rubric:: Push methods

      The push method depends on how data is transferred between the GEE
      Fusion host and GEE Server.

      -  **Local push**. GEE Fusion and Server are on the same host.
      -  **Remote push**. GEE Fusion pushes to a physically separate GEE
         Server.
      -  **Disconnected push**. Data is transferred from the Fusion host
         to the GEE Server by hard disk instead of a network. For
         information about pushing disconnected databases, see
         :doc:`../fusionAdministration/publishDBWithDiscPublishing`.

      .. rubric:: Check your server is running correctly

      If your attempts to push a database result in an error message:
      “No status message returned from request:
      http://your.GEEServer.com Unable to contact stream data server,”
      GEE Server may not be running. Try these steps to determine the
      state of GEE Server:

      #. Try restarting GEE Server:

         ``sudo /etc/init.d/geserver restart``

      #. Enter the URL of GEE Server in a browser. You should see the
         Google Earth Enterprise Server splash screen.
      #. Log in to the Admin console of GEE Server:

         -  Go to *myserver.mydomainname.com/admin*, replacing
            *myserver* and *mydomainname* with your server and domain.
         -  Sign in with the default credentials:

            Username: *geapacheuser*

            Password: *geeadmin*

         .. tip::

            To reset the username and password, run the following
            command, which prompts you to enter a new username and
            password:

            ``sudo /opt/google/gehttpd/bin/htpasswd -c``
            ``/opt/google/gehttpd/conf.d/.htpasswd new_user_name``

            If you do not know your username and password, contact your
            system administrator.

      #. Review the HTTP and Postgres processes. They should look
         something like this:

         .. code-block:: none

            # ps -ef | grep http
            root 9220 1 0 Mar13 ? 00:00:00 /opt/google/gehttpd/bin/gehttpd
            505 14608 9220 0 Mar15 ? 00:00:00 /opt/google/gehttpd/bin/gehttpd
            505 14609 9220 0 Mar15 ? 00:00:00 /opt/google/gehttpd/bin/gehttpd

            # ps -ef | grep post
            gepguser 9206 9195 0 Mar13 ? 00:00:00 postgres: writer process
            gepguser 9207 9195 0 Mar13 ? 00:00:00 postgres: stats buffer process
            gepguser 9208 9207 0 Mar13 ? 00:00:00 postgres: stats collector process
            gepguser 10639 9195 0 Mar13 ? 00:00:00 postgres: geuser gesearch 127.0.0.1(32772) idle
            gepguser 10640 9195 0 Mar13 ? 00:00:00 postgres: geuser gesearch 127.0.0.1(32773) idle
            gepguser 10641 9195 0 Mar13 ? 00:00:00 postgres: geuser gepoi 127.0.0.1(32774) idle
            gepguser 10642 9195 0 Mar13 ? 00:00:00 postgres: geuser geplaces 127.0.0.1(32775) idle

         If you have any defunct processes or other unusual entries,
         stop them or try to find out why they are running. Run
         ``/etc/init.d/geserver restart`` to make sure that the server
         shuts down and starts up quickly with no error messages. If you
         still get error messages, take the following steps:

         -  Shut down the server:

            ``/etc/init.d/geserver stop``.

         -  Delete the ``postmaster.pid`` file:

            ``rm /var/opt/google/pgsql/data/postmaster.pid``.

            (The ``postmaster.pid`` file may not have been deleted if
            PostgreSQL services have not been stopped correctly, thereby
            preventing another instance of GEE Server from starting.)

         -  Reboot the GEE server.
         -  Re-run the two ``ps`` commands and the ``geserver restart``
            command to make sure that everything is running properly.

      #. Review the ``wsgi:ge`` processes, which are GEE Server services
         that support pushing and publishing. They should look something
         like this:

         .. code-block:: none

            ps -ef | grep 'wsgi:ge'
            65609 7272 3445 0 Aug10 ? 00:00:11 (wsgi:ge_push_serve) -k start
            65609 7273 3445 0 Aug10 ? 00:00:12 (wsgi:ge_publish_serve) -k start
            65609 7274 3445 0 Aug10 ? 00:00:11 (wsgi:ge_publish_aux_serve) -k start

      .. rubric:: Check your hostnames
         :name: check-your-hostnames

      Check to be sure that ``hostname -f`` returns the hostname you think it
      should. Make sure that the ``hostname -f`` is consistent between
      the GEE server, the DNS entry for the GEE server, and any local
      hosts files. When you install Fusion and GEE Server on your
      machine, the software queries the hostname of the server. This is
      used in all asset builds on Fusion. You can see what Fusion has
      registered as your hostname by looking at the host entry in the
      ``volumes.xml`` file in ``/ASSET_ROOT/.config/volumes.xml``. Do
      not edit this file by hand.

      To correct the hostname for all assets on your server, run
      ``geconfigureassetroot --fixmasterhost``.

      .. rubric:: Check your GEE Server with geserveradmin
         :name: check-your-gee-server-with-geserveradmin

      From the Fusion server, list the virtual servers on the GEE server
      and show the databases that have been pushed:

      ``geserveradmin --stream_server_url http://earth.int --listvhs``

      ``geserveradmin --stream_server_url http://earth.int --listdbs``

      Show the databases that are currently published:

      ``geserveradmin --stream_server_url http://earth.int --publisheddbs``

      These commands should all work without error.

      .. rubric:: Check your server associations
         :name: check-your-server-associations

      Open the **Server Associations Manager** tool from Fusion. Make
      sure that the server associations are correct. Open the server
      association that you are trying to push to. There should be no
      error messages when you open it.

      .. rubric:: Verify which user account you are pushing with
         :name: verify-which-user-account-you-are-pushing-with

      All pushes from the Fusion system should be performed by a
      *non-root* user account. A basic user account has sufficient
      privileges to push databases locally on the Fusion system or
      remotely to the GEE Server system. Using the root account for
      pushes can introduce file-level permission problems (see next
      topic).

      .. rubric:: Verify the umask settings are 0022

      Temporary files are written into the ``/tmp`` folder of the Fusion
      system during a publish. These files inherit permission settings of the
      user account used to publish them (``geuser``, ``root``, etc.). These
      files are then read by the GEE Server user accounts
      (``geapacheuser``), which belong to the ``gegroup`` user group.
      Publish failures can happen if the ``gegroup`` accounts cannot
      read the files in ``/tmp``, e.g., if they have very restrictive umask
      settings such as 0077 for all user accounts, or in the case of
      publishing while logged in as root. To check the unmask settings
      for your GEE Server accounts, type ``umask`` on the command line
      while logged into the Fusion system.

      .. rubric:: Check your disk space
         :name: check-your-disk-space

      If the GEE Server runs out of disk space, the push will fail. Run
      ``df -h`` to see if you have space left on the server. If you are
      out of space, use ``geserveradmin`` to delete some of the old
      databases that have been pushed to the server and then run
      ``garbagecollect`` to clean up the old data.

      Garbage collection deletes unused data from database versions
      deleted with ``geserveradmin``. It is not the same as deleting a
      database version in its entirety. If a database version is deleted
      directly from the file system, it usually breaks all subsequent
      versions of the database.

      You can use the ``geserveradmin`` command on the Fusion server to
      manage both locally pushed and remotely pushed databases. The
      ``geserveradmin`` command includes a ``--stream_server_url``
      option that directs it to the server IP address or URL where the
      function should be run. If you omit this option, the
      ``geserveradmin`` command defaults to the local machine.

      The commands below show how to use ``geserveradmin`` to clean up
      unused data from the ``earth.int`` server.

      Assuming that ``fusion.int`` is the Fusion system and
      ``server.int`` is the GEE Server system, the commands on the
      remote server (``earth.int``) are:

      #. To list all pushed databases, optionally using ``--portable``
         to specify portable databases only:

         ``geserveradmin --stream_server_url http://earth.int --listdbs [--portable]``

      #. To list published databases, optionally using ``--portable`` to
         specify portable databases only:

         ``geserveradmin --stream_server_url http://earth.int --publisheddbs [--portable]``

      #. To delete specific database versions:

         ``geserveradmin --stream_server_url http://earth.int --deletedb /path/to/mydatabase.kdatabase/verZYX/gedb``

      #. To perform garbage collecting for deleted databases (stream):
         ``geserveradmin --stream_server_url http://earth.int --garbagecollect``

      .. index:: Troubleshoot Publishing Issues
      .. rubric:: Troubleshoot Publishing Issues
         :name: troubleshoot-publishing-issues

      Errors in the publish operation may occur when GEE Server cannot
      publish to the specified publish point.

      .. rubric:: Delete your broken publish and try again

      If you have had a successful publish previously, then you should
      already have at least one good copy of the database pushed to the
      GEE Server. If a recent publish keeps failing, you can remove the
      failed publish and try again.

      Show the pushed and published databases, adding the optional
      ``--portable`` to specify portable databases only:

      ``geserveradmin --stream_server_url http://earth.int --listdbs [--portable]``

      ``geserveradmin --stream_server_url http://earth.int --publisheddbs [--portable]``

      Use ``geserveradmin`` to publish one of your older database
      versions, then use ``geserveradmin`` to delete the recent, failed
      publish. Clean up the garbage, then try the publish again.

      For example, if version 2 was working but version 4 is broken,
      re-try pushing version 4 of the database:

      ``geserveradmin --stream_server_url http://earth.int --deletedb /gevol/assets/Databases/GoogleEarth.kdatabase/gedb.kda/ver004/gedb``

      ``geserveradmin --garbagecollect --server_type stream``

      ``geserveradmin --stream_server_url http://earth.int --adddb /gevol/assets/Databases/GoogleEarth.kdatabase/gedb.kda/ver004/gedb``

      ``geserveradmin --stream_server_url http://earth.int --pushdb /gevol/assets/Databases/GoogleEarth.kdatabase/gedb.kda/ver004/gedb``


         .. tip::

            Instead of using <code>geserveradmin --adddb/pushdb</code> commands, you can push the database directly from Fusion. See :doc:`../fusionAdministration/pushAndPublishDB`.

      .. index:: Troubleshoot Disconnected Publishing Issues
      .. rubric:: Disconnected publishing issues

      If you publish a disconnected database and it fails when you
      attempt to push the database, it may be that
      ``gedisconnectedsend --sendpath`` created folders with user/group
      permissions only, preventing the file from being read by
      ``geapacheuser:gegroup``, which is used by
      ``geserveradmin --pushdb``.

      Try resetting the permissions on the folders created by
      ``gedisconnectedsend --sendpath`` and try the ``--pushdb`` again.

      -  See :doc:`../fusionAdministration/publishDBWithDiscPublishing`.

      .. rubric:: Check your log files

      If you are having push or publishing issues, there are several log
      files you can review for errors.

      .. index:: Troubleshoot Push and Publish issues - log files
      .. list-table::
         :widths: 50 50
         :header-rows: 1

         * - Log file
           - Error logging reported
         * - ``/opt/google/gehttpd/logs/error_log``
           -  Log file containing GEE Server publishing errors and authentication notices.
         * - ``/opt/google/gehttpd/logs/access_log``
           -  Log file containing HTTP GET requests for GEE Server.
         * - ``/opt/google/gehttpd/logs/gestream_publisher.out`` ``/opt/google/gehttpd/logs/gesearch_publisher.out``
           - Log files containing detailed GEE Server publishing errors.
         * - ``/var/opt/google/pgsql/logs/pg.log``
           - Log file containing postgres processing information for GEE Server. Note that “root” privileges are required to open this log file: use sudo su.
         * - ``/home_dir_of_user/.fusion/gepublishdatabase.date.time``
           - Log file containing information about push attempts from Fusion.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
