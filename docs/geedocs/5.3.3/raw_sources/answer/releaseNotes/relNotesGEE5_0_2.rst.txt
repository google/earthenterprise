|Google logo|

========================
Release notes: GEE 5.0.2
========================

   .. container:: content

      .. rubric:: Overview

      This document includes the following information about the Google
      Earth Enterprise (GEE) 5.0.2 release:

      GEE 5.0.2 is an incremental release of
      :doc:`GEE 5.0 <../releaseNotes/relNotesGEE5_0>` and
      :doc:`GEE 5.0.1 <../releaseNotes/relNotesGEE5_0_1>`. It addresses a
      vulnerability reported by the `OpenSSL
      Project <http://www.openssl.org/>`_, as well as fixes to
      several minor bugs.

      .. rubric:: Supported Platforms

      The Google Earth Enterprise 5.0.2 release is supported on
      64-bit versions of the following operating systems:

      -  Red Hat Enterprise Linux versions 6.0 to 6.5, including
         the most recent security patches
      -  CentOS 6.5
      -  Ubuntu 10.04 and 12.04 LTS

      Google Earth Enterprise 5.0.2 is compatible with Google
      Earth Enterprise Client (EC) and Plugin versions 7.0.1 -
      7.1.1.1888 for Windows, Mac, and Linux.

      .. rubric:: Library updates

      OpenSSL update to address a `vulnerability <http://www.openssl.org/news/vulnerabilities.html>`_
      reported by the `OpenSSL Project <http://www.openssl.org/>`_

      -  Updates ``openssl`` from v0.9.8y to v0.9.8za.

      .. rubric:: Known Issues

      .. list-table::
         :widths: 25 25 50
         :header-rows: 1

         * - Number
           - Description
           - Workaround
         * - 16701881
           - When attempting to set the maximum number of CPUs used for
             Fusion processing tasks, **geselectassetroot --numcpus** fails to update the value specified.
           - For GEE releases 5.0.0-5.0.2, the maximum number of CPUs can be set using the
             ``maxjobs`` option in ``/opt/google/etc/systemrc``.

                #. Stop Fusion daemons:
                   ``sudo /etc/init.d /gefusion stop``
                #. Set ``maxjobs`` in ``/opt/google/etc/ systemrc``.
                #. Start Fusion daemons:
                   ``sudo /etc/init.d/gefusion start``

      .. rubric:: Resolved Issues

      .. list-table::
         :widths: 25 25 50
         :header-rows: 1

         * - Number
           - Description
           - Resolution
         * - 16187780
           - When attempting to import the **GDAL** library installed with
             ``/opt/google/gepython/Python-2.7.5/bin/python`` Python 2.7.5, from
             ``osgeo import gdal_array``, a traceback error occurs:
             ``ImportError: No module named _gdal_array``.
           - Fixed. The Python numpy library, which is required to load the ``gdal_array`` module,
             is now bundled with GEE Fusion.
         * - 11823150
           - When setting the maximum number of CPUs available for Fusion processes using the
             ``geselectassetroot --numcpus`` command, you cannot increase the value beyond 8.
           - Fixed. You can now set the maximum number of CPUs using the
             ``geselectassetroot --numcpus`` command to a maximum value of 64. You can
             verify the value you set using ``getop`` The default setting for the maximum value is 8.
             For more information, see
             :doc:`Running Fusion on a machine with multiple CPUs <../fusionAdministration/multipleCPUConfig>`.
         * - 15391728, 15386589, 15429155
           - Potential cross-site scripting vulnerabilities have been identified.
           - Fixed. Enhancements have been made to make GEE more secure.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
