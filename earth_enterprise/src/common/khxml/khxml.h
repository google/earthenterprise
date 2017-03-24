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

#include <string>
#include <xercesc/util/XMLString.hpp>
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
  return QString::fromUcs2(xmlch);
}


#endif /* __KHXML_H */
