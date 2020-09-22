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


#include <gstRecord.h>
#include <gstGeode.h>
#include <qtextcodec.h>
#include <gstJobStats.h>

// ----------------------------------------------------------------------------

int gstHeaderImpl::hcount = 0;

gstHeaderImpl::gstHeaderImpl() {
  ++hcount;
  codec_ = NULL;
}

gstHeaderImpl::gstHeaderImpl(const gstHeaderImpl& that)
    : field_specs_(that.field_specs_) {
  ++hcount;
  codec_ = NULL;
}

gstHeaderImpl::~gstHeaderImpl() {
  --hcount;
}

bool gstHeaderImpl::operator==(const gstHeaderImpl& b) const {
  if (numColumns() != b.numColumns())
    return false;

  return field_specs_ == b.field_specs_;
}

bool gstHeaderImpl::operator!=(const gstHeaderImpl& b) const {
  return !(*this == b);
}

void gstHeaderImpl::SetCodec(QTextCodec* codec) {
  codec_ = codec;
}

 std::uint32_t gstHeaderImpl::RawSize(const gstRecordHandle rec) {
  if (rec->IsEmpty()) {
    return 0;
  } else {
    std::uint32_t sz = 0;
    for (unsigned int ii = 0; ii < numColumns(); ++ii)
      sz += rec->Field(ii)->RawSize();
    return sz;
  }
}

char* gstHeaderImpl::ToRaw(const gstRecordHandle rec, char* buf) {
  if (rec->IsEmpty())
    return NULL;

  int bufsz = RawSize(rec);

  if (buf == NULL)
    buf = static_cast<char*>(malloc(bufsz * sizeof(char)));

  char* tbuf = buf;

  for (unsigned int ii = 0; ii < numColumns(); ++ii) {
    gstValue* field = rec->Field(ii);
    field->GetRaw(tbuf);
    tbuf += field->RawSize();
  }

  return buf;
}

int gstHeaderImpl::FieldPosByName(const QString& fieldName) {
  for (unsigned int col = 0; col < numColumns(); ++col) {
    if (Name(col) == fieldName) {
      return col;
    }
  }
  return -1;
}

#ifdef JOBSTATS_ENABLED
enum {JOBSTATS_FROMRAW, JOBSTATS_NEWREC, JOBSTATS_SETRAW, JOBSTATS_RAWSIZE};
static gstJobStats::JobName JobNames[] = {
  {JOBSTATS_FROMRAW,  "From Raw     "},
  {JOBSTATS_NEWREC,   "+ New Record "},
  {JOBSTATS_SETRAW,   "+ Set Raw    "}
};
gstJobStats* raw_stats = new gstJobStats("RAW RECORD", JobNames, 3);
#endif

gstRecordHandle gstHeaderImpl::FromRaw(const char* buf, int size) {
  JOBSTATS_SCOPED(raw_stats, JOBSTATS_FROMRAW);
  if (buf == NULL || size == 0) {
    notify(NFY_FATAL, "cannot create a record from nothing!");
  }

  gstRecordHandle rec = khRefGuardFromNew(
      new gstRecordImpl(numColumns(), khRefGuardFromThis()));

  // make a copy of the raw buffer for future expansion
  // and set all fields to NULL indicating that they still
  // need expansion
  rec->lazy_expansion_ = true;
  rec->raw_buffer_ = std::string(buf, size);
  rec->fields_.resize(numColumns(), NULL);
  rec->raw_offsets_.resize(numColumns(), 0);

  // must find the beginning of each field in the raw buffer
  const char *tbuf = buf;
  for (unsigned int col = 0; col < numColumns(); ++col) {
    rec->raw_offsets_[col] = tbuf - buf;
    tbuf += gstValue::RawSize(ftype(col), tbuf);
  }

  return rec;
}

gstRecordHandle gstHeaderImpl::NewRecord() {
  gstRecordHandle rec = khRefGuardFromNew(
      new gstRecordImpl(numColumns(), khRefGuardFromThis()));

  for (unsigned int ii = 0; ii < numColumns(); ++ii) {
    gstValue* val = gstValue::NewValueByType(ftype(ii));
    if (codec_)
      val->SetCodec(codec_);
    rec->AddField(val);
  }

  return rec;
}

void gstHeaderImpl::addSpec(const FieldSpec& spec) {
  field_specs_.push_back(spec);
}

void gstHeaderImpl::addSpec(const char* n, std::uint32_t t, int l, double m) {
  FieldSpec fspec;
  fspec.name = QString::fromUtf8(n);
  fspec.ftype = t;
  fspec.length = l;
  fspec.multiplier = m;
  addSpec(fspec);
}

void gstHeaderImpl::addSpec(const QString &n, std::uint32_t t, int l, double m) {
  FieldSpec fspec;
  fspec.name = n;
  fspec.ftype = t;
  fspec.length = l;
  fspec.multiplier = m;
  addSpec(fspec);
}

