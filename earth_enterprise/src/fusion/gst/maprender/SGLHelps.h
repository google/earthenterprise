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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_MAPRENDER_SGLHELPS_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_MAPRENDER_SGLHELPS_H_

#include <string>
#include <map>
#include <set>

#include <SkColor.h>
#include <SkFontHost.h>
#include <SkTypeface.h>


class QColor;
class MapTextStyleConfig;

SkColor SkColorFromQColor(const QColor &color);

namespace maprender {

// FontInfo is a support class wraping SkFontHost. Previously SkFontHost had
// interfaces to read and store font files. Since it is no more there,
// we read font files through SkFontHost, but store the details in a map here.
//
// Also we enforce requirement of /opt/google/share/fonts/fontlist file.
class FontInfo {
 public:
  FontInfo(const std::string &name, SkTypeface::Style style) :
      name_(name), style_(style) {}

  FontInfo(): style_(SkTypeface::kNormal) {}

  const FontInfo& operator=(const FontInfo& other) {
    new (this) FontInfo(other.name_, other.style_);
    return *this;
  }

  bool operator<(const FontInfo& other) const {
    return (name_ < other.name_) ||
           ((name_ == other.name_) && (style_ < other.style_));
  }

  static const std::map<FontInfo, SkTypeface*>& GetFontMap()
      { return font_map_; }

  static void LoadFontConfig();

  const std::string name_;
  const SkTypeface::Style style_;

  static SkTypeface* GetTypeFace(const std::string &name,
                                 SkTypeface::Style style) {
    const FontInfo font_info(name, style);
    std::map<FontInfo, SkTypeface*>::const_iterator i =
        font_map_.find(font_info);
    return (i != font_map_.end() ? i->second : NULL);
  }

  // Given a TextStyle represented by font name (e.g Sans) and weight (e.g
  // normal/bold/underlined etc.) this method checks whether we have the TTF
  // files corresponding to that in fontlist. If so it returns true. If not, it
  // returns false as well as updates the font to default font and adds the
  // font to missing_fonts (missing fonts set).
  static bool CheckTextStyleSanity(
      MapTextStyleConfig* config, std::set<FontInfo>* missing_fonts);

 private:
  static std::map<FontInfo, SkTypeface*> font_map_;
};

}  // namespace maprender

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_MAPRENDER_SGLHELPS_H_

