|Google logo|

=========================
PostgreSQL authentication
=========================

.. container::

   .. container:: content

      Client authentication determines which users are allowed to view
      published databases. By default, the GEE Server does not require
      authentication for user access. However, you can configure
      authentication for your server.

      If you use a load-balanced grid of GEE servers, a single common
      authentication source (such as PostgreSQL or LDAP) makes user
      administration easier. When you add users, you can add them to a
      single database instead of to each virtual server.

      Authentication encrypts passwords before transmitting and storing
      them. Besides the security benefits, there are performance
      benefits to encrypting passwords instead of storing them in
      plaintext. As a plaintext password file grows to more than 200
      users, there will likely be a noticeable slowdown. This is because
      the entire password file is read into memory each time the virtual
      server is accessed to search for a password.

      If you have a PostgreSQL database, you can use standard Apache
      authentication methods such as Digest authentication to control
      user access. This is useful if you already store user passwords in
      a PostgreSQL table, or if you want to use a unified authentication
      mechanism other than LDAP for your GEE Server and other systems.

      .. rubric:: Before you begin

      -  Do not use the PostgreSQL database that ships with GEE. Instead,
         install PostgreSQL on a separate port or a different host.
      -  You must set up client authentication for each virtual server
         you want to use authentication with.

      .. rubric:: Apache modules
         :name: apache-modules

      The Apache configuration below uses these modules:

      -  The ``mod_authn_dbd`` module provides the authentication
         back-end with a PostgreSQL server. For more information about
         the module, see the
         `mod_authn_dbd <http://www.google.com/url?q=http%3A%2F%2Fhttpd.apache.org%2Fdocs%2F2.2%2Fmod%2Fmod_authn_dbd.html&sa=D&sntz=1&usg=AFrqEzcLzX90MM12j49RNuWEy_X7y6wTrQ>`__
         section in the Apache documentation.
      -  The ``mod_auth_digest`` module is used for Digest
         authentication. For more information about ``mod_auth_digest``,
         see the
         `mod_auth_digest <http://www.google.com/url?q=http%3A%2F%2Fhttpd.apache.org%2Fdocs%2F2.2%2Fmod%2Fmod_auth_digest.html&sa=D&sntz=1&usg=AFrqEzfaZylqvLk4_vXVqc3jCj7EJESuhw>`__
         section in the Apache documentation.

      .. rubric:: Terms
         :name: terms

      In the steps below, you can replace these names with the names you
      want to use. For example, you can replace *geeauthdb* with the
      name of your database.

      -  ``geeauthdb``. The database to connect to.
      -  ``geepasswd``. The table that stores user information.
      -  ``geeuser``. The user login to connect to the database.
      -  ``postgres``. The default PostgreSQL adminstrator user on
         Ubuntu.

      .. rubric:: To configure client authentication for PostgreSQL:
         :name: to-configure-client-authentication-for-postgresql

      #. Connect to the PostgreSQL server:

         .. code-block:: none

            sudo su - postgres
            psql --port=5433

      #. Create the database and user:

         .. code-block:: none

            CREATE USER geeuser WITH PASSWORD 'geeuserpasswd';
            CREATE DATABASE geeauthdb;
            GRANT ALL PRIVILEGES ON DATABASE geeauthdb to mypguser;
            \q

      #. Create a table to store the passwords:

         .. code-block:: none

            psql --port=5433 --username=geeuser geauthdb
            CREATE TABLE geepasswd (username varchar(32) NOT NULL, realm varchar(255)
            NOT NULL, passwd varchar(255) NOT NULL,
               realname varchar(255), PRIMARY KEY (username, realm));
            \q

         The table includes these fields:

            -  ``username``.
            -  ``realm``.
            -  ``password``.
            -  ``realname``. *Optional*. The realname field does not require
               a value, but is included in case you want to use it. You can
               also add other optional fields if needed.

      #. Create the encrypted password by inserting encyption
         functions into the database with the ``pgcrypto`` contributed
         module:

         ``psql --port=5433 --username=geeuser geauthdb < /usr/share/postgresql/8.4/contrib/pgcrypto.sql``

         Note: You can substitute a different method for this step. The `Password Formats
         <http://www.google.com/url?q=http%3A%2F%2Fhttpd.apache.org%2Fdocs%2F2.2%2Fmisc%2Fpassword_encryptions.html&sa=D&sntz=1&usg=AFrqEzdBJJpsOLV3eL6UCAatZv_IhxEZdg>`__
         section in the Apache documentation gives examples in multiple
         programming languages for encrypting the password so that it is
         readable by Apache.

      #. Make sure the database works by inserting a test user:

         .. code-block:: none

            psql --port=5433 --username=geeuser geauthdb
            INSERT INTO geepasswd VALUES ('jsmith', 'realm', encode(digest( 'jsmith' || ':' || 'realm' || ':' ||'password', 'md5'), 'hex'), 'Jane Smith');
            \q

         Replace "jsmith", "realm", "password" and "Jane Smith" with the
         values you want to use.

         Note: The password is hashed with MD5 because the Apache
         ``mod_authn_dbd`` module requires it. This also prevents the
         password from appearing in plaintext.

      #. Create a file named ``pgsql-auth.conf`` at

         ``/opt/google/gehttpd/conf/extra/pgsql-auth.conf``.

      #. Add these lines to the ``pgsql-auth.conf`` file:

         .. code-block:: none

            ``DBDriver pgsql``
            ``DBDParams "hostaddr=yourpgserver port=yourport user=geuser password=yourpassword dbname=gee_auth"``
            ``DBDMin 4``
            ``DBDKeep 8``
            ``DBDMax 20``
            ``DBDExptime 300``

         Replace ``yourpgserver`` and ``yourport`` with the address or
         hostname of your PostgreSQL database. Replace ``geuser`` and
         ``yourpassword`` with the username and password of your test
         user.

      #. Open the ``/opt/google/gehttpd/conf/gehttpd.conf`` file and
         insert the line:

         ``Include /opt/google/gehttpd/conf/extra/pgsql-auth.conf``

         above the line:

         ``Include conf.d/*.conf``

         The result is:

         .. code-block:: none

            # Include Google Earth Server-specific files
            Include /opt/google/gehttpd/conf/extra/pgsql-auth.conf
            Include conf.d/*.conf

      #. Add the following lines at the beginning of the
         ``<Location>`` directive of your virtual server:

         .. code-block:: none

            AuthType Digest
            AuthName "realm"
            AuthDigestDomain '/default_map/'
            AuthDigestProvider dbd
            AuthDBDUserRealmQuery "SELECT passwd FROM geeauth WHERE username = %s and realm = %s"
            BrowserMatch "MSIE" AuthDigestEnableQueryStringHack=On

         If this is a ``_ge_ virtual`` server, add:
         ``BrowserMatch "GoogleEarth" AuthDigestEnableQueryStringHack=On``

         If this is a ``_map_ virtual`` server, add:
         ``BrowserMatch "MSIE" AuthDigestEnableQueryStringHack=On``

         For more information about ``AuthDigestEnableQueryStringHack``,
         see the `mod_auth_digest
         <http://www.google.com/url?q=http%3A%2F%2Fhttpd.apache.org%2Fdocs%2F2.2%2Fmod%2Fmod_auth_digest.html%23msie&sa=D&sntz=1&usg=AFrqEze9uh13hmi22IsT3-GMw3t8j7VHcA>`__
         section in the Apache documentation.

         The final ``<Location>`` directive looks like:

         .. code-block:: none

            <Location "/default_map/*">
            AuthType Digest
            AuthName "realm"
            AuthDigestDomain '/default_map/'

            AuthDigestProvider dbd
            AuthDBDUserRealmQuery "SELECT passwd FROM geeauth WHERE username = %s and realm = %s"
            BrowserMatch "MSIE" AuthDigestEnableQueryStringHack=On

            Require valid-user
            SetHandler gedb-handler
            Include
            conf.d/virtual_servers/runtime/default_map_runtime
            </Location>

      #. Save and close the virtual server configuration file.

      #. Restart the server:

         ``/etc/init.d/geserver restart``

         After you verify the configuration with your test user, you can
         add your users to the database.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
