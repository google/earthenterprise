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
    if (strcmp(mode, "w") == 0) {
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

bool POSIXFileAccessor::GetLinesFromFile(std::vector<std::string> &lines, const size_t bufsize) {
  char buf[bufsize];
  buf[0] = 0;
  std::string content;

  try {
    ssize_t bytesRead = read(fileDescriptor, buf, bufsize -1);

    while (bytesRead > 0) {
      buf[bytesRead] = 0;
	    content += buf;	
	    bytesRead = read(fileDescriptor, buf, bufsize -1);
    }

    TokenizeString(content, lines, "\n");

    for (auto it = std::begin(lines); it != std::end(lines); ++it) {
      CleanString(&(*it), "\r\n");
    }

    return true;
  }

  catch (...) {
    return false;
  }
}

void POSIXFileAccessor::fprintf(const char *format, ...) {
  va_list arguments;
  va_start(arguments, format);
  ::dprintf(fileDescriptor, format, arguments);
  va_end(arguments);
}