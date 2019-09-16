|Google logo|

=======================
Manage snippet profiles
=======================

.. container::

   .. container:: content

      You can customize your database view and available features in
      Google Earth Enterprise Client (EC) by setting up *snippet
      profiles* in the the GEE Server admin console.

      *Snippets* are different properties and options that may be
      specified for a Fusion 3D database. When you connect to a database
      in Google Earth EC, snippets control its appearance and behavior.
      GEE Server lets you set your snippet preferences, then combine
      snippets into a *snippet profile*, which can then be applied to
      any database that you publish. The snippet profile settings then
      modify the database's behavior and appearance when you connect to
      it in Google Earth EC.

      For example, many of these preferences (or *snippets*) apply to
      display characteristics, such as showing or hiding different
      elements such as the Google logo or your own co-brand logo. You
      can also apply other settings, such as caching data on disk,
      enabling authentication, hiding user data in the **About** dialog,
      and specifying a reverse geocoder server URL.

      .. tip::

         Google Earth EC recognizes only the settings made by the first
         database that you connect to in Google Earth EC. This applies
         when you are connecting to multiple databases.

      **Caution:** If you are working with multiple GEE Server users on
      multiple workstations, it is important to remember that snippet
      profiles can be accessed by all users at the same time. Be sure to
      coordinate with any other users to avoid overwriting snippet profile
      settings.

      -  :ref:`Creating a snippet profile <Create_Snippet_Profile>`
      -  :ref:`Editing snippet settings <Edit_Snippet_Settings>`
      -  :ref:`Table of snippet settings <Table_Snippet_Settings>`
      -  :ref:`Modifying snippet profiles <Modify_Snippet_Profile>`
      -  :ref:`Deleting snippet profiles <Delete_Snippet_Profile>`

      .. _Create_Snippet_Profile:
      .. rubric:: To create a snippet profile:

      #. Access the Google Earth Enterprise Server Admin console in a
         browser window by going to *myserver.mydomainname*.com/admin,
         replacing *myserver* and *mydomainname* with your server and
         domain.
      #. Sign in with the default credentials or the username and
         password assigned to you:

         -  Default username: *geapacheuser*
         -  Default password: *geeadmin*

         **Note:** If you do not know your username and password,
         contact your Google Earth Enterprise Server System
         Administrator.

      #. Click **Snippet profiles** to display your snippet profiles.
      #. Click **Create New**. The **Create new snippet profile** dialog
         appears.

         |GEE Server Create Snippet Profile dialog|

      #. Enter a name for the new snippet profile and click the **Create** button.
         The snippet profile name appears in red in the **Existing
         snippet profile** list and the **Snippet editor** opens.

      .. rubric:: To edit snippet settings:
         :name: to-edit-snippet-settings

      #. Click the **Add a new snippet set to the profile** drop-down to
         display the list of available snippets.

         |GEE Server snippet profile list|

      #. Select a snippet from the list. The selected snippet options
         are added to the current snippet profile and appear in the
         profile list.

         |GEE Server snippet examples|

         You can select some snippets multiple times per profile, For
         example, you may want to add more than one customized logo to
         be displayed in Google Earth EC. For those snippets that cannot
         be added to a profile more than once, such as disable
         authentication, the snippet name is grayed out and a
         checkmark appears next to it in the snippet list.

         |GEE Server snippet profile list-greyed|

      #. Edit the snippet profile to specify values for the snippets.
         See the following :ref:`Snippet Settings <Edit_Snippet_Settings>` table for
         snippet settings.

         .. tip::

            When you add a snippet, you must enter values in the empty
            fields; if you want the snippet string to be empty, which is
            the default setting, you should not add the snippet and edit
            its values.

      #. When you have finished adding snippets and specifying values
         for your snippet profile, click the **Save changes** button.

         Now you can apply your snippet profile to any 3D database you
         :doc:`publish <../geeServerAdmin/publishDatabasesPortables>`.

      .. _Edit_Snippet_Settings:
      .. rubric:: Snippet Settings

      The following table lists the syntax for all of the available
      dbRoot snippets.

      .. _Table_Snippet_Settings:
      .. container::

         +-----------------+-----------------+-----------------+-----------------+
         | Snippet Name    | Purpose         | Syntax          | Notes           |
         +=================+=================+=================+=================+
         | ``bbs_server_in | Specify BBS     | ``base_url``—UR |                 |
         | fo``            | Server info.    | L               |                 |
         |                 |                 | of the server,  |                 |
         |                 |                 | including       |                 |
         |                 |                 | protocol,       |                 |
         |                 |                 | domain name,    |                 |
         |                 |                 | and port.       |                 |
         |                 |                 | ``file_submit_p |                 |
         |                 |                 | ath``—Path      |                 |
         |                 |                 | on server where |                 |
         |                 |                 | files can be    |                 |
         |                 |                 | submitted.      |                 |
         |                 |                 | ``name``—Name   |                 |
         |                 |                 | that will be    |                 |
         |                 |                 | displayed in    |                 |
         |                 |                 | context menu to |                 |
         |                 |                 | user. Must be   |                 |
         |                 |                 | translated.     |                 |
         |                 |                 | ``post_wizard_p |                 |
         |                 |                 | ath``—Path      |                 |
         |                 |                 | on server where |                 |
         |                 |                 | wizard can be   |                 |
         |                 |                 | found           |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``client_option | Disable disk    | ``disable_disk_ | Default value   |
         | s``             | caching in      | cache``         | is False        |
         |                 | Google Earth    |                 |                 |
         |                 | EC.             |                 |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``cobrand_info` | Add custom logo | ``logo_url``—UR | ``logo_url``    |
         | `               | to Google Earth | L               | can be remote   |
         |                 | EC display      | of image to use | or local.       |
         |                 | window.         | as logo         | ``screen_size`` |
         |                 |                 | ``screen_size`` | makes logo      |
         |                 |                 | —Positive       | scalable with   |
         |                 |                 | value <=1       | screen by       |
         |                 |                 | specifies scale | forcing its     |
         |                 |                 | with screen.    | width to occupy |
         |                 |                 | ``tie_point``—C | a fraction of   |
         |                 |                 | ontrols         | the screen. For |
         |                 |                 | the reference   | example, a      |
         |                 |                 | point in the    | value of 0.25   |
         |                 |                 | overlay         | sets the given  |
         |                 |                 | ``x_coord.is_re | logo to occupy  |
         |                 |                 | lative``—If     | 25% of the      |
         |                 |                 | True, the       | screen.         |
         |                 |                 | coordinate is   |                 |
         |                 |                 | relative to the |                 |
         |                 |                 | screen          |                 |
         |                 |                 | ``x_coord.value |                 |
         |                 |                 | ``—Coordinate   |                 |
         |                 |                 | value           |                 |
         |                 |                 | Interpretation  |                 |
         |                 |                 | depends on      |                 |
         |                 |                 | value set in    |                 |
         |                 |                 | ``x_coord.is_re |                 |
         |                 |                 | lative``.       |                 |
         |                 |                 | ``y_coord.is_re |                 |
         |                 |                 | lative``—If     |                 |
         |                 |                 | True, the       |                 |
         |                 |                 | coordinate is   |                 |
         |                 |                 | relative to the |                 |
         |                 |                 | screen          |                 |
         |                 |                 | ``y_coord.value |                 |
         |                 |                 | ``—Coordinate   |                 |
         |                 |                 | value           |                 |
         |                 |                 | Interpretation  |                 |
         |                 |                 | depends on      |                 |
         |                 |                 | value set in    |                 |
         |                 |                 | ``y_coord.is_re |                 |
         |                 |                 | lative``.       |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``default_web_p | Default         | ``https://www.g | Can be set to   |
         | age_intl_url``  | location of web | oogle.com/?hl=% | an internal IP  |
         |                 | page in Google  | 251``           | or host name    |
         |                 | Earth EC.       |                 | address.        |
         |                 |                 |                 | Default web     |
         |                 |                 |                 | page value in   |
         |                 |                 |                 | GEE is an empty |
         |                 |                 |                 | string.         |
         +-----------------+-----------------+-----------------+-----------------+
         | ``disable_authe | Disable session | ``boolean``     | Indicates that  |
         | ntication``     | cookie-based    |                 | this database   |
         |                 | authentication. |                 | does not        |
         |                 |                 |                 | require session |
         |                 |                 |                 | cookie-based    |
         |                 |                 |                 | authentication. |
         +-----------------+-----------------+-----------------+-----------------+
         | ``earth_intl_ur | Location of     | ``http://earth. |                 |
         | l``             | international   | google.com``    |                 |
         |                 | page for Google |                 |                 |
         |                 | Earth.          |                 |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``elevation_ser | Terrain         | ````            | If field is     |
         | vice _base_url` | elevation       |                 | empty, service  |
         | `               | service URL.    |                 | is unavailable. |
         +-----------------+-----------------+-----------------+-----------------+
         | ``hide_user_dat | ``True`` =      | ``boolean``     | Default is      |
         | a``             | Suppress user   |                 | False.          |
         |                 | name in the     |                 |                 |
         |                 | Help -> About   |                 |                 |
         |                 | window.         |                 |                 |
         |                 | ``False`` =     |                 |                 |
         |                 | Display user    |                 |                 |
         |                 | name.           |                 |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``keyboard_shor | URL for         | ````            | Can be set to   |
         | tcuts_url``     | keyboard        |                 | an internal IP  |
         |                 | shortcuts page. |                 | or host name    |
         |                 | If not          |                 | address.        |
         |                 | specified, this |                 |                 |
         |                 | URL is built    |                 |                 |
         |                 | from            |                 |                 |
         |                 | user_guide_intl |                 |                 |
         |                 | _url            |                 |                 |
         |                 | as              |                 |                 |
         |                 | user_guide_intl |                 |                 |
         |                 | _url"ug_keyboar |                 |                 |
         |                 | d.html"         |                 |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``model``       |                 | ``compressed_ne |                 |
         |                 |                 | gative_altitude |                 |
         |                 |                 | _threshold``—Th |                 |
         |                 |                 | reshold         |                 |
         |                 |                 | below which     |                 |
         |                 |                 | negative        |                 |
         |                 |                 | altitudes are   |                 |
         |                 |                 | compressed      |                 |
         |                 |                 | ``elevation_bia |                 |
         |                 |                 | s``—Elevation   |                 |
         |                 |                 | bias            |                 |
         |                 |                 | ``flattening``— |                 |
         |                 |                 | Planet          |                 |
         |                 |                 | flattening.     |                 |
         |                 |                 | Default value   |                 |
         |                 |                 | is              |                 |
         |                 |                 | 1.0/298.2572235 |                 |
         |                 |                 | 63              |                 |
         |                 |                 | (from WGS84)    |                 |
         |                 |                 | ``negative_alti |                 |
         |                 |                 | tude_exponent_b |                 |
         |                 |                 | ias``—Bias      |                 |
         |                 |                 | for negative    |                 |
         |                 |                 | altitude so     |                 |
         |                 |                 | that ocean      |                 |
         |                 |                 | tiles can be    |                 |
         |                 |                 | streamed to     |                 |
         |                 |                 | older clients   |                 |
         |                 |                 | ``radius``—Mean |                 |
         |                 |                 | planet radius.  |                 |
         |                 |                 | Default value   |                 |
         |                 |                 | is the WGS84    |                 |
         |                 |                 | model for earth |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``privacy_polic | URL for privacy | ``IP address or | Can be set to   |
         | y_url``         | policy.         |  host name``    | an internal IP  |
         |                 |                 |                 | or host name    |
         |                 |                 |                 | address.        |
         +-----------------+-----------------+-----------------+-----------------+
         | ``release_notes | URL for release | ``IP address or | Can be set to   |
         | _url``          | notes.          |  host name``    | an internal IP  |
         |                 |                 |                 | or host name    |
         |                 |                 |                 | address.        |
         +-----------------+-----------------+-----------------+-----------------+
         | ``reverse_geoco | Reverse         | ``numeric value | Default is 3    |
         | der _protocol_v | geocoder        | ``              | which is the    |
         | ersion``        | protocol        |                 | protocol        |
         |                 | version.        |                 | supported by    |
         |                 |                 |                 | newer clients.  |
         +-----------------+-----------------+-----------------+-----------------+
         | ``reverse_geoco | Reverse         | ````            |                 |
         | der_url``       | geocoder server |                 |                 |
         |                 | URL.            |                 |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``show_signin_b | If True, shows  | ``boolean``     |                 |
         | utton``         | the signin      |                 |                 |
         |                 | button in the   |                 |                 |
         |                 | top-right       |                 |                 |
         |                 | corner of the   |                 |                 |
         |                 | display window. |                 |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``startup_tips_ | Localize        | ````            |                 |
         | intl_url``      | international   |                 |                 |
         |                 | URL from which  |                 |                 |
         |                 | to load startup |                 |                 |
         |                 | tips for Earth  |                 |                 |
         |                 | 7.0 or higher.  |                 |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``support_answe | Localize        | ``https://suppo |                 |
         | r_intl_url``    | international   | rt.google.com/e |                 |
         |                 | URL for support | arth/#topic=436 |                 |
         |                 | answers.        | 3013``          |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``support_cente | Localize        | ``http://suppor |                 |
         | r_intl_url``    | international   | t.google.com/ea |                 |
         |                 | URL for the     | rth/``          |                 |
         |                 | support center. |                 |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``support_reque | Localize        | ``https://suppo |                 |
         | st_intl_url``   | international   | rt.google.com/e |                 |
         |                 | URL for support | arth/#topic=236 |                 |
         |                 | requests.       | 4258``          |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``support_topic | Localize        | ``http://www.go |                 |
         | _intl_url``     | international   | ogle.com/earth/ |                 |
         |                 | URL for support | learn/``        |                 |
         |                 | topics.         |                 |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``swoop_paramet | Controls how    | ``start_dist_in |                 |
         | ers``           | far from a      | _meters``       |                 |
         |                 | target swooping |                 |                 |
         |                 | should start.   |                 |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``tutorial_url` | URL for         | ``http://www.go |                 |
         | `               | tutorial page.  | ogle.com/earth/ |                 |
         |                 | If URL is not   | learn/``        |                 |
         |                 | specified, this |                 |                 |
         |                 | URL is built    |                 |                 |
         |                 | from            |                 |                 |
         |                 | ``user_guide_in |                 |                 |
         |                 | tl_url``        |                 |                 |
         |                 | as              |                 |                 |
         |                 | ``user_guide_in |                 |                 |
         |                 | tl_url + "tutor |                 |                 |
         |                 | ials/index.html |                 |                 |
         |                 | "``.            |                 |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``use_ge_logo`` | Shows/hides     | ``boolean``     | Default is      |
         |                 | Google Earth    |                 | True.           |
         |                 | logo in lower   |                 |                 |
         |                 | right corner of |                 |                 |
         |                 | display.        |                 |                 |
         +-----------------+-----------------+-----------------+-----------------+
         | ``user_guide_in | Localize        | ``http://www.go | Defaults to     |
         | tl_url``        | international   | ogle.com/earth/ | local PDF file  |
         |                 | URL for         | learn/``        | for Google      |
         |                 | documentation.  |                 | Earth EC. Can   |
         |                 |                 |                 | be set to an    |
         |                 |                 |                 | internal IP or  |
         |                 |                 |                 | hostname        |
         |                 |                 |                 | address.        |
         +-----------------+-----------------+-----------------+-----------------+
         | ``valid_databas | Validates the   | ``database_name |                 |
         | e``             | database name   | ``—Human-readab |                 |
         |                 | and URL.        | le              |                 |
         |                 |                 | name of         |                 |
         |                 |                 | database, for   |                 |
         |                 |                 | example,        |                 |
         |                 |                 | "Primary        |                 |
         |                 |                 | Database" or    |                 |
         |                 |                 | "Digital Globe  |                 |
         |                 |                 | Database"       |                 |
         |                 |                 | ``database_url` |                 |
         |                 |                 | `—URL           |                 |
         |                 |                 | of server. This |                 |
         |                 |                 | can include a   |                 |
         |                 |                 | path and a      |                 |
         |                 |                 | query, and must |                 |
         |                 |                 | be a            |                 |
         |                 |                 | well-formed,    |                 |
         |                 |                 | absolute URL    |                 |
         +-----------------+-----------------+-----------------+-----------------+

      .. _Modify_Snippet_Profile:
      .. rubric:: To modify a snippet profile:

      #. To change the snippets in a snippet profile, click the snippet
         profile name in the **Existing snippet profiles** list that you
         want to edit.

         The **Snippet editor** appears with the name of your selected
         snippet profile and the list of included snippets.

         All previously selected snippets can be edited with new
         options and settings.

      #. To add a snippet, click the drop-down list to select a new
         snippet.
      #. To delete a snippet from your profile, click **delete** in the
         snippet settings.

         |GEE Server Snippet editor: delete snippet|

      #. Click **Save Changes**.

      .. _Delete_Snippet_Profile:
      .. rubric:: To delete a snippet profile:

      #. Hover your cursor over the name of the snippet profile that you
         want to delete. The **delete** option appears.

         |GEE Server Snippet profiles: delete snippet profile|

         A message prompts you to confirm that you want to delete the
         selected snippet profile.

      #. Click **Yes**.

         The snippet profile disappears from the **Existing snippet
         profiles** list.

      .. rubric:: Learn more
         :name: learn-more
         :doc:`Google Earth Enterprise Client (EC) <../googleEarthEnterpriseClient/whatisEC>`

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
.. |GEE Server Create Snippet Profile dialog| image:: ../../art/server/snippet_profiles/snippet_profilesCreateDialog.png
.. |GEE Server snippet profile list| image:: ../../art/server/snippet_profiles/snippet_profiles-list2.png
.. |GEE Server snippet examples| image:: ../../art/server/snippet_profiles/snippet_profiles-examples2.png
.. |GEE Server snippet profile list-greyed| image:: ../../art/server/snippet_profiles/snippet_profiles-list-greyed.png
.. |GEE Server Snippet editor: delete snippet| image:: ../../art/server/snippet_profiles/snippet_profiles-delete-snippet.png
.. |GEE Server Snippet profiles: delete snippet profile| image:: ../../art/server/snippet_profiles/snippet_profiles-delete.png
