// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


#include <stdlib.h>
#include <alloca.h>
#include <ctype.h>
#include <stdlib.h>
#include <fnmatch.h>
#include <qstring.h>
#include <qtextcodec.h>
#include <qregexp.h>

#include <gstValue.h>
#include <gstMisc.h>
#include <notify.h>

#define BUFLEN 8192

int gstValue::vcount = 0;
int gstValue::scount = 0;
int gstValue::icount = 0;
int gstValue::dcount = 0;

const int kMaxString = 128;

const char* FormatAsCSV(const char* string) {
  static char output[BUFLEN];
  memset(output, '\0', BUFLEN);

  // input must be valid
  if (string == NULL || *string == '\0')
    return output;

  // any comma or double-quote forces the text to be enclosed in double-quotes
  // commas can then exist without any other special treatment
  // double-quotes must be 'escaped' by inserting a second double-quote

  const char* in = string;
  const char* end = string + strlen(string);
  char* out = &output[0];

  *out = '"';
  ++out;

  while (in < end) {
    if (*in == '"') {
      *out = '"';
      ++out;
    }

    *out = *in;
    ++out;
    ++in;

    // must always have room for two more characters, plus
    // closing quote and terminator in order to continue
    if (out >= (output + BUFLEN - 4))
      break;
  }

  *out = '"';
  ++out;

  return output;
}

std::string ExtractString(const char* ibuf, int sz, bool* check_highbit) {
  int len = (sz == -1) ? strlenSafe(ibuf) : sz;
  std::string out_str(ibuf, len);

  if (check_highbit) {
    *check_highbit = false;
    for (std::string::iterator pos = out_str.begin();
         pos != out_str.end(); ++pos) {
      if (static_cast<unsigned char>(*pos) > 127) {
        *check_highbit = true;
        break;
      }
    }
  }

  // trim trailing whitespace
  while (out_str.length() && isspace(out_str[out_str.length() - 1]))
    out_str.resize(out_str.length() - 1);

  // if first char is a double-quote, take everything
  // between the quotes
  if (out_str.length() >= 2 && out_str[0] == '\"' &&
      out_str[out_str.length() - 1] == '\"') {
    out_str.resize(out_str.length() - 1);
    out_str.erase(0, 1);
  }

  return out_str;
}


//
// The following string to number conversions use the stdlib.
// strto* functions to allow hex and octal.  From the man page...
//
// If base is zero, the string itself determines the base as follows: After
// an optional leading sign a leading zero indicates octal conversion, and a
// leading ``0x'' or ``0X'' hexadecimal conversion.  Otherwise, decimal
// conversion is used.
//

int StringToInt(const char* ibuf, int sz) {
  if (sz == 0)
    return 0;
  return strtol(ExtractString(ibuf, sz).c_str(), NULL, 0);
}

unsigned int StringToUInt(const char* ibuf, int sz) {
  if (sz == 0)
    return 0;
  return strtoul(ExtractString(ibuf, sz).c_str(), NULL, 0);
}

std::int64_t StringToInt64(const char* ibuf, int sz) {
  if (sz == 0)
    return 0;
  return strtoll(ExtractString(ibuf, sz).c_str(), NULL, 0);
}

 std::uint64_t StringToUInt64(const char* ibuf, int sz) {
  if (sz == 0)
    return 0;
  return strtoull(ExtractString(ibuf, sz).c_str(), NULL, 0);
}

double StringToDouble(const char* ibuf, int sz) {
  if (sz == 0)
    return 0.0;

  std::string str = ExtractString(ibuf, sz);
  std::string::size_type idx = str.find("D");
  if (idx != std::string::npos) {
    str[idx] = 'E';
  }

  return strtod(str.c_str(), NULL);
}


void gstValue::Init() {
  ++vcount;

  switch (type_) {
    case gstTagBoolean:
      num_data_.bVal = 0;
      ++icount;
      break;
    case gstTagInt:
      num_data_.iVal = 0;
      ++icount;
      break;
    case gstTagUInt:
      num_data_.uiVal = 0;
      ++icount;
      break;
    case gstTagInt64:
      num_data_.i64Val = 0;
      ++icount;
      break;
    case gstTagUInt64:
      num_data_.ui64Val = 0;
      ++icount;
      break;
    case gstTagFloat:
      num_data_.fVal = 0.;
      ++dcount;
      break;
    case gstTagDouble:
      num_data_.dVal = 0.;
      ++dcount;
      break;
    case gstTagString:
    case gstTagUnicode:
      qstring_ = new QString();
      ++scount;
      break;
  }

  codec_ = NULL;
  need_codec_ = false;
  name_ = NULL;
}

