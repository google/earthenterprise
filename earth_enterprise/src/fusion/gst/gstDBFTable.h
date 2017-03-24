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

#ifndef KHSRC_FUSION_GST_GSTDBFTABLE_H__
#define KHSRC_FUSION_GST_GSTDBFTABLE_H__

#include <khArray.h>
#include <gstTable.h>

// must be exactly 32 bytes long
struct DBF_FileHeader {
  uchar Version[1];
  uchar Date[3];
  uchar RecordNum[4];
  uchar HeaderLength[2];
  uchar RecordLength[2];
  uchar Reserved[20];
};

struct DBF_FileHeader_i {
  int Version;
  uint Year;
  uint Month;
  uint Day;
  uint RecordNum;
  uint16 HeaderLength;
  uint16 RecordLength;
};

// must be exactly 32 bytes long
struct DBF_FieldDescriptor {
  char Name[11];
  uchar Type[1];
  uchar Offset[4];
  uchar Length[1];
  uchar DecimalCount[1];
  uchar Reserved[14];
};

struct DBF_FieldDescriptor_i {
  char Name[12];
  uint32 Type;
  uint32 Offset;
  uint16 Length;
  uint16 DecimalCount;
};



class gstDBFTable : public gstTable {
 public:
  gstDBFTable(const char* n);
  virtual ~gstDBFTable();

  gstStatus Open(gstReadMode m);
  gstStatus Close();

  gstRecordHandle Row(uint32 r);

  gstStatus ReadHeader();
  gstStatus ReadFieldDescriptors();

 private:
  FILE* file_pointer_;

  khArray<DBF_FieldDescriptor_i*> _fieldList;
  DBF_FileHeader_i _fileHeader;
};

#endif  // !KHSRC_FUSION_GST_GSTDBFTABLE_H__
