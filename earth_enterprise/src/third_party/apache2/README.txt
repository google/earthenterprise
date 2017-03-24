Project: apache-ge

Description:
This project builds a custom/sandboxed version of Apache HTTPD for
Google Earth Enterprise Fusion.
It includes custom configuration files and the binary is renamed gehttpd
so that it is uniquely identifiable from the standard httpd binary.
It links with the OSSP mm - Shared Memory Allocation project.

A development RPM is built as well, necessary for build/dev machines to compile
against, and this development RPM is shipped with the Fusion Server to allow
customers to build their own modules compatible with gehttpd.

Source Files: //depot/googleclient/third_party/apache2
              //depot/googleclient/third_party/mm

Install Location: /opt/google/gehttpd

Build Info:
For Fusion 3.2 and earlier, we build apache-ge-2.2.9.0.x86_64.rpm which is a
sandboxed version of apache2.
A development RPM is built as well, necessary for build/dev machines to compile
against.

As of 2008-10-23:
apache-ge-2.2.9.0.x86_64.rpm was built using rpmbuild apache-ge.spec
on a SLES 9 box (compatible with SLES 9,10, RHEL 4,5 and Ubuntu 6,8)

To Build, you will need to build on a SLES 9 box using the scripts found in
//depot/googleclient/geo/fusion/RPMS/build_scripts
