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


#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_KHXML_KHDOM_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_KHXML_KHDOM_H_

#include "common/khxml/khxml.h"

#include <sstream>
#include <vector>
#include <deque>
#include <list>
#include <set>
#include <map>
#include <memory>
#include <cstdint>

#include <Qt/qstring.h>
#include <Qt/qcolor.h>
#include <xercesc/dom/DOM.hpp>

//#include "common/khTypes.h"
#include <cstdint>
#include "common/khExtents.h"
#include "common/khTileAddr.h"
#include "common/khInsetCoverage.h"
#include "common/verref_storage.h"
#include "common/khstrconv.h"
#include "common/notify.h"
#include "common/khMetaData.h"
#include "common/khException.h"
#include "common/Defaultable.h"
#include "common/EncryptedQString.h"
#include "common/geRange.h"
#include "common/SharedString.h"


/******************************************************************************
 ***  Some general helper funcs
 ******************************************************************************/
extern std::string ListElementTagName(const std::string &tagname);

class khDOMException : public khException
{
 public:
  enum Type { MissingElement, MissingText, InvalidFieldValue,
              MissingAttribute, InvalidAttribute, InvalidAttributeValue,
              InvalidVersion };

  khDOMException(Type t,
                 const std::string &s1,
                 const std::string &s2 = std::string())
      : khException(ComposeErrorString(t, s1, s2)) { }

  static QString
  ComposeErrorString(Type t, const std::string &s1,
                     const std::string &s2)
  {
    QString qs1(QString::fromUtf8(s1.c_str()));
    QString qs2(QString::fromUtf8(s2.c_str()));
    switch (t) {
      case MissingElement:
        return kh::tr("Element '%1' missing '%2'").arg(qs1, qs2);
      case MissingText:
        return kh::tr("Element '%1' missing text").arg(qs1);
      case InvalidFieldValue:
        return kh::tr("Field '%1' has invalid value '%2'").arg(qs1, qs2);
      case MissingAttribute:
        return kh::tr("Element '%1' missing attribute '%2'").arg(qs1, qs2);
      case InvalidAttribute:
        return kh::tr("Element '%1' invalid attribute '%2'").arg(qs1, qs2);
      case InvalidAttributeValue:
        return kh::tr("Attribute '%1' invalid value '%2'")
          .arg(qs1, qs2);
      case InvalidVersion:
        return kh::tr("Element '%1' invalid version '%2'")
          .arg(qs1, qs2);
    }
    return QString();  // keep warnings at bay
  }
};




/******************************************************************************
 ***  some helper functions for DOM writing
 ******************************************************************************/
inline
void
ToElement(khxml::DOMElement *elem, const QString &value)
{
  elem->appendChild(
      elem->getOwnerDocument()->createTextNode((const XMLCh*)value.utf16()));
}

inline
void
ToElement(khxml::DOMElement *elem, const EncryptedQString &value)
{
  elem->setAttribute(ToXMLStr("method"), ToXMLStr("simple"));
  QString tmp = value;
  for (auto i = 0; i < tmp.length(); ++i) {
    tmp[i] = tmp[i].unicode() + 13;
  }
  elem->appendChild(
      elem->getOwnerDocument()->createCDATASection((const XMLCh*)tmp.utf16()));
}

template <class T>
void
ToElement(khxml::DOMElement *elem, const T &value)
{
  std::string str = ToString(value);
  elem->appendChild(elem->getOwnerDocument()->createTextNode(ToXMLStr(str)));
}


template <class T>
void
ToElementWithChildName(khxml::DOMElement *elem,
                       const std::string &childTagName,
                       const T &container);

inline void
ToElementWithChildName(khxml::DOMElement *elem,
                       const std::string &childTagName,
                       const VerRefGen &val);

template <class T>
void
ToElement(khxml::DOMElement *elem, const std::vector<T> &vec);

template <class T>
void
ToElement(khxml::DOMElement *elem, const std::deque<T> &deque);

template <class T>
void
ToElement(khxml::DOMElement *elem, const std::list<T> &list);

template <class T>
void
ToElement(khxml::DOMElement *elem, const std::set<T> &set);

