#include "khXML.h"
#include <string>
#include <thread>
#include <mutex>

static khMutexBase xmlLibLock = KH_MUTEX_BASE_INITIALIZER;

bool khXML::doOp(unique_ptr<khXMLOperation> op)
{
  khLockGuard guard(xmlLibLock);
  return op->op();
}

class khXMLOperation
{
  khXMLOperation(const khXMLOperation&) = delete;
  khXMLOperation(khXMLOperation&&) = delete;
  khXMLOperation& khXMLOperation(const khXMLOperation&) = delete;
  khXMLOperation& khXMLOperation(khXMLOperation&&) = delete;

protected:
   

public:
  khXMLOperation()
  {
    XMLPlatformUtils::Initialize();
  }

  virtual ~khXMLOperation()
  {
    khXMLOperation::Terminate();
  }

  virtual bool op();
};

class khXMLWriteToFile : public khXMLOperation
{

public:
  khXMLWriteToFile() : khXMLOperation()
  {}
};

class khXMLWriteToString : public khXMLOperation
{
public:
  khXMLWriteToString() : khXMLOperation()
  {}
};

class khXMLReadFromFile : public khXMLOperation
{
public:
  khXMLReadFromFile() : khXMLOperation()
  {}
};

class khXMLReadFromString : public khXMLOperation
{
public:
  khXMLReadFromString() : khXMLOperation()
  {}
};

 
