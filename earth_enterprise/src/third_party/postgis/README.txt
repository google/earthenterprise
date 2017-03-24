Project: postgis-ge

Description:
This project builds a custom/sandboxed version of PostGIS for
Google Earth Enterprise Fusion.
It is a standard build of PostGIS, only sandboxed.

Source Files: //depot/googleclient/third_party/postgis

Install Location: /opt/google/{bin,lib,share}

Build Info:
For Fusion 3.2 and earlier, we build postgis-ge-1.1.7-1.x86_64.rpm which is a
sandboxed version of PostGIS.

As of 2008-10-23:
postgis-ge-1.1.7-1.x86_64.rpm was built using rpmbuild postgis-ge.spec
on a SLES 9 box (compatible with SLES 9,10, RHEL 4,5 and Ubuntu 6,8)

To Build, you will need to build on a SLES 9 box using the scripts found in
//depot/googleclient/geo/fusion/RPMS/build_scripts
