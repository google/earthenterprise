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

#ifndef KHSRC_FUSION_GST_GSTRECORD_H__
#define KHSRC_FUSION_GST_GSTRECORD_H__

#include <gstRegistry.h>
#include <gstMisc.h>
#include <gstValue.h>
#include <khRefCounter.h>
#include <khMTTypes.h>
#include <deque>

class QTextCodec;
class gstSource;
class gstHeaderImpl;
typedef khRefGuard<gstHeaderImpl> gstHeaderHandle;
class gstRecordImpl;
typedef khRefGuard<gstRecordImpl> gstRecordHandle;
typedef std::deque<gstRecordHandle> RecordList;
typedef RecordList::const_iterator RecordListIterator;

class gstHeaderImpl : public khMTRefCounter {
 public:
  struct FieldSpec {
    QString name;
    std::uint32_t ftype;
    int length;
    double multiplier;
    bool operator==(const FieldSpec& b) const {
      // forcing the column name comparison is now case-insensitive.
      return (name.lower() == b.name.lower() && ftype == b.ftype &&
              length == b.length && multiplier == b.multiplier);
    }
  };

  static gstHeaderHandle Create() {
    return khRefGuardFromNew(new gstHeaderImpl());
  }

  static gstHeaderHandle Create(gstHeaderHandle that) {
    return khRefGuardFromNew(new gstHeaderImpl(*that));
  }

  ~gstHeaderImpl();

  static int hcount;

  bool operator==(const gstHeaderImpl& b) const;
  bool operator!=(const gstHeaderImpl& b) const;

  void SetCodec(QTextCodec* codec);

  // returns -1 if not found, otherwise returns index
  int FieldPosByName(const QString& fieldName);

  const QString & Name(int c) const { return field_specs_[c].name; }
  //  const char* name(int c) const { return field_specs_[c].name.latin1(); }
  std::uint32_t ftype(int c) const { return field_specs_[c].ftype; }
  int length(int c) const { return field_specs_[c].length; }
  double mult(int c) const { return field_specs_[c].multiplier; }
  unsigned int numColumns() const { return field_specs_.size(); }
  bool HasAttrib(void) const {
    return (numColumns() > 0);
  }

  void addSpec(const FieldSpec& spec);
  void addSpec(const char* n, std::uint32_t t, int l = -1, double m = 0.0);
  void addSpec(const QString &n, std::uint32_t t, int l = -1, double m = 0.0);

  gstRecordHandle NewRecord();
  std::uint32_t RawSize(const gstRecordHandle rec);
  gstRecordHandle FromRaw(const char* buf, int size);
  char* ToRaw(const gstRecordHandle rec, char* buf);

  static gstHeaderHandle BuildFromRegistry(gstRegistry::Group* grp);

 private:
  std::vector<FieldSpec> field_specs_;
  gstHeaderImpl();
  gstHeaderImpl(const gstHeaderImpl& that);
  QTextCodec *codec_;
};

// ----------------------------------------------------------------------------

class gstRecordImpl : public khMTRefCounter {
 public:
  gstRecordImpl();
  ~gstRecordImpl();

  unsigned int NumFields() const { return fields_.size(); }
  bool IsEmpty() const { return NumFields() == 0; }
  gstValue* Field(unsigned int c);
  gstValue* FieldByName(const QString& fieldName);
  gstHeaderHandle Header() const { return header_; }
  bool ValidateEncoding() const;

  static int rcount;
 private:
  friend class gstHeaderImpl;
  friend void TestSelector();
  gstRecordImpl(unsigned int l, gstHeaderHandle header);
  void AddField(gstValue* v) { fields_.push_back(v); }

  gstHeaderHandle header_;
  std::vector<gstValue*> fields_;

  bool lazy_expansion_;
  std::string raw_buffer_;
  std::vector<int> raw_offsets_;
};

// ----------------------------------------------------------------------------

class gstRecordFormatter {
 public:
  gstRecordFormatter(const QString& fmt, gstHeaderHandle&);
  gstRecordFormatter(const gstRecordFormatter& fmt);
  ~gstRecordFormatter();
  QString out(gstRecordHandle& rec);

 private:
  QString format_;
  std::vector<int> arg_pos_;
  std::vector<int> arg_;
};

#endif  // !KHSRC_FUSION_GST_GSTRECORD_H__
