Project: gdal-ge

Description:
This project builds a custom/sandboxed version of GDAL libraries and utilities
for Google Earth Enterprise Fusion.
It includes:
  open jpeg2000 decoder
  MrSID decoder (if MrSID SDK library is installed)
and patches:
  gdal-xerces-conftest.patch
  gdal-mrsiddataset.patch
  gdal-GBdatum.patch
  gdal-performance.patch


Install Location: /opt/google/{bin,lib}


To build GDAL with MrSID Raster Format, MrSID library needs to be installed on your platform:
1- Get MrSID Decoding SDK library (https://www.lizardtech.com/developer/)
2- Copy MrSDK library into a folder. Ex: /opt
3- Ensure that 'libltidsdk.so' can be disovered by `ldconfig` command:
	3.1- Add the full path of 'libltidsdk.so' to ‘/etc/ld.so.conf’. You can edit that file as root admin.
	3.2- Rebuild the libraries path cache (‘/etc/ld.so.cache’): `$sudo ldconfig`
	3.2- Verify that `ldconfig` command can now detect 'libltidsdk.so' library: `$ldconfig -p | grep libltidsdk.so`
