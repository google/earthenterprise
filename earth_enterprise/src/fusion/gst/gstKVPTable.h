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

#ifndef KHSRC_FUSION_GST_GSTKVPTABLE_H__
#define KHSRC_FUSION_GST_GSTKVPTABLE_H__

#include <gstTable.h>
#include <khArray.h>
#include <khTimer.h>

class gstKVPTable : public gstTable {
 public:
  struct FileHeader {
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t numRecords;
    std::int32_t numFields;
    std::int32_t fieldDefsSize;
    std::int32_t filler;
    std::int64_t indexOffset;
  };

  struct RecordPos {
    std::int64_t offset;
    std::uint32_t size;
    std::uint32_t filler;
  };

  gstKVPTable(const char*);
  ~gstKVPTable();

  gstStatus Open(gstReadMode);
  gstStatus Close();

  gstRecordHandle Row(std::uint32_t r);

  gstStatus AddRecord(const gstRecordHandle rec);

  void SetHeader(const gstHeaderHandle& hdr);

 private:
  int fd_;
  gstReadMode mode_;

  FileHeader file_header_;

  gstStatus ReadHeader();
  gstStatus Flush();
  bool ReadIndex();

  bool modified_;
  bool have_header_;

  RecordPos* index_map_;
  khArray<RecordPos> index_array_;
};

#endif  // !KHSRC_FUSION_GST_GSTKVPTABLE_H__
