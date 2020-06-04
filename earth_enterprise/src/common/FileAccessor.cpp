#include "FileAccessor.h"

std::unique_ptr<AbstractFileAccessor> AbstractFileAccessor::getAccessor(const std::string &fname) {
  return std::unique_ptr<POSIXFileAccessor>(new POSIXFileAccessor());
}

// ****************************************************************************
// ***  POSIXFileAccessor
// ****************************************************************************
int POSIXFileAccessor::Open(const std::string &fname, const char *mode, int flags, mode_t createMask) {
  int result;
  if (mode) {
    if (mode == "w") {
      result = khOpenForWrite(fname, createMask);
    }
    else {
      result = khOpenForRead(fname);
    }
  }
  else {
    result = khOpen(fname, flags, createMask);
  }
  fileDescriptor = result;
  return result;
}

int POSIXFileAccessor::FsyncAndClose() {
  int result = khFsyncAndClose(fileDescriptor);
  this->invalidate();
  return result;
}

int POSIXFileAccessor::Close() {
  int result = khClose(fileDescriptor);
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

int POSIXFileAccessor::fgets(void *buf, size_t bufsize) {
  return read(fileDescriptor, buf, bufsize);
}

void POSIXFileAccessor::fprintf(const char *format, ...) {
  va_list arguments;
  va_start(arguments, format);
  ::dprintf(fileDescriptor, format, arguments);
  va_end(arguments);
}