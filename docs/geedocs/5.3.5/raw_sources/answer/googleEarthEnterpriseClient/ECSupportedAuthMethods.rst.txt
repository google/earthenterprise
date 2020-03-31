|Google logo|

===================================
EC Supported Authentication Methods
===================================

.. container::

   .. container:: content

      .. list-table::
         :widths: 25 25 25 25
         :header-rows: 1
 
         * - Authentication Type
           - Version 7.0.8
           - Version 7.3.2
           - Version 7.3.3
         * - Basic
           - Yes
           - Yes
           - Yes
         * - PKI
           - Yes
           - Yes
           - Yes
         * - OAuth 2.0
           - No
           - No
           - No
         * - SAML 2.0
           - No
           - No
           - No

      .. rubric:: Basic Authentication
         
      Basic authentication is the simplest login method. The user supplies a
      username and password that are compared against known username/password
      combinations to verify the user's identity.

      .. rubric:: PKI Authentication

      With Public Key Infrastructure authentication, the user has their own
      security certificate, with a public key, that they provide to the server
      to verify their identity. This is often in the form of a smart card
      that can be read by the client's computer.

      .. rubric:: SAML 2.0 and OAuth 2.0 (Unsupported as of 7.3.3)

      SAML and OAuth operate differently under the hood, but the underlying
      concept is similar. In both cases, when a user attempts to access a
      service, they are redirected to a separate Identity Provider to login.
      Once their identity has been verified, an access token is created that
      allows access the requested service. 

        .. note:: 

           In SAML and OAuth workflows, Basic and PKI login can still be used by
           the Identity Provider to validate the user. However, the resulting
           access token may allow access to multiple sites or services. This is
           often referred to as Single Sign-On.

.. |Google logo| image:: ../../art/common/googlelogo_color_260x88dp.png
   :width: 130px
   :height: 44px