gstValue::~gstValue() {
  --vcount;

  switch (type_) {
    case gstTagBoolean:
    case gstTagInt:
    case gstTagUInt:
    case gstTagInt64:
    case gstTagUInt64:
      --icount;
      break;
    case gstTagFloat:
    case gstTagDouble:
      --dcount;
      break;
    case gstTagString:
    case gstTagUnicode:
      delete qstring_;
      --scount;
      break;
  }

  if (name_)
    delete [] name_;
}

bool gstValue::IsEmpty() const {
  switch (type_) {
    case gstTagBoolean:
      return (num_data_.bVal == 0);
    case gstTagInt:
      return (num_data_.iVal == 0);
    case gstTagUInt:
      return (num_data_.uiVal == 0);
    case gstTagInt64:
      return (num_data_.i64Val == 0);
    case gstTagUInt64:
      return (num_data_.ui64Val == 0);
    case gstTagFloat:
      return (num_data_.fVal == 0.0);
    case gstTagDouble:
      return (num_data_.dVal == 0.0);
    case gstTagString:
    case gstTagUnicode:
      return (qstring_->isEmpty());
  }

  return false;
}

// return number of bytes required to hold
// this value
int gstValue::RawSize() {
  switch (type_) {
    case gstTagBoolean:
      return sizeof(bool);
    case gstTagInt:
      return sizeof(int);
    case gstTagUInt:
      return sizeof(unsigned int);
    case gstTagInt64:
      return sizeof(std::int64_t);
    case gstTagUInt64:
      return sizeof(std::uint64_t);
    case gstTagFloat:
      return sizeof(float);
    case gstTagDouble:
      return sizeof(double);
    case gstTagString:
    case gstTagUnicode:
      return strlen(qstring_->utf8()) + 1;
  }

  return 0;
}

// like above, but allows computing the size
// from the type and raw buffer without having
// to construct the object
int gstValue::RawSize(unsigned char type, const char* buf) {
  switch (type) {
    case gstTagBoolean:
      return sizeof(bool);
    case gstTagInt:
      return sizeof(int);
    case gstTagUInt:
      return sizeof(unsigned int);
    case gstTagInt64:
      return sizeof(std::int64_t);
    case gstTagUInt64:
      return sizeof(std::uint64_t);
    case gstTagFloat:
      return sizeof(float);
    case gstTagDouble:
      return sizeof(double);
    case gstTagString:
    case gstTagUnicode:
      return strlen(buf) + 1;
  }

  return 0;
}

std::string gstValue::ValueAsCSV() const {
  switch (type_) {
    case gstTagBoolean:
    case gstTagInt:
      return ValueAsString();
    case gstTagUInt:
      return ValueAsString();
    case gstTagInt64:
      return ValueAsString();
    case gstTagUInt64:
      return ValueAsString();
    case gstTagFloat:
      return ValueAsString();
    case gstTagDouble:
      return ValueAsString();
    case gstTagString:
    case gstTagUnicode:
      return std::string(FormatAsCSV(qstring_->latin1()));
  }

  return 0;
}

void gstValue::SetCodec(QTextCodec* c) {
  if (type_ != gstTagUnicode && type_ != gstTagString)
    return;
  codec_ = c;
}


bool gstValue::operator==(const gstValue& v) const {
  switch (type_) {
    case gstTagBoolean:
      return ValueAsBoolean() == v.ValueAsBoolean();
    case gstTagInt:
      return ValueAsInt() == v.ValueAsInt();
    case gstTagUInt:
      return ValueAsUInt() == v.ValueAsUInt();
    case gstTagInt64:
      return ValueAsInt64() == v.ValueAsInt64();
    case gstTagUInt64:
      return ValueAsUInt64() == v.ValueAsUInt64();
    case gstTagFloat:
      return ValueAsFloat() == v.ValueAsFloat();
    case gstTagDouble:
      return ValueAsDouble() == v.ValueAsDouble();
    case gstTagString:
    case gstTagUnicode:
      return (*qstring_) == v.ValueAsUnicode();
  }
  return false;
}

