Project: postgresql-ge

Description:
This project builds a custom/sandboxed version of PostGreSQL for
Google Earth Enterprise Fusion.
It is a normal build of PostGreSQL.

Source Files: //depot/googleclient/third_party/postgresql

Install Location: /opt/google/bin, /var/opt/google/pgsql/

Build Info:
For Fusion 3.2 and earlier, we build postgresql-ge-8.1.8-4.x86_64.rpm which is a
sandboxed version of PostGreSQL.
A development RPM is built as well, necessary for build/dev machines to compile
against.

As of 2008-10-23:
postgresql-ge-8.1.8-4.x86_64.rpm was built using rpmbuild postgresql-ge.spec
on a SLES 9 box (compatible with SLES 9,10, RHEL 4,5 and Ubuntu 6,8)

To Build, you will need to build on a SLES 9 box using the scripts found in
//depot/googleclient/geo/fusion/RPMS/build_scripts
