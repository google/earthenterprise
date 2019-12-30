|Google logo|

====================================
Dynamically adjusting logging levels
====================================

.. container::

   .. container:: content

      In some instances, it may be desirable to change the logging level
      of gesystemmanager dynamically during execution. The log level can
      be set in ``/etc/opt/google/systemrc`` with the ``<logLevel>``
      tag. Accetable values range from 0-7, with 7 corresponding to the
      highest level. When gesystemmanager loads, it will give preference
      to the ``KH_NFY_LEVEL`` environment variable. To dynamically
      reload the systemrc file: ``/etc/init.d/gefusion reload``

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
