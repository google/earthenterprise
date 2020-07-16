|Google logo|

=================
Installation tips
=================

.. container::

   .. container:: content

      .. rubric:: Before installation
         :name: before-installation

      -  With your development team, plan where you will place your
         asset root and publish root.
      -  Make sure you have enough disk space in your resource locations
         to build and publish your databases.
      -  Do not use a portable USB disk for the asset root or publish
         root because they are prone to I/O errors when reading or
         writing to the drive.
      -  Disable any auxiliary security, such as sudo blocks or sticky
         bits, before you install and use GEE.
      -  To install and administer GEE, log in as root or create a
         profile with root-level privileges.
      -  Create a user-level profile for using Fusion and publishing to
         the GEE Server.

      .. rubric:: During installation
         :name: during-installation

      -  Make sure that you run both the Fusion and GEE Server
         installers, and that both are at the same version number.
      -  When prompted during installation, choose **Upgrade your asset
         root**. This ensures that the resources will work with the
         version you are installing.
      -  The GEE Server installer allows you to customize users and
         groups. Most use cases do not require user and group
         customizations -- for your initial installation, use the
         default settings.

      .. rubric:: After installation
         :name: after-installation

      -  After installing, switch over to your user profile; i.e.,
         ``geeuser``. Publishing or building resources under
         the root profile can create various permissions issues that
         will cause further issues later.
      -  Go to ``/opt/google/share/support``, copy the ``GEEcheck`` folder
         to your system's ``/tmp/`` folder, and then run the ``GEEcheck``
         diagnostic script on your system. Review the output ``.txt``
         file for any post-install errors.
      -  Do not modify the ``default_ge``, ``default_map``, or
         ``default_search`` stream servers. If you need to change a
         configuration for a virtual server, create a new one from the
         example files at ``/opt/google/share/gehttp/examples``.
      -  Complete the GEE tutorial and create the sample database for
         testing. It is fast to create, process, publish, and modify. If
         you plan to enable authentication on your installation, first
         make sure that everything is installed correctly by using the
         tutorial database. After you have verified the functionality of
         the entire suite (Fusion, GEE Server, and client) using the
         tutorial settings, enabling authentication should go smoothly.
      -  GEE Server is designed to have direct communications to the
         client. DNS must be configured correctly for it to send data
         and receive requests. If you are using a DNS configuration that
         entails an Internal/External designation for the same GEE
         machine, test with the internal configuration to ensure all
         aspects are functioning correctly, and then add external
         addressing settings.
      -  If you have chosen to automatically start daemons after
         installation, you should see the services starting up and
         you will be returned to the command prompt. If there is an error,
         contact GEE Support.

      .. rubric:: Learn more
         :name: learn-more-i

      -  :doc:`Install Google Earth Enterprise <../installGEE/installOverviewGEE>`
      -  :doc:`Open GEE Wiki <../installGEE/wikiGEE>`

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