template <class T, class U>
void
ToElement(khxml::DOMElement *elem, const std::map<T, U> &map);

template <class U>
void
ToElement(khxml::DOMElement *elem, const std::map<std::string, U> &map);

template <class U>
void
ToElement(khxml::DOMElement *elem, const std::map<QString, U> &map);

template <class U>
void
ToElement(khxml::DOMElement *elem, const std::map<SharedString, U> &map);

template <class T>
void
ToElement(khxml::DOMElement *elem, const khSize<T> &size);

template <class T>
void
ToElement(khxml::DOMElement *elem, const khOffset<T> &offset);

template <class T>
void
ToElement(khxml::DOMElement *elem, const khExtents<T> &extents);

inline void
ToElement(khxml::DOMElement *elem, const khLevelCoverage &cov);

inline void
ToElement(khxml::DOMElement *elem, const khInsetCoverage &cov);

inline void
ToElement(khxml::DOMElement *elem, const khMetaData &meta);

inline void
ToElement(khxml::DOMElement *elem, const QColor &color);

template <class T>
inline void
ToElement(khxml::DOMElement *elem, const geRange<T> &range);

inline
void
AddAttribute(khxml::DOMElement *elem, const std::string &attrname,
             const QString &value)
{
  elem->setAttribute(ToXMLStr(attrname), (const XMLCh*)value.utf16());
}

inline
void
AddAttribute(khxml::DOMElement *elem, const std::string &attrname,
             const std::string &str)
{
  elem->setAttribute(ToXMLStr(attrname), ToXMLStr(str));
}

template <class T>
void
AddAttribute(khxml::DOMElement *elem, const std::string &attrname,
             const T &value)
{
  std::string str = ToString(value);
  elem->setAttribute(ToXMLStr(attrname), ToXMLStr(str));
}

template <class T>
void
AddElement(khxml::DOMElement *parent, const std::string &tagname,
           const T &value)
{
  khxml::DOMElement* elem =
    parent->getOwnerDocument()->createElement(ToXMLStr(tagname));
  ToElement(elem, value);
  parent->appendChild(elem);
}

template <class T>
khxml::DOMElement*
AddElementWithChildName(khxml::DOMElement *parent,
                        const std::string &tagname,
                        const std::string &childTagname,
                        const T &value)
{
  khxml::DOMElement* elem =
    parent->getOwnerDocument()->createElement(ToXMLStr(tagname));
  ToElementWithChildName(elem, childTagname, value);
  parent->appendChild(elem);
  return elem;
}

// Only used for maps where the key is a QString
template <class T>
khxml::DOMElement*
AddUTF8Element(khxml::DOMElement *parent,
               const QString &tagname,
               const T &value)
{
  khxml::DOMElement* elem =
    parent->getOwnerDocument()->createElement
    ((const XMLCh*)tagname.utf16());
  ToElement(elem, value);
  parent->appendChild(elem);
  return elem;
}

template <class T>
void
ToElementWithChildName(khxml::DOMElement *elem,
                       const std::string &childTagName,
                       const T &container)
{
    for (const auto& iter : container)
    {
        AddElement(elem, childTagName, iter);
    }
}


inline void
ToElementWithChildName(khxml::DOMElement *elem,
                       const std::string &childTagName,
                       const VerRefGen &val)
{
  if (!val.empty()) {
    AddElement(elem, childTagName, ToString(val));
  }
}

template <class T>
void
ToElement(khxml::DOMElement *elem, const std::vector<T> &vec)
{
  std::string childname = ListElementTagName(FromXMLStr(elem->getTagName()));
  ToElementWithChildName(elem, childname, vec);
}

template <class T>
void
ToElement(khxml::DOMElement *elem, const std::deque<T> &deque)
{
  std::string childname = ListElementTagName(FromXMLStr(elem->getTagName()));
  ToElementWithChildName(elem, childname, deque);
}

template <class T>
void
ToElement(khxml::DOMElement *elem, const std::list<T> &list)
{
  std::string childname = ListElementTagName(FromXMLStr(elem->getTagName()));
  ToElementWithChildName(elem, childname, list);
}

