|Google logo|

=================
Permission denied
=================

.. container::

   .. container:: content

      .. rubric:: Error message

      ``Permission denied``
      
      .. rubric:: Description

      Fusion tried to read from or write to disk and permission was
      denied, indicating that read/write permissions are not set
      correctly.

      Background daemons run as the ``gefusionuser:gegroup`` user and
      group build assets in Fusion. Even if a file has the correct
      permissions set, the Fusion daemons cannot access it unless all of
      the parent directories have both ``read`` and ``execute``
      permissions set for ``gefusionuser:gegroup``. You must be
      particularly careful to set permissions that allow these Fusion
      daemons to access any source files that you create or copy.
      Typically, you should grant ``read`` access for all.

      Publishing while logged in as ``root`` can cause permissions
      errors such as this. If you were previously able to publish and
      are now getting this error, correct the errors and publish under a
      user-level profile.

      .. rubric:: Resolution

      Enter this command:

      ``chmod -R 755 /src``
      
      Replace ``src`` with the top-level folder that contains your
      resources.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
