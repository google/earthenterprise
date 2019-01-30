// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/Wrapper4InputSource.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <notify.h>
#include <khThread.h>
#include <khFileUtils.h>
#include "khxml.h"
#include "khdom.h"
#include <string>
#include <array>
#include <algorithm>
#include <exception>
#include "common/khConfigFileParser.h"
#include <map>
#include <cstdlib>
using namespace khxml;

static khConfigFileParser config_parser;

std::string
ListElementTagName(const std::string &tagname)
{
  if (tagname[tagname.length()-1] == 's') {
    return tagname.substr(0, tagname.length()-1);
  } else {
    return tagname + "_element";
  }
}

XMLSSize_t  initialDOMHeapAllocSize;
XMLSSize_t  maxDOMHeapAllocSize;
XMLSSize_t  maxDOMSubAllocationSize;

const std::string INIT_HEAP_SIZE = "INIT_HEAP_SIZE";
const std::string MAX_HEAP_SIZE = "MAX_HEAP_SIZE";
const std::string BLOCK_SIZE = "BLOCK_SIZE";
const std::string XMLConfigFile = "/etc/opt/google/XMLparams";
static std::array<std::string,3> options
{{
    INIT_HEAP_SIZE,
    MAX_HEAP_SIZE,
    BLOCK_SIZE
}};

class tsMap
{
private:
    static std::map<std::string, uint32_t> readFiles;
    static std::map<std::string, uint32_t> writtenFiles;
    tsMap() = default;
    static khMutexBase mapLock;
public:
    static tsMap& instance()
    {
        static tsMap _instance;
        return _instance;
    }

    static void insertRead(std::string fn)
    {
        khLockGuard guard(mapLock);
        if (readFiles.find(fn) == readFiles.end())
        {
            auto elem = std::make_pair(fn,1);
            readFiles.emplace(elem);
        }
        else
        {
            auto elem = readFiles.find(fn);
            ++elem->second;       
        }
        notify(NFY_DEBUG, "%s read %d time(s)",
               fn.c_str(), readFiles.find(fn)->second);
    }

    static void insertWritten(std::string fn)
    {
        khLockGuard guard(mapLock);
        if (writtenFiles.find(fn) == writtenFiles.end())
        {
            auto elem = std::make_pair(fn,1);
            writtenFiles.emplace(elem);
        }
        else
        {
            auto elem = writtenFiles.find(fn);
            ++elem->second;
        }
        notify(NFY_DEBUG, "%s written %d time(s)",
               fn.c_str(), writtenFiles.find(fn)->second);
        
    }

    static uint32_t getNumWritten(std::string fn)
    {
        khLockGuard guard(mapLock);
        auto elem = writtenFiles.find(fn);

        uint32_t retval;
        if (elem == writtenFiles.end()) retval = 0;
        else retval = elem->second;
        return retval;
    }

    static uint32_t getNumRead(std::string fn)
    {
        khLockGuard guard(mapLock);
        auto elem = readFiles.find(fn);

        uint32_t retval;
        if (elem == readFiles.end()) retval = 0;
        else retval = elem->second;
        return retval;
    }
};

std::map<std::string, uint32_t> tsMap::readFiles;
std::map<std::string, uint32_t> tsMap::writtenFiles;
khMutexBase tsMap::mapLock = KH_MUTEX_BASE_INITIALIZER;

class XmlParamsException : public std::exception {};
class MinValuesNotMet : public XmlParamsException
{
public:
    const char* what() const noexcept 
    { 
        return "Initialization Parameters must be 1024 or greater"; 
    }
};
class SizeError : public XmlParamsException
{
public:
    const char* what() const noexcept
    {
        return "Initial heap size and block allocation size must be less than the max heap size";
    }
};

void validateXMLParameters()
{
    // using 1024 as the lowest setting
    XMLSSize_t lowestBlock = 0x400;

    // check to make sure they meet the minimum size
    if (initialDOMHeapAllocSize < lowestBlock ||
   	maxDOMHeapAllocSize < lowestBlock)     
    {
	throw MinValuesNotMet();
    }

    // check to make sure that the initial size is less than the max size
    if (maxDOMHeapAllocSize < initialDOMHeapAllocSize ||
        maxDOMHeapAllocSize < maxDOMSubAllocationSize)
    {
        throw SizeError();
    }	
}

