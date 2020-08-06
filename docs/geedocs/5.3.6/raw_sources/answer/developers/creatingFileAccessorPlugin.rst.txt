|Google logo|

===============================
Creating a File Accessor Plugin
===============================

.. container::

   .. container:: content

      .. rubric:: Experimental

      Please note that this functionality is currently experimental. Creating
      and using a plugin is not guaranteed to work as expected at this time.

      .. rubric:: Description

      GEE Server implements a plugin system for file accessors. That is, plugins
      can be implemented that provide the ability to read from different types
      of file systems, either local or remote. The default POSIX accessor is
      built in, but additional plugins can be loaded at runtime.
      Currently, file accessors are only used when serving data, not when
      building, pushing, and publishing. In those cases, the standard file
      system functions are used to read and write data.

      .. rubric:: How to Implement a Plugin

      A file accessor plugin needs 3 components. The first is a function called
      ``get_factory_v1`` that returns a ``FileAccessorFactory`` pointer. The
      signature should be as follows:

      .. code-block:: cpp

        FileAccessorFactory* get_factory_v1 ();
         
      The second component is a factory class that inherits from
      ``FileAccessorFactory`` and overrides the following method:

      .. code-block:: cpp

         virtual AbstractFileAccessor* GetAccessor(const std::string &fileName);

      The final component is a class that derives from ``AbstractFileAccessor``
      and overrides all of the pure virtual methods. See FileAccessor.h for the
      complete interface.

      .. rubric:: Example

      To see a barebones sample implementation, look at
      ``earthenterprise/earth_enterprise/src/common/test_plugins/FileAccessorTestPlugin.cpp``.

      .. rubric:: Additional Information

      - ``get_factory_v1`` should be exported as ``extern "C"`` to avoid C++
        name mangling.
      - Plugins need to be built with the same version of the C++ compiler that
        is used to build Open GEE to ensure compatibility.
      - Plugins should be built as shared libraries (.so)
      - Once built, plugins can be placed in ``/opt/google/plugin/fileaccessor/``
        prior to running GEE Server. If a plugin fails to load, see the error
        log for more information.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
      
