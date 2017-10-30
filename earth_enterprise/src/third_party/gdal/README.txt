Project: gdal-ge

Description:
This project builds a custom/sandboxed version of GDAL libraries and utilities
for Google Earth Enterprise Fusion.
It includes a number of extensions:
  Kakadu jpeg2000 decoder
  MrSID decoder (geoexpress)
and patches:
  kakadu-fPIC.patch
  gdal-kakadu-build.patch
  gdal-jp2kak-backportrasterio.patch
  gdal-xerces-conftest.patch
  gdal-mrsid-sdk7-configure.patch
  gdal-mrsiddataset.patch
  gdal-kakopen.patch
  gdal-GBdatum.patch
  gdal-performance.patch

Source Files: //depot/googleclient/third_party/gdal
              //depot/googleclient/third_party/kakadu
              //depot/googleclient/third_party/geoexpress

Install Location: /opt/google/{bin,lib}

Build Info:
For Fusion 3.2 and earlier, we build gdal-ge-1.4.2-2.x86_64.rpm which is a
sandboxed version of gdal.
A development RPM is built as well, necessary for build/dev machines to compile
against.

As of 2008-10-23:
gdal-ge-1.4.2-2.x86_64.rpm was built using rpmbuild gdal-ge.spec
on a SLES 9 box (compatible with SLES 9,10, RHEL 4,5 and Ubuntu 6,8)

To Build, you will need to build on a SLES 9 box using the scripts found in
//depot/googleclient/geo/fusion/RPMS/build_scripts

To build GDAL with MrSID Raster Format, MrSID library needs to be installed on your platform:
1- Get MrSID Decoding SDK library (https://www.lizardtech.com/developer/)
2- Copy MrSDK library into a folder. Ex: /opt
3- Ensure that 'libltidsdk.so' can be disovered by `ldconfig` command:
	3.1- Add the full path of 'libltidsdk.so' to ‘/etc/ld.so.conf’. You can edit that file as root admin.
	3.2- Rebuild the libraries path cache (‘/etc/ld.so.cache’): `$sudo ldconfig`
	3.2- Verify that `ldconfig` command can now detect 'libltidsdk.so' library: `$ldconfig -p | grep libltidsdk.so`
