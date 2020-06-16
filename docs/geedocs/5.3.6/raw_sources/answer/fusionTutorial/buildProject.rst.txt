|Google logo|

==============================
Defining and Building Projects
==============================

.. container::

   .. container:: content

      The following exercises guide you through defining and building
      imagery, terrain, and vector projects using the resources you
      created in the previous lesson.

      -  :ref:`Define an Imagery Project <Define_Imagery_Project>`

         -  :ref:`Add Resources to an Imagery Project <Add_Resources_To_Imagery_Project>`
         -  :ref:`Build an Imagery Project <Build_an_Imagery_Project>`
         -  :ref:`Preview an Imagery Project <Preview_an_Imagery_Project>`
         -  :ref:`Specify an Imagery Project as Your Base Map <Specify_an_Imagery_Project_as_BaseMap>`

      -  :ref:`Define a Terrain Project <Define_Terrain_Project>`

         -  :ref:`Add Resources to a Terrain Project <Add_Resource_To_Terrain_Project>`
         -  :ref:`Build a Terrain Project <Build_Terrain_Project>`

      -  :ref:`Define a Vector Project <Define_Vector_Project>`

         -  :ref:`Add Resources to a Vector Project <Add_Resources_To_Vector_Project>`
         -  :ref:`Configuring Layer Properties for a Vector Project <Conf_Layer_Prop_Vector_Project>`
         -  :ref:`Configure Display Rules for a Vector Project <Conf_Display_Rules_for_Vector_Project>`

            -  :ref:`Configure the Default Select All Rule <Conf_Select_All_Rule>`
            -  :ref:`Display Rules for Major Freeways <Disp_Rules_For_Major_Freeways>`

         -  :ref:`Build a Vector Project <Build_Vector_Project>`

      .. _Define_Imagery_Project:
      .. rubric:: Define an Imagery Project

      If you intend to use your own imagery data, it makes sense to
      define and build the imagery project before adding vector data.
      That way, you can use the imagery project as the base image map in
      the Preview pane in Google Earth Enterprise Fusion when you
      develop your vector project, making it easier to visualize how
      your vector data will appear over your actual imagery.

      .. _Add_Resources_To_Imagery_Project:
      .. rubric:: Add Resources to an Imagery Project

      Although you can change the display order of imagery and terrain
      resources within a project, the order is ultimately determined by
      the resolution of the source files. That is, lower-resolution
      insets are automatically ordered below higher-resolution insets.
      So in reality, you can change the order of resources with the same
      resolution only.

      The following example shows a number of imagery resources in a
      project ordered by resolution. The resolution of each resource
      appears in parentheses after the resource name.

      The order in which the imagery or terrain resource data appears in
      the Imagery and Terrain Project Editors is the same as the
      stacking order of the insets in Google Earth EC. That is,
      higher-resolution insets appear above lower-resolution insets, so
      that viewing preference is given to the higher-quality imagery.
      The stacking order of same-resolution insets follows the order you
      define in the project. The following graphic illustrates this
      concept.

      |Imagery Resources Ordered by Resolution|

      .. rubric:: To add resources to an imagery project:

      #. Select **Asset Manager** from the **Tools** menu. The Asset Manager
         appears.
      #. Click |Imagery Project Icon| on the toolbar. The Imagery
         Project Editor appears.

         |Imagery Project Editor|

      #. Click |New Resource Icon|. The Open dialog appears.
      #. Navigate to the ``ASSET_ROOT/Resources/Imagery`` folder.
      #. Select **BlueMarble**, and click **Open**. The BlueMarble
         resource appears in the Imagery Project Editor.
      #. Repeat steps 3 through 5 for each of the following resources:

         -  **SFBayAreaLanSat**
         -  **i3_15Meter**
         -  **SFHighResInset**

         The resources appear in order by resolution, with the higher
         resolution imagery at the top of the list.

         |Imagery Project Editor: list of resources|

      #. Select **File > Save**.
      #. Navigate to the ``ASSET_ROOT/Projects/Imagery``\ folder.
      #. Enter **SFBayArea** as the name of your project, and click
         **Save**. The new project appears in the Asset Manager when you
         select **ASSET_ROOT/Projects/Imagery** in the asset navigation
         tree.

      .. _Build_an_Imagery_Project:
      .. rubric:: Build an Imagery Project

      As with resources, you do not have to build projects right away.
      You can define several projects and then build them all, or you
      can wait until you include them in a database and build the entire
      data hierarchy at the same time. However, in this lesson, you
      will build the imagery project right away, so you can use it as a base
      map in the Preview pane.

      .. note::

         It could take up to 10 minutes for the project to
         build. Do this exercise when you can spend some time away from
         your computer while the project builds.

      #. In the Asset Manager, navigate to
         **ASSET_ROOT/Projects/Imagery** in the asset navigation tree.
      #. Right-click **SFBayArea** and select **Build** from the context
         menu.

         The status of the project immediately changes to **Queued** and
         then **In Progress**. (The status may change so rapidly
         that it appears to change directly to **In Progress**.)

      As with building imagery resources, you can view the progress of
      the build by double-clicking the Current Version or Current State
      column for the project. The Version Properties dialog displays the
      most recent version of that project. You can expand the version
      tree to view the status of the build in real time by clicking the
      + signs.

      When the status of the imagery project build is **Succeeded**, go
      on to the next exercise.

      .. _Preview_an_Imagery_Project:
      .. rubric:: Preview an Imagery Project

      You can preview an imagery project after you build it.

      .. note::

         Only bounding boxes appear in the Preview pane, not
         the actual imagery.

      .. rubric:: To preview an imagery project:

      #. Double-click the name of the imagery project you built in the
         last exercise, **SFBayArea**. The Imagery Project Editor for
         that project opens.
      #. Check the box next to **Preview** above the list of source
         files, and then switch to the main Google Earth Enterprise
         Fusion window.

         Bounding boxes indicate the extents of each of the individual
         imagery resources. The largest bounding box is around the
         entire Earth, which is the BlueMarble imagery. (You might have
         to zoom out a bit to see it.) The other three bounding boxes
         are in the San Francisco Bay Area. You can zoom in to see them.

         .. note::

            The name of the project does not appear in the
            Preview List pane. When you close the Imagery Project Editor
            or uncheck the box next to **Preview**, the bounding boxes
            disappear. You can reset the view by pressing **Ctrl+R**.

         |Project Preview|

      .. _Specify_an_Imagery_Project_as_BaseMap:
      .. rubric:: Specify an Imagery Project as Your Base Map

      When the imagery project build is done, you can specify it as your
      base map.

      .. rubric:: To specify an imagery project as your base map:

      #. Select **Preferences** from the **Edit** menu. The Preferences
         dialog appears.
      #. Under Background Imagery, select **Imagery Project** from the
         drop-down list.
      #. Click |Project Browser Icon| to the right of the drop-down
         list. The Open dialog appears.
      #. Navigate to ``ASSET_ROOT/Projects/Imagery``, and select
         **SFBayArea**.

         The full path within the tutorial asset root appears to the
         right of the drop-down list.

         |Imagery Project Preferences|

      #. Click the **OK** button. The Preview pane displays the specified imagery.
         The specified imagery will remain the background imagery until
         you change it to another image or return it to the default
         imagery in the Preferences dialog.

      .. _Define_Terrain_Project:
      .. rubric:: Define a Terrain Project

      The terrain project for this tutorial is very simple. It includes
      one of the resources you built in the previous lesson.

      .. _Add_Resource_To_Terrain_Project:
      .. rubric:: Add a Resource to a Terrain Project

      The following procedure provides the steps to add resources to a
      terrain project.

      .. rubric:: To add resources to a terrain project:

      #. Select **Asset Manager** from the **Tools** menu. The Asset Manager
         appears.
      #. Click |Terrain Project Icon| on the toolbar. The **Terrain
         Project** editor appears.
      #. Click |New Icon|. The Open dialog appears.
      #. Navigate to the ``ASSET_ROOT/Resources/Terrain`` folder.
      #. Select **SFTerrain**, and click **Open**. The SFTerrain
         resource appears in the Terrain Project Editor.
      #. Check the box next to **Preview**.
      #. Right-click the name of the resource, and select **Zoom to
         Layer** from the context menu.

         The Preview pane zooms in to the bounding box that indicates
         the extent of the terrain resource.

         |Terrain Project Preview|

      #. Uncheck the box next to **Preview**.
      #. Click |New Icon|. The Open dialog appears.
      #. Navigate to the ``ASSET_ROOT/Resources/Terrain`` folder.
      #. Select **WorldTopography**, and click **Open**. The
         WorldTopography resource appears in the Terrain Project Editor.

      |Terrain Project Editor|

      #. In the Terrain Project Editor, select **File > Save**.
      #. Navigate to the ``ASSET_ROOT/Projects/Terrain``\ folder.
      #. Enter **SFTerrain** as the name of your project, and click **Save**.
         The new project appears in the Asset Manager when you select
         **ASSET_ROOT/Projects/Terrain** in the asset navigation tree.

      .. _Build_Terrain_Project:
      .. rubric:: Build a Terrain Project

      As with the imagery project, in this exercise, you build the
      terrain project right away.

      #. In the Asset Manager, right-click **SF Terrain**.
      #. Select **Build** from the context menu.

         The status of the project immediately changes to **Queued** and
         then **In Progress**.

         .. note::

            It could take a while for this project to build,
            depending on the speed of your workstation.

      When the status of the terrain project build is **Succeeded**, go
      on to the next exercise.

      .. _Define_Vector_Project:
      .. rubric:: Define a Vector Project

      The following exercises cover how to define, configure, and build
      a vector project using the resources you created in the previous
      lesson.

      You spend the majority of your time in Google Earth Enterprise
      Fusion configuring display rules for vector projects, determining
      how they look in Google Earth EC. Using the data in your vector
      source files, you can designate specific data for a variety of
      display purposes, such as road labels, features lines, and icons
      at viewing altitudes that are most appropriate for each feature.
      For example:

      |Road Display Example|

      .. _Add_Resources_To_Vector_Project:
      .. rubric:: Add Resources to a Vector Project

      Before specifying the display rules for this vector project, you
      must add the resource you created in :doc:`Defining and Building
      Resources <../fusionTutorial/buildResource>`.

      .. rubric:: To add resources to a vector project:

      #. Select **Asset Manager** from the **Tools** menu. The Asset Manager
         appears.
      #. Click |Vector Project Icon| on the toolbar. The Vector Project
         Editor appears.

         |Vector Project Editor|

      #. Click |New Icon|. The Open dialog appears.
      #. Navigate to the ``ASSET_ROOT/Resources/Vector`` folder.
      #. Select **CAHighways**, and click **Open**. The CAHighways
         resource appears in the Vector Project Editor.
      #. Repeat steps 3 through 5 to add **USPopulation** to the
         project. Notice that a check box appears next to each
         resource/layer in the project.
      #. Check the box next to **CAHighways**.
      #. Right-click **CAHighways**, and select **Zoom to Layer** from
         the context menu.

         The roads in the CAHighways resource appear in the Preview
         pane.

         |Vector Project Preview1|

      #. Check the box next to **USPopulation**, switch to the Preview
         pane, and zoom out a bit.

         Since this resource contains US census data by county, the
         outlines of counties across the US appear in the Preview pane
         as well as the roads in California.

         |Vector Project Preview2|

         .. note::

            The name of the project does not appear in the
            Preview List pane. When you close the Vector Project Editor
            or uncheck the boxes next to the resources, the vector data
            disappears. You can reset the view, if desired, by pressing
            **Ctrl+R**.

      #. In the Vector Project Editor, click
         **USPopulation**, and click |Delete Icon| to remove the US
         Population resource from the project. A message prompts you to
         confirm the deletion.
      #. Click the **OK** button. The **USPopulation** resource disappears.

         .. note::

            Removing the resource from the project does not
            delete the resource. The resource remains intact and
            available for use by other projects. It is simply not part of
            this particular project any longer.

      #. Select **File > Save**.
      #. Navigate to the ``ASSET_ROOT/Projects/Vector``\ folder.
      #. Enter **CARoads** as the name of your project, and click
         **Save**. The new project appears in the Asset Manager when you
         select **ASSET_ROOT/Projects/Vector** in the asset navigation
         tree.

      Now you are ready to begin configuring the vector layer.

      .. _Conf_Layer_Prop_Vector_Project:
      .. rubric:: Configuring Layer Properties for a Vector Project

      This exercise covers how to configure layer properties for your
      vector project. Layer properties determine a number of aspects of
      how your data appears and is accessed in Google Earth EC.

      .. rubric:: To configure layer properties:

      #. In the Asset Manager, double-click **CARoads** in the
         ``/ASSET_ROOT/Projects/Vector`` folder. The Vector Project
         Editor appears and displays the resource you added in the
         previous lesson.
      #. Right-click **CAHighways**, and select **Layer Properties**
         from the context menu. The Layer Properties dialog appears.

         |Vector Layer Properties dialog|

      #. Click **Off** next to Initial State to change it to **On**.
         Changing the initial state to **On** results in the CAHighways
         layer being automatically checked (turned on) in Google Earth
         EC.
      #. Click the blank field next to **LookAt**. The Open dialog
         appears.
      #. Navigate to ``/opt/google/share/tutorials/fusion/KML``, select
         ``San Francisco View Oblique.kmz``, and click **Open**.

         When you specify a KML/KMZ file in this field, Google Earth EC
         users can fly directly to the specified camera view by
         double-clicking the layer.

         The latitude and longitude of the selected KMZ file appear in
         the LookAt field. (You can only see the beginning of the
         latitude unless you expand the default column width.)

         |Vector Layer Properties dialog with settings|

      #. Click the **OK** button. You return to the Vector Project Editor.
      #. Select **File > Save**. Google Earth Enterprise Fusion saves
         the vector project with the same name.

      .. _Conf_Display_Rules_for_Vector_Project:
      .. rubric:: Configure Display Rules for a Vector Project

      This exercise covers how to specify display rules for your vector
      project. Display rules determine how your data looks in Google
      Earth EC.

      Each resource in a project is known as a *layer*. Each layer can
      have one set of display rules. Each display rule includes feature
      and label formatting that you specify and one or more filters for
      the selected layer. The filters for each display rule determine
      which data in the layer to apply that formatting to. For example,
      in this exercise, you will work with road data that includes major and
      minor highways. You might want the major highways to appear as
      blue lines and the minor highways as yellow lines. To accomplish
      that, you create two display rules for the layer--one for major
      highways and one for minor highways. The filter(s) for each rule
      determine which data are affected by that rule’s formatting
      specifications.

      In this exercise, you create and modify a number of display rules
      for the CAHighways layer, so Google Earth EC displays the road
      information at the desired display levels with appropriate labels
      and formatting to distinguish between major freeways and other
      roads.

      A key part of knowing how to configure display rules for vector
      data requires familiarity with the data fields in the source data.
      You can find information about the CAHighways layer of your vector
      project at this website:

      `https://nationalmap.gov/small_scale/mld/roadtrl.html <https://nationalmap.gov/small_scale/mld/roadtrl.html>`_

      .. rubric:: To configure a vector layer:

      #. In the Vector Project Editor, right-click **CAHighways**, and
         select **Configure Display Rules** from the context menu.

         The Display Rules dialog appears with the Feature tab in the
         foreground and the **default select all** rule highlighted.

      When you first create a vector project, the default display
      rule--\ **default select all--**\ is the only rule listed for each
      layer. The filter for the default rule has no matching criteria,
      so it matches all data. This rule is considered the *catch-all*
      rule. It is designed to catch all of the data that does not match
      any other rules you create.

      .. tip::

         Google Earth Enterprise Fusion executes the display
         rules sequentially, based on the order in which they are listed
         on the Rules list. So you should always make the **default
         select all** rule the last one on the list.

      First, Google Earth Enterprise Fusion attempts to match the filter
      specified for the first rule to the data in the resource. Then it
      applies the formatting specified for that rule to any matching
      data.

      Next, it attempts to match the filter specified for the second
      rule to the remaining data in the resource (that is, data not
      selected for the first filter). It applies the formatting
      specified for that rule to any matching data.

      Then, it applies the formatting you specify for the **default
      select all** rule (or the default formatting, if you do not change
      it) to any data that does not match the previous rules on the
      list, if any. This ensures that all vector data for the layer is
      displayed.

      .. _Conf_Select_All_Rule:
      .. rubric:: Configure the Default Select All Rule

      In this part of the exercise, you define the **default select
      all** rule. This rule applies to all of the surface streets that
      are left after the highways and freeways are covered by other
      rules. You define the additional rules for highways and freeways
      later in this exercise, because they are based on this default
      rule.

      .. rubric:: To define the default select all rule:

      #. Specify the geometry characteristics of the lines:

         a. For **Draw Features As**, select **Lines**.
         b. For **Visibility**, set the lower end of the range to **9**, and
            the upper end of the range to **18**.
         c. For **Maximum Simplification Error**, accept the default
            setting, **0.5**.
         d. For **Maximum Resolution Level**, select **18**. This sets
            visibility level and the point where the Google Earth
            Enterprise Fusion stops building the resource.

      #. Check the box next to **Draw As Roads**, and specify the
         options related to road labels:

         a. For **Road Label Type**, accept the default setting, **Label**.
         b. Under **Draw Style**, for **Line Color**, set it to bright red.
         c. For **Line Width**, enter **2.0**.
         d. For **Elevation/Height Mode**, accept the default setting,
            **Clamp to Ground**.

            These settings display the roads as red lines when Google
            Earth EC is zoomed in fairly close.

      #. In the **Road Label** section on the right:

         a. Click the empty text field. The Label Format dialog appears.
            This option allows you to specify the text that appears on
            the label.
         b. Click the Insert Field drop-down list to display the names
            of all of the fields in your source data.
         c. Select **NAME** from the list. The string «\ **NAME**\ »
            appears in the text field.
         d. Click **OK**. The string «\ **NAME**\ » appears in the Text
            field. The Display Rule dialog shows all of your selections.
            These settings result in Google Earth EC displaying the
            value of the NAME column for each road in your source data.

            |Display Rules Start|

         e. Click the **OK** button to save your display rule.

      #. Verify that the display rule does what you intend:

         a. Ensure that nothing is listed in the Preview List pane. If
            one or more assets are listed, right-click any asset, and
            select **Remove All Layers** from the context menu; then
            click **OK** to confirm the removal.
         b. In the Vector Project Editor, check the box next to
            **CAHighways**.
         c. Right-click **CAHighways**, and select **Zoom to Layer**
            from the context menu.

            No roads appear because your display level is approximately
            7, and you set the visibility level to between **9** and
            **18** in the Display Rules dialog.

         d. Zoom in to a display level of just over **9**. Red lines
            appear for the roads in the Preview pane.

            .. note::

               Labels for vector projects do not appear in the
               Preview pane.

         e. Zoom out to a display level less than **9**. The roads
            disappear from the Preview pane.

      #. Save the vector project by selecting **Save** from the **File**
         menu in the Vector Project Editor.

         This saves the project with the same name. If you want to save
         a project you create outside this tutorial with a different
         name, you can select **Save As** from the **File** menu, and follow
         the instructions in `Add Resources to a Vector
         Project <#34579>`__.

         .. note::

            Whenever you modify display rules or filters for
            your data, it is a good idea to save the project.

      .. _Disp_Rules_For_Major_Freeways:
      .. rubric:: Display Rules for Major Freeways

      This exercise guides you through creating the display rules
      necessary to achieve the desired appearance for the major freeways
      in the San Francisco Bay Area. When you finish this exercise, you
      should have a good understanding of the use of filters in managing
      complex data.

      When setting display rules for vector data, it is critical that
      you are familiar with the source data you are working with and
      have an understanding of the fields used to classify different
      types of vector data. In the source data for this tutorial, the
      FEATURE column sorts the roads and highways into the following
      types:

      -  Principal Highway
      -  Other Through Highway
      -  Other Highway
      -  Limited Access Highway

      In this exercise, you create a second filter and use the FEATURE
      column values to distinguish the limited access highways (major
      freeways) from other types of highways and roads, and then display
      them appropriately.

      .. rubric:: To define a display rule for major freeways:

      #. In the Vector Project Editor, right-click **CAHighways**, and
         select **Configure Display Rules** from the context menu.

         The Display Rules dialog appears with the Feature tab in the
         foreground and the **default select all** rule highlighted.

      #. Click |New Icon| at the bottom-left of the dialog. The New Rule
         dialog appears.
      #. Enter **Major Freeways** in the **New Rule Name** field, and click
         **OK**. The new rule name appears on the Rules list below the
         **default select all** rule.

         |New Display Rule|

      #. Click |Up Arrow| to move the new rule up, so it appears before
         the **default select all** rule.

         Because Google Earth Enterprise Fusion applies display rules in
         the order in which they appear on this list, the MajorFreeways
         rule must appear before the *catch-all* rule (**default select
         all**), which covers all data not covered by other rules.

         |New2 Display Rules|

      #. On the Feature tab, specify the geometry characteristics for
         the Major Freeways rule:

         a. For **Draw Features As**, select **Lines**.
         b. For **Visibility**, set the low end of the range to **9** and
            the high end of the range to **18**.
         c. For **Maximum Simplification Error**, accept the default
            setting, **0.5**.

      #. Check the box next to **Draw as Roads** (if it is not already
         checked).

         a. For **Road Label Type**, select **Shield**.
         b. Under **Draw Style**, for **Line Color**, set it to royal blue.
         c. For **Line Width**, enter **3.0**.
         d. Under **Elevation/Height**, for **Mode**, accept the default,
            **Clamp to Ground**.

            These settings display the roads as thick blue lines when
            Google Earth EC is zoomed in fairly close.

      #. In the **Road Label** section, accept the current settings.
      #. In the **Road Shield** section on the right, click the button next
         to **Icon**. The Icons dialog appears.

         |Icons Dialog|

      #. Scroll down, and select **shield1**, and click **OK**.

         The Display Rule dialog shows all of your selections.

      #. Click the **Filter** tab, and specify the filter for the Major
         Freeways rule:

         a. Accept the default selection, **Match all of the
            following**, at the top of the tab.
         b. Click **More** at the bottom of the tab.

            Two drop-down lists and a text box appear on the list of
            filters.

         c. Select **FEATURE** from the left drop-down list.
         d. Select **equals** from the other drop-down list.
         e. Enter **Limited Access Highway** in the text field.

            The Filter tab shows your selections.

            |Display Rules Filter Tab|

         f. Click the **OK** button to save your changes to the display rule.

            When this data is displayed in Google Earth EC, this filter
            causes the display settings on the Feature tab to be applied
            only to the road segments in your source data with the value
            **Limited Access Highway** in their FEATURE column.

      #. Verify that both filters and rules are working correctly:

         a. In the Vector Project Editor, check the box next to
            **CAHighways**, if it is not already checked.
         b. Zoom in to a display level between **9** and **18**, if
            necessary.

            Both thin red roads and thicker blue roads appear in the
            Preview pane. The thicker blue roads are the roads defined
            in the **MajorFreeways** filter, and the thinner red roads are
            the rest of the roads in the source data, which are defined
            by the **default select all** filter.

            |Filtered Data Preview|

      #. Save the vector project by selecting **Save** from the **File**
         menu in the Vector Project Editor.

         This saves the project with the same name.

      .. _Build_Vector_Project:
      .. rubric:: Build a Vector Project

      As with imagery and terrain projects, in this exercise, you build
      the vector project as soon as you finish configuring display
      rules.

      #. In the Asset Manager, right-click **CARoads**.
      #. Select **Build** from the context menu.

         The status of the project immediately changes to **Queued** and
         then **In Progress**.

      When the status of the vector project build is **Succeeded**,
      close the Asset Manager by clicking the close box (**X**), and go
      on to the :doc:`next lesson <../fusionTutorial/buildDatabase>`.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
