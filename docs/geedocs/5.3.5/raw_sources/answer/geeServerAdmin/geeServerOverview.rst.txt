|Google logo|

=======================
GEE Server Overview 5.x
=======================

.. container::

   .. container:: content

      GEE Server 5.x introduces a flexible and dynamic approach to
      publishing maps and globes: When publishing, you specify unique
      options for a map or globe at a given URL; the map or globe can
      then be easily published multiple times at different URLs with
      different configurations. Likewise, particular configurations can
      quickly be unpublished or modified.

      Virtual hosts (previously called virtual servers) now only specify
      a security protocol and can be associated with multiple published
      globes and maps. This change decouples the task of setting up
      security protocols from publishing maps and globes.

      Associating :doc:`search sets <../geeServerAdmin/createSearchTabs>` and :doc:`snippet
      profiles <../geeServerAdmin/manageSnippetProfiles>` at the time of publishing
      allows you to present different versions of the same underlying
      Fusion database, which can then be made available at different
      URLs and under different security protocols. For example,you can
      protect one set of searchable data but display it on the same
      globe as an unprotected set of searchable data. You can then
      publish the database privately by adding the sensitive search and
      a secured virtual host at one URL, then publish the database
      again, this time adding the non-sensitive search and public
      virtual host at a different URL.

      GEE Server now supports :doc:`Web Map Service
      (WMS) <../geeServerAdmin/makeWMSRequests>`. One of the benefits of using
      the WMS standard is that supported clients can request images from
      multiple WMS servers and then combine those mapping images into a
      single view. Because the WMS standard is used to get all the
      images, they can easily be overlaid on one another.

      :doc:`Publishing <../geeServerAdmin/publishDatabasesPortables>` can now be done in a
      matter of seconds from the GEE Server admin console, and no longer
      requires interaction with Fusion. We hope that this new publishing
      approach makes your life easier and maybe even a little more fun!

      GEE Server also provides tools for :doc:`cutting portable
      globes <../geeServerAdmin/createPortableGlobesMaps>` (``.glb``) and maps (``.glc``).
      :doc:`Composite globes and maps <../geeServerAdmin/createCompositeGlobesMaps>` can be
      assembled from 2D or 3D portable files. The resulting offline globe
      or map is packed as a ``.glc`` file that may be downloaded for use
      with Portable Server.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
