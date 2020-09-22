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

#ifndef __EncryptedQString_h
#define __EncryptedQString_h

#include <qstring.h>


// ****************************************************************************
// ***  Wrapper around QString so it has a different type
// ***   for serialization / deserialization to XML
// ****************************************************************************
class EncryptedQString : public QString {
 public:
  inline EncryptedQString(void) : QString() { }
  inline EncryptedQString(QChar ch) : QString(ch) { }
  inline EncryptedQString(const QString &s) : QString(s) { }
  inline EncryptedQString(const QByteArray &ba) : QString(ba) { }
  inline EncryptedQString(const QChar *unicode, unsigned int length) :
      QString(unicode, length ) { }
  inline EncryptedQString(const char *str) : QString(str) { }
  inline EncryptedQString(const std::string &str) : QString(str.c_str()) { }
};


#endif /* __EncryptedQString_h */
