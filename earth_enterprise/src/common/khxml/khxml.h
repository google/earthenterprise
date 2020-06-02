/*
 * Copyright 2017 Google Inc.
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


#ifndef __KHXML_H
#define __KHXML_H

#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/Wrapper4InputSource.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <khThread.h>
#include <array>
#include <istream>
#include <memory>
#include <string>
#include <qstring.h>

namespace khxml = xercesc;

// ---------------------------------------------------------------------------
//    This is a simple class that lets us do easy trancoding of native strings
//  to XMLCh data.
//    Although it's a class you use it like a function.
//
//  Examples:
//      DOMElement* e = doc->createElement(ToXMLCh("extents"));
//      string val;
//      doc->createTextNode(ToXMLCh(val));
// ---------------------------------------------------------------------------
class ToXMLStr {
 public :
  ToXMLStr(const char* const toTranscode) {
    xmlch = khxml::XMLString::transcode(toTranscode);
  }
  ToXMLStr(const std::string &toTranscode) {
    xmlch = khxml::XMLString::transcode(toTranscode.c_str());
  }

  ~ToXMLStr() {
    khxml::XMLString::release(&xmlch);
  }

  operator const XMLCh* () const {
    return xmlch;
  }

 private :
  XMLCh* xmlch;

  // private and unimplemented to prohibit copying, assigning
  ToXMLStr(const ToXMLStr &);
  ToXMLStr& operator=(const ToXMLStr&);
};


// ---------------------------------------------------------------------------
//    This is a simple function that lets us do easy trancoding of XMLCh data
//  to native strings.
//    BEWARE: The resulting string may be multibyte encoded. Character
//  manipulations on the result string will only work if the source string is
//  entirely representable as single characters.
//
//    ANOTHER BEWARE: Use XMLStr2QString when you know that the input contains
//  UCS-2 or any other non-ascii chars.
//
//  Examples:
// ---------------------------------------------------------------------------
inline
std::string
FromXMLStr(const XMLCh *xmlch)
{
  char *native = khxml::XMLString::transcode(xmlch);
  std::string ret(native);
  khxml::XMLString::release(&native);
  return ret;
}


// ----------------------------------------------------------------------
//    Use XMLStr2QString when you know that the input contains
//  UCS-2 or any other non-ascii chars.
//
// ---------------------------------------------------------------------
inline
QString
XMLStr2QString(const XMLCh *xmlch)
{
  return QString::fromUcs2((const ushort*)xmlch);
}

// ----------------------------------------------------------------------
// The GEXMLObject classes and its sub-classes wrap calls to the Xerces
// library and ensure that the library is initialized before XML
// functions are called and terminated when we're done working with XML.
// There are currently two exceptions to this wrapping, both of which
// are safe.
// 1) The GEDocument class returns DOMElements from some of its
//    functions that are used directly by other code. DOMElements are
//    only valid while their document is valid, so we don't have to
//    worry about calling terminate because we're done with the document
//    while we're still using one of its elements.
// 2) This code and the external code that uses DOMElements can throw
//    XMLExceptions or DOMExceptions that are handled by other code. In
//    all relevant cases, the GEXMLObject remains valid while the
//    exception is handled.
// We could fix the above exceptions to the Xerces wrappers but it would
// take significant effort. Thus, we have settled on this 80/20 solution
// that provides the majority of the benefit for a relatively small
// amount of work.
// ----------------------------------------------------------------------
class GEXMLObject {
  private:
    const static std::string INIT_HEAP_SIZE;
    const static std::string MAX_HEAP_SIZE;
    const static std::string BLOCK_SIZE;
    const static std::string XMLConfigFile;
    const static std::array<std::string,3> options;

    static XMLSize_t initialDOMHeapAllocSize;
    static XMLSize_t maxDOMHeapAllocSize;
    static XMLSize_t maxDOMSubAllocationSize;

    static void initializeXMLParameters();
    static void validateXMLParameters();
    static void setDefaultValues();
  public:
    GEXMLObject();
    // Allow users to specify the initialization file
    static void initializeXMLParametersFromStream(std::istream &);
};

class GEDocument {
  protected:
    khxml::DOMDocument * doc = nullptr;
    // Don't instantiate this class directly. Instead, use one of its sub-classes.
    GEDocument() = default;
  public:
    bool valid() const;
    khxml::DOMElement * getDocumentElement();
    bool writeToFile(const std::string &);
    bool writeToString(std::string &);
    virtual ~GEDocument() = default;
};

class GECreatedDocument : public GEDocument {
  public:
    GECreatedDocument(const std::string &);
    virtual ~GECreatedDocument();
};

class GEParsedDocument : public GEDocument {
  private:
    class FatalErrorHandler : public khxml::DOMErrorHandler {
     public:
      virtual bool handleError(const khxml::DOMError &err);
    };
    FatalErrorHandler fatalErrorHandler;
    khxml::DOMLSParser * parser = nullptr;
    void CreateParser();
  public:
    GEParsedDocument(const std::string &);
    GEParsedDocument(const std::string &, const std::string &);
    virtual ~GEParsedDocument();
};

#endif /* __KHXML_H */