bool gstValue::operator!=(const gstValue& v) const {
  switch (type_) {
    case gstTagBoolean:
      return ValueAsBoolean() != v.ValueAsBoolean();
    case gstTagInt:
      return ValueAsInt() != v.ValueAsInt();
    case gstTagUInt:
      return ValueAsUInt() != v.ValueAsUInt();
    case gstTagInt64:
      return ValueAsInt() != v.ValueAsInt();
    case gstTagUInt64:
      return ValueAsUInt64() != v.ValueAsUInt64();
    case gstTagFloat:
      return ValueAsFloat() != v.ValueAsFloat();
    case gstTagDouble:
      return ValueAsDouble() != v.ValueAsDouble();
    case gstTagString:
    case gstTagUnicode:
      return (*qstring_) != v.ValueAsUnicode();
  }
  return false;
}

bool gstValue::MatchesWildcard(const gstValue& v) const {
  QRegExp rx(v.ValueAsUnicode(), true, true);
  return rx.exactMatch(ValueAsUnicode());
}

bool gstValue::operator<=(const gstValue& v) const {
  switch (type_) {
    case gstTagBoolean:
      return ValueAsBoolean() <= v.ValueAsBoolean();
    case gstTagInt:
      return ValueAsInt() <= v.ValueAsInt();
    case gstTagUInt:
      return ValueAsUInt() <= v.ValueAsUInt();
    case gstTagInt64:
      return ValueAsInt64() <= v.ValueAsInt64();
    case gstTagUInt64:
      return ValueAsUInt64() <= v.ValueAsUInt64();
    case gstTagFloat:
      return ValueAsFloat() <= v.ValueAsFloat();
    case gstTagDouble:
      return ValueAsDouble() <= v.ValueAsDouble();
    case gstTagString:
    case gstTagUnicode:
      return (*qstring_) <= v.ValueAsUnicode();
  }
  return false;
}

bool gstValue::operator>=(const gstValue& v) const {
  switch (type_) {
    case gstTagBoolean:
      return ValueAsBoolean() >= v.ValueAsBoolean();
    case gstTagInt:
      return ValueAsInt() >= v.ValueAsInt();
    case gstTagUInt:
      return ValueAsUInt() >= v.ValueAsUInt();
    case gstTagInt64:
      return ValueAsInt64() >= v.ValueAsInt64();
    case gstTagUInt64:
      return ValueAsUInt64() >= v.ValueAsUInt64();
    case gstTagFloat:
      return ValueAsFloat() >= v.ValueAsFloat();
    case gstTagDouble:
      return ValueAsDouble() >= v.ValueAsDouble();
    case gstTagString:
    case gstTagUnicode:
      return (*qstring_) >= v.ValueAsUnicode();
  }
  return false;
}

bool gstValue::operator<(const gstValue& v) const {
  switch (type_) {
    case gstTagBoolean:
      return ValueAsBoolean() < v.ValueAsBoolean();
    case gstTagInt:
      return ValueAsInt() < v.ValueAsInt();
    case gstTagUInt:
      return ValueAsUInt() < v.ValueAsUInt();
    case gstTagInt64:
      return ValueAsInt64() < v.ValueAsInt64();
    case gstTagUInt64:
      return ValueAsUInt64() < v.ValueAsUInt64();
    case gstTagFloat:
      return ValueAsFloat() < v.ValueAsFloat();
    case gstTagDouble:
      return ValueAsDouble() < v.ValueAsDouble();
    case gstTagString:
    case gstTagUnicode:
      return (*qstring_) < v.ValueAsUnicode();
  }
  return false;
}

bool gstValue::operator>(const gstValue& v) const {
  switch (type_) {
    case gstTagBoolean:
      return ValueAsBoolean() > v.ValueAsBoolean();
    case gstTagInt:
      return ValueAsInt() > v.ValueAsInt();
    case gstTagUInt:
      return ValueAsUInt() > v.ValueAsUInt();
    case gstTagInt64:
      return ValueAsInt64() > v.ValueAsInt64();
    case gstTagUInt64:
      return ValueAsUInt64() > v.ValueAsUInt64();
    case gstTagFloat:
      return ValueAsFloat() > v.ValueAsFloat();
    case gstTagDouble:
      return ValueAsDouble() > v.ValueAsDouble();
    case gstTagString:
    case gstTagUnicode:
      return (*qstring_) > v.ValueAsUnicode();
  }
  return false;
}

