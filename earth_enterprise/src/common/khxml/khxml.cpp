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


#include <notify.h>
#include <khFileUtils.h>
#include "khxml.h"
#include "khdom.h"
#include "common/khConfigFileParser.h"
#include <string>
#include <algorithm>
#include <exception>
#include <cstdlib>
#include <map>
#include <fstream>

using namespace khxml;

std::string
ListElementTagName(const std::string &tagname)
{
  if (tagname[tagname.length()-1] == 's') {
    return tagname.substr(0, tagname.length()-1);
  } else {
    return tagname + "_element";
  }
}

const std::string GEXMLObject::INIT_HEAP_SIZE = "INIT_HEAP_SIZE";
const std::string GEXMLObject::MAX_HEAP_SIZE = "MAX_HEAP_SIZE";
const std::string GEXMLObject::BLOCK_SIZE = "BLOCK_SIZE";
const std::string GEXMLObject::XMLConfigFile = "/etc/opt/google/XMLparams";
const std::array<std::string,3> GEXMLObject::options
{{
    INIT_HEAP_SIZE,
    MAX_HEAP_SIZE,
    BLOCK_SIZE,
}};

XMLSize_t GEXMLObject::initialDOMHeapAllocSize;
XMLSize_t GEXMLObject::maxDOMHeapAllocSize;
XMLSize_t GEXMLObject::maxDOMSubAllocationSize;

static GEXMLObject XMLObj; // Ensures Xerces is initialized

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

