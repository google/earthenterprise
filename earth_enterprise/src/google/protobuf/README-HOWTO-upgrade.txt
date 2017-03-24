IMPORTANT:
Updating c++ protobuf library, you need to update python protobuf library,
located in src/google/protobuf-py.
Please, see src/google/protobuf-py/README-HOWTO-upgrade.txt. 

STEPS FOR UPDATING c++ protobuf library:
2012-07-31, for 2.4.1

Make a new protoc in:
  googleclient/geo/earth_enterprise/tools/bin
with the directions there:
  googleclient/geo/earth_enterprise/tools/bin/README.protoc.txt

To install this lib, you just copy the sources from a public repository.
Adapt SConscript from the old version; basically add all source files
except *test*. Build:

  scons google/protobuf

The build wanted:

  stubs/hash.cc
  stubs/map-util.cc
  stubs/stl_util-inl.cc

so they were copied from our previous version (2.0.3). But these are all
empty stubs, likely something was wrong with the build environment, that it
didn't generate them itself.

Clean the objects + libs so that it will build:
  rm googleclient/geo/earth_enterprise/src/DBG-x86_64/google/protobuf/*
  rm googleclient/geo/earth_enterprise/src/OPT-x86_64/google/protobuf/*
  rm googleclient/geo/earth_enterprise/src/DBG-x86_64/lib/*proto*
  rm googleclient/geo/earth_enterprise/src/OPT-x86_64/lib/*proto*

Then just delete existing generated protobuf .cc + .h files:

  rm googleclient/geo/earth_enterprise/src/DBG-x86_64/protobuf/*
  rm googleclient/geo/earth_enterprise/src/OPT-x86_64/protobuf/*

delete all libs that depend on protobufs:
  rm googleclient/geo/earth_enterprise/src/DBG-x86_64/lib/*dbroot*
  rm googleclient/geo/earth_enterprise/src/OPT-x86_64/lib/*dbroot*
... probably others
