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

  bool Readline(std::uint32_t row);
  virtual gstRecordHandle Row(std::uint32_t r);

  gstStatus Status() const { return status_; }

  gstStatus BuildIndex();
  gstStatus LoadIndex();

  FileType GetFileType() const { return file_type_; }
  void SetFileType(FileType ftype) { file_type_ = ftype; }
  bool SetFileType(const char* ftype);

  const char Delimiter() const { return delimiter_; }
  void SetDelimiter(char d) { delimiter_ = d; }

  std::uint32_t SkipRows() const { return skip_rows_; }
  void SetSkipRows(std::uint32_t s) { skip_rows_ = s; }

 private:
  struct IndexHeader {
    char magic[8];     // KEYINDEX
    std::int32_t numrows;
    std::int32_t numcols;
    std::int64_t mtime;       // mod time of txt file
    std::int64_t offset;      // where index table starts
  };

  int file_descriptor_;
  gstFileInfo* file_info_;

  FileType file_type_;
  char delimiter_;
  std::uint32_t skip_rows_;

  std::vector<off64_t> record_index_;

  gstStatus status_;

  gstReadMode read_mode_;

  std::string row_buffer_;
};

#endif  // !KHSRC_FUSION_GST_GSTTXTTABLE_H__
