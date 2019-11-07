|Google logo|

==============
Write to index
==============

.. container::

   .. container:: content

      .. rubric:: Error message

      ``Any of these conditions occur:``

      -  The project Asset log lists this error:

         ``Fusion Fatal : Unable to write record to index``

      -  Current state / Version Properties reads "In Progress".
      -  Canceling the project build in the GUI or on the command line
         causes the program to hang.
      -  Running the command ``getop`` causes the program to hang.

      .. rubric:: Description

      Your hard disk might be full, or a network drive might be
      disconnected.

      .. rubric:: Resolution

      #. Switch to root and enter:

         .. code-block:: none

            /etc/init.d/gefusion stop
            /etc/init.d/geserver stop
            sleep 5
            killall -9 gesystemmanager
            killall -9 geresourceprovider

      #. Enter:

         .. code-block:: none

            ps aux | grep ge

      #. Look for and stop any Fusion processes in the output.
      #. Make sure that you have enough free disk space, and then enter:

         .. code-block:: none

            /etc/init.d/gefusion start
            /etc/init.d/geserver start

      #. Cancel the build, and then resume it using the ``gecancel`` and
         ``geresume`` commands.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
