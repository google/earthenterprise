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
#include <queue>
#include "common/khConfigFileParser.h"
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <chrono>
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
const std::string PERCENT = "PERCENT";
const std::string XMLConfigFile = "/etc/opt/google/XMLparams";
static XMLSSize_t cacheCapacity = 0;
static std::array<std::string,5> options
{{
    INIT_HEAP_SIZE,
    MAX_HEAP_SIZE,
    BLOCK_SIZE,
    PURGE,
    PERCENT
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


/**************************************************************
 * helper class for synchronizing events related to purging
 * the Xerces cache
 *
 * keeps an internal count and a notifier as to whether or
 * not the mutex on creating docs/parsers is currently locked
 *
 * singeton class, thread-safe
**************************************************************/
class terminateGuard
{
private:
    static khMutexBase qMutex, lockMutex;
    static uint32_t numObjs, numDocs, numParsers;
    static uint64_t totalNumDocs;
    static bool objLock;
    terminateGuard() = default;

public:
    static terminateGuard& instance()
    {
        static terminateGuard _instance;
        return _instance;
    }

    static void addObj(int type)
    {
        khLockGuard guard(qMutex);
        ++numObjs;
        if (type == 0) ++numDocs;
        else ++numParsers;
        ++totalNumDocs;
    }

    static void removeObj(int type)
    {
        khLockGuard guard(qMutex);
        if (numObjs > 0) --numObjs;
        else return;
        if (type == 0) --numDocs;
        else --numParsers;
    }

    static uint32_t getNumDocs()
    {
        khLockGuard guard(qMutex);
        return numDocs;
    }

    static uint32_t getNumParsers()	
    {
        khLockGuard guard(qMutex);
        return numParsers;
    }

    static uint32_t size()
    {
        khLockGuard guard(qMutex);
        return numObjs;
    }

    static uint64_t getTotalNumProcessed()
    {
        khLockGuard guard(qMutex);
        return totalNumDocs;
    }
    static void lock()
    {
        khLockGuard guard(lockMutex);
        objLock = true;
    }

    static void unlock()
    {
        khLockGuard guard(lockMutex);
        objLock = true;
    }

    static bool isLocked()
    {
        khLockGuard guard(lockMutex);
        return objLock;
    }
};

khMutexBase terminateGuard::qMutex = KH_MUTEX_BASE_INITIALIZER; 
khMutexBase terminateGuard::lockMutex = KH_MUTEX_BASE_INITIALIZER; 
uint32_t terminateGuard::numObjs = 0;
uint32_t terminateGuard::numDocs = 0;
uint32_t terminateGuard::numParsers = 0;
uint64_t terminateGuard::totalNumDocs = 0;
bool terminateGuard::objLock = false;

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
   terminateCache = false;
   percent = 1.0;
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
                terminateCache = std::stol(it.second);
            else
            {
                percent = static_cast<float>(std::stol(it.second)/100.0);
            }
        }
    }
    catch (const khConfigFileParserException& e)
    {
        notify(NFY_DEBUG, "%s , using default xerces init values", e.what());
    }
    try
    {
        validateXMLParameters();
    }
    catch (const XMLException& e)
    {
        setDefaultValues();
        notify(NFY_DEBUG, "%s, using default xerces init values",
               FromXMLStr(e.getMessage()).c_str());
    }
}

#include <cassert>

void initXercesValues()
{
    getInitValues();
    try
    {
        XMLPlatformUtils::Initialize(initialDOMHeapAllocSize,
                                     maxDOMHeapAllocSize,
                                     maxDOMSubAllocationSize);
        notify(NFY_DEBUG, "XML initialization values: %s=%zu %s=%zu %s=%zu %s=%d",
               "initialDOMHeapAllocSize", initialDOMHeapAllocSize,
               "maxDOMHeapAllocSize", maxDOMHeapAllocSize,
               "maxDOMSubAllocationSize", maxDOMSubAllocationSize,
               "purge cache", terminateCache);
    }
    catch (const XMLException& toCatch)
    {
        notify(NFY_FATAL, "Unable to initialize Xerces: %s",
               FromXMLStr(toCatch.getMessage()).c_str());
    }
}