template <class T>
void
ToElement(khxml::DOMElement *elem, const std::set<T> &set)
{
  std::string childname = ListElementTagName(FromXMLStr(elem->getTagName()));
  ToElementWithChildName(elem, childname, set);
}

template <class T, class U>
void
ToElement(khxml::DOMElement *elem, const std::map<T, U> &map)
{
  for (const auto& iter : map)
  {
    khxml::DOMElement* item =
        elem->getOwnerDocument()->createElement(ToXMLStr("item"));
    AddElement(item, "name", iter.first);
    AddElement(item, "value", iter.second);
    elem->appendChild(item);
  }
}

template <class String, class U>
inline void
ToElementForStringMap(khxml::DOMElement *elem, const std::map<String, U> &map) {
  for (const auto& iter : map)
  {
    khxml::DOMElement* item =
        elem->getOwnerDocument()->createElement(ToXMLStr("item"));
    AddAttribute(item, "key", iter.first);
    ToElement(item, iter.second);
    elem->appendChild(item);
  }
}

template <class U>
void
ToElement(khxml::DOMElement *elem, const std::map<std::string, U> &map)
{
  ToElementForStringMap(elem, map);
}

template <class U>
void
ToElement(khxml::DOMElement *elem, const std::map<QString, U> &map)
{
  ToElementForStringMap(elem, map);
}

template <class U>
void
ToElement(khxml::DOMElement *elem, const std::map<SharedString, U> &map)
{
  ToElementForStringMap(elem, map);
}

template <class T>
void
ToElement(khxml::DOMElement *elem, const Defaultable<T> &val)
{
  AddAttribute(elem, "useDefault", val.UseDefault());
  if (!val.UseDefault()) {
    ToElement(elem, val.GetValue());
  }
}

template <class T>
void
ToElement(khxml::DOMElement *elem, const khSize<T> &size)
{
  AddElement(elem, "width",  size.width);
  AddElement(elem, "height", size.height);
}

template <class T>
void
ToElement(khxml::DOMElement *elem, const khOffset<T> &offset)
{
  AddElement(elem, "x", offset.x());
  AddElement(elem, "y", offset.y());
}

template <class T>
void
ToElement(khxml::DOMElement *elem, const khExtents<T> &extents)
{
  AddElement(elem, "beginx", extents.beginX());
  AddElement(elem, "endx", extents.endX());
  AddElement(elem, "beginy", extents.beginY());
  AddElement(elem, "endy", extents.endY());
}

inline void
ToElement(khxml::DOMElement *elem, const khLevelCoverage &cov)
{
  AddElement(elem, "level", cov.level);
  AddElement(elem, "extents", cov.extents);
}

inline void
ToElement(khxml::DOMElement *elem, const khInsetCoverage &cov)
{
  AddElement(elem, "beginLevel", cov.beginLevel());
  AddElement(elem, "endLevel",   cov.endLevel());
  AddElement(elem, "levelExtents", cov.RawExtentsVec());
}

inline void
ToElement(khxml::DOMElement *elem, const khMetaData &meta)
{
  ToElement(elem, meta.map);
}

inline void
ToElement(khxml::DOMElement *elem, const QColor &color) {
  QRgb rgb = color.rgb();
  AddAttribute(elem, "r", qRed(rgb));
  AddAttribute(elem, "g", qGreen(rgb));
  AddAttribute(elem, "b", qBlue(rgb));
  AddAttribute(elem, "a", qAlpha(rgb));
}

template <class T>
inline void
ToElement(khxml::DOMElement *elem, const geRange<T> &range) {
  AddAttribute(elem, "min", range.min);
  AddAttribute(elem, "max", range.max);
}

inline void
ToElement(khxml::DOMElement *elem, const VerRefGen &val)
{
  std::string childname = ListElementTagName(FromXMLStr(elem->getTagName()));
  ToElementWithChildName(elem, childname, val);
}

/******************************************************************************
 ***  Some helper functions for DOM reading
 ******************************************************************************/
typedef std::vector<khxml::DOMElement*> khDOMElemList;