gstValue &gstValue::operator*=(const gstValue& v) {
  switch (type_) {
    case gstTagBoolean:
      // ??? Should we do something here?
      break;
    case gstTagInt:
      num_data_.iVal *= v.ValueAsInt();
      break;
    case gstTagUInt:
      num_data_.uiVal *= v.ValueAsUInt();
      break;
    case gstTagInt64:
      num_data_.i64Val *= v.ValueAsInt64();
      break;
    case gstTagUInt64:
      num_data_.ui64Val *= v.ValueAsUInt64();
      break;
    case gstTagFloat:
      num_data_.fVal *= v.ValueAsFloat();
      break;
    case gstTagDouble:
      num_data_.dVal *= v.ValueAsDouble();
      break;
    case gstTagString:
    case gstTagUnicode:
      // ??? Should we do something here?
      break;
  }
  return *this;
}

gstValue& gstValue::operator=(const gstValue& that) {
  Copy(that);
  return *this;
}

//
// compare useful for sorting
//
int gstValue::compare(const gstValue& that) const {
  switch (type_) {
    case gstTagBoolean:
      return ValueAsInt() - that.ValueAsInt();
    case gstTagInt:
      return ValueAsInt() - that.ValueAsInt();
    case gstTagUInt:
      return ValueAsUInt() - that.ValueAsUInt();
    case gstTagInt64:
      return ValueAsInt64() - that.ValueAsInt64();
    case gstTagUInt64:
      return ValueAsUInt64() - that.ValueAsUInt64();
    case gstTagFloat:
      {
        float tmp = ValueAsFloat() - that.ValueAsFloat();
        if (tmp < 0) {
          return -1;
        } else if (tmp > 0) {
          return 1;
        } else {
          return 0;
        }
      }
    case gstTagDouble:
      {
        double tmp = ValueAsDouble() - that.ValueAsDouble();
        if (tmp < 0) {
          return -1;
        } else if (tmp > 0) {
          return 1;
        } else {
          return 0;
        }
      }
    case gstTagString:
    case gstTagUnicode:
      return qstring_->localeAwareCompare(that.ValueAsUnicode());
  }
  return 0;
}

gstValue &gstValue::Copy(const gstValue& that) {
  switch (type_) {
    case gstTagBoolean:
      set(that.ValueAsBoolean());
      break;
    case gstTagInt:
      set(that.ValueAsInt());
      break;
    case gstTagUInt:
      set(that.ValueAsUInt());
      break;
    case gstTagInt64:
      set(that.ValueAsInt64());
      break;
    case gstTagUInt64:
      set(that.ValueAsUInt64());
      break;
    case gstTagFloat:
      set(that.ValueAsFloat());
      break;
    case gstTagDouble:
      set(that.ValueAsDouble());
      break;
    case gstTagString:
    case gstTagUnicode:
      set(that.ValueAsUnicode());
      break;
  }
  return *this;
}

// passing -1 for len means that the string
// is null terminated and we can get the size
// from the string
void gstValue::set(const char* buf, int len) {
  if (buf == NULL)
    buf = "\0";

  int clen = len == -1 ? strlen(buf) : len;

  switch (type_) {
    case gstTagBoolean:
      num_data_.bVal = (clen > 0);
      break;
    case gstTagInt:
      num_data_.iVal = StringToInt(buf, clen);
      break;
    case gstTagUInt:
      num_data_.uiVal = StringToUInt(buf, clen);
      break;
    case gstTagInt64:
      num_data_.i64Val = StringToInt64(buf, clen);
      break;
    case gstTagUInt64:
      num_data_.ui64Val = StringToUInt64(buf, clen);
      break;
    case gstTagFloat:
      num_data_.fVal = static_cast<float>(StringToDouble(buf, clen));
      break;
    case gstTagDouble:
      num_data_.dVal = StringToDouble(buf, clen);
      break;
    case gstTagString:
    case gstTagUnicode:
      if (codec_) {
        *qstring_ = codec_->toUnicode(ExtractString(buf, len).c_str());
      } else {
        *qstring_ = QString(ExtractString(buf, len, &need_codec_).c_str());
      }
      break;
  }
}

