|Google logo|

========================================================
Configure GEE Server Admin pages for LDAP authentication
========================================================

.. container::

   .. container:: content

      You can use LDAP authentication, which is based on Microsoft
      Active Directory, to authenticate users to GEE Server
      admin pages. GEE server software is based on Apache ``httpd``.

      It is easy to administer and manage user accounts. You can use a
      simple ``.htpasswd`` to secure GEE Server admin pages, then you can
      assign the permissions levels via groups or user level. If you
      have an existing Active Directory user catalog, you can use that
      instead of implementing a redundant authentication system. For
      information about enabling your Active Directory for LDAP, see the
      `OpenLDAP <http://www.openldap.org>`_ documentation.

      .. tip::

         This article explains how to configure GEE Server admin pages for LDAP
         authentication.

      .. rubric:: To configure GEE Server admin pages for LDAP authentication:
         :name: to-configure-a-gee-server-admin-for-ldap-authentication

      #. Make sure that the LDAP modules are loaded on your Apache
         server. For most GEE installations, these are loaded
         automatically.

      #. Load the LDAP modules in your
         ``/opt/google/gehttpd/conf/gehttpd.conf``

         After the modules are loaded, you can control access by
         querying the directory for particular attributes.

         .. code-block:: none

           LoadModule authnz_ldap_module modules/mod_authnz_ldap.so
           LoadModule ldap_module modules/mod_ldap.so

      #. Point Apache to the LDAP server ``AuthLDAPURL`` key directive.

         An example of an ``AuthLDAPURL`` directive is:

         ``AuthLDAPURL ldap://ldapserver.example.com:389/ou=Users,dc=example,dc=com?uid``

         The key directive format declares the LDAP server, the
         distinguished name (DN), and the attribute to use in the search
         (typically the ``Uid`` attribute in the ``Users``
         organizational unit). You can also customize these filters for
         additional security constraints.

      #. Create a user account in Active Directory ``geserver``.
         This can be any user account that has limited access. The user
         account only needs access to the global catalog. Make sure that
         your user has the full organizational unit name.

      #. Restart the GEE server to load the modules into the system:

         ``# /etc/init.d/geserver restart``

      #. Add the directives to your config file ``/opt/google/gehttp/conf/gehttpd.conf``
         for ``/opt/google/gehttpd/htdocs/admin`` Directory.

         Comment/remove these lines :

         .. code-block:: none

            AuthUserFile /opt/google/gehttpd/conf.d/.htpasswd
            AuthGroupFile /dev/null

         And Add these lines :

         .. code-block:: none

            AuthBasicProvider ldap
            AuthLDAPBindDN "someuser@example.com"
            AuthLDAPBindPassword "somepassword"
            AuthLDAPURL "ldap://ldapserver.example.com:389/ou=Users,dc=example,dc=com?uid"


      #. For testing purposes, add the lines below to the
         ``gehhtpd.conf`` file:

         .. code-block:: none

            <Directory "/opt/google/gehttpd/htdocs/admin">
                #AuthUserFile /opt/google/gehttpd/conf.d/.htpasswd
                #AuthGroupFile /dev/null
                AuthName "Earth Server admin pages"
                AuthType Basic

                AuthBasicProvider ldap
                AuthLDAPBindDN "someuser@example.com"
                AuthLDAPBindPassword "somepassword"
                AuthLDAPURL "ldap://ldapserver.example.com:389/ou=Users,dc=example,dc=com?uid"

                <Limit GET>
                require valid-user
                </Limit>
            </Directory>

      #. Restart your server and try to access the earth server admin page.
         Authenticate with your LDAP credentials. If you cannot access Apache, open the
         file error log at ``/opt/google/gehttpd/logs/error_log and access_log``.

      .. rubric:: Directives

      -  ``require valid-user``. Allows all users who log in with
         correct passwords.
      -  ``AuthLDAPBindDN``. The distinguished name (DN) of the user
         account that Apache uses to connect to the directory system and
         authenticate the user.
      -  ``AuthLDAPBindPassword``. The password for the user account
         configured with the ``AuthLDAPBindDN`` directive.
      -  ``AuthLDAPURL``. The URL that tells where the directory server
         is, where to look for users, which user attribute is used to
         identify a user, and other miscellaneous things that are
         specific to the LDAP query syntax.
      -  ``AuthBasicProvider``. Tells Apache which authentication module
         to use for Basic Authentication.

      For more information, see the Apache
      `mod_ldap <https://httpd.apache.org/docs/2.4/mod/mod_ldap.html>`_
      `mod_authnz_ldap <https://httpd.apache.org/docs/2.4/mod/mod_authnz_ldap.html>`_
      and
      `mod_auth_basic <https://httpd.apache.org/docs/2.4/mod/mod_auth_basic.html>`_
      documentation.

      .. tip::

         To troubleshoot any connection issues, begin by checking the
         logs on the LDAP server to make sure GEE server is correctly
         making authentication requests.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
