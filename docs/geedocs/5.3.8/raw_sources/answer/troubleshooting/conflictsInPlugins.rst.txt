|Google logo|

====================
Conflicts in plugins
====================

.. container::

   .. container:: content

      .. rubric:: Error message

      ``Conflicts in plugins``

      .. rubric:: Description

      The first time each user runs the Fusion GUI, error messages like
      these appear:

      ``Fusion Notice: Saving /gevol/assets/.config/misc.xml with defaults Conflict in /opt/kde3/lib/kde3/plugins/styles/keramik.so: Plugin uses incompatible Qt library! expected build key "i686 Linux g++-4 full-config", got "i686 Linux g++-3.* ful l-config". Conflict in /opt/kde3/lib/kde3/plugins/styles/kthemestyle.so: Plugin uses incompatible Qt library! expected build key "i686 Linux g++-4 full-config", got "i686 Linux g++-3.* ful l-config". Conflict in /opt/kde3/lib/kde3/plugins/styles/highcolor.so: Plugin uses incompatible Qt library! expected build key "i686 Linux g++-4 full-config", got "i686 Linux g++-3.* ful l-config". Conflict in /opt/kde3/lib/kde3/plugins/styles/light.so: Plugin uses incompatible Qt library! expected build key "i686 Linux g++-4 full-config", got "i686 Linux g++-3.* ful l-config".``

      The messages are triggered by a possible incompatibility between
      the operating system GUI widgets and the Qt library that GEE
      compiles. The messages only appear the first time Fusion is
      launched.

      .. rubric:: Resolution

      GEE uses only compatible plugins, so you can safely ignore the
      error messages. No action is needed.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
