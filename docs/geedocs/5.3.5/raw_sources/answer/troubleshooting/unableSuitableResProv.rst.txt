|Google logo|

===========================================
Unable to find a suitable resource provider
===========================================

.. container::

   .. container:: content

      .. rubric:: Error message

      ``Unable to find a suitable resource provider: no CPU(s) is available to start the task <task_name>``

      .. rubric:: Description

      This error typically occurs when the system is building and no
      more CPUs are available for the build task. This error can also
      occur if there are fewer CPUs (maximum number of concurrent
      tasks/jobs) assigned to Fusion (specifically, to a Fusion machine,
      also called a ``geresourceprovider``) than the minimum CPU setting
      for a task rule.

      .. rubric:: Resolution

      In response to the error, you can do one of the following:

      -  Wait for the other build or task to finish.
      -  Assign more CPUs (maximum number of concurrent tasks/jobs) to
         Fusion.

         You can use the tool
         ``geselectassetroot --assetroot path --numcpus`` to assign more
         CPUs. For example:

         ``geselectassetroot --assetroot /gevol/assets --numcpus 8``

      -  If the task can complete with fewer CPUs, decrease the minimum
         CPU setting for the :doc:`task rule <../fusionAdministration/confTaskRulesForFusionPerf>`.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