gstHeaderHandle gstHeaderImpl::BuildFromRegistry(gstRegistry::Group* regGrp) {
  gstHeaderHandle new_header = gstHeaderImpl::Create();

  for (int ii = 0; ii < regGrp->numGroups(); ++ii) {
    gstRegistry::Group* specGrp = regGrp->group(ii);
    gstValue* name_tag = specGrp->findTag("Name");
    gstValue* type_tag = specGrp->findTag("Type");
    gstValue* len_tag = specGrp->findTag("Length");

    if (!name_tag || !type_tag) {
      notify(NFY_WARN, "Bad Record Spec %d", ii);
      return new_header;
    }

    std::uint32_t type = gstValue::TypeFromName(type_tag->ValueAsString().c_str());
    if (type == gstTagInvalid) {
      notify(NFY_WARN, "Record has bad type");
      return new_header;
    }

    if (len_tag != NULL) {
      new_header->addSpec(name_tag->ValueAsString().c_str(), type,
                          len_tag->ValueAsInt());
    } else {
      new_header->addSpec(name_tag->ValueAsString().c_str(), type);
    }
  }

  return new_header;
}

#if 0
void gstHeaderImpl::BuildRegistry(gstRegistry::Group *regGrp) {
  for (unsigned int ii = 0; ii < field_specs_.size(); ++ii) {
    gstRegistry::Group* specGrp = regGrp->addGroup(gstValue(ii).ValueAsString());
    specGrp->addTag(new gstValue(field_specs_[ii].name, "Name"));
    specGrp->addTag(new gstValue(
                        gstValue::NameFromType(field_specs_[ii].ftype), "Type"));
    if (field_specs_[ii].length != -1)
      specGrp->addTag(new gstValue(field_specs_[ii].length, "Length"));
  }
}
#endif

// ----------------------------------------------------------------------------

int gstRecordImpl::rcount = 0;

gstRecordImpl::gstRecordImpl()
  : lazy_expansion_(false) {
  ++rcount;
}

gstRecordImpl::gstRecordImpl(unsigned int length, gstHeaderHandle header)
  : header_(header),
    lazy_expansion_(false) {
  ++rcount;
  fields_.reserve(length);
}

gstRecordImpl::~gstRecordImpl() {
  for (std::vector<gstValue*>::iterator it = fields_.begin();
       it != fields_.end(); ++it) {
    delete *it;
  }
  --rcount;
}

gstValue* gstRecordImpl::Field(unsigned int col) {
  assert(col < NumFields());
  if (!lazy_expansion_) {
    return fields_[col];
  } else {
    JOBSTATS_SCOPED(raw_stats, JOBSTATS_SETRAW);
    if (fields_[col] == NULL) {
      gstValue* val = gstValue::NewValueByType(header_->ftype(col));
      val->SetRaw(raw_buffer_.data() + raw_offsets_[col]);
      fields_[col] = val;
    }
    return fields_[col];
  }
}

gstValue* gstRecordImpl::FieldByName(const QString& fieldName) {
  for (unsigned int col = 0; col < NumFields(); ++col) {
    if (header_->Name(col) == fieldName) {
      if (fields_[col] == NULL) {
        gstValue* val = gstValue::NewValueByType(header_->ftype(col));
        val->SetRaw(raw_buffer_.data() + raw_offsets_[col]);
        fields_[col] = val;
      }
      return fields_[col];
    }
  }
  return NULL;
}

bool gstRecordImpl::ValidateEncoding() const {
  for (std::vector<gstValue*>::const_iterator it = fields_.begin();
       it != fields_.end(); ++it) {
    if ((*it)->NeedCodec())
      return false;
  }

  return true;
}

// ----------------------------------------------------------------------------

gstRecordFormatter::gstRecordFormatter(
    const QString& format, gstHeaderHandle &hdr)
    : format_(format) {
  const QChar vstart(0xab);  // opening symbol '«'
  const QChar vend(0xbb);    // closing symbol '»'

  int varpos = 0;
  while ((varpos = format_.find(vstart, varpos)) != -1) {
    bool match_found = false;

    // search all field names for a match
    for (unsigned int ii = 0; ii < hdr->numColumns(); ++ii) {
      QString check_me = QString(
          "%1%2%3").arg(vstart).arg(hdr->Name(ii)).arg(vend);
      if (format_.find(check_me, varpos) == varpos) {
        arg_pos_.push_back(varpos);
        arg_.push_back(ii);
        format_ = format_.remove(varpos, check_me.length());
        match_found = true;
        break;
      }
    }

    if (!match_found)
      ++varpos;
  }
}

gstRecordFormatter::gstRecordFormatter(const gstRecordFormatter& that) {
  format_ = that.format_;
  arg_pos_ = that.arg_pos_;
  arg_ = that.arg_;
}


gstRecordFormatter::~gstRecordFormatter() {
}

QString gstRecordFormatter::out(gstRecordHandle &rec) {
  QString result(format_.unicode(), format_.length());

  if (rec) {
    unsigned int pos = arg_.size();
    while (pos) {
      --pos;
      result.insert(arg_pos_[pos], rec->Field(arg_[pos])->ValueAsUnicode());
    }
  }

  return result;
}
