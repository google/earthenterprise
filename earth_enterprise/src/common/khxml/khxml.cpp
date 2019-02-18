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
#include <memory>
#include <exception>
#include "common/khConfigFileParser.h"
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
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
bool terminateCache = false;
float percent;

const std::string INIT_HEAP_SIZE = "INIT_HEAP_SIZE";
const std::string MAX_HEAP_SIZE = "MAX_HEAP_SIZE";
const std::string BLOCK_SIZE = "BLOCK_SIZE";
const std::string PURGE = "PURGE";
const std::string PURGE_LEVEL = "PURGE_LEVEL";
const std::string XMLConfigFile = "/etc/opt/google/XMLparams";
static XMLSSize_t cacheCapacity = 0;
static std::array<std::string,5> options
{{
    INIT_HEAP_SIZE,
    MAX_HEAP_SIZE,
    BLOCK_SIZE,
    PURGE,
    PURGE_LEVEL
}};

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

class PercentError : public XmlParamsException
{
public:
    const char* what() const noexcept
    {
        return "Cache purging levels can only be 1-5";
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

    if (percent > 5 || percent < 1)
    {
        throw PercentError();
    }	
}

void setDefaultValues()
{
   initialDOMHeapAllocSize = 0x4000;
   maxDOMHeapAllocSize     = 0x20000;
   maxDOMSubAllocationSize = 0x1000;
   terminateCache = false;
   percent = 1.5;
}

void getInitValues()
{
    static bool callGuard = false;
    if (callGuard) return;
    callGuard = true;
    setDefaultValues();
    try
    {
        for(const auto& i : options)
        {
            config_parser.addOption(i);
        }
        config_parser.parse(XMLConfigFile.c_str());
        config_parser.validateIntegerValues();
        for (const auto& it : config_parser)
        {
            if (it.first == INIT_HEAP_SIZE)
                initialDOMHeapAllocSize = std::stol(it.second);
            else if (it.first == MAX_HEAP_SIZE)
                maxDOMHeapAllocSize = std::stol(it.second);
            else if (it.first == BLOCK_SIZE)
                maxDOMSubAllocationSize = std::stol(it.second);
            else if (it.first == PURGE)
                terminateCache = std::stoi(it.second);
            else
            {
                switch (std::stoi(it.second))
                {
                    case  1: percent = 0.00; break;
                    case  2: percent = 0.75; break;
                    case  3: percent = 1.50; break;
                    case  4: percent = 2.25; break;
                    case  5: percent = 3.00; break;
                    default: percent = 1.50;
                };
            }
        }
    }
    catch (const khConfigFileParserException& e)
    {
        setDefaultValues();
        notify(NFY_DEBUG, "%s , using default xerces init values", e.what());
    }
    try
    {
        validateXMLParameters();
    }
    catch (const XmlParamsException& e)
    {
        setDefaultValues();
        notify(NFY_DEBUG, "%s, using default xerces init values", e.what());
    }
}

void initXercesValues()
{
    getInitValues();
    try
    {
        XMLPlatformUtils::Initialize(initialDOMHeapAllocSize,
                                     maxDOMHeapAllocSize,
                                     maxDOMSubAllocationSize);
        notify(NFY_DEBUG, "XML initialization values: %s=%zu %s=%zu %s=%zu %s=%d %s=%f",
               "initialDOMHeapAllocSize", initialDOMHeapAllocSize,
               "maxDOMHeapAllocSize", maxDOMHeapAllocSize,
               "maxDOMSubAllocationSize", maxDOMSubAllocationSize,
               "purge cache", terminateCache,
               "level", percent);
    }
    catch (const XMLException& toCatch)
    {
        notify(NFY_FATAL, "Unable to initialize Xerces: %s",
               FromXMLStr(toCatch.getMessage()).c_str());
    }
}

static khMutexBase cacheLock = KH_MUTEX_BASE_INITIALIZER;
static khMutexBase reinitLock = KH_MUTEX_BASE_INITIALIZER;

// A read-write guard to aid in purging
//
// Before the creation of DOMDocuments and DOMLSParsers, a call to
// reInitIfNeeded() is made where it is determined whether or not
// to purge the XML cache.  There are 5 levels at which to purge
// the cache: 1 being the most frequent, 5 being the least frequent
// 
// Every time a DOMDocument enters into a write operation
// a "write lock" is made, which essentially is just keeping a count 
// of write operations remaining. When it is determined it is time 
// for a purge to occur, a "purge lock" is made.
//
// When a "purge lock" occurs, it waits for all "write locks" to finish
// and then takes control. It will execute all purge operations and then
// cede control  back to the control of write operations.
//
// TODO: generalize and move read-write lock to khThread

static pthread_rwlock_t purge_lock = PTHREAD_RWLOCK_INITIALIZER;

class XmlWriteLock
{
private:
    pthread_rwlock_t& lock;
public:
    XmlWriteLock(pthread_rwlock_t& _lock) : lock(_lock)
    {
        pthread_rwlock_rdlock(&lock);
    }
    
