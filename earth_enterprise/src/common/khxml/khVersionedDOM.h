/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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

//

#ifndef COMMON_KHXML_KHVERSIONEDDOM_H__
#define COMMON_KHXML_KHVERSIONEDDOM_H__

#include "geVersionedEnum.h"
#include <cstdint>

// NOTICE!!
// There are intentionally no other includes here. This must be included
// after all the FromElement/ToElement definitions in your .cpp file. They
// will already need the same #includes that this file will need. By not
// putting the includes here, the compiler will catch you if you include
// this at the top of your .cpp instead of near the bottom where it should
// be.

char const * const VersionAttrName = "_version";

template <class T0, class T1>
void Get2VersionedElement(khxml::DOMElement *parent,
                          const std::string &tagname,
                          T1 &val)
{
  khxml::DOMElement *elem = GetFirstNamedChild(parent, tagname);
  if (!elem) {
    throw khDOMException(khDOMException::MissingElement,
                         FromXMLStr(parent->getTagName()),
                         tagname);
  }

  unsigned int version = 0;
  GetAttributeOrDefault(elem, VersionAttrName, version, uint(0));

  switch (version) {
    case 0: {
      T0 tmp0;
      FromElement(elem, tmp0);
      val = T1(tmp0, geVersionedUpgrade);
      break;
    }
    case 1:
      FromElement(elem, val);
      break;
    default:
      throw khDOMException(khDOMException::InvalidVersion,
                           tagname, ToString(version));
  }
}

template <class T0, class T1, class T2>
void Get3VersionedElement(khxml::DOMElement *parent,
                          const std::string &tagname,
                          T2 &val)
{
  khxml::DOMElement *elem = GetFirstNamedChild(parent, tagname);
  if (!elem) {
    throw khDOMException(khDOMException::MissingElement,
                         FromXMLStr(parent->getTagName()),
                         tagname);
  }

  unsigned int version = 0;
  GetAttributeOrDefault(elem, VersionAttrName, version, uint(0));

  switch (version) {
    case 0: {
      T0 tmp0;
      FromElement(elem, tmp0);
      val = T2(T1(tmp0, geVersionedUpgrade), geVersionedUpgrade);
      break;
    }
    case 1: {
      T1 tmp1;
      FromElement(elem, tmp1);
      val = T2(tmp1, geVersionedUpgrade);
      break;
    }
    case 2:
      FromElement(elem, val);
      break;
    default:
      throw khDOMException(khDOMException::InvalidVersion,
                           tagname, ToString(version));
  }
}

template <class T>
void AddVersionedElement(khxml::DOMElement *parent, const std::string &tagname,
                         unsigned int version, const T &value)
{
  khxml::DOMElement* elem =
    parent->getOwnerDocument()->createElement(ToXMLStr(tagname));
  ToElement(elem, value);
  AddAttribute(elem, VersionAttrName, version);
  parent->appendChild(elem);
}



#endif // COMMON_KHXML_KHVERSIONEDDOM_H__
