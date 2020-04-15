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

#ifndef KHSRC_FUSION_GST_GSTDBFTABLE_H__
#define KHSRC_FUSION_GST_GSTDBFTABLE_H__

#include <khArray.h>
#include <gstTable.h>

// must be exactly 32 bytes long
struct DBF_FileHeader {
  unsigned char Version[1];
  unsigned char Date[3];
  unsigned char RecordNum[4];
  unsigned char HeaderLength[2];
  unsigned char RecordLength[2];
  unsigned char Reserved[20];
};

struct DBF_FileHeader_i {
  int Version;
  unsigned int Year;
  unsigned int Month;
  unsigned int Day;
  unsigned int RecordNum;
  std::uint16_t HeaderLength;
  std::uint16_t RecordLength;
};

// must be exactly 32 bytes long
struct DBF_FieldDescriptor {
  char Name[11];
  unsigned char Type[1];
  unsigned char Offset[4];
  unsigned char Length[1];
  unsigned char DecimalCount[1];
  unsigned char Reserved[14];
};

struct DBF_FieldDescriptor_i {
  char Name[12];
  std::uint32_t Type;
  std::uint32_t Offset;
  std::uint16_t Length;
  std::uint16_t DecimalCount;
};



class gstDBFTable : public gstTable {
 public:
  gstDBFTable(const char* n);
  virtual ~gstDBFTable();

  gstStatus Open(gstReadMode m);
  gstStatus Close();

  gstRecordHandle Row(std::uint32_t r);

  gstStatus ReadHeader();
  gstStatus ReadFieldDescriptors();

 private:
  FILE* file_pointer_;

  khArray<DBF_FieldDescriptor_i*> _fieldList;
  DBF_FileHeader_i _fileHeader;
};

#endif  // !KHSRC_FUSION_GST_GSTDBFTABLE_H__
