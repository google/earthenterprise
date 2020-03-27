|Google logo|

==========
SourceFile
==========

.. container::

   .. container:: content

      .. rubric:: Error message

      ``Failed to open source_SourceFile``
      
      .. rubric:: Description

      The vector Asset log displays this error when read-write-execute
      permissions are not set properly. Permissions must be set for
      parent folders up to the root level.

      .. rubric:: Resolution

      At the command line, enter:

      ``sudo chmod -R a+r /gevol``
      
      This command corrects the access permissions on all Fusion-related
      files. If ``/gevol`` is not your asset root, replace it with the
      name of your asset root.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
