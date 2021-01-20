|Google logo|

================================================
Operating System Settings That May Affect Fusion
================================================

.. container::

   .. container:: content

      The Linux operating system implements several limits that may prevent
      Fusion processes from running correctly, particularly when building large
      projects or databases.

      You may need to reboot your system after making the configuration changes
      below to ensure that the new configuration takes effect.

      .. _Number_Of_Processes:
      .. rubric:: Number of processes

      Linux limits the number of processes that each user can run. Since Fusion
      runs many processes to build assets in parallel, you may need to
      increase the limit by adding the following lines to
      ``/etc/security/limits.conf``:

      .. code-block:: none

          gefusionuser  soft    nproc   <num_procs>
          gefusionuser  hard    nproc   <num_procs>

      Fusion starts up to one process per core, plus two more (for the system manager
      and resource provider). ``<num_procs>`` should be considerably larger than
      that number to allow for operating system processes.

      .. _Number_Of_Open_Files:
      .. rubric:: Number of open files

      Linux also limits the number of files each process can open at once.
      Since Fusion may open many files when building an imagery project, for
      example, you may need to increase the limit by adding the following lines
      to ``/etc/security/limits.conf``:

      .. code-block:: none

          gefusionuser  soft    nofile   <num_files>
          gefusionuser  hard    nofile   <num_files>

      ``<num_files>`` should be at least the number of resources in your
      largest project, plus some extra to allow for the additional files Fusion
      uses, such as configuration files.

      .. _Number_Of_Memory_Maps:
      .. rubric:: Number of memory mappings

      Fusion uses memory mapping when reading some types of files. Since Linux
      limits the number of memory mappings each process can define, you may
      need to add the following lines to ``/etc/sysctl.conf``:

      .. code-block:: none

          vm.max_map_count = <num_maps>

      ``<num_maps>`` should be more than twice the number of resources in your
      largest project.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px