void setDefaultValues()
{
   initialDOMHeapAllocSize = 0x4000;
   maxDOMHeapAllocSize     = 0x20000;
   maxDOMSubAllocationSize = 0x1000;
}

// This is used only in the following function
class UsingXMLGuard
{
  friend void InitializeXMLLibrary() throw();

  UsingXMLGuard(void) throw() {
    setDefaultValues();
    std::string fn(XMLConfigFile);
    try {
      for (const auto& a : options)
      {
          config_parser.addOption(a);
      }
      config_parser.parse(fn);
      config_parser.validateIntegerValues();
      for (const auto& a : config_parser)
      {
          // will only set the values that are present, otherwise will stay at defaults
          if (a.first == INIT_HEAP_SIZE)
              initialDOMHeapAllocSize = std::stol(a.second);
          else if (a.first == MAX_HEAP_SIZE)
              maxDOMHeapAllocSize = std::stol(a.second);
          else if (a.first == BLOCK_SIZE)
              maxDOMSubAllocationSize = std::stol(a.second);
      }
    } catch (const khConfigFileParserException& e) {
      notify(NFY_DEBUG, "%s , using default xerces init values", e.what());
    }
    
    try {
      try {
         validateXMLParameters();
      } catch (const XmlParamsException& e) {
         notify(NFY_DEBUG, "%s, using default xerces init values", e.what());
         setDefaultValues();
      }
      XMLPlatformUtils::Initialize(initialDOMHeapAllocSize,
                                   maxDOMHeapAllocSize,
                                   maxDOMSubAllocationSize);
      notify(NFY_DEBUG, "XML initialization values: %s=%zu %s=%zu %s=%zu",
             "initialDOMHeapAllocSize", initialDOMHeapAllocSize,
             "maxDOMHeapAllocSize", maxDOMHeapAllocSize,
             "maxDOMSubAllocationSize", maxDOMSubAllocationSize);
    } catch(const XMLException& toCatch) {
      notify(NFY_FATAL, "Unable to initialize Xerces: %s",
             FromXMLStr(toCatch.getMessage()).c_str());
    }
  }

  ~UsingXMLGuard(void) throw() {
    try {
      XMLPlatformUtils::Terminate();
    } catch (...) {
    }
  }
};

static khMutexBase xmlLibLock = KH_MUTEX_BASE_INITIALIZER;

void InitializeXMLLibrary() throw()
{
  khLockGuard guard(xmlLibLock);
  static UsingXMLGuard XMLLibGuard;
}


DOMDocument *
CreateEmptyDocument(const std::string &rootTagname) throw()
{
  InitializeXMLLibrary();
  try {
    DOMImplementation* impl =
    DOMImplementationRegistry::getDOMImplementation(0);

    DOMDocument* doc = impl->createDocument(0,// root element namespace URI.
                                            ToXMLStr(rootTagname),// root element name
                                            0);// document type object (DTD)
    return doc;
   } catch (...) {
     return 0;
   }
}

