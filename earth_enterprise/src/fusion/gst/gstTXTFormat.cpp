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


#include <khException.h>
#include <notify.h>
#include <gstTXTFormat.h>
#include <gstTXTTable.h>
#include <gstRecord.h>
#include <qfile.h>
#include <qstringlist.h>
#include <khFileUtils.h>

gstTXTFormat::gstTXTFormat(const char *n)
    : gstFormat(n) {
  lat_multiplier_ = lon_multiplier_ = 1.0f;
  table_ =  NULL;
}

gstTXTFormat::~gstTXTFormat() {
  CloseFile();
}


bool gstTXTFormat::DiscoverLayout(const char* fname,
                                  gstRegistry* inforeg) {
  QFile csvfile(fname);
  if (!csvfile.open(IO_ReadOnly | IO_Translate))
    return false;

  char buff[2048] { '\0' };
  if (csvfile.readLine(buff, 2048) == -1)
    return false;
  QString firstline(buff);

  gstRegistry::Group* layout = inforeg->locateGroup("Layout", 1);
  layout->addTag(new gstValue("delimited"))->SetName("FileType");

  // attempt to split with commas
  QChar del(',');
  QStringList fields = QStringList::split(del, firstline, true);
  // and if that fails, try tabs
  if (fields.count() < 2) {
    del = '\t';
    fields = QStringList::split(del, firstline, true);
  }

  layout->addTag(new gstValue(QString(del)))->SetName("Delimiter");

  gstRegistry::Group *field_defs = layout->addGroup("FieldDefinitions");

  // look for coord fields
  unsigned int lat = 0;
  unsigned int lon = 0;
  unsigned int pos = 0;
  bool have_lat = false;
  bool have_lon = false;
  for (QStringList::Iterator it = fields.begin();
       it != fields.end(); ++it, ++pos) {
    QString field = (*it).stripWhiteSpace();
    gstRegistry::Group* grp = field_defs->addGroup(
        QString("%1").arg(pos).latin1());
    if (field.isEmpty()) {
      notify(NFY_WARN, "Empty column name found. Replacing with (null)");
      field = "(null)";
    }
    grp->addTag(new gstValue(field.latin1()))->SetName("Name");

    QString lower = field.lower();
    if (!have_lat && (lower == "dlat" || lower == "latitude" ||
                      lower.startsWith("lat"))) {
      lat = pos;
      have_lat = true;
      grp->addTag(new gstValue("double"))->SetName("Type");
    } else if (!have_lon && (lower == "dlong" || lower == "longitude" ||
                             lower.startsWith("lon"))) {
      lon = pos;
      have_lon = true;
      grp->addTag(new gstValue("double"))->SetName("Type");
    } else {
      grp->addTag(new gstValue("string"))->SetName("Type");
    }
  }

  if (!have_lat || !have_lon) {
    notify(NFY_WARN, "Unable to discover coordinate fields."
           "  Looking for \"latitude\" and \"longitude\"");
    return false;
  }

  layout->addTag(new gstValue(lat))->SetName("Latitude");
  layout->addTag(new gstValue(lon))->SetName("Longitude");
  layout->addTag(new gstValue(1))->SetName("SkipRows");

  return true;
}


