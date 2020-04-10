// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <alloca.h>
#include <string.h>

#include <notify.h>
#include <gstFormat.h>
#include <gstDBFTable.h>
#include <gstRecord.h>

gstDBFTable::gstDBFTable(const char* n)
    : gstTable(n) {
}


gstDBFTable::~gstDBFTable() {
  Close();
}

gstStatus gstDBFTable::Open(gstReadMode read_mode) {
  if ((file_pointer_ = fopen(name(), "r")) <= 0)
    return GST_OPEN_FAIL;

  if (ReadHeader() != GST_OKAY) {
    notify(NFY_WARN, "Unable to read header of dbf file %s", name());
    fclose(file_pointer_);
    return GST_READ_FAIL;
  }

  if (ReadFieldDescriptors() != GST_OKAY) {
    notify(NFY_WARN, "Error parsing field descriptors of dbf file %s", name());
    fclose(file_pointer_);
    return GST_READ_FAIL;
  }

  notify(NFY_DEBUG, "Successfully opened DBF file: %s", name());
  return GST_OKAY;
}

gstStatus gstDBFTable::Close() {
  if (file_pointer_ && fclose(file_pointer_) != 0) {
    notify(NFY_WARN, "Problems closing dbf file");
    return GST_UNKNOWN;
  }

  file_pointer_ = 0;
  return GST_OKAY;
}


gstRecordHandle gstDBFTable::Row(std::uint32_t row) {
  if (row < 0 || row >= _fileHeader.RecordNum) {
    notify(NFY_WARN, "Row %d is outside the valid range of 0-%d for file %s",
           row, _fileHeader.RecordNum - 1, name());
    return gstRecordHandle();
  }

  if (fseek(file_pointer_, _fileHeader.HeaderLength +
            _fileHeader.RecordLength * row, 0) < 0) {
    notify(NFY_WARN, "Problems seeking in dbf file: %s", name());
    return gstRecordHandle();
  }

  char buff[_fileHeader.RecordLength];

  if (fread(buff, _fileHeader.RecordLength, 1, file_pointer_) != 1) {
    notify(NFY_WARN, "Error reading row %d of file %s", row, name());
    return gstRecordHandle();
  }

  gstRecordHandle rec = NewRecord();

  notify(NFY_VERBOSE, "-------");
  for (std::uint32_t jj = 0; jj < NumColumns(); ++jj) {
    int len = _fieldList[jj]->Length;
    char str[len + 1];
    strncpy(str, &buff[_fieldList[jj]->Offset], len);
    str[len] = 0;
    if (*str != '\0') {
      for (char *p = str + len - 1; *p == ' ' && p >= str; --p)
        *p = 0;
    }

    // record->addField(new Field(_fieldList[jj].Type, len, str));
    rec->Field(jj)->set(str);

    notify(NFY_VERBOSE, "%d: %s", jj, str);

#if 0
    fprintf(stderr, "                  : ");
    unsigned char* xout = static_cast<unsigned char*>(&str[0]);
    for (int x = 0; x < strlen(str); x++, xout++)
      fprintf(stderr, "%02x ", *xout);
    fprintf(stderr, "\n");
#endif
  }

  return rec;
}