namespace {
bool
WriteDocumentImpl(DOMDocument *doc, const std::string &filename) throw()
{

  InitializeXMLLibrary();
  bool success = false;

  try {
    // "LS" -> Load/Save extensions
    DOMImplementationLS* impl = (DOMImplementationLS*)
                                 DOMImplementationRegistry::getDOMImplementation(ToXMLStr("LS"));

    DOMLSSerializer* writer = impl->createLSSerializer();

    try {
      // optionally you can set some features on this serializer
      if (writer->getDomConfig()->canSetParameter(XMLUni::fgDOMWRTDiscardDefaultContent, true))
        writer->getDomConfig()->setParameter(XMLUni::fgDOMWRTDiscardDefaultContent, true);
      if (writer->getDomConfig()->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true))
        writer->getDomConfig()->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);

      if (!khEnsureParentDir(filename)) {
        notify(NFY_WARN, "Unable to write %s: Can't make parent directory.",
               filename.c_str());
      } else {
        ToXMLStr fname(filename);
        LocalFileFormatTarget formatTarget(fname);
        DOMLSOutput* lsOutput = impl->createLSOutput();
        lsOutput->setByteStream(&formatTarget);
        if (writer->write(doc, lsOutput)) {
          success = true;
        } else {
          notify(NFY_WARN, "Unable to write %s: Xerces didn't tell me why not.",
                 filename.c_str());
        }
        lsOutput->release();
      }
    } catch (const XMLException& toCatch) {
      notify(NFY_WARN, "Unable to write %s: %s",
             filename.c_str(),
             XMLString::transcode(toCatch.getMessage()));
    } catch (const DOMException& toCatch) {
      notify(NFY_WARN, "Unable to write %s: %s",
             filename.c_str(),
             XMLString::transcode(toCatch.msg));
    } catch (...) {
      notify(NFY_WARN, "Unable to write %s: Unknown exception",
             filename.c_str());
    }
    writer->release();
  } catch (const XMLException& toCatch) {
    notify(NFY_WARN, "Unable to create DOM Writer for %s: %s",
           filename.c_str(), XMLString::transcode(toCatch.getMessage()));
  } catch (const DOMException& toCatch) {
    notify(NFY_WARN, "Unable to create DOM Writer for %s: %s",
           filename.c_str(), XMLString::transcode(toCatch.msg));
  } catch (...) {
    notify(NFY_WARN, "Unable to create DOM Writer for %s: Unknown exception",
           filename.c_str());
  }

  return success;
}
} // anonymous namespace


bool
WriteDocument(DOMDocument *doc, const std::string &filename) throw()
{
  bool retval = true;

  static const std::string newext = ".new";
  static const std::string backupext = ".old";
  const std::string newname = filename + newext;
  const std::string backupname = filename + backupext;
  if (!WriteDocumentImpl(doc, newname)) {
    retval = false;
  }
  else
  {
    tsMap::instance().insertWritten(filename);
  }
  if (retval && !khReplace(filename, newext, backupext)) {
    (void) khUnlink(newname);
    retval = false;
  }
  if (retval && khExists(backupname)) {
    notify(NFY_VERBOSE,"WriteDocument() backupname %s exists", backupname.c_str());
    (void) khUnlink(backupname);
  }
  return retval;
}




bool
WriteDocumentToString(DOMDocument *doc, std::string &buf) throw()
{
  bool success = false;
  InitializeXMLLibrary();

  try {
    // "LS" -> Load/Save extensions
    DOMImplementationLS* impl = (DOMImplementationLS*)
                                DOMImplementationRegistry::getDOMImplementation(ToXMLStr("LS"));

    DOMLSSerializer* writer = impl->createLSSerializer();

    try {
      // optionally you can set some features on this serializer
      if (writer->getDomConfig()->canSetParameter(XMLUni::fgDOMWRTDiscardDefaultContent, true))
          writer->getDomConfig()->setParameter(XMLUni::fgDOMWRTDiscardDefaultContent, true);
      if (writer->getDomConfig()->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true))
          writer->getDomConfig()->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);

      MemBufFormatTarget formatTarget;
      DOMLSOutput* lsOutput = impl->createLSOutput();
      lsOutput->setByteStream(&formatTarget);
      if (writer->write(doc, lsOutput)) {
          buf.append((const char *)formatTarget.getRawBuffer(),
          formatTarget.getLen());
        success = true;
      } else {
        notify(NFY_WARN, "Unable to write XML to string: Xerces didn't tell me why not.");
      }
        lsOutput->release();
      } catch (const XMLException& toCatch) {
        notify(NFY_WARN, "Unable to write XML: %s",
               XMLString::transcode(toCatch.getMessage()));
      } catch (const DOMException& toCatch) {
        notify(NFY_WARN, "Unable to write XML: %s",
               XMLString::transcode(toCatch.msg));
      } catch (...) {
        notify(NFY_WARN, "Unable to write XML: Unknown exception");
      }
      writer->release();
      } catch (const XMLException& toCatch) {
        notify(NFY_WARN, "Unable to create DOM Writer: %s",
               XMLString::transcode(toCatch.getMessage()));
      } catch (const DOMException& toCatch) {
        notify(NFY_WARN, "Unable to create DOM writer: %s",
               XMLString::transcode(toCatch.msg));
      } catch (...) {
        notify(NFY_WARN, "Unable to create DOM writer: Unknown exception");
      }
  return success;
}