gstStatus gstTXTFormat::OpenFile() {
  std::string kdxfile = khReplaceExtension(name(), ".kdx");
  gstRegistry inforeg(kdxfile.c_str());

  // without a kdx, we should try to read the first line and make
  // some attempt to figure out the file structure.
  if (!khExists(kdxfile)) {
    if (!DiscoverLayout(name(), &inforeg)) {
      notify(NFY_WARN, "Generic text loader needs header file \"%s\"",
             kdxfile.c_str());
      return GST_OPEN_FAIL;
    }
  } else if (inforeg.load() != GST_OKAY) {
    notify(NFY_WARN, "Index file \"%s\" is corrupt.", kdxfile.c_str());
    return GST_OPEN_FAIL;
  }

  gstRegistry::Group* layout = inforeg.locateGroup("Layout");
  if (layout == NULL) {
    notify(NFY_WARN, "Can't find required Layout block");
    return GST_OPEN_FAIL;
  }

  //
  // determine the main file type, either fixed width or delimited
  //

  gstValue* filetype_tag = layout->findTag("FileType");
  if (filetype_tag == NULL) {
    notify(NFY_WARN, "Layout needs a FileType (Delimited or FixedWidth)");
    return GST_OPEN_FAIL;
  }

  //
  // we must have geographic coordinates
  //
  gstValue* lat_tag = layout->findTag("Latitude");
  gstValue* lat_multiplier_tag = layout->findTag("LatMultiplier");
  gstValue* lon_tag = layout->findTag("Longitude");
  gstValue* lon_multiplier_tag = layout->findTag("LonMultiplier");
  if (lat_tag == NULL || lon_tag == NULL) {
    notify(NFY_WARN, "Layout needs Latitude and Longitude defined.");
    return GST_OPEN_FAIL;
  }
  lat_field_ = lat_tag->ValueAsUInt();
  lon_field_ = lon_tag->ValueAsUInt();
  if (lat_multiplier_tag)
    lat_multiplier_ = lat_multiplier_tag->ValueAsDouble();
  if (lon_multiplier_tag)
    lon_multiplier_ = lon_multiplier_tag->ValueAsDouble();

  //
  // we will need a valid gstTXTTable for everything else
  // so create it here, but don't open it yet as there still might be
  // a reason to abort
  //

  table_ = new gstTXTTable(name());

  gstRegistry::Group *field_defs = layout->findGroup("FieldDefinitions");
  if (field_defs != NULL) {
    gstHeaderHandle hdr = gstHeaderImpl::BuildFromRegistry(field_defs);
    table_->SetHeader(hdr);
  }

  //
  // Delimited file type needs to know the delimiter character
  //

  if (!table_->SetFileType(filetype_tag->ValueAsString().c_str())) {
    notify(NFY_WARN, "Unsupported file type: %s",
           filetype_tag->ValueAsString().c_str());
    delete table_;
    table_ = NULL;
    return GST_OPEN_FAIL;
  }

  if (table_->GetFileType() == gstTXTTable::Delimited) {
    gstValue  *delimiterTag = layout->findTag("Delimiter");
    if (delimiterTag == NULL || delimiterTag->ValueAsUnicode().isEmpty()) {
      notify(NFY_WARN, "FileType Delimited needs a valid Delimiter character");
      delete table_;
      table_ = NULL;
      return GST_OPEN_FAIL;
    }
    table_->SetDelimiter(*delimiterTag->ValueAsString().c_str());
  } else if (table_->GetFileType() == gstTXTTable::FixedWidth) {
    notify(NFY_INFO, "fixed width");
  }

  //
  // First line might contain column names
  // We need to know this mainly to start the row offset at the correct place
  //

  gstValue* skipRowsTag = layout->findTag("SkipRows");
  if (skipRowsTag)
    table_->SetSkipRows(skipRowsTag->ValueAsUInt());

  if (table_->Open(GST_READONLY) != GST_OKAY) {
    delete table_;
    table_ = NULL;
    notify(NFY_WARN, "table open() fails");
    return GST_OPEN_FAIL;
  }

  //
  // validate geo-coordinate column number
  //

  if (lat_field_ >= table_->NumColumns() || lon_field_ >= table_->NumColumns()) {
    notify(NFY_WARN, "Latitude and Longitude field # must be valid (0 - %d)\n",
           table_->NumColumns() - 1);
    delete table_;
    table_ = NULL;
    return GST_OPEN_FAIL;
  }

  //
  // At this point the gstTXTTable object should be valid so begin to parse it.
  //

  notify(NFY_INFO, "TXT table has %d rows & %d columns",
         table_->NumRows(), table_->NumColumns());

  if (getNotifyLevel() >= NFY_DEBUG) {
    for (unsigned int ii = 0; ii < table_->NumColumns(); ++ii)
      fprintf(stderr, "\t[%d] %s\n", ii, table_->GetHeader()->Name(ii).latin1());
  }

  AddLayer(gstPoint, table_->NumRows(), table_->GetHeader());

  return GST_OKAY;
}

gstStatus gstTXTFormat::CloseFile() {
  delete table_;
  table_ = NULL;

  return GST_OKAY;
}

gstGeodeHandle gstTXTFormat::GetFeatureImpl(std::uint32_t layer, std::uint32_t fidx) {
  // should be checked by gstSource before calling me
  assert(fidx < table_->NumRows() && layer == 0);

  // try to load my row
  gstRecordHandle attrib = table_->Row(fidx);
  if (!attrib || attrib->IsEmpty()) {
    throw khSoftException(kh::tr("Unable to read row from txt file"));
  }

  // get my x & y from the row
  double x = attrib->Field(lon_field_)->ValueAsDouble();
  x *= lon_multiplier_;
  double y = attrib->Field(lat_field_)->ValueAsDouble();
  y *= lat_multiplier_;
  if ( x < -180 || x > 180 || y < -90 || y > 90 ) {
    throw khSoftException(QString().sprintf(
                              "Invalid coordinates: %lf, %lf", x, y));
  }

  // build and return the geode
  gstGeodeHandle new_geodeh = gstGeodeImpl::Create(gstPoint);
  gstGeode *new_geode = static_cast<gstGeode*>(&(*new_geodeh));
  new_geode->AddVertex(gstVertex(x, y));

  return new_geodeh;
}

gstRecordHandle gstTXTFormat::GetAttributeImpl(std::uint32_t layer, std::uint32_t fidx) {
  // should be checked by gstSource before calling me
  assert(fidx < table_->NumRows() && layer == 0);

  // try to load my row
  gstRecordHandle attrib = table_->Row(fidx);
  if (!attrib || attrib->IsEmpty()) {
    throw khSoftException("Unable to read row from txt file");
  }

  return attrib;
}