void gstValue::set(const QString& unicode) {
  switch (type_) {
    case gstTagBoolean:
      num_data_.bVal = !unicode.isEmpty();
      break;
    case gstTagInt:
      num_data_.iVal = unicode.toInt();
      break;
    case gstTagUInt:
      num_data_.uiVal = unicode.toUInt();
      break;
    case gstTagInt64:
      num_data_.i64Val = strtoll(unicode.latin1(), NULL, 0);
      break;
    case gstTagUInt64:
      num_data_.ui64Val = strtoull(unicode.latin1(), NULL, 0);
      break;
    case gstTagFloat:
      num_data_.fVal = unicode.toFloat();
      break;
    case gstTagDouble:
      num_data_.dVal = unicode.toDouble();
      break;
    case gstTagString:
    case gstTagUnicode:
      *qstring_ = unicode;
      break;
  }
}

void gstValue::set(bool v) {
  switch (type_) {
    case gstTagBoolean:
      num_data_.bVal = v;
      break;
    case gstTagInt:
      num_data_.iVal = int(v);
      break;
    case gstTagUInt:
      num_data_.uiVal = uint(v);
      break;
    case gstTagInt64:
      num_data_.i64Val = std::int64_t(v);
      break;
    case gstTagUInt64:
      num_data_.ui64Val = std::uint64_t(v);
      break;
    case gstTagFloat:
      num_data_.fVal = float(v);
      break;
    case gstTagDouble:
      num_data_.dVal = double(v);
      break;
    case gstTagString:
    case gstTagUnicode:
      qstring_->setNum(v);
      break;
  }
}

void gstValue::set(int v) {
  switch (type_) {
    case gstTagBoolean:
      num_data_.bVal = bool(v);
      break;
    case gstTagInt:
      num_data_.iVal = v;
      break;
    case gstTagUInt:
      num_data_.uiVal = uint(v);
      break;
    case gstTagInt64:
      num_data_.i64Val = std::int64_t(v);
      break;
    case gstTagUInt64:
      num_data_.ui64Val = std::uint64_t(v);
      break;
    case gstTagFloat:
      num_data_.fVal = float(v);
      break;
    case gstTagDouble:
      num_data_.dVal = double(v);
      break;
    case gstTagString:
    case gstTagUnicode:
      qstring_->setNum(v);
      break;
  }
}

void gstValue::set(unsigned int v) {
  switch (type_) {
    case gstTagBoolean:
      num_data_.bVal = bool(v);
      break;
    case gstTagInt:
      num_data_.iVal = int(v);
      break;
    case gstTagUInt:
      num_data_.uiVal = v;
      break;
    case gstTagInt64:
      num_data_.i64Val = std::int64_t(v);
      break;
    case gstTagUInt64:
      num_data_.ui64Val = std::uint64_t(v);
      break;
    case gstTagFloat:
      num_data_.fVal = float(v);
      break;
    case gstTagDouble:
      num_data_.dVal = double(v);
      break;
    case gstTagString:
    case gstTagUnicode:
      qstring_->setNum(v);
      break;
  }
}

void gstValue::set(std::int64_t v) {
  switch (type_) {
    case gstTagBoolean:
      num_data_.bVal = bool(v);
      break;
    case gstTagInt:
      num_data_.iVal = int(v);
      break;
    case gstTagUInt:
      num_data_.uiVal = uint(v);
      break;
    case gstTagInt64:
      num_data_.i64Val = v;
      break;
    case gstTagUInt64:
      num_data_.ui64Val = std::uint64_t(v);
      break;
    case gstTagFloat:
      num_data_.fVal = float(v);
      break;
    case gstTagDouble:
      num_data_.dVal = double(v);
      break;
    case gstTagString:
    case gstTagUnicode:
      char buf[kMaxString];
      snprintf(buf, kMaxString, "%llu", (unsigned long long)v);
      *qstring_ = buf;
      break;
  }
}

void gstValue::set(std::uint64_t v) {
  switch (type_) {
    case gstTagBoolean:
      num_data_.bVal = bool(v);
      break;
    case gstTagInt:
      num_data_.iVal = int(v);
      break;
    case gstTagUInt:
      num_data_.uiVal = uint(v);
      break;
    case gstTagInt64:
      num_data_.i64Val = std::int64_t(v);
      break;
    case gstTagUInt64:
      num_data_.ui64Val = v;
      break;
    case gstTagFloat:
      num_data_.fVal = float(v);
      break;
    case gstTagDouble:
      num_data_.dVal = double(v);
      break;
    case gstTagString:
    case gstTagUnicode:
      char buf[kMaxString];
      snprintf(buf, kMaxString, "%llu", (unsigned long long)v);
      *qstring_ = buf;
      break;
  }
}

