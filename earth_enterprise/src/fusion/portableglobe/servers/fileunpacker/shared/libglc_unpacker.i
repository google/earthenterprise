// libglc_unpacker.i - SWIG interface
%module glc_unpacker
%{
#include "fusion/portableglobe/servers/fileunpacker/shared/glc_unpacker.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/glc_reader.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/portable_glc_reader.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/file_unpacker.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/file_package.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/packetbundle.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/dbroot_info_reader.h"
%}

%include fusion/portableglobe/servers/fileunpacker/shared/glc_unpacker.h
%include fusion/portableglobe/servers/fileunpacker/shared/glc_reader.h
%include fusion/portableglobe/servers/fileunpacker/shared/portable_glc_reader.h
%include fusion/portableglobe/servers/fileunpacker/shared/file_unpacker.h
%include fusion/portableglobe/servers/fileunpacker/shared/file_package.h
%include fusion/portableglobe/servers/fileunpacker/shared/packetbundle.h
%include fusion/portableglobe/servers/fileunpacker/shared/dbroot_info_reader.h

