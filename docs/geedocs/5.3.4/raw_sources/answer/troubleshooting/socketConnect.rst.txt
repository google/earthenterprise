|Google logo|

==============
Socket connect
==============

.. container::

   .. container:: content

      .. rubric:: Error message

      ``Socket connect``\ *xxx.xx.xxx.xx*\ \_13031

      (where ``xxx.xx.xxx.xx`` is your IP address)
      
      .. rubric:: Description

      This appears in the vector Asset Log or when starting Fusion from the
      command line when the system daemons are not running.

      .. rubric:: Resolution

      From the command line, enter:

      .. code-block:: none
      
            sudo /etc/init.d/geserver stop
            sudo /etc/init.d/geserver start

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