void ReInitializeXerces()
{
    XMLPlatformUtils::Terminate();
    initXercesValues();
}

void handleSIGABRT(int signal)
{
    notify(NFY_DEBUG, "number of active objs %d, total objs %lu, num docs %d, num parsers %d",
           terminateGuard::instance().size(),
           terminateGuard::instance().getTotalNumProcessed(),
           terminateGuard::instance().getNumDocs(),
           terminateGuard::instance().getNumParsers());
}

void registerSig()
{
    static bool once = false;
    if (once) return;
    once = true;
    notify(NFY_DEBUG, "registering signal...");
    signal(SIGABRT,handleSIGABRT);
}

// This is used only in the following function
class UsingXMLGuard
{
  friend void InitializeXMLLibrary() throw();

  UsingXMLGuard(void) throw() {
    initXercesValues();
  }

  ~UsingXMLGuard(void) throw() {
    try {
      XMLPlatformUtils::Terminate();
    } catch (...) {
    }
  }
};

static khMutexBase xmlLibLock = KH_MUTEX_BASE_INITIALIZER;
static khMutexBase checkTermLock = KH_MUTEX_BASE_INITIALIZER;
//static khMutexBase createLock = KH_MUTEX_BASE_INITIALIZER;

// only terminate when certain conditions are met
//bool
void  readyForTerm()
{
  //bool retval = false;
  khLockGuard guard(checkTermLock);
  static XMLSSize_t threashold = static_cast<XMLSSize_t>
         (maxDOMHeapAllocSize * percent); 
  registerSig();
  if (cacheCapacity >= threashold)
  {
    //while (terminateGuard::instance().size());
    notify(NFY_DEBUG, "READY TO PURGE XERCES CACHE!!!");
    auto start = std::chrono::system_clock::now();
    while (terminateGuard::instance().size())
    {
        auto end = std::chrono::system_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(end-start).count() >= 3)
            return;
    }
    ReInitializeXerces();
    notify(NFY_DEBUG, "XERCES CACHE PURGED!!!");
    cacheCapacity = 0;
    //retval = true;
  }
  //return retval;
}

void InitializeXMLLibrary() throw()
{
  khLockGuard guard(xmlLibLock);
  static UsingXMLGuard XMLLibGuard;
}

//static khMutexBase createLock = KH_MUTEX_BASE_INITIALIZER;

DOMDocument *
CreateEmptyDocument(const std::string &rootTagname) throw()
{
  InitializeXMLLibrary();

  if (terminateCache)
  {
    readyForTerm();
    //khLockGuard guard(createLock);
    //if (readyForTerm()) 
    //{
      //if (terminateGuard::instance().isLocked()) 
      //    notify(NFY_DEBUG, "is locked [CreateEmptyDocument]");
      //xmlLibLock.Lock();
      //terminateGuard::instance().lock();
      //notify(NFY_DEBUG, "READY FOR PURGE [CreateEmptyDocument], %s%d",
      //       "Number of active objs: ", terminateGuard::instance().size());
    //}
    //terminateGuard::instance().addObj();
  }
  try {
    DOMImplementation* impl =
    DOMImplementationRegistry::getDOMImplementation(0);

    DOMDocument* doc = impl->createDocument(0,// root element namespace URI.
                                            ToXMLStr(rootTagname),// root element name
                                            0);// document type object (DTD)
    terminateGuard::instance().addObj(0);
    return doc;
   } catch (...) {
     //if (terminateCache)
     //{
     //  xmlLibLock.Unlock();
     //  terminateGuard::instance().unlock();
     //  terminateGuard::instance().removeObj();
     //}
     return 0;
   }
}

