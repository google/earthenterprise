#include <string>
#include <memory>
#include <khFileUtils.h>

class AbstractFileAccessor {
public:
  static std::unique_ptr<AbstractFileAccessor> getAccessor(const std::string &fname);
  virtual bool isValid() = 0;
  virtual void invalidate() = 0;
  virtual int getFD() = 0;
  virtual int Open(const std::string &fname, int flags, mode_t createMask = 0666) = 0;
  virtual void Open(const char *fname, const char *mode) = 0;
  virtual int FsyncAndClose() = 0;
  virtual int Close() = 0;
  virtual bool PreadAll(void* buffer, size_t size, off64_t offset) = 0;
  virtual bool PwriteAll(const void* buffer, size_t size, off64_t offset) = 0;
  virtual bool ReadStringFromFile(const std::string &filename, std::string &str, std::uint64_t limit = 0) = 0;
  virtual bool Exists(const std::string &filename) = 0;
  virtual int feof() = 0;
  virtual void fgets(char *buf, int bufsize) = 0;
  virtual void fprintf(const char * __restrict format, ...) = 0;
};

class POSIXFileAccessor: public AbstractFileAccessor {
  int fileDescriptor;
  FILE* fp;
public:
  POSIXFileAccessor(int i = -1, FILE* f = NULL) : fileDescriptor{ i }, fp{ f } {}
  ~POSIXFileAccessor(void) {
    if (this->isValid()) {
      this->Close();
    }
  }

  bool isValid() override { return fileDescriptor != -1 || fp != NULL; }
  void invalidate() override {
    fileDescriptor = -1;
    fp = NULL;
  }
  int getFD() override { return fileDescriptor; }
  int Open(const std::string &fname, int flags, mode_t createMask = 0666) override;
  void Open(const char *fname, const char *mode) override;
  int FsyncAndClose() override;
  int Close() override;
  bool PreadAll(void* buffer, size_t size, off64_t offset) override;
  bool PwriteAll(const void* buffer, size_t size, off64_t offset) override;
  bool ReadStringFromFile(const std::string &filename, std::string &str, std::uint64_t limit = 0) override;
  bool Exists(const std::string &filename) override;
  int feof() override;
  void fgets(char *buf, int bufsize) override;
  inline void fprintf(const char * __restrict format, ...) override {
    ::fprintf(fp, format, __va_arg_pack());
  }
};