inline
khDOMElemList
GetChildrenByTagName(khxml::DOMElement *parent, const std::string &tagname)
{
  khDOMElemList kids;
  khxml::DOMNode *node = parent->getFirstChild();
  while (node) {
    if (node->getNodeType() == khxml::DOMNode::ELEMENT_NODE) {
      khxml::DOMElement *elem = static_cast<khxml::DOMElement*>(node);
      if (FromXMLStr(elem->getTagName()) == tagname) {
        kids.push_back(elem);
      }
    }
    node = node->getNextSibling();
  }
  return kids;
}

inline
khxml::DOMElement*
GetFirstNamedChild(khxml::DOMElement *parent, const std::string &tagname)
{
  khxml::DOMNode *node = parent->getFirstChild();
  while (node) {
    if (node->getNodeType() == khxml::DOMNode::ELEMENT_NODE) {
      khxml::DOMElement *elem = static_cast<khxml::DOMElement*>(node);
      if (FromXMLStr(elem->getTagName()) == tagname) {
        return elem;
      }
    }
    node = node->getNextSibling();
  }
  return 0;
}

template <class T>
void
FromElement(khxml::DOMElement *elem, T &val);

inline
void
FromElement(khxml::DOMElement *elem, QString &val);

inline
void
FromElement(khxml::DOMElement *elem, std::string &val);

template <class T>
void
FromElementWithChildName(khxml::DOMElement *elem,
                         const std::string &childTagName,
                         std::vector<T> &vec);

template <class T>
void
FromElementWithChildName(khxml::DOMElement *elem,
                         const std::string &childTagName,
                         std::deque<T> &deque);

template <class T>
void
FromElementWithChildName(khxml::DOMElement *elem,
                         const std::string &childTagName,
                         std::list<T> &list);
template <class T>
void
FromElementWithChildName(khxml::DOMElement *elem,
                         const std::string &childTagName,
                         std::set<T> &set);

inline
void
FromElementWithChildName(khxml::DOMElement *elem,
                         const std::string &childTagName,
                         VerRefGen &val);

template <class T>
void
FromElement(khxml::DOMElement *elem, std::vector<T> &vec);

template <class T>
void
FromElement(khxml::DOMElement *elem, std::deque<T> &deque);

template <class T>
void
FromElement(khxml::DOMElement *elem, std::list<T> &list);

template <class T>
void
FromElement(khxml::DOMElement *elem, std::set<T> &set);
template <class T, class U>
void
FromElement(khxml::DOMElement *elem, std::map<T, U> &map);

template <class U>
void
FromElement(khxml::DOMElement *elem, std::map<std::string, U> &map);

template <class U>
void
FromElement(khxml::DOMElement *elem, std::map<QString, U> &map);

template <class U>
void
FromElement(khxml::DOMElement *elem, std::map<SharedString, U> &map);

template <class T>
void
FromElement(khxml::DOMElement *elem, Defaultable<T> &self);

template <class T>
void
FromElement(khxml::DOMElement *elem, khSize<T> &self);

template <class T>
void
FromElement(khxml::DOMElement *elem, khOffset<T> &self);

template <class T>
void
FromElement(khxml::DOMElement *elem, khExtents<T> &extents);

inline void
FromElement(khxml::DOMElement *elem, khLevelCoverage &cov);

inline void
FromElement(khxml::DOMElement *elem, khInsetCoverage &cov);

inline void
FromElement(khxml::DOMElement *elem, khMetaData &meta);

inline void
FromElement(khxml::DOMElement *elem, VerRefGen &val);


template <class T>
void
GetElement(khxml::DOMElement *parent, const std::string &tagname, T &val)
{
  khxml::DOMElement *elem = GetFirstNamedChild(parent, tagname);
  if (!elem)
    throw khDOMException(khDOMException::MissingElement,
                         FromXMLStr(parent->getTagName()),
                         tagname);
  FromElement(elem, val);
}


template <class T>
void
GetElementOrDefault(khxml::DOMElement *parent,
                    const std::string &tagname,
                    T &val,
                    const T &dflt)
{
  try {
    khxml::DOMElement *elem = GetFirstNamedChild(parent, tagname);
    if (!elem) {
      val = dflt;
    } else {
      FromElement(elem, val);
    }
  } catch (const khDOMException &) {
    val = dflt;
  }
}