gstStatus gstDBFTable::ReadHeader() {
  DBF_FileHeader hdr;

  memset(static_cast<void*>(&hdr), 0, sizeof(DBF_FileHeader));

  if (fread(&hdr, sizeof(DBF_FileHeader), 1, file_pointer_) != 1)
    return GST_READ_FAIL;

  notify(NFY_VERBOSE, "DBF_FileHeader...");
  notify(NFY_VERBOSE, "     HeaderLength[0] = %d", hdr.HeaderLength[0]);
  notify(NFY_VERBOSE, "     HeaderLength[1] = %d", hdr.HeaderLength[1]);
  notify(NFY_VERBOSE, "     RecordLength[0] = %d", hdr.RecordLength[0]);
  notify(NFY_VERBOSE, "     RecordLength[1] = %d", hdr.RecordLength[1]);

  _fileHeader.Version = GET_INT_B_LE(hdr.Version);
  notify(NFY_VERBOSE, "Version: %d", _fileHeader.Version);

  // Date must be the year since 1900
  _fileHeader.Year = hdr.Date[0] + 1900;
  _fileHeader.Month = hdr.Date[1];
  _fileHeader.Day = hdr.Date[2];

  if (_fileHeader.Month > 12 || _fileHeader.Day > 31)
    notify(NFY_WARN, "Bad Date: Year=%04d Month=%02d Day=%02d",
           _fileHeader.Year, _fileHeader.Month, _fileHeader.Day);
  else
    notify(NFY_VERBOSE, "Date: Year=%04d Month=%02d Day=%02d",
           _fileHeader.Year, _fileHeader.Month, _fileHeader.Day);

  _fileHeader.RecordNum = GET_INT_B_LE(hdr.RecordNum);
  notify(NFY_VERBOSE, "RecordNum: %d", _fileHeader.RecordNum);
  num_rows_ = _fileHeader.RecordNum;

  _fileHeader.HeaderLength = GET_INT_B_LE(hdr.HeaderLength);
  notify(NFY_VERBOSE, "HeaderLength: %d", _fileHeader.HeaderLength);

  _fileHeader.RecordLength = GET_INT_B_LE(hdr.RecordLength);
  notify(NFY_VERBOSE, "RecordLength: %d", _fileHeader.RecordLength);

  // HeaderLength is the total size of the header, which
  // includes the main header, and each field descriptor header.
  // Both of these header types are 32 bytes long.
  // So to calculate the number of fields in each record, take
  // the HeaderLength and divide it by 32 to get the number of
  // separate sub-headers.  The first is the main file header,
  // so subtract one from the quotent and this is the number of
  // fields.
  num_columns_ = (_fileHeader.HeaderLength / 32) - 1;

  return GST_OKAY;
}   // End gstDBFTable::ReadHeader()



gstStatus gstDBFTable::ReadFieldDescriptors() {
  _fieldList.init(NumColumns());

  DBF_FieldDescriptor* fields =
    static_cast<DBF_FieldDescriptor*>(alloca(sizeof(DBF_FieldDescriptor) *
                                             NumColumns()));

  notify(NFY_VERBOSE, "About to read %d field descriptors of length %llu",
         NumColumns(),
         static_cast<long long unsigned>(sizeof(DBF_FieldDescriptor)));

  if (static_cast<std::uint32_t>(fread(fields, sizeof(DBF_FieldDescriptor),
                                NumColumns(), file_pointer_)) != NumColumns()) {
    notify(NFY_WARN, "Unable to read field descriptions.");
    return GST_READ_FAIL;
  }

  // Data records are preceeded by one byte that is a space (20H) if the
  // record is not deleted and an asterisk (2AH) if it is deleted.
  int prevOffset = 1;
  int prevSize = 0;
  for (std::uint32_t ii = 0; ii < NumColumns(); ++ii, ++fields) {
    DBF_FieldDescriptor_i *ifield = new DBF_FieldDescriptor_i;
    GET_STR(fields->Name, ifield->Name);
    notify(NFY_VERBOSE, "   Field %d:\t%s", ii, ifield->Name);

    char type = *fields->Type;
    switch (type) {
      case 'C': case 'c':
      case 'D': case 'd':
        ifield->Type = gstTagString;
        break;
      case 'N': case 'n':
      case 'M': case 'm':
      case 'F': case 'f':
        ifield->Type = gstTagDouble;
        break;
      case 'L': case 'l':
        ifield->Type = gstTagBoolean;
        break;
      default:
        notify(NFY_WARN, "Invalid Data Type: %c", type);
        ifield->Type = gstTagDouble;
    }

    notify(NFY_VERBOSE, "      Type\t%s",
           gstValue::NameFromType(ifield->Type));

    // This field is defined as "set in memory".  Why is it in the file?
    ifield->Offset = prevOffset + prevSize;
    notify(NFY_VERBOSE, "      Addr\t%d", ifield->Offset);

    ifield->Length = GET_INT_B_LE(fields->Length);
    notify(NFY_VERBOSE, "      Length\t%d", ifield->Length);

    ifield->DecimalCount = GET_INT_B_LE(fields->DecimalCount);
    notify(NFY_VERBOSE, "      Dec Cnt\t%d", ifield->DecimalCount);

    _fieldList.append(ifield);

    prevOffset = ifield->Offset;
    prevSize = ifield->Length;

    header_->addSpec(ifield->Name, ifield->Type);
  }

  unsigned char term;
  if ((fread(&term, 1, 1, file_pointer_) != 1) || term != 0x0d) {
    notify(NFY_WARN, "Failed to read field descriptions properly."
           " No terminating 0x0d. (%02x)", term);
    return GST_UNKNOWN;
  }

  return GST_OKAY;
}
