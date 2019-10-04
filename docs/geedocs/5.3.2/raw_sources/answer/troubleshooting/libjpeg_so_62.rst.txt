|Google logo|

=============
libjpeg.so.62
=============

.. container::

   .. container:: content

      .. rubric:: Error message

      ``/opt/google/bin/fusion: error while loading shared libraries: libjpeg.so.62: cannot open shared object file: No such file or directory``
      
      .. rubric:: Description

      You are running Ubuntu 12.0.4 and you did not use the
      ``ln -s libcap.so.2.22 libcap.so.1`` command before trying to run
      any Fusion processes.

      .. rubric:: Resolution

      See :doc:`System requirements <../installGEE/sysReqFusionGEEServer>`.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
