/*
 * Copyright 2018 the OpenGEE Contributors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef KHXML_H
#define KHXML_H

#include <memory>
#include "khdom.h"
#include "notify.h"
using namespace khxml;


// class for enumerated types, to allow for derived classes to id themselvesx
enum class khXMLOperationType : uint8_t
{
    KH_XML_READ_FILE = 0,
    KH_XML_READ_STRING = 1,
    KH_XML_WRITE_FILE = 2,
    KH_XML_WRITE_STRING = 3,
    KH_XML_LOAD_FROM_STRING = 4,
    DEFAULT = KH_XML_READ_FILE
};


/*******************************************************************
*
*  Base class for all XML operations. Facilitates interaction with
*  strategy contrete class.
*
*******************************************************************/
class khXMLOperation
{
  khXMLOperation(const khXMLOperation&) = delete;
  khXMLOperation(khXMLOperation&&) = delete;
  khXMLOperation& operator=(const khXMLOperation&) = delete;
  khXMLOperation& operator(khXMLOperation&&) = delete;

protected:
  std::shared_ptr<DOMDocument> doc;
  khXMLOperationType type;

public:
  khXMLOperation(const std::string& rootTagName) throw()
  {
    try {
    XMLPlatformUtils::Initialize();
    } catch (const XMLException& e) {
      notify(NFY_FATAL, "Unable to initialize Xerces: %s",
             FromXMLStr(e.getMessage()).c_str());
    }
    try {

      std::shared_ptr_ptr<DOMImplementation> impl
              (std::move(reinterpret_cast<DOMImplementation*>
              (DOMImplementationRegistry::getDOMImplementation(0))));
      doc.reset(impl->createDocument(0, ToXMLStr(rootTagName), 0));
    } catch (...) {
          std::string error {"unable to create document"};
          notify(NFY_FATAL,"XML error: %s", error.c_str());
          throw khException(kh::tr(error.c_str()));
      }
  }

  virtual ~khXMLOperation()
  {
    try
    {
      doc->release();
    } 
    catch (...) {}
    if (type != khXMLOperationType::KH_XML_LOAD_FROM_STRING) khXMLOperation::Terminate();
  }

  // polymorphic function to allow for use of strategy
  virtual bool op() throw() = 0;
  // allow the derived classes to give type
  virtual inline const khXMLOperationType getType() = 0;

 }

/*******************************************************************
 *
 * Base class for load operations. Perhaps tied to reads, so do not
 * Terminate() in destructor
 *
 ******************************************************************/

class khXMLLoadFromString : public khXMLOperation
{
private:
    std::string buffer;
    std::string ref;
public:
  khXMLLoadFromString(const std::string& _buffer, const std::string& _ref)
      : khXMLOperation()
  {
      type = khXMLOperationType::KH_XML_LOAD_FROM_STRING;
      buffer = _buffer;
      ref = _ref;
  }

  bool op() throw();
};

/*****************************************************************
 *
 * Base class for write operations. Right now, allows for file and
 * string writes. Can be further expanded.
 *
 ****************************************************************/
class khXMLWriteOp : public khXMLOperation
{
protected:
  std::unique_ptr<DOMLSSerializer> writer;
  std::string buffer;
public:
  // polymorphic functions explained above, pass to derived class
  khXMLWriteOp(const std::string& buf)
  {
      buffer = buf;
  }
  virtual bool op() = 0;
};

class khXMLWriteToFile : public khXMLWriteOp
{
private:

  khXMLWriteToFile() = delete;
  std::string filename;

public:
  khXMLWriteToFile(const std::string& _filename, const std::string _buffer)
      : khXMLOperation(_buffer)
  { 
    filename = _filename;
  }

  inline khXMLOperationType getType() { return khXMLOperationType::KH_XML_WRITE_FILE; }
  // op writes the
  bool op() throw();
};


// Usage: buffer = concrete::instance().doOp(writefilestrat)
// buf = writefilestart->getBufferString
class khXMLWriteToString : public khXMLWriteOp
{
private:
  std::unique_ptr<DOMLSSerializer> writer;
  std::string buffer;
  khXMLWriteToString() = delete;
public:
 
  bool op() throw();
  inline khXMLOperationType getType() { return khXMLOperationType::KH_XML_WRITE_STRING; }
  inline std::string getBufferString() { return buffer; }

};



/*****************************************************************
 *
 * Base class for write operations. Right now, allows for file and
 * string reads. Can be further expanded.
 *
 ****************************************************************/


class khXMLReadOp : public khXMLOperation
{
protected:
    std::unique_ptr<DOMLSParser> parser;
    std::shared_ptr<DOMDocument> doc;

public:
    khXMLReadOp() : khXMLOperation() throw();
    virtual khXMLOperationType getType() = 0;
    virtual bool op() = 0;
};

class khXMLReadFromFile : public khXMLReadOp
{
private:
  khXMLReadFromFile() = delete;
  std::string filename;
public:
  khXMLReadFromFile(const std::string& _filename) : khXMLReadOp() throw()
  {
    filename = _filename;
  }

  bool op() throw();
  inline khXMLOperationType getType() { return khXMLOperationType::KH_XML_READ_FILE; }
  inline std::shared_ptr<khxml::DOMDocument> getFile();
};

class khXMLReadFromString : public khXMLOperation
{
private:
  khXMLReadFromString() = delete;
  std::string filename;
  std::string buffer;
public:
  khXMLReadFromString(std::string _filename, const std::string& _buffer) : khXMLOperation() throw()
  {
    filename = std::move(_filename);
    buffer =
  }

  bool op() throw();
  inline std::string getString();
  inline khXMLOperationType getType() { return khXMLOperationType::KH_XML_READ_STRING; }
};


class khXML
{
  khXML() = default;
  khXML(const khXML&) = delete;
  khXML(khXML&&) = delete;
  khXML& operator=(const khXML&) = delete;
  khXML& operator=(khXML&&) = delete;
public:
  khXMLParser() = delete;
  static hXML& instance()
  {
    static khXML _instance;
    return _instance;
  }
	
  static bool doOp(std::shared__ptr<khXMLOperation>) throw();
};

#endif
