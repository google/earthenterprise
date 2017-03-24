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

#ifndef KHSRC_FUSION_GST_GSTTXTTABLE_H__
#define KHSRC_FUSION_GST_GSTTXTTABLE_H__

#include <khArray.h>
#include <gstTable.h>
#include <vector>
#include <string>

class gstRecord;
class gstFileInfo;

#define TXT_INDEX_MAGIC "KEYIDNEX"

class gstTXTTable : public gstTable {
 public:
  enum FileType { Delimited, FixedWidth, Undefined };

  gstTXTTable(const char* n);
  ~gstTXTTable();

  virtual gstStatus Open(gstReadMode);
  virtual gstStatus Close();

  bool Readline(uint32 row);
  virtual gstRecordHandle Row(uint32 r);

  gstStatus Status() const { return status_; }

  gstStatus BuildIndex();
  gstStatus LoadIndex();

  FileType GetFileType() const { return file_type_; }
  void SetFileType(FileType ftype) { file_type_ = ftype; }
  bool SetFileType(const char* ftype);

  const char Delimiter() const { return delimiter_; }
  void SetDelimiter(char d) { delimiter_ = d; }

  uint32 SkipRows() const { return skip_rows_; }
  void SetSkipRows(uint32 s) { skip_rows_ = s; }

 private:
  struct IndexHeader {
    char magic[8];     // KEYINDEX
    int32 numrows;
    int32 numcols;
    int64 mtime;       // mod time of txt file
    int64 offset;      // where index table starts
  };

  int file_descriptor_;
  gstFileInfo* file_info_;

  FileType file_type_;
  char delimiter_;
  uint32 skip_rows_;

  std::vector<off64_t> record_index_;

  gstStatus status_;

  gstReadMode read_mode_;

  std::string row_buffer_;
};

#endif  // !KHSRC_FUSION_GST_GSTTXTTABLE_H__
