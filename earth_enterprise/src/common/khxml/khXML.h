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

enum class khXMLOperationType : uint8_t
{
    KH_XML_READ_FILE = 0,
    KH_XML_READ_STRING = 1,
    KH_XML_WRITE_FILE = 2,
    KH_XML_WRITE_STRING = 3,
    DEFAULT = KH_XML_READ_FILE
};
// declare base class for XML operations
class khXMLOperation
{
  khXMLOperation(const khXMLOperation&) = delete;
  khXMLOperation(khXMLOperation&&) = delete;
  khXMLOperation& khXMLOperation(const khXMLOperation&) = delete;
  khXMLOperation& khXMLOperation(khXMLOperation&&) = delete;

protected:
  std::unique_ptr<DOMDocument> doc;
  std::unique_ptr<DOMLSParser> parser; 

public:
  khXMLOperation() throw()
  {
    try {
      XMLPlatformUtils::Initialize();
      unique_ptr<DOMImplementation> impl(std::move(reinterpret_cast<DOMImplementation*>
                                        (DOMImplementationRegistry::getDOMImplementation(0))));

      } catch (const XMLException& e) {
        notify(NFY_FATAL, "Unable to initialize Xerces: %s",
               FromXMLStr(e.getMessage()).c_str());
      }
  }

  virtual ~khXMLOperation()
  {
    try
    {
      doc->release();
    } 
    catch (...) {}
    try
    {
      parser->release();
    }
    catch (...) {}
    khXMLOperation::Terminate(); 
  }

  // declare list of operations to be  virtual
  virtual bool op(std::string, std::string&) throw(); //write to string
  virtual bool op(std::string) throw(); //read/write to file
  virtual bool op(std::string,const std::string&) throw(); //read from string

  virtual

 }


class khXMLWriteToFile : public khXMLOperation
{
private:
  std::unique_ptr<DOMLSSerializer> writer;
  khXMLWriteToFile() = delete;
  std::string filename;

public:
  khXMLWriteToFile(std::string _filename) : khXMLOperation() 
  { 
    filename = std::move(_filename); 
  }
  

  bool op() throw();
};

class khXMLWriteToString : public khXMLOperation
{
private:
  std::unique_ptr<DOMLSSerializer> writer;
  std::string filename;
  std::string buffer;
  khXMLWriteToString() = delete;
public:
  khXMLWriteToString(std::string _filename, std::string _buffer) : khXMLOperation()
  {
    filename = std::move(_filename);
    buffer = std::move(_buffer);
  }
 
  bool op() throw();
};

class khXMLReadFromFile : public khXMLOperation
{
private:
  khXMLReadFromFile() = delete;
  std::string filename;
public:
  khXMLReadFromFile(std::string _filename) : khXMLOperation()
  {
    filename = std::move(_filename);
  }

  bool op() throw();
};

class khXMLReadFromString : public khXMLOperation
{
private:
  khXMLReadFromString() = delete;
  std::string filename;
  std::string buffer;
public:
  khXMLReadFromString(std::string _filename, std::string _buffer) : khXMLOperation()
  {
    filename = std::move(_filename);
    buffer = std::move(_buffer);
  }

  bool op() throw();
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