template <class T>
void
GetElementWithChildName(khxml::DOMElement *parent,
                        const std::string &tagname,
                        const std::string &childTagname,
                        T &val)
{
  khxml::DOMElement *elem = GetFirstNamedChild(parent, tagname);
  if (!elem)
    throw khDOMException(khDOMException::MissingElement,
                         FromXMLStr(parent->getTagName()),
                         tagname);
  FromElementWithChildName(elem, childTagname, val);
}


template <class T>
void
GetElementWithChildNameOrDefault(khxml::DOMElement *parent,
                                 const std::string &tagname,
                                 const std::string &childTagname,
                                 T &val, const T &dflt)
{
  try {
    khxml::DOMElement *elem = GetFirstNamedChild(parent, tagname);
    if (!elem) {
      val = dflt;
    } else {
      FromElementWithChildName(elem, childTagname, val);
    }
  } catch (const khDOMException &) {
    val = dflt;
  }
}

template <class T, class Deprecated>
void
GetElementWithDeprecated(khxml::DOMElement *parent,
                         const std::string &tagname, T &val,
                         Deprecated &dep)
{
  khxml::DOMElement *elem = GetFirstNamedChild(parent, tagname);
  if (!elem)
    throw khDOMException(khDOMException::MissingElement,
                         FromXMLStr(parent->getTagName()),
                         tagname);
  FromElementWithDeprecated(elem, val, dep);
}


template <class T, class Deprecated>
void
GetElementWithDeprecatedOrDefault(khxml::DOMElement *parent,
                                  const std::string &tagname, T &val,
                                  const T &dflt, Deprecated &dep)
{
  try {
    khxml::DOMElement *elem = GetFirstNamedChild(parent, tagname);
    if (!elem) {
      val = dflt;
    } else {
      FromElementWithDeprecated(elem, val, dep);
    }
  } catch (const khDOMException &) {
    val = dflt;
  }
}

inline
khxml::DOMAttr*
GetNamedAttr(khxml::DOMElement *elem, const std::string &attrname)
{
  return elem->getAttributeNode(ToXMLStr(attrname));
}

template <class T>
void
FromAttribute(khxml::DOMAttr *attr, T &val)
{
  std::string valStr = FromXMLStr(attr->getValue());
  FromString(valStr, val);
}

inline
void
FromAttribute(khxml::DOMAttr *attr, QString &val)
{
  val = QString::fromUtf16((const ushort*)attr->getValue());
}

inline
void
FromAttribute(khxml::DOMAttr *attr, std::string &val)
{
  val = FromXMLStr(attr->getValue());
}

inline
QString
GetTextAndCDATA(khxml::DOMElement *elem)
{
  QString result;
  khxml::DOMNode *node = elem->getFirstChild();
  while (node) {
    if ((node->getNodeType() == khxml::DOMNode::TEXT_NODE) ||
        (node->getNodeType() == khxml::DOMNode::CDATA_SECTION_NODE)) {
      khxml::DOMCharacterData* data = static_cast<khxml::DOMCharacterData*>(node);
      if (data->getLength() > 0) {
        result.append(QString::fromUtf16((const ushort*)data->getData()));
      }
    }
    node = node->getNextSibling();
  }
  return result;
}

template <class T>
void
GetAttribute(khxml::DOMElement *elem, const std::string &attrname, T &val)
{
  khxml::DOMAttr *attr = GetNamedAttr(elem, attrname);
  if (!attr)
    throw khDOMException(khDOMException::MissingAttribute,
                         FromXMLStr(elem->getTagName()),
                         attrname);
  FromAttribute(attr, val);
}

template <class T>
void
GetAttributeOrDefault(khxml::DOMElement *elem, const std::string &attrname,
                      T &val, const T &dflt)
{
  try {
    khxml::DOMAttr *attr = GetNamedAttr(elem, attrname);
    if (!attr) {
      val = dflt;
    } else {
      FromAttribute(attr, val);
    }
  } catch (const khDOMException &) {
    val = dflt;
  }
}

template <class T>
void
GetElementBody(khxml::DOMElement *elem, T &val)
{
  QString body = GetTextAndCDATA(elem);
  FromQString(body, val);
}


