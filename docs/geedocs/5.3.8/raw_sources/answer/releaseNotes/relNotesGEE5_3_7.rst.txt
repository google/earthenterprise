|Google logo|

=============================
Release notes: Open GEE 5.3.7
=============================

.. container::

   .. container:: content

      .. rubric:: New Features

      **New feature here**.
      New feature description goes here.

      In 5.3.7, the Fusion UI was updated from using the Qt3 UI toolkit
      to Qt4. Qt4 was bundled with Earth Enterprise. This offers a more
      modern appearence for fusion and makes it easier to upgrade other
      libraries.

      .. rubric:: Supported Platforms

      The Open GEE 5.3.7 release is supported on 64-bit versions of the
      following operating systems:

      -  Red Hat Enterprise Linux version 6.x and 7.x, including the
         most recent security patches
      -  CentOS 6.x and 7.x
      -  Ubuntu 16.04 LTS

      NOTE: CentOS 6 was officially retired in November 2020, and is no
      longer available for download except through CentOS archives. CentOS 6
      was not tested for this release or 5.3.6. CentOS 6 will not be
      supported in future releases.

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
           - Corrected variable names
         * - 1858
           - Remove Apache 2.2 references from the docs
           - Documentation updated
         * - 1856
           - WMS tiles stitching is mixed with fetching
           - Moved stitching into separate function

      .. rubric:: Known Issues

      See the `GitHub issues page <https://github.com/google/earthenterprise/issues>`_
      for a list of known issues.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
