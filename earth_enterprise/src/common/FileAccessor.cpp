#include "FileAccessor.h"

// ****************************************************************************
// ***  POSIXFileAccessor
// ****************************************************************************
int POSIXFileAccessor::Open(const std::string &fname, int flags, mode_t createMask) {
  return khOpen(fname, flags, createMask);
}

int POSIXFileAccessor::FsyncAndClose() {
  return khFsyncAndClose(this->getAsFD());
}

int POSIXFileAccessor::Close() {
  return khClose(this->getAsFD());
}

bool POSIXFileAccessor::PreadAll(void* buffer, size_t size, off64_t offset) {
  return khPreadAll(this->getAsFD(), buffer, size, offset);
}

bool POSIXFileAccessor::PwriteAll(const void* buffer, size_t size, off64_t offset) {
  return khPwriteAll(this->getAsFD(), buffer, size, offset);
}