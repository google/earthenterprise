Project: openldap-ge

Description:
This project builds a custom/sandboxed version of OpenLDAP for
Google Earth Enterprise Fusion.
This is a standard build of the OpenLDAP, only sandboxed.

Source Files: //depot/googleclient/third_party/openldap

Install Location: /etc/opt/google/openldap, /opt/google/lib

Build Info:
For Fusion 3.2 and earlier, we build openldap-ge-2.3.39-0.x86_64.rpm which is a
sandboxed version of openldap.
A development RPM is built as well, necessary for build/dev machines to compile
against.

As of 2008-10-23:
openldap-ge-2.3.39-0.x86_64.rpm was built using rpmbuild openldap-ge.spec
on a SLES 9 box (compatible with SLES 9,10, RHEL 4,5 and Ubuntu 6,8)

To Build, you will need to build on a SLES 9 box using the scripts found in
//depot/googleclient/geo/fusion/RPMS/build_scripts
