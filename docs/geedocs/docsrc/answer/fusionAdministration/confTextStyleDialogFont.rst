|Google logo|

=========================================
Configure fonts for the Text Style dialog
=========================================

.. container::

   .. container:: content

      When you are specifying a map layer in Google Earth Enterprise
      Fusion, the Text Style dialog allows users to select the text
      style for vector labels. (See :doc:`4412684`.) Google Earth
      Enterprise Fusion supplies only one font and style, *Sans
      regular*. However, if you want to provide a choice of fonts for
      your users to select in the Text Style dialog, you can create a
      font list file that contains information about the fonts you want
      to support. This section explains how to create a font list file.

      .. rubric:: To create a font list file:
         :name: to-create-a-font-list-file

      #. Create an ASCII text file and save it as
         ``/opt/google/share/fonts/fontlist``.
      #. For each font you want to support, enter one line in the file
         that contains the four following pieces of information:

         -  **Font name** - the name of the font, which appears on the
            drop-down list of fonts available in the Text Style dialog
            (for example, ``TimesRoman``). No spaces are allowed in the
            font face name. Multiple variations of the font, such as
            regular, bold, italic, are automatically grouped under the
            same name in the drop-down list.
         -  **File Path** - the full path to the TrueType (``.ttf``)
            font file. No spaces are allowed in this path.
         -  **Bold** - 1 if the font is bold; 0 if not.
         -  **Italic** - 1 if the font is italic (or oblique); 0 if not.

      #. Save the file.

      For example, your font list file might contain:

      ::

         LucidaBrightDemi /usr/local/lib/fonts/LucidaBrightDemiBold.ttf 1 0
         LucidaBrightDemi /usr/local/lib/fonts/LucidaBrightDemiItalic.ttf 0 1
         LucidaBright /usr/local/lib/fonts/LucidaBrightItalic.ttf 0 1
         LucidaBright /usr/local/lib/fonts/LucidaBrightRegular.ttf 0 0
         LucidaSansDemi /usr/local/lib/fonts/LucidaSansDemiBold.ttf 1 0
         LucidaSans /usr/local/lib/fonts/LucidaSansRegular.ttf 0 0
         LucidaTypewriter /usr/local/lib/fonts/LucidaTypewriterBold.ttf 1 0
         LucidaTypewriter /usr/local/lib/fonts/LucidaTypewriterRegular.ttf 0 0
         LucidaTypewriter /usr/local/lib/oblique-fonts/LucidaTypewriterBoldOblique.ttf 1 1
         LucidaSansDemi /usr/local/lib/oblique-fonts/LucidaSansDemiOblique.ttf 0 1
         LucidaSans /usr/local/lib/oblique-fonts/LucidaSansOblique.ttf 0 1
         LucidaTypewriter /usr/local/lib/oblique-fonts/LucidaTypewriterOblique.ttf 0 1


.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
