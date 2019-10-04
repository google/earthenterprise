|Google logo|

======================================================
Hostname and Fully Qualified Domain Name Configuration
======================================================

.. container::

   .. container:: content

      .. rubric:: Hostname and Fully Qualified Domain Name Configuration

      In order to make sure that the Apache server correctly determines
      the Fully Qualified Domain Name (FQDN) of the server and that the
      virtual hosts use the correct server URL, the hostname should be
      configured correctly. This is preferred instead of modifying the
      Apache server name configuration so that all the software looks to
      the same source for determining FQDNs. If the command
      ``hostname -f`` returns the correct FQDN hostname then the system
      is correctly configured to run Open GEE. If not the following
      instructions may help.

      Ideally the network where GEE is installed has a DNS set up to
      resolve hostnames so that IP traffic can be routed appropriately.
      However in cases where there is no DNS, static hostnames can be
      used as well. With static hostnames the ``hostname`` or
      ``hostnamectl`` command should be used to set the hostname to its
      FQDN. The ``/etc/hostname`` file should be updated to match as
      well so the name stays after reboots. If setting the hostname to
      the FQDN is not possible, then setting the FQDN in the
      ``/etc/hosts`` file is necessary. When using a static hostname,
      some operating systems may still give a warning on Apache startup
      even if it correctly determined the FQDN. Adding the FQDN to
      ``/etc/hosts`` would fix this as well. If adding the FQDN to
      ``/etc/hosts`` is needed, then the FQDN followed by just the host
      portion should be added to the line that contains the IP address
      the server will be listening on. This should be the first line of
      the file:

      ``127.0.0.1  myserver.mydomainname.com myserver localhost``

      If GEE is not using DNS other clients that connect to the server
      non-locally would need to have the same static mapping set up in
      their hosts files since the server hostname would not be resolved
      dynamically.

      When determining where to look for the correct FQDN Apache calls
      ``hostname`` which will use a resolver. The system configuration
      determines what the order is for resolving the FQDN. If the
      ``/etc/nsswitch.conf`` file is being used there is a line like
      this that defines the order:

      ``hosts:      dns myhostname files``

      This example shows that the DNS on the network would be queried
      first. If that does not define the FQDN then the hostname would be
      queried. If the FQDN is still not defined then configuration files
      would be queried, mainly ``/etc/hosts``.

      If the ``/etc/host.conf`` file is being used to resolve the
      hostname, then there is a line like this that defines the order:

      ``order hosts,bind``

      In this example hosts refers to the ``/etc/hosts`` file and is
      checked first, and bind refers to the DNS.

      Here is :doc:`another note <../geeServerConfigAndSecurity/serverHostname>` on the server
      hostname.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