void gstValue::set(float v) {
  switch (type_) {
    case gstTagBoolean:
      num_data_.bVal = bool(v);
      break;
    case gstTagInt:
      num_data_.iVal = int(v);
      break;
    case gstTagUInt:
      num_data_.uiVal = uint(v);
      break;
    case gstTagInt64:
      num_data_.i64Val = std::int64_t(v);
      break;
    case gstTagUInt64:
      num_data_.ui64Val = std::uint64_t(v);
      break;
    case gstTagFloat:
      num_data_.fVal = v;
      break;
    case gstTagDouble:
      num_data_.dVal = double(v);
      break;
    case gstTagString:
    case gstTagUnicode:
      qstring_->setNum(v);
      break;
  }
}

void gstValue::set(double v) {
  switch (type_) {
    case gstTagBoolean:
      num_data_.bVal = bool(v);
      break;
    case gstTagInt:
      num_data_.iVal = int(v);
      break;
    case gstTagUInt:
      num_data_.uiVal = uint(v);
      break;
    case gstTagInt64:
      num_data_.i64Val = std::int64_t(v);
      break;
    case gstTagUInt64:
      num_data_.ui64Val = std::uint64_t(v);
      break;
    case gstTagFloat:
      num_data_.fVal = float(v);
      break;
    case gstTagDouble:
      num_data_.dVal = v;
      break;
    case gstTagString:
    case gstTagUnicode:
      qstring_->setNum(v);
      break;
  }
}

// store value into memory buffer
void gstValue::GetRaw(const void* buf) {
  switch (type_) {
    case gstTagBoolean:
      *((bool*)buf) = num_data_.bVal;
      break;
    case gstTagInt:
      *((int*)buf) = num_data_.iVal;
      break;
    case gstTagUInt:
      *((uint*)buf) = num_data_.uiVal;
      break;
    case gstTagInt64:
      *((std::int64_t*)buf) = num_data_.i64Val;
      break;
    case gstTagUInt64:
      *((std::uint64_t*)buf) = num_data_.ui64Val;
      break;
    case gstTagFloat:
      *((float*)buf) = num_data_.fVal;
      break;
    case gstTagDouble:
      *((double*)buf) = num_data_.dVal;
      break;
    case gstTagString:
    case gstTagUnicode:
      if (qstring_->isEmpty()) {
        *((char*)buf) = '\0';
      } else {
        strcpy((char*)buf, qstring_->utf8());
      }
      break;
  }
}

// retrive value from memory buffer
int gstValue::SetRaw(const void* buf) {
  switch (type_) {
    case gstTagBoolean:
      num_data_.bVal = *((bool*)buf);
      break;
    case gstTagInt:
      num_data_.iVal = *((int*)buf);
      break;
    case gstTagUInt:
      num_data_.uiVal = *((uint*)buf);
      break;
    case gstTagInt64:
      num_data_.i64Val = *((std::int64_t*)buf);
      break;
    case gstTagUInt64:
      num_data_.ui64Val = *((std::uint64_t*)buf);
      break;
    case gstTagFloat:
      num_data_.fVal = *((float*)buf);
      break;
    case gstTagDouble:
      num_data_.dVal = *((double*)buf);
      break;
    case gstTagString:
    case gstTagUnicode:
      *qstring_ = QString::fromUtf8((char*)buf);
      return strlen((char*)buf) + 1;
  }

  return RawSize();
}


#if 0
// return a pointer to the actual memory for this object
void* gstValue::ValueAsVoidStar() const {
  switch (type_) {
    case gstTagInt:     return reinterpret_cast<void*>(&num_data_.iVal);
    case gstTagUInt:    return reinterpret_cast<void*>(&num_data_.uiVal);
    case gstTagInt64:   return reinterpret_cast<void*>(&num_data_.i64Val);
    case gstTagUInt64:  return reinterpret_cast<void*>(&num_data_.ui64Val);
    case gstTagFloat:   return reinterpret_cast<void*>(&num_data_.fVal);
    case gstTagDouble:  return reinterpret_cast<void*>(&num_data_.dVal);
    case gstTagString:
    case gstTagUnicode: return reinterpret_cast<void*>(qstring_);
  }
  return (void*)NULL;
}
#endif

