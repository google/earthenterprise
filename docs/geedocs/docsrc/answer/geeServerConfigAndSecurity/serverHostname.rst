|Google logo|

===============
Server hostname
===============

.. container::

   .. container:: content

      When you install GEE on a server, it queries for the name of the
      local host by issuing a ``hostname -f`` command. Each time you
      build a resource, project, or database on the server, the host
      name is added to all the files in each asset’s subdirectory. If
      you change the hostname or if you move assets to another server,
      you must also update the ``assetroot host`` entry to match the new
      host name.

      To change the asset root host entry, use the
      ``geconfigureassetroot`` command:

      ``sudo /opt/google/bin/geconfigureassetroot --fixmasterhost``


.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
