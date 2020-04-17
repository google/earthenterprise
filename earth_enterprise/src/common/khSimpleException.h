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

// Exceptions that don't need QString

#ifndef COMMON_KHSIMPLEEXCEPTION_H__
#define COMMON_KHSIMPLEEXCEPTION_H__

#include <exception>
#include <sstream>
#include <errno.h>
// for khstrerror
#include <notify.h>
#include <Qt/qbytearray.h>

// ****************************************************************************
// ***  khSimpleException
// ***
// ***  This uses std::string internally instead of QString. This means the
// ***  msg strings cannot be localized.
// ***
// ***  khSimpleException acts like a stream so you can compose error
// ***  strings right in the throw statement.
// ***
// ***  --- sample usages ---
// ***  throw khSimpleException("An error happened");
// ***  throw khSimpleException() << "Unable to release lock";
// ***  throw khSimpleException("Unable to find asset") << assetname;
// ****************************************************************************
class khSimpleException : public std::exception
{
 private:
  std::string msg;

 public:
  khSimpleException(void) : std::exception() { }
  khSimpleException(const std::string &msg_) : std::exception(), msg(msg_) { }
  virtual ~khSimpleException(void) throw() { }
  virtual const char* what() const throw() {
    return msg.empty() ? "Unknown error" : msg.c_str();
  }

  template <class T>
  inline khSimpleException& operator<<(const T &t) {
    std::ostringstream out;
    out << t;
    msg += out.str();
    return *this;
  }

  inline khSimpleException operator<<(const QByteArray& q)
  {
      std::ostringstream out;
      out << q.constData();
      msg += out.str();
      return *this;
  }
};

// ****************************************************************************
// ***  khSimpleNotFoundException
// ***
// ***  This is used to allow an exception handler to differentiate between
// ***  a "not found" exception and other exceptions.  The server uses this
// ***  to avoid logging "not found" type exceptions.  Since khSimpleException
// ***  is the base class, this exception will also be caught by an attempt
// ***  to catch khSimpleException.
// ****************************************************************************
class khSimpleNotFoundException : public khSimpleException
{
 public:
  khSimpleNotFoundException() : khSimpleException() {}
  khSimpleNotFoundException(const std::string &msg_)
      : khSimpleException(msg_) {}
  virtual ~khSimpleNotFoundException() throw() {}

  template <class T>
  inline khSimpleNotFoundException& operator<<(const T &t) {
    khSimpleException::operator<<(t);
    return *this;
  }
};

// ****************************************************************************
// ***  khSimpleErrnoException
// ***
// ***  Like khSimpleException, but automatically adds ": <error description>"
// ***  to the end of the supplied message
// ***
// ***  --- sample usages ---
// ***  throw khSimpleErrnoException("Unable to open ") << filename;
// ****************************************************************************
class khSimpleErrnoException : public std::exception
{
 private:
  int errno_;
  std::string msg;
  mutable std::string what_str;

 public:
  static std::string errorString(int err) {
    return khstrerror(err);
  }

  khSimpleErrnoException(void) : std::exception(), errno_(errno) { }
  khSimpleErrnoException(const std::string &msg_) :
      std::exception(), errno_(errno), msg(msg_) { }
  virtual ~khSimpleErrnoException(void) throw() { }

  int errnum(void) const { return errno_; }
  std::string errstr(void) const { return errorString(errno_); }
  virtual const char* what() const throw() {
    what_str = msg + ": " + errstr();
    return what_str.c_str();
  }

  template <class T>
  inline khSimpleErrnoException& operator<<(const T &t) {
    std::ostringstream out;
    out << t;
    msg += out.str();
    return *this;
  }
};




#endif // COMMON_KHSIMPLEEXCEPTION_H__
