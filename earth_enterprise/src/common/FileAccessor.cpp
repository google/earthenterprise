#include "FileAccessor.h"

// ****************************************************************************
// ***  POSIXFileAccessor
// ****************************************************************************
int POSIXFileAccessor::Open(const std::string &fname, const char *mode, int flags, mode_t createMask) {
  int result = -1;
  if (mode) {
    filePointer = ::fopen(fname.c_str(), mode);
  }
  else {
    result = khOpen(fname, flags, createMask);
    fileDescriptor = result;
  }

  return result;
}

int POSIXFileAccessor::FsyncAndClose() {
  assert (fileDescriptor != -1);
  int result = khFsyncAndClose(fileDescriptor);
  this->invalidate();
  return result;
}

int POSIXFileAccessor::Close() {
  int result = 0;
  if (filePointer) {
    result = ::fclose(filePointer);
  }
  else {
    assert (fileDescriptor != -1);
    result = khClose(fileDescriptor);
  }
  this->invalidate();
  return result;
}

bool POSIXFileAccessor::PreadAll(void* buffer, size_t size, off64_t offset) {
  assert (fileDescriptor != -1);
  return khPreadAll(fileDescriptor, buffer, size, offset);
}

bool POSIXFileAccessor::PwriteAll(const void* buffer, size_t size, off64_t offset) {
  assert (fileDescriptor != -1);
  return khPwriteAll(fileDescriptor, buffer, size, offset);
}

bool POSIXFileAccessor::ReadStringFromFile(const std::string &filename, std::string &str, std::uint64_t limit) {
  assert (fileDescriptor != -1);
  return khReadStringFromFile(filename, str, limit);
}

bool POSIXFileAccessor::Exists(const std::string &filename) {
  assert (fileDescriptor != -1);
  return khExists(filename);
}

int POSIXFileAccessor::feof() {
  VerifyFileStream("r");
  return ::feof(filePointer);
}

void POSIXFileAccessor::fgets(char *buf, int bufsize) {
  VerifyFileStream("r");
  ::fgets(buf, bufsize, filePointer);
}

std::unique_ptr<AbstractFileAccessor> AbstractFileAccessor::getAccessor(const std::string &fname) {
  return std::unique_ptr<POSIXFileAccessor>(new POSIXFileAccessor());
}