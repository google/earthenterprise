|Google logo|

=============================
Implement Google Earth Plugin
=============================

.. container::

   .. container:: content

      Included with Google Earth Enterprise is an example of how to
      implement the Google Earth Plugin in
      ``/opt/google/gehttpd/htdocs/earth/earth_local.html``. By default,
      the virtual servers point to ``earth_local.html`` when accessing
      globes through a web browser. Please note, however, that the
      Google Earth Plugin is deprecated and is not supported by modern
      browsers.

      The following HTML is the content of the ``earth_local.html``
      file.

      .. code-block:: html

         <html>
         <head>
               <title>Google Earth Enterprise Earth Plug-in: Local Example</title>
               <link rel='stylesheet' type='text/css' href='/earth/earth.css' />

               <script type='text/javascript'>
                     // To serve your static content from a different server, simply override
                     // GEE_SERVER_URL to the URL of your GEE Server, e.g.:
                     // "http://yourhost.com/default_ge/" var GEE_SERVER_URL = "";
               </script>

               <!-- Load the required Javascript files. The loader for the Earth Plug-in API:
         earth_plugin_loader.js. Utilities for this example UI: fusion_utils.js and search_tabs.js The main routine: geeInit() is found in  fusion_earthplugin.js which defines the example UI and behaviors. -->
               <script type='text/javascript' src='/js/earth_plugin_loader.js'></script>
               <script type='text/javascript' src='/js/fusion_utils.js'></script>
               <script type='text/javascript' src='/js/search_tabs.js'></script>
               <script type='text/javascript' src='/js/fusion_earthplugin.js'></script>
         </head>

         <body onload='geeInit()' onresize='geeResizeDivs();'>

               <div id='header'>
                     <div id='logo'> <img src="/earth/images/gee.gif" align="left" />
                     </div>
                     <div id='search_tabs'>
                     </div>
               </div>

               <div style="clear: both;"></div>

                     <table cellspacing="0" cellpadding="0" width="100%">
                           <tr valign="top">
                                 <td id='left_panel_cell'>
                                       <div id='left_panel'></div> </td>
                                 <td>
                                       <div id='map'></div>
                                 </td>
                           </tr>
                     </table>
         </body>
         </html>

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
