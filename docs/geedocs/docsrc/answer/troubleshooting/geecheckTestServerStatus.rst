|Google logo|

=====================================
Geecheck: test your GEE Server status
=====================================

.. container::

   .. container:: content

      **Geecheck** is a tool that checks the health and status of a GEE
      Server installation. With GEE 5.1.x, you can use geecheck to run
      tests and then view the results in a browser-based dashboard.
      Results are organized into *suites* of results so that you can
      quickly review the status of Fusion, GEE Server, as well as see
      the output of other tests that you write.

      .. tip::

         You can also run geecheck from the command line. In GEE 4.4 and
         earlier versions, geecheck runs from the command line only.

      -  :ref:`How geecheck works <How_geecheck_Works>`
      -  :ref:`Access test results from GEE Server Admin
         console <Access_Test_Results_GEE_Server_Admin_Console>`
      -  :ref:`Run the tests again <Run_Tests_Again>`
      -  :ref:`Create your own tests <Create_Your_Own_Tests>`
      -  :ref:`Run geecheck on the command line <Run_geecheck_Command_Line>`
      -  :ref:`Run geecheck in a browser <Run_geecheck_Browser>`
      -  :ref:`About moving tests <About_Moving_Tests>`

      .. _How_geecheck_Works:
      .. rubric:: How geecheck works

      Geecheck is a Python application, ``geecheck.py``, and is
      installed in the following location:

      ``/opt/google/gehttpd/cgi-bin         geecheck.py         geecheck_tests/             user_tests/             fusion_tests/             server_tests/         run_geecheck.py         set_geecheck_config.py``

      -  ``geecheck.py`` runs and displays the results from any tests
         located in the subfolders:

         ``geecheck_tests/             user_tests/             fusion_tests/             server_tests/``

      -  Tests must follow the naming convention ``*_test.py``, for
         example, ``dns_test.py``. Test results are organized by the
         subfolder or suite they belong to, i.e., all tests in
         ``user_tests`` are categorized under that results heading.

      .. _Access_Test_Results_GEE_Server_Admin_Console:
      .. rubric:: Access test results from GEE Server Admin console

      .. rubric:: To access geecheck from GEE Server Admin console:

      #. Go to *myserver.mydomainname*.com/admin, replacing *myserver*
         and *mydomainname* with your server and domain.
      #. Sign in with the default credentials:

         -  Username: *geapacheuser*
         -  Password: *geeadmin*

         .. tip::

            To reset the username and password:

            ``sudo /opt/google/gehttpd/bin/htpasswd -c``

            ``/opt/google/gehttpd/conf.d/.htpasswd geapacheuser``

      #. Click the |gear icon| **Settings** icon and select
         **Diagnostics** from the menu to display the Geecheck page.
      #. View the **Test Summary** and **Test Details** sections to
         verify the results of the tests that ``geecheck`` ran.

      .. _Run_Tests_Again:
      .. rubric:: Run the tests again

      If you find you need to perform some troubleshooting based on the
      results of any of your tests and you need to run the tests again
      to confirm whether you have fixed the issue, simply restart GEE
      Server, then open the Geecheck page again.

      -  On the command line:

         ``$ sudo /etc/init.d/geserver restart``

      -  :ref:`Access test results from GEE Server Admin
         console <Access_Test_Results_GEE_Server_Admin_Console>`.

      .. _Create_Your_Own_Tests:
      .. rubric:: Create your own tests

      GEE 5.1 includes some basic tests that you can run but ``geecheck`` is
      a framework that lets you easily plug in your own tests and view
      the results in a browser or on the command line. Tests should be
      Python unit tests and must be copied to one of the three test
      subfolders to be run from geecheck:

      ``geecheck_tests/       user_tests/       fusion_tests/       server_tests/``

      To get some idea of how you can write your own test, you can study
      the structure of the sample test, ``sample_test.py``, provided in
      the ``user_tests`` subfolder. Use the code as a guide or simply
      copy and edit it if you are unfamiliar with Python unit tests.

      .. _Run_geecheck_Command_Line:
      .. rubric:: Run geecheck on the command line

      You can run ``geecheck.py`` from the command line with options to
      exclude tests and specify the output. The default settings
      includes all tests and output text.

      .. rubric:: Exclude tests:

      -  ``--no_user_tests``
      -  ``--no_fusion_tests``
      -  ``--no_server_tests``

      .. rubric:: Specify format:

      -  ``json``
      -  ``text``

      For example, to output results in JSON format, excluding
      ``/user_tests``:

      ``$ /opt/google/gehttpd/cgi-bin  python geecheck.py --no_user_tests json``

      .. _Run_geecheck_Browser:
      .. rubric:: Run geecheck in a browser

      To provide browser-based output, geecheck uses a wrapper:
      ``/opt/google/gehttpd/cgi-bin/run_geecheck.py``, which runs the
      ``geecheck.py`` script and makes the output available to browsers
      here: ``http://MY_SERVER/cgi-bin/run_geecheck.py``.

      The JSON response is used with the following page to make those
      results readable: ``http://MY_SERVER/admin/geecheck.html``, which
      is the page that displays when you click the |gear icon| **Settings** menu icon and select **Diagnostics** in
      the GEE Server Admin console.

      .. _About_Moving_Tests:
      .. rubric:: About moving tests

      You may want to run geecheck from a different location, e.g.,
      if you have already created a large number of tests in a
      different directory. Geecheck can be run successfully from a
      different location with a few simple rules:

      #. You can move ``geecheck_tests`` but you need to maintain the
         subfolder structure for geecheck to run:

         ``geecheck_tests/           user_tests/           fusion_tests/           server_tests/``

      #. ``geecheck.py`` and ``set_geecheck_config.py`` must be moved
         with ``geecheck_tests/``:

         ``geecheck.py``

         ``set_geecheck_config.py``

         ``geecheck_tests/``

      ``run_geecheck.py`` looks for test scripts to run. It should not
      be moved. Once test scripts have been moved,
      ``set_geecheck_config.py`` should be run:
      ``$ ./set_geecheck_config.py``

      This action updates the configuration found at
      ``/opt/google/geecheck/conf``, which tells ``run_geecheck.py``
      where to look for tests.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
.. |gear icon| image:: ../../art/server/admin/accounts_icon_gear_padded.gif
