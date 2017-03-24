// glc_unpacker.i - SWIG interface
%module glc_unpacker
%{
#include "glc_unpacker.h"
#include "glc_reader.h"
#include "portable_glc_reader.h"
#include "file_unpacker.h"
#include "file_package.h"
#include "packetbundle.h"
%}

%include glc_unpacker.h
%include glc_reader.h
%include portable_glc_reader.h
%include file_unpacker.h
%include file_package.h
%include packetbundle.h

