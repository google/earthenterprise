|Google logo|

================================
Manually halt the cutter process
================================

.. container::

   .. container:: content

      Sometimes, a globe cutting procedure might be interrupted from the
      GEE Portable Globe Cutting web page. This can result in a
      "runaway" or "zombie" process on the system that hosts the
      Portable Globe Cutter tool. These processes must be halted at the
      operating system level.

      The GEE Portable Globe Cutting system uses the command line tools
      listed below. You can search for these as processes on the
      operating system:

      ``/opt/google/bin/gepolygontoqtnodes /opt/google/bin/gerewritedbroot /opt/google/bin/gekmlgrabber /opt/google/bin/geportableglobebuilder /opt/google/bin/geportableglobepacker /opt/google/gehttpd/cgi-bin/globe_cutter.py``
      You can use either of these methods to halt the processes from the
      command line:

      -  Invoke the Linux ``top`` command and look for any of the
         command line tools in the active process list. To stop the
         processes directly from the ``top`` command, press the **k**
         key and enter the process id number.
      -  Invoke the Linux ``ps`` command and search for any of the
         command line tools. For example: ``ps aux | grep geportable``.
         If you find a process, you can halt it with the ``kill``
         command: ``kill -9 process id``

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