    ~XmlWriteLock()
    {
        pthread_rwlock_unlock(&lock);
    }
};

class PurgeLock
{
private:
    pthread_rwlock_t& lock;
public:
    PurgeLock(pthread_rwlock_t& _lock) : lock(_lock)
    {
        pthread_rwlock_wrlock(&lock);
    }

    ~PurgeLock()
    {
        pthread_rwlock_unlock(&lock);
    }
};

void ReInitializeXerces()
{
    khLockGuard guard(reinitLock);
    try
    {
        XMLPlatformUtils::Terminate();
    }
    catch(...)
    {
        notify(NFY_WARN, "Failure to terminate in ReInitializeXerces()");
    }
    initXercesValues();
}


// This is used only in the following function
class UsingXMLGuard
{
  friend void InitializeXMLLibrary() throw();

  UsingXMLGuard(void) throw() {
    khLockGuard guard(reinitLock);
    initXercesValues();
  }

  ~UsingXMLGuard(void) throw() {
    if (!terminateCache)
    {
        try {
            khLockGuard guard(reinitLock);
            XMLPlatformUtils::Terminate();
        } catch (...) {
            notify(NFY_DEBUG, "Failure to terminate in ~UsingXMLGuard");
        }
    }
  }
};

void  reInitIfReady()
{
  khLockGuard guard(cacheLock);
  static XMLSSize_t threashold = static_cast<XMLSSize_t>
         (maxDOMHeapAllocSize * percent); 
  if (cacheCapacity >= threashold)
  {
    PurgeLock pguard(purge_lock);
    ReInitializeXerces();
    cacheCapacity = 0;
  }
}

void InitializeXMLLibrary() throw()
{
  static UsingXMLGuard XMLLibGuard;
}



DOMDocument *
CreateEmptyDocument(const std::string &rootTagname) throw()
{
  InitializeXMLLibrary();
  
  if (terminateCache)
  {
    reInitIfReady();
  }
  try {
    DOMImplementation* impl =
    DOMImplementationRegistry::getDOMImplementation(0);

    DOMDocument* doc = impl->createDocument(0,// root element namespace URI.
                                            ToXMLStr(rootTagname),// root element name
                                            0);// document type object (DTD)
    return doc;
   } catch (...) {
     notify(NFY_DEBUG, "Error when trying to create DOMDocument.");
     return nullptr;
   }
}

namespace {
bool
WriteDocumentImpl(DOMDocument *doc, const std::string &filename) throw()
{
  // will initially be nullptr, reset to do a write lock
  // release lock when it goes out of scope at the end of
  // the function
  std::shared_ptr<XmlWriteLock> guard;
  if (terminateCache) 
  {
    guard.reset(new XmlWriteLock(purge_lock));
  }
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
    struct stat check;
    stat(filename.c_str(), &check);
    khLockGuard guard(cacheLock);
    cacheCapacity += check.st_size;  
  }

  if (retval && !khReplace(filename, newext, backupext)) {
    (void) khUnlink(newname);
    retval = false;
  }
  if (retval && khExists(backupname)) {
    (void) khUnlink(backupname);
  }
  return retval;
}




bool
WriteDocumentToString(DOMDocument *doc, std::string &buf) throw()
{
  std::shared_ptr<XmlWriteLock> guard;
  if (terminateCache)
  {
    guard.reset(new XmlWriteLock(purge_lock));
  }
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

      MemBufFormatTarget formatTarget;
      DOMLSOutput* lsOutput = impl->createLSOutput();
      lsOutput->setByteStream(&formatTarget);
      if (writer->write(doc, lsOutput)) {
          buf.append((const char *)formatTarget.getRawBuffer(),
          formatTarget.getLen());
          khLockGuard guard(cacheLock);
          cacheCapacity += buf.size();
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
  if (terminateCache)
  {
    reInitIfReady();
  }
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
    notify(NFY_DEBUG, "Error when trying to create DOMLSParser");
    return nullptr;
  }
}

khxml::DOMDocument*
ReadDocument(khxml::DOMLSParser *parser, const std::string &filename) throw()
{
  std::shared_ptr<XmlWriteLock> guard;
  if (terminateCache)
  {
    guard.reset(new XmlWriteLock(purge_lock));
  }

  DOMDocument* doc = nullptr;

  try {
    // Note: parseURI doesn't handle missing files nicely...returns
    // invalid doc object. Must check file existence ourselves.
    if (khExists(filename)) {
         doc = parser->parseURI(filename.c_str());
         // TODO: find out size of contents in doc, not immediately available
         struct stat file;
         stat(filename.c_str(), &file);
         khLockGuard guard(cacheLock);
         cacheCapacity += file.st_size;
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
  std::shared_ptr<XmlWriteLock> guard;
  if (terminateCache)
  {
    guard.reset(new XmlWriteLock(purge_lock));
  }

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
    khLockGuard guard(cacheLock);
    cacheCapacity += buf.size();
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
    notify(NFY_DEBUG, "Error when trying to release DOMDocument");
  }
  return retval;
}


bool
DestroyParser(khxml::DOMLSParser *parser) throw()
{
  bool retval = false;
  try {
    parser->release();
    retval = true;
  } catch (...) {
    notify(NFY_DEBUG, "Error when trying to release DOMLSParser");
  }
  return retval;
}
