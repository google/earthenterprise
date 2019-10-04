|Google logo|

==========================================
Before you install Google Earth Enterprise
==========================================

.. container::

   .. container:: content

      -  :ref:`Overview <Overview_Before_Install_GEE>`
      -  :ref:`Configuring Your Host Volumes <Configuring_Host_Volumes>`
      -  :ref:`Configuring Multiple Storage Devices <Configuring_Multiple_Storage_Devices_GEE>`
      -  :ref:`Planning the Location of Your Asset Root <Planning_Location_Asset_Root_GEE>`
      -  :ref:`Planning the Location of Your Publish Root <Planning_Location_Publish_Root_GEE>`
      -  :ref:`Setting Up Google Earth Enterprise Users <Setting_Up_GEE_Users>`
      -  :ref:`Deciding Which Products To Install <Deciding_Which_Products_Install>`

      .. _Overview_Before_Install_GEE:
      .. rubric:: Overview

      Before you install the Google Earth Enterprise software, you must
      configure your hardware, network, and Google Earth Enterprise
      users. You must also think about where you want to store
      Google Earth Enterprise Fusion data and where you want to publish
      Google Earth Enterprise Fusion databases. You will need to
      provide this information during installation, but you should
      consider these decisions in advance before you begin the
      installation procedure.

      The following sections provide information about what the Google
      Earth Enterprise software requires in each of these areas.

      .. warning::

         Be sure to complete all of the tasks described in these
         sections before installing the Google Earth Enterprise
         software.

      .. _Configuring_Host_Volumes:
      .. rubric:: Configuring The Host Volumes

      Google Earth Enterprise Fusion data (resources, projects, and
      databases) require a local name and network path to resolve the
      locations of both source files and related Google Earth Enterprise
      Fusion data. For that reason, you cannot change the network naming
      convention you adopt for host volumes without invalidating Google
      Earth Enterprise Fusion data.

      On a single workstation setup (non-network based), the network
      path and local path for Google Earth Enterprise Fusion data are
      identical. However, because migration to a network-based
      configuration is inevitable, we recommend that you use a network
      naming convention for any new volume hosting Google Earth
      Enterprise Fusion data or source data, whether it is part of a
      network initially or not.

      Because Linux systems frequently use either ``/vol(*)`` or
      ``/``\ ``data(*)`` as the local volume definition on a new system,
      using this convention for the initial Google Earth Enterprise
      Fusion data location can cause name conflicts if you later switch
      from a single workstation to a network-based configuration. For
      example, if you initially define ``/vol1/assets`` as the network
      location for a Google Earth Enterprise Fusion *asset root*, and
      you later add another workstation that has a local volume called
      ``/vol1``, that workstation cannot reference
      ``/vol1/assets``\ through the network because of the name conflict
      with its local volume definition. (See :ref:`Planning the Location of
      Your Asset Root <Planning_Location_Asset_Root_GEE>` for more information about the asset root.)

      You can work around this problem by adopting a unique naming
      convention for all volumes on your network (such as ``/vol1`` ...
      ``/voln``). However, we suggest that you use ``/gevol`` as an
      alternative volume naming convention because it is unlikely to
      conflict with standard Linux volume definitions. The following
      diagram illustrates this point.

      .. note::

         On a single workstation that does not mount
         ``/gevol`` on a network, ``/gevol`` is also required as a local
         volume definition.

      .. _Configuring_Multiple_Storage_Devices_GEE:
      .. rubric:: Configuring Multiple Storage Devices

      Google Earth Enterprise Server and the Google Earth Enterprise
      Fusion asset root, source volumes, and publish root require large
      amounts of disk storage space. Google Earth Enterprise Fusion
      requires about three times as much storage space as Google Earth
      Enterprise Server. The storage space can be either in the form of
      internal disks, directly attached storage devices,
      network-attached storage (NAS) devices, or storage area network
      (SAN) devices. Typically, these devices are configured into
      redundant arrays of independent disks (RAIDs) and presented to the
      operating system as volumes. Volumes can be several hundred
      gigabytes up to tens of terabytes.

      The difference between configuring a workstation with a single
      hard disk and a workstation with multiple volumes relates to the
      mount point definitions for the source and asset volumes.When
      configuring a Linux workstation, we strongly recommend that you
      use the following mount point naming conventions:

      -  **Single volume**

         Mount the single drive to slash (/). All data
         (``/gevol/assets``, ``/gevol/src``, and
         ``/gevol/published_dbs``) resides on that drive with the local
         path defined using the ``/gevol`` naming convention.

      -  **Two volumes - a small system volume and a larger data
         volume**

         Mount the small system drive to slash (``/``). Mount the larger
         data drive to ``/gevol``. Source and asset data volumes can
         then be defined as ``/gevol/assets`` and ``/gevol/src``.

      -  **Three volumes - a small system volume and two larger data
         volumes**

         Mount the small system drive to slash (``/``). Mount the first
         large data drive to ``/gevol/assets``. Mount the second large
         data drive to ``/gevol/src``.

      -  **More than three volumes**

         There are several strategies for storing very large data sets.
         Google Earth Enterprise Fusion can read from and write to
         multiple volumes. For more information, you can request help on
         our `Slack channel <http://slack.opengee.org/>`_.

      It is also important to keep internal and external storage devices
      separated so that if your internal server goes down, it does not
      affect your ability to serve published data to external clients.
      Likewise, if your external server goes down, you can replace it
      and publish from the internal storage device. In addition (and
      perhaps more important), keeping your internal and external
      storage devices separate reduces the possibility of performance
      problems that could occur if you are building a large data set or
      a client requests a time-consuming search.

      .. _Planning_Location_Asset_Root_GEE:
      .. rubric:: Planning the Location of Your Asset Root

      During the Google Earth Enterprise Fusion installation procedure,
      you must specify a location for your *asset root*. The asset root
      is the main location where all of the assets (resources, map
      layers, projects, and databases) are stored that are created with
      Google Earth Enterprise Fusion.

      The asset root must be located on a single volume. It cannot be
      split across multiple volumes. Therefore, it is important to think
      ahead and allocate as much storage space as possible for the asset
      root.

      Unless you have an established partitioning scheme for all of your
      storage devices, we recommend that you accept the default
      partitioning scheme presented to you while installing Linux. That
      scheme gives you a reasonable amount of space in ``/opt`` for
      Google Earth Enterprise and other system software, a small amount
      of space for ``/home``, and the remaining space on your storage
      device for the asset root.

      We also recommend that you accept the default volume designation
      for your asset root during installation (``/gevol/assets``),
      unless that name conflicts with your established naming
      conventions.

      .. note::

         we recommend that you dedicate a network-attached
         storage device (NAS) for your asset root.

      .. _Planning_Location_Publish_Root_GEE:
      .. rubric:: Planning the Location of Your Publish Root

      During the Google Earth Enterprise Server installation procedure,
      you must specify a volume for the *publish root*. The publish root
      is the directory in which all of your published databases are
      stored.

      If you specify the same volume as the asset root, when you publish
      a database, Google Earth Enterprise Fusion registers the database
      on the specified volume and sets symbolic links to the database
      files. If you specify a different volume than the asset root, when
      you publish a database, Google Earth Enterprise Fusion registers
      the database on the specified volume and then copies all of the
      database files to the designated volume.

      For example, if you specify ``/gevol/assets`` for your asset root
      and ``/gevol/published_dbs`` for your publish root, when you
      publish a database, Google Earth Enterprise Fusion registers the
      database on ``gevol`` and sets hard links to the database files;
      no copying is necessary.

      However, if you specify ``/gevol/assets`` for your asset root and
      ``/data1/published_dbs`` for your publish root, when you publish a
      database, Google Earth Enterprise Fusion copies all of the
      database files from ``/gevol/assets`` to ``/data1/published_dbs``
      (unless you allow symbolic links during installation). Copying
      takes more time as well as extra disk space.

      .. _Setting_Up_GEE_Users:
      .. rubric:: Setting Up Google Earth Enterprise Users

      The Google Earth Enterprise installer automatically configures
      certain system users to perform background tasks at the system
      level. If you accept the default user names and allow the
      installer to create those users on your local workstation, you are
      implementing local authentication only. Local authentication is
      designed for standalone workstations only.

      If you are using Google Earth Enterprise over a network with at
      least two workstations, storage devices, and/or servers, we
      strongly recommend that you use a centralized network
      authentication system, such as LDAP, NIS, or one of the many
      commercially available systems.

      If you use a centralized network authentication system, you must
      add the following users to your authentication system’s user list:

      -  **gefusionuser**
      -  **geapacheuser**
      -  **gepguser**

      The primary group for all of these users is **gegroup**.

      If you are in a multi-user environment in which multiple
      workstations share a common asset root on a NAS/SAN, **these users
      must have the same UID on all devices, so you must assign them
      explicitly in both your network authentication system and in
      GEE**.

      Be sure to configure each GEE workstation, storage device, and GEE
      Server to use your network authentication system. For more
      information, see your network authentication system documentation.

      .. rubric:: Customizing Google Earth Enterprise User Names

      You can use customized user names and group names. Specify the
      custom user and group names when running the installers.

      .. _Deciding_Which_Products_Install:
      .. rubric:: Deciding Which Products To Install

      You do not need to install all products on all devices. To
      determine which products to install, follow these general
      guidelines:

      -  Install Google Earth Enterprise Fusion and Google Earth
         Enterprise Server on the server machine selected to import your
         source GIS data and create flyable 3D and 2D databases. A
         system in this configuration is typically called the
         "development machine" since its primary task is to build
         flyable databases and only a small number of users will view
         the flyable data for quality assurance testing.
      -  Install the Google Earth Enterprise Fusion tutorial files on
         the "development machine" for users who are new to Google Earth
         Enterprise Fusion.
      -  Install only the Google Earth Enterprise Server software on the
         server machine selected to only host flyable 3D and 2D
         databases only. All authorized end users will connect to this
         system -- typically referred to as the "production machine" --
         to view the flyable databases. This machine must be accessible
         through the network from the development machine in order for
         database publishes. Users who will not have direct network
         access to their production machines, or users who plan to
         update remote systems with external hard drives, must also
         install the "Disconnected Publishing Add-on" for additional
         tools.

      The following diagram shows a sample system configuration.

      |Networking diagram|

      In this example, there are two server-class machines assigned to
      data building and data hosting tasks, plus one workstation to be
      used for data management.

      A GIS specialist uses the workstation to remotely log in to the
      development machine with Google Earth Enterprise Fusion Pro
      installed to import source GIS data and output a flyable globe to
      publish for end users. Google Earth Enterprise Server software is
      also installed on the development machine so the GIS specialist
      may perform quality assurance tests on the data before publishing
      to the production machine.

      Only Google Earth Enterprise Server is installed on the production
      machine which authorized end users in the network may access with
      Google Earth Enterprise Client (EC) for 3D databases, or a
      compatible web browser for 2D databases.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
.. |Networking diagram| image:: ../../art/fusion/install/entnetwork.jpg
