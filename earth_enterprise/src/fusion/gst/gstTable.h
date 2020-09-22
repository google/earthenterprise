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

//
// This is a virtual table
//
// This means that the records are only read in when
// requested.  Initial opening of the db prepares it
// for subsequent record requests.
//

#ifndef KHSRC_FUSION_GST_GSTTABLE_H__
#define KHSRC_FUSION_GST_GSTTABLE_H__

#include <gstTypes.h>
#include <notify.h>
#include <gstMemory.h>
#include <gstRecord.h>
#include <khRefCounter.h>

class gstTable : public gstMemory {
 public:
  gstTable(const char* n);
  virtual ~gstTable();

  // derived tables must implement these
  virtual gstStatus Open(gstReadMode) = 0;
  virtual gstStatus Close() = 0;

  virtual gstRecordHandle Row(std::uint32_t r) = 0;

  std::uint32_t NumRows() const { return num_rows_; }
  std::uint32_t NumColumns() const { return num_columns_; }

  gstRecordHandle NewRecord();

  gstHeaderHandle GetHeader() const { return header_; }

  virtual void SetHeader(const gstHeaderHandle hdr);

 protected:
  std::uint32_t num_rows_;
  std::uint32_t num_columns_;                 // per record
  gstHeaderHandle header_;
};

#endif  // !KHSRC_FUSION_GST_GSTTABLE_H__