template <class T>
void
FromElement(khxml::DOMElement *elem, T &val)
{
  khxml::DOMNode *valNode = elem->getFirstChild();
  if (!valNode ||
      valNode->getNodeType() != khxml::DOMNode::TEXT_NODE) {
    throw khDOMException(khDOMException::MissingText,
                         FromXMLStr(elem->getTagName()));
  }
  std::string valStr = FromXMLStr(((khxml::DOMText*)valNode)->getData());
  FromString(valStr, val);
}


inline
void
FromElement(khxml::DOMElement *elem, QString &val)
{
  khxml::DOMNode *valNode = elem->getFirstChild();
  if (!valNode ||
      (valNode->getNodeType() != khxml::DOMNode::TEXT_NODE)) {
    val = "";
    val.squeeze();
  } else {
    val = QString::fromUtf16((const ushort*)(static_cast<khxml::DOMText*>(valNode))->getData());
  }
}

inline
void
FromElement(khxml::DOMElement *elem, EncryptedQString &val)
{
  khxml::DOMAttr *attr = GetNamedAttr(elem, "method");
  if (!attr) {
    throw khDOMException(khDOMException::MissingAttribute,
                         FromXMLStr(elem->getTagName()),
                         "method");
  }
  std::string method;
  FromAttribute(attr, method);


  khxml::DOMNode *node = elem->getFirstChild();
  while (node) {
    if ((node->getNodeType() == khxml::DOMNode::CDATA_SECTION_NODE)) {
      khxml::DOMCharacterData* data = static_cast<khxml::DOMCharacterData*>(node);;
      if (data->getLength() > 0) {
        if (method == "plaintext") {
          val = QString::fromUtf16((const ushort*)data->getData());
          return;
        } else if (method == "simple") {
          QString tmp = QString::fromUtf16((const ushort*)data->getData());
          for (auto i = 0; i < tmp.length(); ++i) {
            tmp[i] = tmp[i].unicode() - 13;
          }
          val = tmp;
          return;
        } else {
          throw khDOMException(khDOMException::InvalidAttribute,
                               FromXMLStr(elem->getTagName()),
                               "method");
        }
      }
      break;
    }
    node = node->getNextSibling();
  }
  val = "";
  val.squeeze();
}

inline
void
FromElement(khxml::DOMElement *elem, std::string &val)
{
  khxml::DOMNode *valNode = elem->getFirstChild();
  if (!valNode ||
      (valNode->getNodeType() != khxml::DOMNode::TEXT_NODE)) {
    val.clear();
    val.shrink_to_fit();
  } else {
    khxml::DOMText* valText = static_cast<khxml::DOMText*>(valNode);
    val = FromXMLStr(valText->getData());
  }
}

inline
void
FromElement(khxml::DOMElement *elem, SharedString &val)
{
  std::string valStr;
  FromElement(elem, valStr);
  val = valStr;
}

template <class T>
void
FromElementWithChildName(khxml::DOMElement *elem,
                         const std::string &childTagName,
                         std::vector<T> &vec)
{
  vec.clear();
  vec.shrink_to_fit();
  khDOMElemList kids = GetChildrenByTagName(elem, childTagName);

  for (const auto& iter : kids)
  {
    T tmp;
    FromElement(iter, tmp);
    vec.push_back(tmp);
  }
}

template <class T>
void
FromElementWithChildName(khxml::DOMElement *elem,
                         const std::string &childTagName,
                         std::deque<T> &deque)
{
  deque.clear();
  deque.shrink_to_fit();
  khDOMElemList kids = GetChildrenByTagName(elem, childTagName);

  for (const auto& iter : kids)
  {
    T tmp;
    FromElement(iter, tmp);
    deque.push_back(tmp);
  }
}

template <class T>
void
FromElementWithChildName(khxml::DOMElement *elem,
                         const std::string &childTagName,
                         std::list<T> &list)
{
  list.clear();
  khDOMElemList kids = GetChildrenByTagName(elem, childTagName);

  for (const auto& iter : kids)
  {
    T tmp;
    FromElement(iter,tmp);
    list.push_back(tmp);
  }
}

