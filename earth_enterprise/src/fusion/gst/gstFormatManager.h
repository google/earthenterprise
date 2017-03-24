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

#ifndef KHSRC_FUSION_GST_GSTFORMATMANAGER_H__
#define KHSRC_FUSION_GST_GSTFORMATMANAGER_H__

#include <qstring.h>
#include <gstFormat.h>
#include <vector>
#include <common/base/macros.h>

class QRegExp;

class MetaFormat {
 public:
  MetaFormat(gstFormatNewFmtFunc fmtFunc,
             const char* desc, const char* shortDesc,
             const char* filter);
  ~MetaFormat();

  gstFormat* Match(const char* nm);
  const char* ShortDesc() const { return short_description_.latin1(); }
  const char* Filter() const { return filter_.latin1(); }
  QString FileDialogFilterString() const;

 private:
  gstFormatNewFmtFunc new_format_func_;
  QString description_;
  QString short_description_;
  QString filter_;

  std::vector<QRegExp*> patterns_;

  DISALLOW_COPY_AND_ASSIGN(MetaFormat);
};

class gstFormatManager {
 public:
  gstFormatManager();
  ~gstFormatManager();

  gstFormat* FindFormat(const char* path);

  int GetFilterCount() { return formats_.size(); }
  QString GetFilter(int idx) {
    return formats_[idx]->FileDialogFilterString();
  }

  template <class T>
  void RegisterFormat(const char* desc, const char* short_desc,
                      const char* filter) {
    MetaFormat* meta_format = new MetaFormat(
        &gstFormatManager::CreateFormat<T>, desc,
        short_desc, filter);
    formats_.push_back(meta_format);
  }

  template <class T, bool is_mercator_imagery>
  void RegisterFormatAsset(const char* desc, const char* short_desc,
                      const char* filter) {
    MetaFormat* meta_format = new MetaFormat(
        &gstFormatManager::CreateTextFormat<T, is_mercator_imagery>, desc,
        short_desc, filter);
    formats_.push_back(meta_format);
  }

 private:
  std::vector<MetaFormat*> formats_;

  template <class T>
  static gstFormat *CreateFormat(const char* n) {
    return new T(n);
  }

  template<class T, bool is_mercator_imagery>
  static gstFormat* CreateTextFormat(const char* n) {
    return new T(n, is_mercator_imagery);
  }
};

extern gstFormatManager* theFormatManager(void);

#endif  // !KHSRC_FUSION_GST_GSTFORMATMANAGER_H__
