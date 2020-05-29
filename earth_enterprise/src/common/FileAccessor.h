#include <string>
#include <memory>
#include <khFileUtils.h>

class AbstractFileAccessor {
public:
  virtual bool isValid() { return false; }
  virtual void invalidate() {};
  virtual void setFD(int i) {};
  virtual int getAsFD() { return -1; }

  virtual int Open(const std::string &fname, int flags, mode_t createMask) = 0;
  virtual int FsyncAndClose() = 0;
  virtual int Close() = 0;
  virtual bool PreadAll(void* buffer, size_t size, off64_t offset) = 0;
  virtual bool PwriteAll(const void* buffer, size_t size, off64_t offset) = 0;
};

class POSIXFileAccessor: public AbstractFileAccessor {
  int fileDescriptor;
public:
  POSIXFileAccessor(int i = -1) : fileDescriptor{ i } {}
  bool isValid() override { return fileDescriptor != -1; }
  void invalidate() override { fileDescriptor = -1; }
  void setFD(int i) override { fileDescriptor = i; }
  int getAsFD() override { return fileDescriptor; }

  int Open(const std::string &fname, int flags, mode_t createMask) override;
  int FsyncAndClose() override;
  int Close() override;
  bool PreadAll(void* buffer, size_t size, off64_t offset) override;
  bool PwriteAll(const void* buffer, size_t size, off64_t offset) override;
};

namespace FileAccessorFactory {
  static std::unique_ptr<AbstractFileAccessor> getAccessor(const std::string &fname, int flags, mode_t createMask);
}