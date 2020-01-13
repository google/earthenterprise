|Google logo|

==========================
Portable Release Notes 5.0
==========================

.. container::

   .. container:: content

      This document includes the following information about the Google
      Earth Portable 5.0 release:

      .. rubric:: Overview

      Google Earth Enterprise (GEE) Portable 5.0 introduces a
      flexible and dynamic approach to serving portable maps and
      globes. It includes the following major changes:

      -  **User interface changes**
      -  **Updated support for .glc files**
      -  **Windows support for large-size portable files**

         -  Files that are 2 GB or larger can now be served on the
            Windows version of GEE Portable 5.0

      -  **Linux support**

         -  Red Hat Enterprise Linux versions 5.6 to 6.2,
            including the most recent security patches
         -  Ubuntu 10.04 LTS and 12.04 LTS

      -  **Google Maps JavaScript API version updated to v3.14**
      -  **Support for .glr files has been deprecated**

            This also means that the broadcasting "key" is
            deprecated. Now when you broadcast, clients connect
            directly to your Portable Server. The main reason for
            this is that we now use relative paths in the dbroot,
            which makes use of the local proxy server unnecessary. It
            also means that when changing ports, there is better
            support for non-standard port usage , i.e., port 9335
            is no longer fixed.

      .. rubric:: Supported Platforms

      GEE Portable 5.0 is supported on the following operating
      systems:

         -  Windows 7 or later
         -  Mac OS X 10.7 and later
         -  Red Hat Enterprise Linux versions 6.0 to 6.4, including
            the most recent security patches
         -  CentOS 6.4
         -  Ubuntu 10.04 LTS and 12.04 LTS

      GEE Portable 5.0 is compatible with Google Earth Enterprise
      Client (EC) and Plugin versions 7.0.1 - 7.1.1.1888 for
      Windows, Mac, and Linux.

      .. rubric:: System Requirements

      .. note::
         If you plan to serve large globes from Portable
         Server, you may also need to provide more storage, depending
         on their size.

      **Windows 7 or later**

         -  Minimum dual-core Intel or AMD CPUs; 2.0 GHz or higher
            recommended.
         -  Minimum 2 GB RAM.
         -  Minimum 2 GB of free space.
         -  Graphics card: DirectX9 and 3D capable with 256 MB of VRAM
            (discrete GPU recommended).
         -  Screen: 1280x1024, "32-bit True Color."

      .. note::
         We recommend using NTFS for attached storage
         devices hosting the portable globes, especially if the
         ``.glx`` files are > 4 GB

      **Mac OS X 10.7 or later**

         -  Minimum dual-core Intel Mac
         -  Minimum 2 GB RAM.
         -  Minimum 2 GB of free space.
         -  Graphics card: DirectX9 and 3D capable with 256 MB of VRAM
            (discrete GPU recommended).
         -  Screen: 1280x1024, "Millions of Colors."

      **Linux**

      Operating systems supported:

         -  Red Hat Enterprise Linux versions 6.0 to 6.4, including
            the most recent security patches.
         -  CentOS 6.4
         -  Ubuntu 10.04 and 12.04 LTS

      Hardware requirements:

         -  Minimum dual-core Intel or AMD CPUs; 2.0 GHz or higher
            recommended.
         -  Minimum 4 GB RAM.
         -  Minimum 2 GB of free space.
         -  Graphics card: at least 64 MB video RAM (NVIDIA GeForce4
            or higher preferred).

      .. rubric:: New features

      **Broadcasting set by portable.cfg file**
      Broadcasting no longer can be set from the default user
      interface; instead you can set it using either of the
      following methods:

         -  ``http://localhost:9335/?cmd=set_key&accept;_all_requests=t``
         -  Add ``accept_all_requests True`` to your ``portable.cfg``
            file

      You can also disable broadcasting via the ``portable.cfg``
      file. This prevents a rogue page from turning on
      broadcasting via a localhost reference.

         -  Add ``disable_broadcasting True`` to your
            ``portable.cfg`` file

      .. rubric:: Known Issues

      .. list-table::
         :widths: 10 40 40
         :header-rows: 1

         * - Number
           - Description
           - Work Around
         * -
           - The new Preview options for 2D maps in GEE Server and Portable do not support the display of
             Plate Carrée 2D maps.
           - With Portable, you can still view existing .glm Plate Carrée 2D map files via the old preview
             interface, which you can access by pointing GEE Portable at http://localhost:9335/list.

      .. rubric:: Resolved Issues

      .. list-table::
         :widths: 10 40 10
         :header-rows: 1

         * - Number
           - Description
           - Resolution
         * - 8627202
           - Portable for Windows 4.4.1 fails to read map or globe files greater than 2 GB in size.
           - Fixed

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
