#include <string>
#include <cstring>
#include <memory>
#include <khFileUtils.h>
#include "khStringUtils.h"

class AbstractFileAccessor {
public:
  virtual ~AbstractFileAccessor() = default;
  static std::unique_ptr<AbstractFileAccessor> getAccessor(const std::string &fname);
  virtual bool isValid() = 0;
  virtual void invalidate() = 0;
  virtual int getFD() = 0;
  virtual void Open(const std::string &fname, const char *mode = nullptr, int flags = 00, mode_t createMask = 0666) = 0;
  virtual int FsyncAndClose() = 0;
  virtual int Close() = 0;
  virtual bool PreadAll(void* buffer, size_t size, off64_t offset) = 0;
  virtual bool PwriteAll(const void* buffer, size_t size, off64_t offset) = 0;
  virtual bool ReadStringFromFile(const std::string &filename, std::string &str, std::uint64_t limit = 0) = 0;
  virtual bool Exists(const std::string &filename) = 0;
  virtual bool GetLinesFromFile(std::vector<std::string> &lines, const std::string &filename) = 0;
  virtual void fprintf(const char *format, ...) = 0;
};

class POSIXFileAccessor: public AbstractFileAccessor {
  int fileDescriptor;
public:
  POSIXFileAccessor(int i = -1) : fileDescriptor{ i }{}
  ~POSIXFileAccessor(void) {
    if (this->isValid()) {
      this->Close();
    }
  }

  bool isValid() override { return fileDescriptor != -1; }
  void invalidate() override { fileDescriptor = -1; }
  int getFD() override { return fileDescriptor; }
  void Open(const std::string &fname, const char *mode = nullptr, int flags = 00, mode_t createMask = 0666) override;
  int FsyncAndClose() override;
  int Close() override;
  bool PreadAll(void* buffer, size_t size, off64_t offset) override;
  bool PwriteAll(const void* buffer, size_t size, off64_t offset) override;
  bool ReadStringFromFile(const std::string &filename, std::string &str, std::uint64_t limit = 0) override;
  bool Exists(const std::string &filename) override;
  bool GetLinesFromFile(std::vector<std::string> &lines, const std::string &filename) override;
  void fprintf(const char *format, ...) override;
};