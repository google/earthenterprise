#ifndef COMMON_FILEACCESSORPINTERFACE_H_
#define COMMON_FILEACCESSORPINTERFACE_H_

#include <string>

class FileAccessorInterface {
public:
    virtual int GetSomething() = 0;
};

class FileAccessorFactoryInterface {
// NOTE: Is this an unnecessary layer? Should it be replaced by a function
// in the plugin module? One good thing is that it prevents you from having to
// make repeated dlsym calls.
public:
    virtual FileAccessorInterface* GetAccessor(const std::string &fileName) = 0;
};


#endif //COMMON_FILEACCESSORPINTERFACE_H_