bool gstValue::ValueAsBoolean() const {
  switch (type_) {
    case gstTagBoolean: return num_data_.bVal;
    case gstTagInt:    return bool(num_data_.iVal);
    case gstTagUInt:   return bool(num_data_.uiVal);
    case gstTagInt64:  return bool(num_data_.i64Val);
    case gstTagUInt64: return bool(num_data_.ui64Val);
    case gstTagFloat:  return bool(num_data_.fVal);
    case gstTagDouble: return bool(num_data_.dVal);
    case gstTagString:
    case gstTagUnicode:
      return !qstring_->isEmpty();
  }
  return 0;
}

int gstValue::ValueAsInt() const {
  switch (type_) {
    case gstTagBoolean: return int(num_data_.bVal);
    case gstTagInt:    return num_data_.iVal;
    case gstTagUInt:   return int(num_data_.uiVal);
    case gstTagInt64:  return int(num_data_.i64Val);
    case gstTagUInt64: return int(num_data_.ui64Val);
    case gstTagFloat:  return int(num_data_.fVal);
    case gstTagDouble: return int(num_data_.dVal);
    case gstTagString:
    case gstTagUnicode:
      if (qstring_->isEmpty()) {
        return 0;
      } else {
        return qstring_->toInt();
      }
  }
  return 0;
}

unsigned int gstValue::ValueAsUInt() const {
  switch (type_) {
    case gstTagBoolean: return uint(num_data_.bVal);
    case gstTagInt:    return uint(num_data_.iVal);
    case gstTagUInt:   return num_data_.uiVal;
    case gstTagInt64:  return uint(num_data_.i64Val);
    case gstTagUInt64: return uint(num_data_.ui64Val);
    case gstTagFloat:  return uint(num_data_.fVal);
    case gstTagDouble: return uint(num_data_.dVal);
    case gstTagString:
    case gstTagUnicode:
      if (qstring_->isEmpty()) {
        return 0;
      } else {
        return qstring_->toUInt();
      }
  }
  return 0;
}

std::int64_t gstValue::ValueAsInt64() const {
  switch (type_) {
    case gstTagBoolean: return std::int64_t(num_data_.bVal);
    case gstTagInt:    return std::int64_t(num_data_.iVal);
    case gstTagUInt:   return std::int64_t(num_data_.uiVal);
    case gstTagInt64:  return std::int64_t(num_data_.i64Val);
    case gstTagUInt64: return num_data_.ui64Val;
    case gstTagFloat:  return std::int64_t(num_data_.fVal);
    case gstTagDouble: return std::int64_t(num_data_.dVal);
    case gstTagString:
    case gstTagUnicode:
      if (qstring_->isEmpty()) {
        return 0;
      } else {
        return std::int64_t(strtoll(qstring_->latin1(), NULL, 0));
      }
  }
  return 0;
}

 std::uint64_t gstValue::ValueAsUInt64() const {
  switch (type_) {
    case gstTagBoolean: return std::uint64_t(num_data_.bVal);
    case gstTagInt:     return std::uint64_t(num_data_.iVal);
    case gstTagUInt:    return std::uint64_t(num_data_.uiVal);
    case gstTagInt64:   return std::uint64_t(num_data_.i64Val);
    case gstTagUInt64:  return num_data_.ui64Val;
    case gstTagFloat:   return std::uint64_t(num_data_.fVal);
    case gstTagDouble:  return std::uint64_t(num_data_.dVal);
    case gstTagString:
    case gstTagUnicode:
      if (qstring_->isEmpty()) {
        return 0;
      } else {
        return static_cast<std::uint64_t>(strtoull(qstring_->latin1(), NULL, 0));
      }
  }
  return 0;
}

float gstValue::ValueAsFloat() const {
  switch (type_) {
    case gstTagBoolean: return static_cast<float>(num_data_.bVal);
    case gstTagInt:    return static_cast<float>(num_data_.iVal);
    case gstTagUInt:   return static_cast<float>(num_data_.uiVal);
    case gstTagInt64:  return static_cast<float>(num_data_.i64Val);
    case gstTagUInt64: return static_cast<float>(num_data_.ui64Val);
    case gstTagFloat:  return num_data_.fVal;
    case gstTagDouble: return static_cast<float>(num_data_.dVal);
    case gstTagString:
    case gstTagUnicode:
      if (qstring_->isEmpty()) {
        return 0.0;
      } else {
        return qstring_->toFloat();
      }
  }
  return 0.0;
}

