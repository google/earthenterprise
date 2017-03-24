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

#ifndef KHSRC_FUSION_GST_GSTVALUE_H__
#define KHSRC_FUSION_GST_GSTVALUE_H__

#include <stdio.h>
#include <string>
#include <assert.h>
#include <gstTypes.h>
#include <string.h>
#include <common/base/macros.h>

class QString;
class QTextCodec;

extern const char* FormatAsCSV(const char *);
extern std::string ExtractString(const char* ibuf, int sz,
                                 bool* check_highbit = NULL);

class gstValue {
 public:
  gstValue(int v)            : type_(gstTagInt)     { Init(); set(v); }
  gstValue(uint v)           : type_(gstTagUInt)    { Init(); set(v); }
  gstValue(int64 v)          : type_(gstTagInt64)   { Init(); set(v); }
  gstValue(uint64 v)         : type_(gstTagUInt64)  { Init(); set(v); }
  gstValue(float v)          : type_(gstTagFloat)   { Init(); set(v); }
  gstValue(double v)         : type_(gstTagDouble)  { Init(); set(v); }
  gstValue(const char* v)    : type_(gstTagUnicode) { Init(); set(v); }
  gstValue(const QString& v) : type_(gstTagUnicode) { Init(); set(v); }

  gstValue(const gstValue& that) : type_(that.type_) { Init(); Copy(that); }

  ~gstValue();

  static gstValue* NewValueByType(uint32 type) {
    int default_type = 0;
    gstValue* nv = new gstValue(default_type);
    nv->type_ = type;
    nv->Init();
    return nv;
  }

  gstValue& Copy(const gstValue& v);

  uint32 Type() const { return type_; }

  bool IsNumber() const {
    return type_ == gstTagInt || type_ == gstTagUInt ||
           type_ == gstTagInt64 || type_ == gstTagUInt64 ||
           type_ == gstTagFloat || type_ == gstTagDouble;
  }

  bool IsString() const {
    return type_ == gstTagString || type_ == gstTagUnicode;
  }

  // Comparison operators
  bool operator==(const gstValue& v) const;
  bool operator!=(const gstValue& v) const;
  bool operator<=(const gstValue& v) const;
  bool operator>=(const gstValue& v) const;
  bool operator<(const gstValue& v) const;
  bool operator>(const gstValue& v) const;

  bool MatchesWildcard(const gstValue& v) const;

  gstValue& operator*=(const gstValue& v);

  // Copy operator
  gstValue& operator=(const gstValue& v);

  // sorting compare
  int compare(const gstValue& v) const;

  void set(bool v);
  void set(int v);
  void set(uint v);
  void set(int64 v);
  void set(uint64 v);
  void set(float v);
  void set(double v);
  void set(const char* buf, int len = -1);
  void set(const QString& v);

  void* ValueAsVoidStar() const;
  bool ValueAsBoolean() const;
  int ValueAsInt() const;
  uint ValueAsUInt() const;
  int64 ValueAsInt64() const;
  uint64 ValueAsUInt64() const;
  float ValueAsFloat() const;
  double ValueAsDouble() const;
  std::string ValueAsString() const;
  QString ValueAsUnicode() const;
  std::string ValueAsCSV() const;

  void SetCodec(QTextCodec* c);
  QTextCodec* Codec() const { return codec_; }
  bool NeedCodec() const { return need_codec_; }

  static uint32 TypeFromName(const char *str);
  static const char* NameFromType(uint32 type);

  void GetRaw(const void* data);
  int SetRaw(const void* data);
  int RawSize();
  static int RawSize(uchar type, const char* buf);

  bool IsEmpty() const;

  void SetName(const char* name);
  const char* Name() const;

  static int vcount;
  static int scount;
  static int icount;
  static int dcount;

 private:
  void Init();

  // WARNING: fields arranged for efficient packing
  // be careful what you add (and where you add it)

  // can only be one numeric type
  union {
    bool   bVal;
    int    iVal;
    uint   uiVal;
    int64  i64Val;
    uint64 ui64Val;
    float  fVal;
    double dVal;
  } num_data_;

  QString* qstring_;
  QTextCodec* codec_;
  char* name_;

  uchar type_;
  bool need_codec_;
  // trigger creation of new string space whenever value
  // has been changed
  mutable bool reset_;
};

#endif  // !KHSRC_FUSION_GST_GSTVALUE_H__
