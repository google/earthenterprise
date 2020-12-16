|Google logo|

=============================
Release notes: Open GEE 5.3.7
=============================

.. container::

   .. container:: content

      .. rubric:: New Features

      **New feature here**.
      New feature description goes here.

      In 5.3.7, the Fusion UI was updated from Qt3 to Qt4. Qt4 was bundled
      with earth enterprise to make it easy to acquire and build with.

      .. rubric:: Supported Platforms

      The Open GEE 5.3.7 release is supported on 64-bit versions of the
      following operating systems:

      -  Red Hat Enterprise Linux version 6.x and 7.x, including the
         most recent security patches
      -  CentOS 6.x and 7.x
      -  Ubuntu 16.04 LTS

      NOTE: CentOS 6 was officially retired in November 2020, and is no
      longer available for download except thru CentOS archives. CentOS 6
      is not being tested for this release and will not be supported in
      future releases.

      Google Earth Enterprise 5.3.7 is compatible with Google Earth
      Enterprise Client (EC) version 7.1.5 and above.


      To upgrade from Open GEE 5.2.0, do NOT uninstall it. We recommend
      that you upgrade Open GEE 5.2.0 by simply installing Open GEE
      5.3.7. Installing Open GEE 5.3.7 on top of Open GEE 5.2.0 will
      ensure that your PostgreSQL databases are backed up and upgraded
      correctly to the new PostgreSQL version used by Open GEE 5.3.7.

      .. rubric:: Resolved Issues

      .. list-table::
         :widths: 10 30 55
         :header-rows: 1

         * - Number
           - Description
           - Resolution
         * - 1868
           - Apache proxy configurations missing in HTTP modules
           - Added missing apache module for gehttpd server build
         * - 1861
           - Invalid variables when reporting postgres errors
           - Corrected variable name
         * - 1858
           - Remove Apache 2.2 references from the docs
           - Documentation updated
         * - 1856
           - WMS tiles stitching is mixed with fetching
           - Moved stitching into seperate function

      .. rubric:: Known Issues

      See the `GitHub issues page <https://github.com/google/earthenterprise/issues>`_
      for a list of known issues.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