template <class T>
void
FromElementWithChildName(khxml::DOMElement *elem,
                         const std::string &childTagName,
                         std::set<T> &set)
{
  set.clear();
  khDOMElemList kids = GetChildrenByTagName(elem, childTagName);

  for (const auto& iter : kids)
  {
    T tmp;
    FromElement(iter, tmp);
    set.insert(tmp);
  }
}

inline void
FromElementWithChildName(khxml::DOMElement *elem,
                         const std::string &childTagName,
                         VerRefGen &val)
{
  khDOMElemList kids = GetChildrenByTagName(elem, childTagName);
  khDOMElemList::iterator iter = kids.begin();
  // Note: only get first item from list (latest version). It is enough to
  // initialize the version reference generator. All other references are
  // generated on the fly.
  if (iter != kids.end()) {
    std::string tmp;
    FromElement(*iter, tmp);
    FromString(tmp, val);
  }
}

template <class T>
void
FromElement(khxml::DOMElement *elem, std::vector<T> &vec)
{
  std::string childname = ListElementTagName(FromXMLStr(elem->getTagName()));
  FromElementWithChildName(elem, childname, vec);
}

template <class T>
void
FromElement(khxml::DOMElement *elem, std::deque<T> &deque)
{
  std::string childname = ListElementTagName(FromXMLStr(elem->getTagName()));
  FromElementWithChildName(elem, childname, deque);
}

template <class T>
void
FromElement(khxml::DOMElement *elem, std::list<T> &list)
{
  std::string childname = ListElementTagName(FromXMLStr(elem->getTagName()));
  FromElementWithChildName(elem, childname, list);
}

template <class T>
void
FromElement(khxml::DOMElement *elem, std::set<T> &set)
{
  std::string childname = ListElementTagName(FromXMLStr(elem->getTagName()));
  FromElementWithChildName(elem, childname, set);
}

template <class T, class U>
void
FromElement(khxml::DOMElement *elem, std::map<T, U> &map)
{
  map.clear();
  khDOMElemList kids = GetChildrenByTagName(elem, "item");

  for (const auto& iter : kids)
  {
    T name;
    U value;
    GetElement(iter, "name", name);
    GetElement(iter, "value", value);
    map.insert(std::make_pair(name, value));
  }
}

template<class String>
String FromXMLStrForMap(const XMLCh *xmlch);

template<>
inline std::string
FromXMLStrForMap(const XMLCh *xmlch) {
  return FromXMLStr(xmlch);
}

template<>
inline QString
FromXMLStrForMap(const XMLCh *xmlch) {
  return QString::fromUtf16((const ushort*)xmlch);
}

template<>
inline SharedString
FromXMLStrForMap(const XMLCh *xmlch) {
  return FromXMLStr(xmlch);
}

template <class String, class U>
void
FromElementForStringMap(khxml::DOMElement *elem, std::map<String, U> &map)
{
  map.clear();
  // see if this is the old way or the new way
  // Old way used keys for tagnames. New way has "item" elements w/
  // "key" attributes
  khxml::DOMElement *item = GetFirstNamedChild(elem, "item");
  if (item && GetNamedAttr(item, "key")) {
    // new way
    khDOMElemList kids = GetChildrenByTagName(elem, "item");

    for (const auto& iter : kids)
    {
      String key;
      U value;
      GetAttribute(iter, "key", key);
      FromElement(iter, value);
      map.insert(std::make_pair(key, value));
    }
  } else {
    // old way
    khxml::DOMNode *node = elem->getFirstChild();
    while (node) {
      if (node->getNodeType() == khxml::DOMNode::ELEMENT_NODE) {
        khxml::DOMElement *elem = static_cast<khxml::DOMElement*>(node);
        String name = FromXMLStrForMap<String>(elem->getTagName());
        U value;
        FromElement(elem, value);
        map.insert(std::make_pair(name, value));
      }
      node = node->getNextSibling();
    }
  }
}

template <class U>
void
FromElement(khxml::DOMElement *elem, std::map<std::string, U> &map) {
  FromElementForStringMap(elem, map);
}