.. |Imagery Resources Ordered by Resolution| image:: ../../art/fusion/tutorial/imageryLevels.png
.. |Imagery Project Icon| image:: ../../art/fusion/tutorial/iconProjImagery.png
.. |Imagery Project Editor| image:: ../../art/fusion/tutorial/imageryProjEditor.png
.. |New Resource Icon| image:: ../../art/fusion/tutorial/icon_new.gif
.. |Imagery Project Editor: list of resources| image:: ../../art/fusion/tutorial/imageryProjEditor-full2.png
.. |Project Preview| image:: ../../art/fusion/tutorial/previewProject.png
.. |Project Browser Icon| image:: ../../art/fusion/tutorial/iconProjBrowser.png
.. |Imagery Project Preferences| image:: ../../art/fusion/tutorial/prefsImageryProject-full.png
.. |Terrain Project Icon| image:: ../../art/fusion/tutorial/iconProjTerrain.png
.. |New Icon| image:: ../../art/fusion/tutorial/icon_new.gif
.. |Terrain Project Preview| image:: ../../art/fusion/tutorial/terrainProjPreview.png
.. |Terrain Project Editor| image:: ../../art/fusion/tutorial/terrainProjEditor-full.png
.. |Road Display Example| image:: ../../art/fusion/tutorial/tutRoadDisplay.png
.. |Vector Project Icon| image:: ../../art/fusion/tutorial/iconProjVector.png
.. |Vector Project Editor| image:: ../../art/fusion/tutorial/vectorProjEditor.png
.. |Vector Project Preview1| image:: ../../art/fusion/tutorial/previewVectorProject1.png
.. |Vector Project Preview2| image:: ../../art/fusion/tutorial/previewVectorProject2.png
.. |Delete Icon| image:: ../../art/fusion/tutorial/iconDelete.png
.. |Vector Layer Properties dialog| image:: ../../art/fusion/tutorial/vectorLayerProp.png
.. |Vector Layer Properties dialog with settings| image:: ../../art/fusion/tutorial/vectorLayerProp-full.png
.. |Display Rules Start| image:: ../../art/fusion/tutorial/displayRuleStart.png
.. |New Display Rule| image:: ../../art/fusion/tutorial/displayRulesNew.png
.. |Up Arrow| image:: ../../art/fusion/tutorial/arrowUp.gif
.. |New2 Display Rules| image:: ../../art/fusion/tutorial/displayRulesNew2.png
.. |Icons Dialog| image:: ../../art/fusion/tutorial/iconsDialogShield.png
.. |Display Rules Filter Tab| image:: ../../art/fusion/tutorial/displayRuleFilter.png
.. |Filtered Data Preview| image:: ../../art/fusion/tutorial/previewFilteredData.png
