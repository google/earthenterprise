Project: openssl-ge

Description:
This project builds a custom/sandboxed version of OpenSSL libraries for
Google Earth Enterprise Fusion.
These are sandboxed because we could not rely on link compatibility with
the openssl libraries found in standard distributions and could not safely
replace the default libraries (would cause link problems for other binaries in
the distribution).

Source Files: //depot/googleclient/third_party/openssl

Install Location: /opt/google/lib

Build Info:
For Fusion 3.2 and earlier, we build openssl-ge-0.9.8h-0.x86_64.rpm which is a
sandboxed version of openssl.
A development RPM is built as well, necessary for build/dev machines to compile
against.

As of 2008-10-23:
openssl-ge-0.9.8h-0.x86_64.rpm was built using rpmbuild openssl-ge.spec
on a SLES 9 box (compatible with SLES 9,10, RHEL 4,5 and Ubuntu 6,8)

To Build, you will need to build on a SLES 9 box using the scripts found in
//depot/googleclient/geo/fusion/RPMS/build_scripts

As of 2009-12-02:
openssl-ge-0.9.8l-0.x86_64.rpm was built using rpmbuild openssl-ge.spec
on a SLES 9 box (seb) (compatible with SLES 9,10, RHEL 4,5 and Ubuntu 6,8)
for 3.2.3 (branch build).

For Trunk we already have a crosstool based Sconscript to take in latest 3rd
party libraries.