template <class U>
void
FromElement(khxml::DOMElement *elem, std::map<QString, U> &map)
{
  FromElementForStringMap(elem, map);
}

template <class U>
void
FromElement(khxml::DOMElement *elem, std::map<SharedString, U> &map)
{
  FromElementForStringMap(elem, map);
}

template <class T>
void
FromElement(khxml::DOMElement *elem, khSize<T> &self)
{
  GetElement(elem, "width",  self.width);
  GetElement(elem, "height", self.height);
}

template <class T>
void
FromElement(khxml::DOMElement *elem, khOffset<T> &self)
{
  T x, y;
  GetElement(elem, "x", x);
  GetElement(elem, "y", y);
  self = khOffset<T>(XYOrder, x, y);
}

template <class T>
void
FromElement(khxml::DOMElement *elem, khExtents<T> &extents)
{
  T bx, ex, by, ey;

  GetElement(elem, "beginx", bx);
  GetElement(elem, "endx", ex);
  GetElement(elem, "beginy", by);
  GetElement(elem, "endy", ey);
  extents = khExtents<T>(XYOrder, bx, ex, by, ey);
}

inline void
FromElement(khxml::DOMElement *elem, khLevelCoverage &cov)
{
  unsigned int level;
  khExtents<std::uint32_t> extents;

  GetElement(elem, "level", level);
  GetElement(elem, "extents", extents);
  cov = khLevelCoverage(level, extents);
}

inline void
FromElement(khxml::DOMElement *elem, khInsetCoverage &cov)
{
  unsigned int beginLevel;
  unsigned int endLevel;
  std::vector<khExtents<std::uint32_t> > levelExtents;

  GetElement(elem, "beginLevel", beginLevel);
  GetElement(elem, "endLevel", endLevel);
  if (endLevel < beginLevel) {
    throw khException
      (kh::tr("Internal Error: Bad level for khInsetCoverage"));
  }
  unsigned int numLevels = endLevel - beginLevel;
  levelExtents.reserve(numLevels);

  GetElement(elem, "levelExtents", levelExtents);
  if (levelExtents.size() != numLevels) {
    throw khException
      (kh::tr("Internal Error: Bad sizes for khInsetCoverage"));
  }

  cov = khInsetCoverage(beginLevel, endLevel, levelExtents);
}

inline void
FromElement(khxml::DOMElement *elem, khMetaData &meta)
{
  FromElement(elem, meta.map);
}

template <class T>
void
FromElement(khxml::DOMElement *elem, Defaultable<T> &self)
{
  bool useDefault;
  GetAttributeOrDefault(elem, "useDefault", useDefault, true);
  if (useDefault) {
    self.SetUseDefault();
  } else {
    T val;
    FromElement(elem, val);
    self = val;
  }
}

inline void
FromElement(khxml::DOMElement *elem, QColor &color)
{
  int r, g, b, a;
  GetAttribute(elem, "r", r);
  GetAttribute(elem, "g", g);
  GetAttribute(elem, "b", b);
  GetAttributeOrDefault(elem, "a", a, 255);
  color.setRgb(qRgba(r, g, b, a));
}

template <class T>
void
FromElement(khxml::DOMElement *elem, geRange<T> &range)
{
  GetAttribute(elem, "min", range.min);
  GetAttribute(elem, "max", range.max);
}

inline void
FromElement(khxml::DOMElement *elem, VerRefGen &val)
{
  std::string childname = ListElementTagName(FromXMLStr(elem->getTagName()));
  FromElementWithChildName(elem, childname, val);
}

/******************************************************************************
 ***  DOM Parser routines that catch Xerces' exceptions
 ******************************************************************************/

extern std::unique_ptr<GEDocument>
CreateEmptyDocument(const std::string &rootTagname) throw();

extern bool
WriteDocument(GEDocument *doc, const std::string &filename) throw();

extern bool
WriteDocumentToString(GEDocument *doc, std::string &buf) throw();

extern std::unique_ptr<GEDocument>
ReadDocument(const std::string &filename) throw();

extern std::unique_ptr<GEDocument>
ReadDocumentFromString(const std::string &buf,
                       const std::string &ref) throw();

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_KHXML_KHDOM_H_
