// Copyright 2017 Google Inc.
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

// KmlLog - write KML formatted log file, viewable in Google Earth.
// This log file shows the differences between the sources.

#include <iostream>
#include <iomanip>
#include "kmllog.h"

namespace gecrawler {

const std::string KmlLog::kSource1Str = "Src1";
const std::string KmlLog::kSource2Str = "Src2";
const std::string KmlLog::kBothStr = "Both";

const std::string KmlLog::kKmlHeader =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<kml xmlns=\"http://earth.google.com/kml/2.1\">\n"
"<Document>\n"
"  <Style id=\"" + kSource1Str + "\">\n"
"    <LineStyle>\n"
"      <color>2fff0000</color>\n"
"      <width>3</width>\n"
"    </LineStyle>\n"
"    <IconStyle>\n"
"      <Icon>\n"
"        <href>http://maps.google.com/mapfiles/kml/pal3/icon0.png</href>\n"
"      </Icon>\n"
"    </IconStyle>\n"
"  </Style>\n"
"  <Style id=\"" + kSource2Str + "\">\n"
"    <LineStyle>\n"
"      <color>2f0000ff</color>\n"
"      <width>3</width>\n"
"    </LineStyle>\n"
"    <IconStyle>\n"
"      <Icon>\n"
"        <href>http://maps.google.com/mapfiles/kml/pal3/icon1.png</href>\n"
"      </Icon>\n"
"    </IconStyle>\n"
"  </Style>\n"
"  <Style id=\"" + kBothStr + "\">\n"
"    <LineStyle>\n"
"      <color>ffffffff</color>\n"
"      <width>4</width>\n"
"    </LineStyle>\n"
"    <IconStyle>\n"
"      <Icon>\n"
"        <href>http://maps.google.com/mapfiles/kml/pal3/icon33.png</href>\n"
"      </Icon>\n"
"    </IconStyle>\n"
"  </Style>\n"
;

const std::string KmlLog::kKmlEnd =
"</Document>\n"
"</kml>\n"
;

//---------------------------------------------------------------------------

KmlFolder::KmlFolder(const QuadtreePath &qt_path,
                     const std::string &label)
    : str_(std::ios::out | std::ios::app) {
  Reset(qt_path, label);
}

KmlFolder::~KmlFolder() {
}

void KmlFolder::Reset(const QuadtreePath &qt_path,
                      const std::string &label) {
  str_.str("");
  folder_name_ = label + " - " + qt_path.AsString();
}

void KmlFolder::AddPoint(const QuadtreePath &qt_path,
                         const std::string &style,
                         const std::string &description) {
  khExtents<double> extent = GetDegExtents(qt_path);
  double lat = 0.5 * (extent.north() + extent.south());
  double lon = 0.5 * (extent.east() + extent.west());
  str_  << std::setprecision(12)
        << "    <Placemark>\n"
        << "      <name> " << qt_path.AsString() << " </name>\n"
        << "      <styleUrl>#" << style << "</styleUrl>\n"
        << "      <description> " << "<![CDATA[ <pre>"
        << description << "</pre> ]]> </description>\n"
        << "      <Point>\n"
        << "        <coordinates> "
        << lon << "," << lat << " </coordinates>\n"
        << "      </Point>\n"
        << "    </Placemark>\n";
}

void KmlFolder::AddBox(const QuadtreePath &qt_path,
                       const std::string &style,
                       const std::string &description) {
  khExtents<double> extent = GetDegExtents(qt_path);
  str_ << std::setprecision(12)
       << "    <Placemark>\n"
       << "      <name> " << qt_path.AsString() << " </name>\n"
       << "      <styleUrl>#" << style << "</styleUrl>\n"
       << "      <LinearRing id=\"\">\n"
       << "        <tessellate>1</tessellate>\n"
       << "        <altitudeMode>clampToGround</altitudeMode>\n"
       << "        <coordinates>\n"
       << "          " << extent.west() << "," << extent.north() << "\n"
       << "          " << extent.west() << "," << extent.south() << "\n"
       << "          " << extent.east() << "," << extent.south() << "\n"
       << "          " << extent.east() << "," << extent.north() << "\n"
       << "          " << extent.west() << "," << extent.north() << "\n"
       << "        </coordinates>\n"
       << "      </LinearRing>\n"
       << "    </Placemark>\n";
}

void KmlFolder::Get(std::string *folder_string) const {
  *folder_string += "  <Folder>\n    <name>";
  *folder_string += folder_name_;
  *folder_string += "</name>\n";
  *folder_string += str_.str();
  *folder_string += "  </Folder>\n";
}

void KmlFolder::Append(const KmlFolder &other) {
  str_ << other.str_.str();
}

khExtents<double> KmlFolder::GetDegExtents(const QuadtreePath &qt_path) {
  return khLevelCoverage(khTileAddr(qt_path)).degExtents(
      ClientTmeshTilespaceFlat);
}

//---------------------------------------------------------------------------

KmlLog::KmlLog(geFilePool &file_pool, const std::string &log_name)
    : log_writer_(TransferOwnership(
                      new geFilePool::Writer(geFilePool::Writer::WriteOnly,
                                             geFilePool::Writer::Truncate,
                                             file_pool,
                                             log_name))),
      position_(0) {
  Append(kKmlHeader);
}

void KmlLog::Close() {
  Append(kKmlEnd);
  log_writer_->SyncAndClose();
  log_writer_.clear();
}

void KmlLog::Append(const std::string &text) {
  log_writer_->Pwrite(text.data(), text.size(), position_);
  position_ += text.size();
}

void KmlLog::LogFolder(const KmlFolder &folder) {
  std::string new_text;
  folder.Get(&new_text);
  Append(new_text);
}

void KmlLog::LogQtpDiff(const QuadtreePath &qt_path,
                        const std::string &style,
                        const std::string &label) {
  KmlFolder folder(qt_path, style);
  folder.AddPoint(qt_path, style, label);
  folder.AddBox(qt_path, style, "Bounds");
  LogFolder(folder);
}

void KmlLog::AddDescription(const std::string description) {
  Append("  <description> <![CDATA[ " + description + " ]]> </description>\n");
}

} // namespace gecrawler