khxml::DOMLSParser*
CreateDOMParser(void) throw()
{
  InitializeXMLLibrary();
  class FatalErrorHandler : public DOMErrorHandler {
   public:
    virtual bool handleError(const DOMError &err) {
      if (err.getSeverity() >= DOMError::DOM_SEVERITY_FATAL_ERROR) {
        char* message = XMLString::transcode(err.getMessage());
        notify(NFY_DEBUG, "XML Error: %s", message);
        XMLString::release(&message);
        throw DOMException(DOMException::SYNTAX_ERR);
      }
      return true;
    }
  };
  static FatalErrorHandler fatalHandler;

  try {
    // "LS" -> Load/Save extensions
    DOMImplementationLS* impl = (DOMImplementationLS*)
                                DOMImplementationRegistry::getDOMImplementation(ToXMLStr("LS"));
    DOMLSParser* parser =
      impl->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
    // optionally you can set some features on this builder
    if (parser->getDomConfig()->canSetParameter(XMLUni::fgDOMValidate, true))
      parser->getDomConfig()->setParameter(XMLUni::fgDOMValidate, true);
    if (parser->getDomConfig()->canSetParameter(XMLUni::fgDOMNamespaces, true))
      parser->getDomConfig()->setParameter(XMLUni::fgDOMNamespaces, true);
    parser->getDomConfig()->setParameter(XMLUni::fgDOMErrorHandler,
                                         &fatalHandler);
    return parser;
  } catch (...) {
    return 0;
  }
}

khxml::DOMDocument*
ReadDocument(khxml::DOMLSParser *parser, const std::string &filename) throw()
{

  DOMDocument* doc = nullptr;

  try {
    // Note: parseURI doesn't handle missing files nicely...returns
    // invalid doc object. Must check file existence ourselves.
    if (khExists(filename)) {
         doc = parser->parseURI(filename.c_str());
         tsMap::instance().insertRead(filename.c_str());
    } else {
      notify(NFY_WARN, "XML file does not exist: %s", filename.c_str());
    }
  } catch (const XMLException& toCatch) {
    notify(NFY_WARN, "Unable to read XML: %s",
           XMLString::transcode(toCatch.getMessage()));
  } catch (const DOMException& toCatch) {
    notify(NFY_WARN, "Unable to read XML: %s",
           XMLString::transcode(toCatch.msg));
  } catch (...) {
    notify(NFY_WARN, "Unable to read XML");
  }
  return doc;
}

khxml::DOMDocument*
ReadDocumentFromString(khxml::DOMLSParser *parser,
                       const std::string &buf,
                       const std::string &ref) throw()
{
  DOMDocument *doc = nullptr;

  try {
    MemBufInputSource memBufIS(
        (const XMLByte*)buf.data(),
        buf.size(),
        ref.c_str(),
        false);  // don't adopt buffer
    Wrapper4InputSource inputSource(&memBufIS,
                                    false);  // don't adopt input source
    doc = parser->parse(&inputSource);
  } catch (const XMLException& toCatch) {
    notify(NFY_WARN, "Unable to read XML: %s",
           XMLString::transcode(toCatch.getMessage()));
  } catch (const DOMException& toCatch) {
    notify(NFY_WARN, "Unable to read XML: %s",
           XMLString::transcode(toCatch.msg));
  } catch (...) {
    notify(NFY_WARN, "Unable to read XML");
  }
  return doc;
}

bool
DestroyDocument(khxml::DOMDocument *doc) throw()
{
  bool retval = false;
  try {
    doc->release();
    retval = true;
  } catch (...) {
  }
  return retval;
}


bool
DestroyParser(khxml::DOMLSParser *parser) throw()
{
  try {
    parser->release();
    return true;
  } catch (...) {
  }
  return false;
}
