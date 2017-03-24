IMPORTANT:
The python google protobuf library needs to be updated together with c++ protobuf (see
src/google/protobuf/README-HOWTO-upgrade.txt).

STEPS FOR UPDATING python protobuf library:
 - Update c++ protobuf library  (see src/google/protobuf/README-HOWTO-upgrade.txt);
 - Copy content from python folder of unpacked protobuf tarball into the src/google/protobuf-py;
 - Copy .proto files that we need for compiling from c++ protobuf library:
   (might put next actions in SConscript, but we need to modify setup.py anyway.) 
    In v. 2.4.1 we copied two files: 
      descriptor.proto,
      plugin.proto
   $ cp src/google/protobuf/descriptor src/google/protobuf-py/google/protobuf/
   $ cp src/google/protobuf/compiler/plugin.proto src/google/protobuf-py/google/protobuf/compiler/

 - Modify setup.py (see notes in previous version):
   - path to protoc, paths to *.proto files, ..;
 - Update SConscript if it is needed.