double gstValue::ValueAsDouble() const {
  switch (type_) {
    case gstTagBoolean: return static_cast<double>(num_data_.bVal);
    case gstTagInt:     return static_cast<double>(num_data_.iVal);
    case gstTagUInt:    return static_cast<double>(num_data_.uiVal);
    case gstTagInt64:   return static_cast<double>(num_data_.i64Val);
    case gstTagUInt64:  return static_cast<double>(num_data_.ui64Val);
    case gstTagFloat:   return static_cast<double>(num_data_.fVal);
    case gstTagDouble:  return num_data_.dVal;
    case gstTagString:
    case gstTagUnicode:
      if (qstring_->isEmpty()) {
        return 0.0;
      } else {
        return qstring_->toDouble();
      }
  }
  return 0.0;
}

std::string gstValue::ValueAsString() const {
  if (type_ == gstTagString || type_ == gstTagUnicode) {
    return std::string(qstring_->utf8());
  }

  // handle all numeric types below
  char str[kMaxString];
  switch (type_) {
    case gstTagBoolean:
      snprintf(str, kMaxString, "%d", num_data_.bVal);
      break;
    case gstTagInt:
      snprintf(str, kMaxString, "%d", num_data_.iVal);
      break;
    case gstTagUInt:
      snprintf(str, kMaxString, "%u", num_data_.uiVal);
      break;
    case gstTagInt64:
      snprintf(str, kMaxString, "%lld", (long long)num_data_.i64Val);
      break;
    case gstTagUInt64:
      snprintf(str, kMaxString, "%llu", (unsigned long long)num_data_.ui64Val);
      break;
    case gstTagFloat:
      snprintf(str, kMaxString, "%.16f", num_data_.fVal);
      break;
    case gstTagDouble:
      snprintf(str, kMaxString, "%.20lf", num_data_.dVal);
      break;
  }

  // cleanup float precision digits
  if (type_ == gstTagFloat || type_ == gstTagDouble) {
    // remove trailing zeros
    char* end = &str[strlen(str) - 1];
    while (end > str && *end == '0') {
      *end = '\0';
      --end;

      // look for lonley decimal point
      if (*end == '.') {
        *end = '\0';
        break;
      }
    }
  }

  return std::string(str);
}

QString gstValue::ValueAsUnicode() const {
  if (type_ == gstTagString || type_ == gstTagUnicode) {
    return *qstring_;
  }

  QString string;
  switch (type_) {
    case gstTagBoolean:
      string.setNum(num_data_.bVal);
      break;
    case gstTagInt:
      string.setNum(num_data_.iVal);
      break;
    case gstTagUInt:
      string.setNum(num_data_.uiVal);
      break;
    case gstTagInt64:
      string = ValueAsString().c_str();
      break;
    case gstTagUInt64:
      string = ValueAsString().c_str();
      break;
    case gstTagFloat:
      string = ValueAsString().c_str();
      break;
    case gstTagDouble:
      string = ValueAsString().c_str();
      break;
  }

  return string;
}



 std::uint32_t gstValue::TypeFromName(const char* str) {
  if (!strcmp(str, "int")) {
    return gstTagInt;
  } else if (!strcmp(str, "uint")) {
    return gstTagUInt;
  } else if (!strcmp(str, "int64")) {
    return gstTagInt64;
  } else if (!strcmp(str, "uint64")) {
    return gstTagUInt64;
  } else if (!strcmp(str, "float")) {
    return gstTagFloat;
  } else if (!strcmp(str, "double")) {
    return gstTagDouble;
  } else if (!strcmp(str, "string")) {
    return gstTagString;
  } else if (!strcmp(str, "unicode")) {
    return gstTagUnicode;
  }

  notify(NFY_WARN, "Unknown type specified: %s", str);
  return gstTagInvalid;
}

const char* gstValue::NameFromType(std::uint32_t type) {
  switch (type) {
    case gstTagBoolean:
      return "bool";
    case gstTagInt:
      return "int";
    case gstTagUInt:
      return "uint";
    case gstTagInt64:
      return "int64";
    case gstTagUInt64:
      return "uint64";
    case gstTagFloat:
      return "float";
    case gstTagDouble:
      return "double";
    case gstTagString:
      return "string";
    case gstTagUnicode:
      return "unicode";
  }

  notify(NFY_WARN, "Unknown type specified: %d", type);
  return NULL;
}


void gstValue::SetName(const char* name) {
  delete [] name_;
  name_ = strdupSafe(name);
}

const char* gstValue::Name() const {
  return name_;
}