namespace {
bool
WriteDocumentImpl(DOMDocument *doc, const std::string &filename) throw()
{

  //InitializeXMLLibrary();
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
    cacheCapacity += check.st_size;  
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
  //InitializeXMLLibrary();

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
  //notify(NFY_DEBUG, "creating parser, obj # %lu", terminateGuard::instance().getTotalNumProcessed() + 1);
  if (terminateCache)
  {
    readyForTerm();
    //khLockGuard guard(createLock);
    //if (terminateGuard::instance().isLocked())
    //    notify(NFY_DEBUG, "is locked [CreateDOMParser"); 
    //if (readyForTerm())
    //{
    //   xmlLibLock.Lock();
    //   notify(NFY_DEBUG, "READY FOR PURGE [CreateDOMParser] %s%d",
    //         "Number of active objs: ", terminateGuard::instance().size());
    //   terminateGuard::instance().lock();
    //}
    //terminateGuard::instance().addObj();
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
    terminateGuard::instance().addObj(1);
    return parser;
  } catch (...) {
    //if (terminateCache)
    //{
    //  xmlLibLock.Unlock();
    //  terminateGuard::instance().unlock();
    //  terminateGuard::instance().removeObj();
    //}
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
         // TODO: find out size of contents in doc, not immediately available
         struct stat file;
         stat(filename.c_str(), &file);
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


void terminate(std::string loc)
{
	notify(NFY_DEBUG, "in [%s]", loc.c_str());
    //terminateGuard::instance().removeObj();
    if (terminateGuard::instance().isLocked())
    {
        notify(NFY_DEBUG, "in [%s], its locked. num docs %d",
               loc.c_str(), terminateGuard::instance().size());
        if (terminateGuard::instance().size() == 0)
        {
            notify(NFY_DEBUG, "XML cache purge from [%s], total num docs %lu",
                   loc.c_str(), terminateGuard::instance().getTotalNumProcessed());
            ReInitializeXerces();
            terminateGuard::instance().unlock();
            xmlLibLock.Unlock();
        }
        notify(NFY_DEBUG, "leaving [%s] while locked", loc.c_str());
    }
}

// want to purge when there are no active objects and when
// we know that there is a lock on creating new objects
// 
// ideally, when those conditions are met, 
bool
DestroyDocument(khxml::DOMDocument *doc) throw()
{
  bool retval = false;
  try {
    doc->release();
    retval = true;
  } catch (...) {
  }
  if (terminateCache)
  {
  //      khLockGuard guard(checkTermLock);
        /*notify(NFY_DEBUG, "in [DestroyDocument]");
        terminateGuard::instance().removeObj();
        if (terminateGuard::instance().isLocked()) {
            notify(NFY_DEBUG, "in [DestroyDocument], its locked. num docs %d",
                   terminateGuard::instance().size());
            if (terminateGuard::instance().size() == 0)
            {
                notify(NFY_DEBUG, "XML cache purge from DestroyDocument(), total num docs %lu", 
                       terminateGuard::instance().getTotalNumProcessed());
                ReInitializeXerces();
                terminateGuard::instance().unlock();
                xmlLibLock.Unlock();
            }
            notify(NFY_DEBUG, "leaving [DestroyDocument] while locked");
		}*/
  //      terminate("DestroyDocument");
  
    terminateGuard::instance().removeObj(0);
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
  }
  if (terminateCache)
  {
        //khLockGuard guard(checkTermLock);
        /*terminateGuard::instance().removeObj();
        if (terminateGuard::instance().isLocked()) 
        {
           notify(NFY_DEBUG, "in [DestroyParser], its locked. num docs %d",
                  terminateGuard::instance().size());
            if (terminateGuard::instance().size() == 0)
            {
                notify(NFY_WARN, "XML cache purge from DestroyParser(), total num docs %lu", 
                       terminateGuard::instance().getTotalNumProcessed());
                ReInitializeXerces();
                terminateGuard::instance().unlock();
                xmlLibLock.Unlock();
            }
            notify(NFY_DEBUG, "leaving [DestroyParser] while locked");
        }*/
        //terminate("DestroyParser");
        terminateGuard::instance().removeObj(1);
  }
  return retval;
}
