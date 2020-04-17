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


#include "fusion/gst/maprender/SGLHelps.h"

#include <SkForceLinking.h>
#include <qcolor.h>

#include "common/khFileUtils.h"
#include "common/notify.h"
#include "common/geInstallPaths.h"
#include "fusion/autoingest/.idl/storage/MapSubLayerConfig.h"


SkColor SkColorFromQColor(const QColor &color) {
  QRgb rgb = color.rgb();
  return SkColorSetARGB(qAlpha(rgb), qRed(rgb), qGreen(rgb), qBlue(rgb));
}

namespace maprender {

std::map<FontInfo, SkTypeface*> FontInfo::font_map_;

SkTypeface* SkTypeface_CreateFromFile(const std::string& ttf_file_name) {
  return SkTypeface::CreateFromFile(ttf_file_name.c_str());
}

// LoadFontConfig
//
// Static, run-once initialization.  Loads fonts for map rendering as
// specified in configuration file.
//
// The configuration file consists of one line per font, with four
// fields per line (fields are separated by white space):
//
// 1. Typeface name. This will be displayed in the font chooser menu.
// 2. Font file name: full path name of the font.
// 3. Bold: 1 if font is bold, otherwise 0.
// 4. Italic: 1 if font is italic, otherwise 0.
//
// For example, a typeface named "GoogleSans" having regular, bold,
// italic, and bold-italic fonts would be entered as follows:
//
//    GoogleSans /opt/google/share/fonts/GoogleSansRegular.ttf 0 0
//    GoogleSans /opt/google/share/fonts/GoogleSansBold.ttf 1 0
//    GoogleSans /opt/google/share/fonts/GoogleSansItalic.ttf 0 1
//    GoogleSans /opt/google/share/fonts/GoogleSansBoldItalic.ttf 1 1
//
// Empty lines as well as lines starting with # as first non blank character
// in the configuration file are ignored and may be used as desired.
void FontInfo::LoadFontConfig() {
  static bool runonce = true;
  if (!runonce) {
    return;
  }

  std::string kFontConfigPath(khComposePath(kGESharePath, "fonts/fontlist"));
  std::string buffer;
  bool passed = khReadStringFromFile(kFontConfigPath, buffer);
  if (passed) {
    notify(NFY_VERBOSE, "Processing font config file %s: \n%s",
           kFontConfigPath.c_str(),
           buffer.c_str());
    std::istringstream config(buffer);

    try {
      std::string config_line;
      std::string family_name;
      std::string face_file;
      bool bold, italic;
      for (unsigned line_number = 0; config.good(); ++line_number) {
        std::getline(config, config_line);
        // Skip empty or comment lines
        std::string::size_type not_white =
            config_line.find_first_not_of(" \t\n");
        if (not_white == std::string::npos || config_line[not_white] == '#') {
          continue;
        }
        std::istringstream line_stream(config_line);
        line_stream >> family_name >> face_file >> bold >> italic;
        const char* message = NULL;
        if (!khIsFileReadable(face_file)) {
          message = "The font file is not readable.";
        } else {
          const SkTypeface::Style style =
              bold ? (italic ? SkTypeface::kBoldItalic : SkTypeface::kBold)
                   : (italic ? SkTypeface::kItalic : SkTypeface::kNormal);
          const FontInfo font_info(family_name, style);
          if (font_map_.count(font_info) != 0) {
            message = "Ignoring duplicate entry for same font and style.";
          } else {
            font_map_[font_info] = SkTypeface_CreateFromFile(face_file);
          }
        }
        if (message != NULL) {
          notify(NFY_WARN, "Ignoring entry in fontlist: \"%s\"\n"
                 " at line %u of %s. %s", config_line.c_str(), line_number,
                 kFontConfigPath.c_str(), message);
        }
      }
      // Add a default font (if same type font has not been used by user).
      {
        const FontInfo font_info(kDefaultFont, SkTypeface::kNormal);
        if (font_map_.count(font_info) == 0) {
          font_map_[font_info] = SkTypeface_CreateFromFile(
              khComposePath(kGESharePath, kDefaultFontFileName));
        }
      }
    }
    catch(...) {
      passed = false;
    }
  }
  if (!passed) {
    notify(NFY_WARN, "Failed to load font configuration file: \"%s\"\n",
                      kFontConfigPath.c_str());
  }

  runonce = false;
}

bool FontInfo::CheckTextStyleSanity(MapTextStyleConfig* config,
                                    std::set<FontInfo>* missing_fonts) {
  // translate "weight" to SkTypeface::Style
  SkTypeface::Style style = SkTypeface::kNormal;
  if (config->weight == MapTextStyleConfig::Bold)
    style = SkTypeface::kBold;
  else if (config->weight == MapTextStyleConfig::Italic)
    style = SkTypeface::kItalic;
  else if (config->weight == MapTextStyleConfig::BoldItalic)
    style = SkTypeface::kBoldItalic;
  const FontInfo font_info(config->font.toUtf8().constData(), style);
  if (font_map_.count(font_info) != 0) {
    return true;
  }
  missing_fonts->insert(font_info);
  config->font = QString(kDefaultFont.c_str());
  config->weight = MapTextStyleConfig::Regular;
  return false;
}

}  // namespace maprender

// Always leave these two lines last, otherwise it will
// silence unused variables from the code below.
#pragma GCC diagnostic ignored "-Wunused-variable"
__SK_FORCE_IMAGE_DECODER_LINKING;
