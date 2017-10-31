Project: gdal-ge

Description:

This project builds a custom/sandboxed version of GDAL libraries and utilities
for Google Earth Enterprise Fusion.
It includes a number of extensions:
* Open jpeg2000 decoder
* MrSID decoder

and patches:
* gdal-xerces-conftest.patch
* gdal-mrsiddataset.patch
* gdal-GBdatum.patch
* gdal-performance.patch


Install Location: /opt/google/{bin,lib}

To build GDAL with MrSID Raster Format, MrSID library needs to be installed on your platform:

1. Get MrSID Decoding SDK library [Lizardtech Developer Page](https://www.lizardtech.com/developer/)
1. Copy MrSDK library into a folder. Ex: /opt
1. Ensure that 'libltidsdk.so' can be disovered by `ldconfig` command:
   1. Add the full path of 'libltidsdk.so' to ‘/etc/ld.so.conf’. You can edit that file as root admin.
   1. Rebuild the libraries path cache (‘/etc/ld.so.cache’): `$sudo ldconfig`
   1. Verify that `ldconfig` command can now detect 'libltidsdk.so' library: `$ldconfig -p | grep libltidsdk.so`