void GEXMLObject::validateXMLParameters()
{
  // using 1024 as the lowest setting
  const XMLSize_t lowestBlock = 0x400;
  
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

void GEXMLObject::setDefaultValues()
{
   initialDOMHeapAllocSize = 0x4000;
   maxDOMHeapAllocSize     = 0x20000;
   maxDOMSubAllocationSize = 0x1000;
}

void GEXMLObject::initializeXMLParameters() {
  static bool callGuard = false;
  if (callGuard) return;
  callGuard = true;

  std::ifstream file(XMLConfigFile.c_str());
  initializeXMLParametersFromStream(file);
}

// This function does not perform any synchronization. Callers must avoid
// concurrency issues.
void GEXMLObject::initializeXMLParametersFromStream(std::istream & input) {
  setDefaultValues();
  khConfigFileParser config_parser;
  try
  {
    for(const auto& i : options)
    {
      config_parser.addOption(i);
    }
    config_parser.parse(input);
    for (const auto& it : config_parser)
    {
      if (it.first == INIT_HEAP_SIZE)
        initialDOMHeapAllocSize = std::stol(it.second);
      else if (it.first == MAX_HEAP_SIZE)
        maxDOMHeapAllocSize = std::stol(it.second);
      else if (it.first == BLOCK_SIZE)
        maxDOMSubAllocationSize = std::stol(it.second);
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

GEXMLObject::GEXMLObject() {
  try {
    initializeXMLParameters();
    XMLPlatformUtils::Initialize(initialDOMHeapAllocSize,
                                  maxDOMHeapAllocSize,
                                  maxDOMSubAllocationSize);
    notify(NFY_DEBUG, "XML initialization values:\n"
                      "initialDOMHeapAllocSize=%zu\n"
                      "maxDOMHeapAllocSize=%zu\n"
                      "maxDOMSubAllocationSize=%zu\n",
           initialDOMHeapAllocSize,
           maxDOMHeapAllocSize,
           maxDOMSubAllocationSize);
  }
  catch (const XMLException& toCatch)
  {
    notify(NFY_FATAL, "Unable to initialize Xerces: %s",
           FromXMLStr(toCatch.getMessage()).c_str());
  }
}

std::unique_ptr<GEDocument>
CreateEmptyDocument(const std::string &rootTagname) throw()
{
  std::unique_ptr<GEDocument> doc(new GECreatedDocument(rootTagname));
  if (doc->valid()) {
    return doc;
  }
  else {
    return nullptr;
  }
}

GECreatedDocument::GECreatedDocument(const std::string & rootTagname) {
  try {
    DOMImplementation* impl =
        DOMImplementationRegistry::getDOMImplementation(0);
    doc = impl->createDocument(0,// root element namespace URI.
                               ToXMLStr(rootTagname),// root element name
                               0); // document type object (DTD)
  } catch (...) {
    notify(NFY_WARN, "Error when trying to create DOMDocument. Root tag name: %s",
           rootTagname.c_str());
    doc = nullptr;
  }
}

GECreatedDocument::~GECreatedDocument() {
  try {
    if (doc) {
      doc->release();
    }
  }
  catch (...) {
    notify(NFY_DEBUG, "Error when trying to release DOMDocument");
  }
}

bool GEDocument::writeToFile(const std::string &filename) {
  if (!valid()) return false;
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
        try {
          lsOutput->setByteStream(&formatTarget);
          if (writer->write(doc, lsOutput)) {
            success = true;
          } else {
            notify(NFY_WARN, "Unable to write %s: Xerces didn't tell me why not.",
                   filename.c_str());
          }
        } catch (...) {
          notify(NFY_WARN, "Unexpected error writing %s", filename.c_str());
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

bool
WriteDocument(GEDocument * doc, const std::string &filename) throw()
{
  bool retval = true;
  static const std::string newext = ".new";
  static const std::string backupext = ".old";
  const std::string newname = filename + newext;
  const std::string backupname = filename + backupext;
  if (!doc->writeToFile(newname)) {
    retval = false;
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
WriteDocumentToString(GEDocument *doc, std::string &buf) throw()
{
  return doc->writeToString(buf);
}

bool GEDocument::writeToString(std::string &buf) {
  if (!valid()) return false;
  bool success = false;
  try {
    // "LS" -> Load/Save extensions
    DOMImplementationLS* impl = static_cast<DOMImplementationLS*>(
                                DOMImplementationRegistry::getDOMImplementation(ToXMLStr("LS")));

    DOMLSSerializer* writer = impl->createLSSerializer();

    try {
      // optionally you can set some features on this serializer
      if (writer->getDomConfig()->canSetParameter(XMLUni::fgDOMWRTDiscardDefaultContent, true))
          writer->getDomConfig()->setParameter(XMLUni::fgDOMWRTDiscardDefaultContent, true);
      if (writer->getDomConfig()->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true))
          writer->getDomConfig()->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);

      MemBufFormatTarget formatTarget;
      DOMLSOutput* lsOutput = impl->createLSOutput();
      try {
        lsOutput->setByteStream(&formatTarget);
        if (writer->write(doc, lsOutput)) {
          buf.append((const char *)formatTarget.getRawBuffer(),
          formatTarget.getLen());
          success = true;
        } else {
          notify(NFY_WARN, "Unable to write XML to string: Xerces didn't tell me why not.");
        }
      } catch (...) {
        notify(NFY_WARN, "Unexpected error writing to string buffer.");
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

bool GEParsedDocument::FatalErrorHandler::handleError(const DOMError &err) {
  if (err.getSeverity() >= DOMError::DOM_SEVERITY_FATAL_ERROR) {
    char* message = XMLString::transcode(err.getMessage());
    notify(NFY_DEBUG, "XML Error: %s", message);
    XMLString::release(&message);
    throw DOMException(DOMException::SYNTAX_ERR);
  }
  return true;
}

void GEParsedDocument::CreateParser() {
  try {
    // "LS" -> Load/Save extensions
    DOMImplementationLS* impl = static_cast<DOMImplementationLS*>(
        DOMImplementationRegistry::getDOMImplementation(ToXMLStr("LS")));
    parser =
      impl->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS, 0);
    // optionally you can set some features on this builder
    if (parser->getDomConfig()->canSetParameter(XMLUni::fgDOMValidate, true))
      parser->getDomConfig()->setParameter(XMLUni::fgDOMValidate, true);
    if (parser->getDomConfig()->canSetParameter(XMLUni::fgDOMNamespaces, true))
      parser->getDomConfig()->setParameter(XMLUni::fgDOMNamespaces, true);
    parser->getDomConfig()->setParameter(XMLUni::fgDOMErrorHandler,
                                         &fatalErrorHandler);
  } catch (...) {
    notify(NFY_DEBUG, "Error when trying to create DOMLSParser");
    parser = nullptr;
  }
}

std::unique_ptr<GEDocument>
ReadDocument(const std::string &filename) throw()
{
  std::unique_ptr<GEDocument> doc(new GEParsedDocument(filename));
  if (doc->valid()) {
    return doc;
  }
  else {
    return nullptr;
  }
}

GEParsedDocument::GEParsedDocument(const std::string &filename) {
  CreateParser();

  try {
    if (parser) {
      // Note: parseURI doesn't handle missing files nicely...returns
      // invalid doc object. Must check file existence ourselves.
      if (khExists(filename)) {
        doc = parser->parseURI(filename.c_str());
      } else {
        notify(NFY_WARN, "XML file does not exist: %s", filename.c_str());
      }
    }
  } catch (const XMLException& toCatch) {
    notify(NFY_WARN, "Unable to read XML: %s",
           XMLString::transcode(toCatch.getMessage()));
    doc = nullptr;
  } catch (const DOMException& toCatch) {
    notify(NFY_WARN, "Unable to read XML: %s",
           XMLString::transcode(toCatch.msg));
    doc = nullptr;
  } catch (...) {
    notify(NFY_WARN, "Unable to read XML");
    doc = nullptr;
  }
}

std::unique_ptr<GEDocument>
ReadDocumentFromString(const std::string &buf,
                       const std::string &ref) throw()
{
  std::unique_ptr<GEDocument> doc(new GEParsedDocument(buf, ref));
  if (doc->valid()) {
    return doc;
  }
  else {
    return nullptr;
  }
}

GEParsedDocument::GEParsedDocument(const std::string &buf,
                                   const std::string &ref) {
  CreateParser();
  try {
    if (parser) {
      MemBufInputSource memBufIS(
          (const XMLByte*)buf.data(),
          buf.size(),
          ref.c_str(),
          false);  // don't adopt buffer
      Wrapper4InputSource inputSource(&memBufIS,
                                      false);  // don't adopt input source
      doc = parser->parse(&inputSource);
    }
  } catch (const XMLException& toCatch) {
    notify(NFY_WARN, "Unable to read XML: %s",
           XMLString::transcode(toCatch.getMessage()));
    doc = nullptr;
  } catch (const DOMException& toCatch) {
    notify(NFY_WARN, "Unable to read XML: %s",
           XMLString::transcode(toCatch.msg));
    doc = nullptr;
  } catch (...) {
    notify(NFY_WARN, "Unable to read XML");
    doc = nullptr;
  }
}

GEParsedDocument::~GEParsedDocument() {
  try {
    if (parser) {
      parser->release();
      // Don't release the document - releasing the parser will release the document
    }
  }
  catch (...) {
    notify(NFY_DEBUG, "Error when trying to release DOMLSParser");
  }
}

bool GEDocument::valid() const {
  return (doc != nullptr);
}

khxml::DOMElement * GEDocument::getDocumentElement() {
  if (valid()) {
    return doc->getDocumentElement();
  }
  else {
    return nullptr;
  }
}
