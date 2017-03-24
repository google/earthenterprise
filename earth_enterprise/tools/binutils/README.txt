Project: binutils-ge

Description:
This project builds a custom/sandboxed version of GNU Bin Utils Project which
is used for building parts of Google Earth Enterprise Fusion.
It is a standard build of binutils.

Source Files: //depot/googleclient/third_party/binutils

Install Location: /opt/google/bin

Build Info:
For Fusion 3.2 and earlier, we build binutils-ge-2.2.9.0.x86_64.rpm which is a
sandboxed version of binutils.

As of 2008-10-23:
binutils-ge-2.2.9.0.x86_64.rpm was built using rpmbuild binutils-ge.spec
on a SLES 9 box (compatible with SLES 9,10, RHEL 4,5 and Ubuntu 6,8)

To Build, you will need to build on a SLES 9 box using the scripts found in
//depot/googleclient/geo/fusion/RPMS/build_scripts
