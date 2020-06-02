#include "FileAccessor.h"

// ****************************************************************************
// ***  POSIXFileAccessor
// ****************************************************************************
int POSIXFileAccessor::Open(const std::string &fname, int flags, mode_t createMask) {
  int result = khOpen(fname, flags, createMask);
  fileDescriptor = result;
  return result;
}

void POSIXFileAccessor::Open(const char *fname, const char *mode) {
  fp = ::fopen(fname, mode);
}

int POSIXFileAccessor::FsyncAndClose() {
  int result = khFsyncAndClose(fileDescriptor);
  this->invalidate();
  return result;
}

int POSIXFileAccessor::Close() {
  int result = 0;
  if (fileDescriptor != -1) {
    result = khClose(fileDescriptor);
  }
  else {
    result = ::fclose(fp);
  }
  this->invalidate();
  return result;
}

bool POSIXFileAccessor::PreadAll(void* buffer, size_t size, off64_t offset) {
  return khPreadAll(fileDescriptor, buffer, size, offset);
}

bool POSIXFileAccessor::PwriteAll(const void* buffer, size_t size, off64_t offset) {
  return khPwriteAll(fileDescriptor, buffer, size, offset);
}

bool POSIXFileAccessor::ReadStringFromFile(const std::string &filename, std::string &str, std::uint64_t limit) {
  return khReadStringFromFile(filename, str, limit);
}

bool POSIXFileAccessor::Exists(const std::string &filename) {
  return khExists(filename);
}

int POSIXFileAccessor::feof() {
  return ::feof(fp);
}

void POSIXFileAccessor::fgets(char *buf, int bufsize) {
  ::fgets(buf, bufsize, fp);
}

std::unique_ptr<AbstractFileAccessor> AbstractFileAccessor::getAccessor(const std::string &fname) {
  return std::unique_ptr<POSIXFileAccessor>(new POSIXFileAccessor());
}