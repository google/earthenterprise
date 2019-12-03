|Google logo|

========================================
Configure GEE Server 5.1.0 for SSL/HTTPS
========================================

.. container::

   .. container:: content

      Data transmission between Google Earth EC and GEE Server occurs on
      unencrypted HTTP by default. However, you may have strict
      requirements that secure HTTP (HTTPS) be used for all data
      communications. This article provides the steps to configure a GEE
      Server release 5.1.0 for use with HTTPS.

      We also include the steps required to generate a self-signed SSL
      certificate for your server but we recommend you obtain a
      third-party certificate from a CA (Certificate Authority).
      Third-party certificates generally are trusted and do not lead to
      any issues with warning messages or exceptions. However, you may
      want to set up your own self-signed certificates to get up and
      running quickly.

      -  :ref:`Requirements <Requirements>`
      -  :ref:`Generate self-signed SSL certificate and
         key <Generate_SSL_Certificate_Key>`
      -  :ref:`Apply third-party/CA-verified certificates and
         keys <Apply_Certificates_Keys>`
      -  :ref:`Setting up SSL/HTTPS <Setting_Up_SSLHTTPS>`
      -  :ref:`Set your virtual host as a SSL server <Set_Host_SSL_Server>`

      .. _Requirements:
      .. rubric:: Requirements

      -  Google Earth Enterprise Server 5.1.0
      -  A third-party or self-signed SSL certificate. Instructions for
         generating the latter are provided in the following setup
         procedure.

      .. _Generate_SSL_Certificate_Key:
      .. rubric:: Generate self-signed SSL certificate and key

      A self-signed server certificate is generated for demonstration
      purposes in the following steps. If you are using a CA-verified
      server certificates and keys, see the following section, :ref:`Apply third-party/CA-verified certificates and keys <Apply_Certificates_Keys>`.

      .. rubric:: To generate a self-signed SSL certificate and key:

      #. Change directory to the default certificate folder:

         ``cd /opt/google/gehttpd/conf``

         The default SSL certificate and key files generated in the
         following steps and used in this example virtual host are
         ``/opt/google/gehttpd/conf/server.crt`` and
         ``/opt/google/gehttpd/conf/server.key`` respectively.

         .. tip::

            Your certificate location and names may be different but
            make sure that they match the entries in the
            ``httpd-ssl.conf`` file, as shown in :ref:`Set your virtual host
            as a SSL server <Set_Host_SSL_Server>`.

      #. Generate the server key:

         ``openssl genrsa –out server.key 1024``

         .. tip::

            It is recommended that you do not use the ``–des3`` option,
            which adds password protection when a key is created. While
            this adds an extra layer of security, it also requires
            manual input of the password should your system accidentally
            power down and restart, for example. Instead, generate the
            server key without a password or strip out the password with
            ``openssl rsa -in server.key -out myservername_nopasswd.key``
            and use that instead.

      #. Generate the server certificate based on the server key:

         ``openssl req –new –x509 –days 365 –key server.key –out server.crt``

         .. tip::

            Include as much information into the certificate as desired
            or accept the defaults, that is, Country, State, City,
            Company Name, Department, Server Name, and Administrator email
            address.

      #. Test the server certificate and verify all information is
         correct:

         ``openssl x509 -noout -text -in server.crt``


      .. _Apply_Certificates_Keys:
      .. rubric:: Apply third-party/CA-verified certificates and keys

      If you are using third-party/CA-verified certificates and keys, we
      recommend renaming them to use the default names for the virtual
      host configuration:

      #. Change your third-party server certificate file name to
         ``SSLCertificateFile /opt/google/gehttpd/conf/server.crt``
      #. Change your third-party/CA verified key file name to
         ``SSLCertificateKeyFile /opt/google/gehttpd/conf/server.key``

      Optionally, if you choose not to use the default certificate and
      key names, you will need to modify the entries in
      ``/opt/google/gehttpd/conf/extra/httpd-ssl.conf`` with the custom
      names accordingly, listed under ``# Server Certificate`` and
      ``# Server Private Key`` respectively.

      .. _Setting_Up_SSLHTTPS:
      .. rubric:: Setting up SSL/HTTPS

      In this example procedure, you perform the following steps:

      -  Add a virtual host ``ssl``
      -  Set up the Apache server configuration to serve virtual hosts
         over HTTPS.
      -  Restart GEE Server

      .. note::

         The virtual host name “secure” is reserved for GEE
         Server use.

      .. rubric:: To add a virtual host for HTTPS serving:
         :name: to-add-a-virtual-host-for-https-serving

      #. Register your new virtual host using the ``geserveradmin``
         command. See :doc:`../geeServerAdmin/manageVirtualHosts`.

         ``geserveradmin –-addvh <Virtual Host Name> --ssl``

         The **--ssl** option registers the newly created virtual host
         by creating a configuration file with the naming convention:
         **\_host.location_ssl** located in the path
         ``<Apache path>/conf.d/virtual_servers/``.

         For example, to create a location-based virtual host with a
         configuration file that specifies SSL:

         .. code-block:: none

            # /opt/google/bin$ ./geserveradmin --addvh test_ssl --ssl
            Registering Virtual Host: test_ssl ...
            Virtual Host registration successful.
            Location-based Virtual Host created:

            /conf.d/virtual_servers/test_ssl_host.location_ssl

      #. The newly created virtual host configuration file in this
         example,
         ``/opt/google/gehttpd/conf.d/virtual_servers/test_ssl_host.location_ssl``,
         includes the ``<Location>`` directives for SSL, in this case,
         ``test_ssl``.

         .. code-block:: none

            <Location “/test_ssl_host/*”>
               SetHandler fdb-handler
               SSLRequireSSL
               SSLVerifyClient none
            </Location>

         .. tip::

            Use of the ``SSLRequireSSL`` directive prevents all HTTP
            requests that do not use SSL, thereby protecting your data
            from all but HTTPS requests.
            See `Apache HTTP Server Version 2.2 Documentation
            <http://httpd.apache.org/docs/2.2/mod/mod_ssl.html#sslrequiressl>`_
            for more information.

         .. tip::

            Use of the ``SSLVerifyClient`` directive specifies the level
            of certificate verification required for the client.
            See `Apache HTTP Server Version 2.2 Documentation for SSLVerifyClient
            <http://httpd.apache.org/docs/2.2/mod/mod_ssl.html#sslverifyclient>`_
            for more information.


      .. _Set_Host_SSL_Server:
      .. rubric:: Set your virtual host as a SSL server

      .. tip::

         All commands must be executed as the root user unless otherwise
         specified.


      .. rubric:: To set your virtual host as a SSL server:

      #. Edit the Apache HTTP server configuration file,
         ``/opt/google/gehttpd/conf/gehttpd.conf`` file, as follows:

         a. Uncomment and change ``ServerName www.example.com`` to
            ``ServerName MyServerName``, where ``MyServerName`` is the
            real address users would enter in the network.
         b. Check that ``Include conf/extra/httpd-ssl.conf`` appears and
            uncomment it. Note that this ``Include`` for the
            ``httpd-ssl.conf`` configuration is commented out by default
            as it should only be loaded if you serve a virtual host over
            HTTPS.
         c. Save and close the ``/opt/google/gehttpd/conf/gehttpd.conf``
            file.

      #. Edit the Apache server configuration file,
         ``/opt/google/gehttpd/conf/extra/httpd-ssl.conf`` file, which
         provides SSL support. It contains the configuration directives
         to instruct the server how to serve pages over an HTTPS
         connection. For detailed information about these directives see
         `Apache 2.2 documentation <http://httpd.apache.org/docs/2.2/mod/mod_ssl.html>`__.

         a. Ensure the ``ServerName www.example.com`` is uncommented and
            matches the name defined in the
            ``/opt/google/gehttpd/conf/gehttpd.conf`` file, that is, the
            alias or real address users would enter in the network.
         b. Check that the SSL virtual hosts configuration file location
            is already included in the ``<VirtualHost _default_:443>`` 
            list of directives:

            ``<VirtualHost_default_:443>``
               ``Include conf.d/virtual_servers/*.location_ssl``

         c. Save and close the
            ``/opt/google/gehttpd/conf/extra/httpd-ssl.conf`` file.

      #. Restart the Google Earth Enterprise Server software:

         ``/etc/init.d/geserver restart``

      #. Publish a database to the SSL/HTTPS virtual host.
      #. Test the connections with Google Earth Enterprise Client for
         HTTP and HTTPS-based virtual servers.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
