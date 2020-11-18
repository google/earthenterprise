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


#ifndef __khException_h
#define __khException_h

#include <stdexcept>
#include <errno.h>
#include <cstring>
#include <Qt/qstring.h>
#include <Qt/qobject.h>

// for khstrerror
#include <notify.h>

namespace kh {
inline QString tr(const char *srcText, const char *comment = 0) {
  return QObject::tr(srcText, comment);
}

inline QString trUtf8(const char * sourceText, const char * disambiguation = 0, int n = -1)
{
    return QObject::trUtf8(sourceText, disambiguation, n);
}

inline QString no_tr(const char *srcText) {
  return QString::fromAscii(srcText);
}
};

class khException : public std::runtime_error
{
 public:
  khException(const std::string &msg)
      : std::runtime_error(msg) {}
  khException(const char * msg)
      : std::runtime_error(msg) {}
  khException(const QString &msg)
      : std::runtime_error(std::string(msg.toUtf8().constData())) { }
  virtual ~khException(void) throw() { }
  QString qwhat(void) const throw() {
    return QString::fromUtf8(what());
  }
};


// ****************************************************************************
// ***  Some subsystems distinguich between soft and hard exceptions
// ***  Soft exceptions generate warning up to a certain number and them
// ***  become fatal. This is used primarily in the GUI to allow some
// ***  tolerance in source data quality while previewing new data. Later on
// ***  import, soft errors are treated the same as hard errors.
// ***
// ***  Note that this inherits from khException. This means that places
// ***  that don't expliditly handle khSoftException but do handle
// ***  khException will simple treat soft ones as hard ones.
// ****************************************************************************
class khSoftException : public khException
{
 public:
  khSoftException(const QString &msg) : khException(msg) { }
  virtual ~khSoftException(void) throw() { }
};


class khErrnoException : public khException
{
 protected:
  int errno_;
 public:
  static QString errorString(int err) {
    return QString::fromLocal8Bit(khstrerror(err).c_str());
  }

  khErrnoException(const QString &msg) :
      khException(msg.isEmpty() ?
                  errorString(errno_ = errno) :
                  msg + ": " + errorString(errno_ = errno)) { }
  virtual ~khErrnoException(void) throw() { }
  int errnum(void) const { return errno_; }
  QString errstr(void) const { return errorString(errno_); }
};


// ****************************************************************************
// ***  routine to set up reporting of bad exceptions
// ***
// ***  This will only matter when there is a problem with exception handling
// ***  It will help give more output in the hopes that the real problem can be
// ***  solved.
// ****************************************************************************
extern void ReportBadExceptions(void);




#endif /* __khException_h */
