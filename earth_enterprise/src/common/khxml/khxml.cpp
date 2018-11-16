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
#include <fstream>
#include <string>
using namespace khxml;

class XMLParser
{};

std::string
ListElementTagName(const std::string &tagname)
{
  if (tagname[tagname.length()-1] == 's') {
    return tagname.substr(0, tagname.length()-1);
  } else {
    return tagname + "_element";
  }
}


// This is used only in the following function
class UsingXMLGuard
{
  friend void InitializeXMLLibrary(bool) throw();

  XMLSSize_t  initialDOMHeapAllocSize;
    XMLSSize_t  maxDOMHeapAllocSize;
    XMLSSize_t  maxDOMSubAllocationSize;
  UsingXMLGuard(void) throw()
  {
      std::string fn("/home/ec2-user/xerces_init_defaults.txt");
         try {
             std::ifstream file;
             file.exceptions(std::ifstream::failbit);
             file.open(fn.c_str());
             file >> initialDOMHeapAllocSize
                  >> maxDOMHeapAllocSize
                  >> maxDOMSubAllocationSize;
             file.close();
         } catch (std::ifstream::failure &readError) {
             initialDOMHeapAllocSize = 0x4000;
             maxDOMHeapAllocSize = 0x20000;
             maxDOMSubAllocationSize = 0x1000;
             notify(NFY_WARN,"xerces heap defaults file %s not found!",
                    fn.c_str());
         }
   try {
      XMLPlatformUtils::Initialize(/*MiscConfig::Instance().*/initialDOMHeapAllocSize,
                                   /*MiscConfig::Instance().*/maxDOMHeapAllocSize,
                                   /*MiscConfig::Instance().*/maxDOMSubAllocationSize);
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

//  ------------------------------------------------------------------
//  From: https://xml.apache.org/xerces-c-new/faq-parse.html#faq-4
//
//  Is it OK to call the XMLPlatformUtils::Initialize/Terminate pair
//  of routines multiple times in one program?
//
//  Yes. Since Xerces-C++ 1.5.2, the code has been enhanced so that
//  calling XMLPlatformUtils::Initialize/Terminate pair of routines
//  multiple times in one process is now allowed.
//
//  But the application needs to guarantee that only one thread has
//  entered either the method XMLPlatformUtils::Initialize() or the
//  method XMLPlatformUtils::Terminate() at any one time.
//
//  If you are calling XMLPlatformUtils::Initialize() a number of
//  times, and then follow with XMLPlatformUtils::Terminate() the same
//  number of times, only the first XMLPlatformUtils::Initialize() will
//  do the initialization, and only the last XMLPlatformUtils::Terminate()
//  will clean up the memory. The other calls are ignored.
//
//  To ensure all the memory held by the parser are freed, the number of
//  XMLPlatformUtils::Terminate() calls should match the number
//  of XMLPlatformUtils::Initialize() calls.
//  -------------------------------------------------------------------
    void ReInitXerces(void) throw()
    {
        try {
            XMLPlatformUtils::Terminate();
        } catch (const XMLException& toCatch) {
            notify(NFY_FATAL,"Unable to Terminate and ReInitialize Xerces: %s",
                   FromXMLStr(toCatch.getMessage()).c_str());
        }
        try {
            XMLPlatformUtils::Initialize(/*MiscConfig::Instance().*/initialDOMHeapAllocSize,
                                         /*MiscConfig::Instance().*/maxDOMHeapAllocSize,
                                         /*MiscConfig::Instance().*/maxDOMSubAllocationSize);
        } catch (const XMLException& toCatch) {
            notify(NFY_FATAL,"Unable to ReInitialize Xerces: %s",
                   FromXMLStr(toCatch.getMessage()).c_str());
        }
       notify(NFY_NOTICE,"successful xerces reinit...");
    }

};

static khMutexBase xmlLibLock = KH_MUTEX_BASE_INITIALIZER;
static khMutexBase reInitOuterLock = KH_MUTEX_BASE_INITIALIZER;
static khMutexBase reInitInnerLock = kH_MUTEX_BASE_RECURSIVE;//;kH_MUTEX_BASE_RECURSIVE;


void InitializeXMLLibrary(bool reinit=false) throw()
{
  khLockGuard guard(xmlLibLock);
  static UsingXMLGuard XMLLibGuard;
  if (reinit) XMLLibGuard.ReInitXerces();
}

// Logic:
//
// $ kill -1 <PID>
//
// 1. ReInitializeXMLLibrary declared to handle SIGHUP signal in
//    UsingXMLGuard::UsingXMLGuard
// 2. ReInitializeXMLLibrary handles this signal and calls
//    InitializeXMLLibrary, and uses a flag to signify reinit
// 3. InitializeXMLLibrary has the lock and calls UsingXMLGuard::ReInitXerces
//    which calls XMLPlatformUtils::Terminate, then Initialize

void ReInitializeXMLLibrary(int sig) throw()
{
    khLockGuard outer(reInitOuterLock);
    khLockGuard inner(reInitInnerLock);
    InitializeXMLLibrary(true);
}


DOMDocument *
CreateEmptyDocument(const std::string &rootTagname) throw()
{
  khLockGuard outer(reInitOuterLock);
  {
      khLockGuard inner(reInitInnerLock);
      InitializeXMLLibrary();

      try {
        DOMImplementation* impl =
          DOMImplementationRegistry::getDOMImplementation(0);

        DOMDocument* doc =
          impl->createDocument( 0,// root element namespace URI.
                                ToXMLStr(rootTagname),// root element name
                                0);// document type object (DTD)
        return doc;
      } catch (...) {
        return 0;
      }
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
  khLockGuard outer(reInitOuterLock);
  bool retval = true;
  {
      khLockGuard inner(reInitInnerLock);
      static const std::string newext = ".new";
      static const std::string backupext = ".old";
      const std::string newname = filename + newext;
      const std::string backupname = filename + backupext;
      if (!WriteDocumentImpl(doc, newname)) {
        retval = false;
      }

      if (retval && !khReplace(filename, newext, backupext)) {
        (void) khUnlink(newname);
        retval = false;
      }
      if (khExists(backupname)) {
        notify(NFY_VERBOSE,"WriteDocument() backupname %s exists", backupname.c_str());
        (void) khUnlink(backupname);
      }
  }
  return retval;
}




bool
WriteDocumentToString(DOMDocument *doc, std::string &buf) throw()
{
  khLockGuard outer(reInitOuterLock);
  bool success = false;
  {
      khLockGuard inner(reInitInnerLock);
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
  khLockGuard outer(reInitOuterLock);
  DOMDocument* doc = nullptr;
      {
      khLockGuard inner(reInitInnerLock);

      try {
        // Note: parseURI doesn't handle missing files nicely...returns
        // invalid doc object. Must check file existence ourselves.
        if (khExists(filename)) {
            notify(NFY_NOTICE,"ReadDocument, filename: %s",
                   filename.c_str());
             doc = parser->parseURI(filename.c_str());
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
  }
  return doc;
}

khxml::DOMDocument*
ReadDocumentFromString(khxml::DOMLSParser *parser,
                       const std::string &buf,
                       const std::string &ref) throw()
{
  khLockGuard outer(reInitOuterLock);
  DOMDocument *doc = nullptr;
  {
      khLockGuard inner(reInitInnerLock);

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
  }
  return doc;
}

bool
DestroyDocument(khxml::DOMDocument *doc) throw()
{
  try {
    doc->release();
    return true;
  } catch (...) {
  }
  return false;
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
