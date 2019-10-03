|Google logo|

=======================
Maps display grey tiles
=======================

.. container::

   .. container:: content

      Publishing issues (such as 2D browser-based maps that display grey
      tiles or no imagery) can indicate a hostname error. For example,
      accidentally setting the hostname to ``localhost.localdomain``
      instead of ``myserver.com``.

      To verify the hostname:

      Query the Fusion 2D map configuration directly from the web
      browser:

      ``http://myserver.com/mymaps/query?request=LayerDefs``

      A response that includes a line like the example below indicates
      that GEE is directing users to your local host instead of the
      publishing server.

      ``var stream_url = "http://localhost.localdomain/default_map";``
      
      To correct the hostname:

      #. Query ``hostname -f`` on the server, and verify that the path
         to the hostname can be resolved. (In other words, verify that
         users can access the URL.)
      #. To correct the hostname for all assets on your server, run
         ``geconfigureassetroot --fixmasterhost``.
      #. Republish the database to the virtual server.
      #. Verify that the URL displays the map correctly.

      If the URL does not display your map at all, run
      ``geserveradmin --publisheddbs`` on your Fusion machine to verify
      that you published the database to the correct virtual server. If
      you don't see ``http://myserver.com/mymaps`` listed with the
      database you want to publish, then republish the database.

      To further diagnose the issue, review the
      ``/opt/google/gehttpd/logs/error_log`` or run the
      :doc:`geecheck <../troubleshooting/fusionDiagnostics>` script.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
