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

#ifndef KHSRC_FUSION_GST_GSTKVPTABLE_H__
#define KHSRC_FUSION_GST_GSTKVPTABLE_H__

#include <gstTable.h>
#include <khArray.h>
#include <khTimer.h>

class gstKVPTable : public gstTable {
 public:
  struct FileHeader {
    uint32 magic;
    uint32 version;
    uint32 numRecords;
    int32 numFields;
    int32 fieldDefsSize;
    int32 filler;
    int64 indexOffset;
  };

  struct RecordPos {
    int64 offset;
    uint32 size;
    uint32 filler;
  };

  gstKVPTable(const char*);
  ~gstKVPTable();

  gstStatus Open(gstReadMode);
  gstStatus Close();

  gstRecordHandle Row(uint32 r);

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
