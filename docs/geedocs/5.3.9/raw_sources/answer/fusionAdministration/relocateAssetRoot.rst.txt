|Google logo|

========================
Relocate your asset root
========================

.. container::

   .. container:: content

      When you install Google Earth Enterprise, you configure a source
      volume and asset root for your production data. If you accepted
      the default values, they are ``/gevol/src`` and ``/gevol/assets``,
      respectively.

      You may need to relocate your asset root to a different volume for
      a variety of reasons. You may be migrating from a single- to a
      multiple-workstation configuration or modifying the local
      path of a network-mounted source volume (for example, when adding
      a larger drive for source data).

      .. note::

         Your asset root and publish root,
         ``/published_dbs/``, should be located on the same volume,
         enabling Fusion to use symbolic links. Keeping your asset root
         and publish root on separate volumes causes Fusion to make
         copies of your databases, obviously leading to greater use of
         disk space.

      -  :ref:`Relocate your asset root and publish root <Relocate_Asset_Publish_Roots>`
      -  :ref:`Relocate your asset root only <Relocate_Asset_Root>`

      .. _Relocate_Asset_Publish_Roots:
      .. rubric:: Relocate your asset root and publish root

      .. rubric:: To relocate your asset root and publish root:
         :name: to-relocate-your-asset-root-and-publish-root

      #. Copy your asset root to the new location.
      #. Select your new asset root:

         ``sudo /opt/google/geselectassetroot --assetroot new_path``

      #. You may need to repair the relocated asset root, fixing various
         inconsistencies in the asset root (such as permissions,
         ownership, missing ID files, etc.). When you run this
         command, the tool auto-detects the problems that need to be
         repaired and fixes them.

         ``sudo /opt/google/bin/geconfigureassetroot --repair --chown new_path``

         .. tip::

            You may relocate/change other volume settings by editing
            volumes. You can add a volume to the selected asset root,
            modify the localpath definition for an existing volume, or
            add a volume definition:
            ``sudo /opt/google/bin/geconfigureassetroot --editvolume``

         .. tip::

            Make sure that the update to the asset root is successful
            before attempting further changes to volume settings.

      #. Run Fusion and verify that you see all your databases.
      #. Create your new publish root:

         ``sudo /opt/google/bin/geconfigurepublishroot``

      #. Once you have configured the new publish root, you must
         republish all of the databases, by pushing and then publishing
         them. You can use the **Fusion GUI** to push and the **GEE
         Server Admin console** to publish your databases or you can
         perform these steps on the command line:

         ``geserveradmin -adddb``

         ``geserveradmin -pushdb``

         ``geserveradmin -publishdb``

      #. Optionally, you can delete your old asset root and your publish
         root.

      .. _Relocate_Asset_Root:
      .. rubric:: Relocate your asset root only

      Your asset root and publish root, ``/published_dbs/``, should be
      located on the same volume, enabling Fusion to use symbolic links.
      If you maintain your asset root and publish root on separate
      volumes, Fusion makes copies of your databases, obviously leading
      to greater use of disk space.

      .. rubric:: To relocate your asset root if your publish root will
         not be on the same volume:
         :name: to-relocate-your-asset-root-if-your-publish-root-will-not-be-on-the-same-volume

      #. Unpublish all databases on the **Databases** page of the **GEE
         Server Admin console** or on the command line:

         ``geserveradmin --unpublish target_path``

      #. Delete all pushed databases:

         ``geserveradmin --deletedb db_name``

      #. Clean up deleted databases using ``garbagecollect``. This is
         equivalent to emptying the *Trash* on some operating systems.

         ``geserveradmin --garbagecollect``

      #. Copy the asset root to the new location.
      #. Select the new asset root:

         ``sudo /opt/google/geselectassetroot --assetroot new_path``

      #. You may need to repair the relocated asset root, fixing various
         inconsistencies in the asset root (such as permissions,
         ownership, missing ID files, etc.). When you run this
         command, the tool auto-detects the problems that need to be
         repaired and fixes them.

         ``sudo /opt/google/bin/geconfigureassetroot --repair --chown new_path``

      #. Run Fusion and verify that you see all of the databases.
      #. Once you have configured the new publish root, you must
         republish all your databases, by pushing and then publishing
         them. You can use the **Fusion GUI** to push and the **GEE
         Server Admin console** to publish your databases or you can
         perform these steps on the command line:

         ``geserveradmin -adddb``

         ``geserveradmin -pushdb``

         ``geserveradmin -publishdb``

      #. Optionally, you can delete your old asset root.

      .. rubric:: Learn more
         :name: learn-more

      -  :doc:`../fusionAdministration/pushAndPublishDB`
      -  :doc:`../fusionTutorial/confTutorialWS`
      -  :doc:`../fusionAdministration/backupFusionServers`

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
