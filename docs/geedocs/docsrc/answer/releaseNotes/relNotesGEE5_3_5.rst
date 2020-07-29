|Google logo|

=============================
Release notes: Open GEE 5.3.5
=============================

.. container::

   .. container:: content

      .. rubric:: Supported Platforms

      The Open GEE 5.3.5 release is supported on 64-bit versions of the
      following operating systems:

      -  Red Hat Enterprise Linux version 6.x and 7.x, including the
         most recent security patches
      -  CentOS 6.x and 7.x
      -  Ubuntu 16.04 LTS

      Google Earth Enterprise 5.3.5 is compatible with Google Earth
      Enterprise Client (EC) version 7.1.5 and above.


      To upgrade from Open GEE 5.2.0, do NOT uninstall it. We recommend
      that you upgrade Open GEE 5.2.0 by simply installing Open GEE
      5.3.5. Installing Open GEE 5.3.5 on top of Open GEE 5.2.0 will
      ensure that your PostgreSQL databases are backed up and upgraded
      correctly to the new PostgreSQL version used by Open GEE 5.3.5.

      .. rubric:: Resolved Issues

      .. list-table::
         :widths: 10 85
         :header-rows: 1

         * - Number
           - Description
         * - 1782
           - RPM name change causes Fusion and Server services to be off after upgrade
         * - 1778
           - Databases published with SSL virtual hosts fail to cut
         * - 1774
           - CGI Error Handler Slows Cutter
         * - 1772
           - Geecheck reports server is not installed in dual configuration
         * - 1768
           - Option to not install documentation
         * - 1749
           - Allow users to serve WMS of 3D files from server UI
         * - 1742
           - Mis-configured Apache ErrorDocuments
         * - 1735
           - Vector packet description string retrieved by geglxinfo is incorrect
         * - 1717
           - Use standard static_assert instead of home grown compile assert
         * - 1715
           - Documentation currently references old/unsupported configurations
         * - 1714
           - Separate Portable Builds
         * - 1711
           - Portable Archive Contains FileUnpacker Build Directory
         * - 1698
           - Standardize the use of unsigned integers
         * - 1680
           - Broken Links in Portable Windows Documentation
         * - 1675
           - Move to gradle 6
         * - 1677
           - Some mercator maps fail to display in dual-server setups
         * - 1667
           - FusionTutorial files are available on s3.  download_tutorial.sh needs to be updated.
         * - 1666
           - User sees "plugin failed to load" alerts when browsing to portable server page
         * - 1657
           - fatal error: autoingest/sysman/plugins/MapLayerAssetD.h: No such file or directory
         * - 1650
           - WMS https doesn't work behind reverse proxies
         * - 1649
           - Consolidate XML loading code into AssetSerializer
         * - 1643
           - geserver_init still relies on initscripts
         * - 1642
           - Getop does not have an option to print status and exit
         * - 1634
           - Portable server on Windows stops working if user clicks inside terminal window
         * - 1622
           - Make public unpacker function for accessing globe directories
         * - 1621
           - EL6 scons uses Python 2.6
         * - 1616
           - Allow missing rows in publish_context_table
         * - 1608
           - Cutter fails when polygon level is 24
         * - 1604
           - WMS with reverse proxies doesn't work
         * - 1602
           - Create a make.bat file to generate html from rst file for Windows users
         * - 1598
           - Incorrect RedHat 7 Install Instructions
         * - 1593
           - Python Dependency Issues on EL6
         * - 1579
           - Add an option to misc.xml for disabling minification
         * - 474
           - Running gee_check on some supported platforms reports that the platform is not supported
         * - 254
           - Automasking fails for images stored with UTM projection

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
