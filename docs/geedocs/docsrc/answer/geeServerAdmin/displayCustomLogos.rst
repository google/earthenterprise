|Google logo|

====================
Display custom logos
====================

.. container::

   .. container:: content

      Google Earth Enterprise Client (EC) 5.0 and later can display
      multiple custom logos with your globes and maps when connected to
      a GEE Server.

      Custom logos are added as database preferences or *snippets*,
      which are managed using :doc:`snippet profiles <../geeServerAdmin/manageSnippetProfiles>` you create
      using GEE Server. Once you have saved a custom logo in a snippet
      profile, you can assign it to any database you publish. You can
      apply a snippet profile to as many databases as you need.

      .. rubric:: Cobrand_info snippet

      You can use the **cobrand_info** snippet within a snippet profile
      to place your logos anywhere in the Google Earth EC window.

      The benefits of using **cobrand_info** are:

      -  You can access logos at both local and remote addresses (UNC,
         HTTP, and HTTPS). However, we recommend that you host your logos
         on GEE Server to ensure accessiblity to the server and Google
         Earth EC.
      -  PNG, GIF, and JPEG image formats are supported.
      -  Transparent logos are supported.
      -  You can place logos anywhere on the screen.
      -  You can display multiple logos at the same time.
      -  You can use either absolute pixel values or relative screen
         size values to place logos.
      -  Logos can either be a fixed size or scaled to the width of the
         screen.
      -  Logos are included with saved images and printouts from EC.

      .. rubric:: Cobrand_info snippet definition

      You define the **cobrand_info** snippet by specifying the URL from
      which the logo will be served, the screen width size, the tie
      point (which controls the reference point used when overlaying the
      logo), and x and y coordinates, expressed as either absolute values
      in pixels or relative values of percentage of screen width or
      height.

      The following **cobrand_info** example snippet places the
      *top_right.jpg* logo in the top-right corner of the window (the
      top right of the image is 5 pixels from the right edge of the
      window edge, using an absolute x coordinate value, and 95% from
      the bottom using a relative y coordinate value), and sets it to
      dynamically scale to 5% of the screen width as the Google Earth EC
      window is resized.

      |GEE Server cobrand_info snippet example|

      .. rubric:: Cobrand_info parameters

      The following table describes the cobrand_info snippet parameters,
      all of which are required.

      .. list-table:: Cobrand_info parameters
         :widths: 25 40
         :header-rows: 1

         * - Name
           - Description
         * - ``logoUrl``
           - The location of the logo file, e.g.,``http://yourserver.org/top-left-logo.gif``
             Logos can be accessed locally or remotely via UNC, HTTP, or HTTPS. Supported logo formats are PNG, GIF, and JPEG.
         * - ``screen_size``
           - Specifies a fraction of window width the logo should be scaled to.
             Value must be between 0.0 and 1.0, where 0.0 disables the scaling feature.
         * - ``tiePoint``
           - Specifies the part of the logo file that is placed at the specified ``x_coord.value`` and ``y_coord.value`` values.
             The following screen positions are allowed:

               - BOTTOM_LEFT
               - BOTTOM_CENTER
               - BOTTOM_RIGHT
               - TOP_LEFT
               - BOTTOM_LEFT
               - MID_LEFT
               - TOP_RIGHT
               - TOP_CENTER
               - MID_RIGHT
               - MID_CENTER

         * - ``x_coord.is_relative``
           - Used in conjunction with ``x_coord.value`` to specify absolute or relative pixel placement of the ``tiePoint``.

               - False means ``x_coord.value`` is a pixel value.
               - True means ``x_coord.value`` is a window size fraction.

         * - ``x_coord.value``
           - The EC horizontal (X axis) coordinate for ``tiePoint``. ``x_coord.value`` specifies a distance
             from the left edge of the Earth window for logo tiePoint placement.

             ``x_coord.value`` is used in conjunction with ``x_coord.is_relative``, which specifies whether
             the distance is measured in pixel or relative values.

               - If ``x_coord.is_relative`` is False (default), ``x_coord.is_relative`` is a pixel value.
                 You can use negative values to specify a distance from the right edge.
               - If ``x_coord.is_relative`` is True, allowed values for ``x_coord.is_relative`` are
                 between 0.0 (left edge of window) and 1.0 (right edge).

         * - ``y_coord.is_relative``
           -  Used in conjunction with ``y_coord.value`` to specify absolute or relative pixel placement of the ``tiePoint``.

               - False means ``y_coord.value`` is a pixel value
               - True means ``y_coord.value`` is a window size fraction.

         * - ``y_coord.value``
           - The EC vertical (Y axis) coordinate position for ``tiePoin``t. ``y_coord.value`` specifies a distance
             from the bottom edge of the Earth window for logo ``tiePoint`` placement.

             ``y_coord.value`` is used in conjunction with ``y_coord.is_relative``, which specifies whether the distance
             is measured in pixel or relative values.

               - If ``y_coord.is_relative`` is False, this is a pixel value. You can use negative values to specify a
                 distance from the top edge.
               - If ``y_coord.is_relative`` is True, allowed values are between 0.0 (bottom edge of window) and 1.0 (top edge).

      .. rubric:: Add a custom logo

      You can add any number of custom logos to your maps and globes,
      adding each one using a **cobrand_info** snippet. The ideal size
      for your logo is 64 x 64 pixels.

      .. rubric:: To add a custom logo:

      #. Upload your logo file to a web server in the network. To host
         the file on your GEE Server (for example,
         ``http://servername/filename.ext``), copy it to
         ``/opt/google/gehttpd/htdocs``.
      #. Access the Google Earth Enterprise Server Admin console in a
         browser window by going to *myserver.mydomainname*.com/admin,
         replacing *myserver* and *mydomainname* with your server and
         domain.
      #. Sign in with the default credentials or the username and
         password assigned to you:

         -  Default username: *geapacheuser*
         -  Default password: *geeadmin*

         .. note::

            If you do not know your username and password,
            contact your Google Earth Enterprise Server System
            Administrator.

      #. Click **Snippet profiles** to display your snippet profiles.
      #. Click **Create New**. The **Create new snippet profile** dialog
         appears.

         |GEE Server Create Snippet Profile dialog|

      #. Enter a name for the new snippet profile and click **Create**.
         The snippet profile name appears in red in the **Existing
         snippet profile** list and the **Snippet editor** opens.
      #. Click the **Add a new snippet set to the profile** drop-down to
         display the list of available snippets.
      #. Select the **cobrand_info** snippet from the list.

         |GEE Server Select cobrand_info Snippet|

      #. Enter a URL for the path of the custom logo. Select whether
         your x and y coordinate values are expressed as relative or
         absolute and enter the parameter values. All fields must be
         completed.

         |GEE Server cobrand_info Snippet definition|

      #. Click **Save changes** to save the snippet profile.

      .. rubric:: To apply a custom logo to a globe or map:

      #. In the GEE Server Admin console, click **Databases**. The list
         of databases on GEE server appears.
      #. Check the box next to the database to which you want to apply
         your custom logo. Click **Publish**. The Publish dialog
         appears.
      #. Select your snippet profile for your custom logo from the
         **Snippet profiles** drop-down list.

         |GEE Server Publish dialog snippet profile|

         .. tip::

            If you have already published your database, you need to
            **Unpublish** before publishing again, this time adding your
            snippet profile to apply your custom logo.

      #. Click the **Publish** button to publish your database with the added
         snippet profile.

         Now, when you view your database in Google Earth EC, your custom
         logo is displayed.

      .. tip::

         Google Earth EC recognizes only the snippet profile settings
         made by the first database that you connect to. This applies
         when you are connecting to multiple databases.

      .. rubric:: cobrand_info snippet definition examples

      .. rubric:: Single logo at top-left

      This example places one logo in the top-left corner of the
      window (the top-left of the image is 5% of the window width
      from the left side of the window, and 95% from the bottom), and
      sets it to dynamically scale to 10% of the window width as the
      window is resized.

      |cobrand_info top-left example|

      .. rubric:: Single logo at mid right

      This example places one logo at the vertical midpoint on the
      right side of the window using absolute and relative coordinate
      values. The center-right logo tiePoint is placed 5 pixels from
      the right window edge and at 50% of the relative window height.
      EC requests the file using HTTPS (as specified in **logo_url**)
      and does not scale the logo.

      |cobrand_info right-center example|

      .. rubric:: Single logo at top-center

      This example places one logo at the top-center of the window.
      The top-center tiePoint of the logo is placed, by relative
      coordinates, at the 50% window width and 98% window height, and
      the logo dynamically scales to 9% of the window width.

      |cobrand_info top-center example|

      .. rubric:: Single logo at bottom-right corner

      This example places one logo in the bottom-right corner of the
      window by combining relative and absolute coordinate values.
      The bottom-right logo tiePoint is placed at 80% of the window
      width from the left window edge, leaving a 20% margin at the
      right window edge, and 30 pixels from the bottom window edge.
      The logo is dynamically scaled to occupy 20% of the window
      width.

         |cobrand_info bottom-right example|

      .. rubric:: Single logo at top-left corner

      This example places one logo in the top-left corner of the
      window using absolute pixel values. The top-left logo tiePoint
      is placed 5 pixels from the left window edge and 5 pixels from
      the top window edge, and the logo is displayed without scaling.

         |cobrand_info upper-left example|

      .. rubric:: Learn more

      -  :doc:`../geeServerAdmin/manageSnippetProfiles`

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
.. |GEE Server cobrand_info snippet example| image:: ../../art/server/custom_logos/Snippet_cobrand-full.png
.. |GEE Server Create Snippet Profile dialog| image:: ../../art/server/snippet_profiles/snippet_profilesCreateDialog.png
.. |GEE Server Select cobrand_info Snippet| image:: ../../art/server/custom_logos/Snippet_cobrandSelect.png
.. |GEE Server cobrand_info Snippet definition| image:: ../../art/server/custom_logos/Snippet_cobrand.png
.. |GEE Server Publish dialog snippet profile| image:: ../../art/server/custom_logos/Publish_snippet.png
.. |cobrand_info top-left example| image:: ../../art/server/custom_logos/cobrand_top_left.png
.. |cobrand_info right-center example| image:: ../../art/server/custom_logos/cobrand_center_right.png
.. |cobrand_info top-center example| image:: ../../art/server/custom_logos/cobrand_top_center.png
.. |cobrand_info bottom-right example| image:: ../../art/server/custom_logos/cobrand_bottom_right.png
.. |cobrand_info upper-left example| image:: ../../art/server/custom_logos/cobrand_upper_left.png
