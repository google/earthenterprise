|Google logo|

=============================
Release notes: Open GEE 5.3.6
=============================

.. container::

   .. container:: content

      .. rubric:: New Features

      In 5.3.6, a ``--secure`` option was added for ``geconfigureassetroot``
      and ``geupgradeassetroot`` that removes world-writable permissions from
      special files. This helps Open GEE perform properly in strict security
      environments. ``--secure`` is disabled by default.

      .. rubric:: Supported Platforms

      The Open GEE 5.3.6 release is supported on 64-bit versions of the
      following operating systems:

      -  Red Hat Enterprise Linux version 6.x and 7.x, including the
         most recent security patches
      -  CentOS 6.x and 7.x
      -  Ubuntu 16.04 LTS

      Google Earth Enterprise 5.3.6 is compatible with Google Earth
      Enterprise Client (EC) version 7.1.5 and above.


      To upgrade from Open GEE 5.2.0, do NOT uninstall it. We recommend
      that you upgrade Open GEE 5.2.0 by simply installing Open GEE
      5.3.6. Installing Open GEE 5.3.6 on top of Open GEE 5.2.0 will
      ensure that your PostgreSQL databases are backed up and upgraded
      correctly to the new PostgreSQL version used by Open GEE 5.3.6.

      .. rubric:: Resolved Issues

      .. list-table::
         :widths: 35 65
         :header-rows: 1

         * - Number
           - Description
         * - 1851
           - Open GEE does not function properly on systems with a restrictive umask
         * - 1806
           - Confusing "geserver.service changed on disk" message after uninstalling Fusion or Server
         * - 1800
           - Upgrade from 5.3.4 to 5.3.5 leaves a non-working ExampleSearch
         * - 1785
           - Imagery Projects - Transparent areas appear as black
         * - 1761
           - Tab Order wrong for Fusion Add Vector Resource and Add Terrain Resource
         * - 1741
           - Invalid git version in CentOS/RHEL install instructions

      .. rubric:: Known Issues

      See the `GitHub issues page <https://github.com/google/earthenterprise/issues>`_
      for a list of known issues.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
