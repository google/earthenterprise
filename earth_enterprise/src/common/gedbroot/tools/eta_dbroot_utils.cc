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
//

#include "common/gedbroot/tools/eta_dbroot_utils.h"

#include <qstring.h>
#include <qurl.h>


#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <utility>

#include "common/gedbroot/tools/eta_parser.h"
#include "common/gedbroot/strings/numbers.h"
#include "google/protobuf/text_format.h"  // For google::protobuf::TextFormat.
#include "google/protobuf/stubs/strutil.h"  // For UnescapeCEscapeString.
#include "common/khstl.h"
#include "common/khStringUtils.h"

using std::string;
using std::vector;
using std::map;

namespace {

// Basic structure type in ETA.
const char kEtStructType[] = "etStruct";

void AddStringToProto(const string& str,
                      keyhole::dbroot::StringIdOrValueProto* proto) {
  proto->clear_string_id();
  proto->set_value(str);
}

}  // anonymous namespace

namespace keyhole {
namespace eta_utils {

const string EtaReader::GetString(
    const string& name, const string& default_val) const {
  if (const keyhole::EtaStruct* child =
      eta_struct_->GetChildByName(name, "etString")) {
    return child->value();
  } else {
    return default_val;
  }
}
std::int32_t EtaReader::GetInt(const string& name, std::int32_t default_val) const {
  if (const keyhole::EtaStruct* child =
      eta_struct_->GetChildByName(name, "etInt")) {
    return child->GetInt32Value();
  } else {
    return default_val;
  }
}
std::uint32_t EtaReader::GetUInt(const string& name, std::uint32_t default_val) const {
  if (const keyhole::EtaStruct* child =
      eta_struct_->GetChildByName(name, "etInt")) {
    return child->GetUInt32Value();
  } else {
    return default_val;
  }
}
float EtaReader::GetFloat(const string& name, float default_val) const {
  if (const keyhole::EtaStruct* child =
      eta_struct_->GetChildByName(name, "etFloat")) {
    return child->GetFloatValue();
  } else {
    return default_val;
  }
}
bool EtaReader::GetBool(const string& name, bool default_val) const {
  if (const keyhole::EtaStruct* child =
      eta_struct_->GetChildByName(name, "etBool")) {
    return child->GetBoolValue();
  } else {
    return default_val;
  }
}


// Template method that will be specialized into types we need to read in this
// file.
template <typename T>
void GetValueFromEtaStruct(const EtaStruct* eta_struct, T* value);

template <>
void GetValueFromEtaStruct<string>(const EtaStruct* eta_struct, string* value) {
  *value = eta_struct->value();
}

template <>
void GetValueFromEtaStruct<std::int32_t>(const EtaStruct* eta_struct, std::int32_t* value) {
  *value = eta_struct->GetInt32Value();
}

template <>
void GetValueFromEtaStruct<std::uint32_t>(const EtaStruct* eta_struct, std::uint32_t* value) {
  *value = eta_struct->GetUInt32Value();
}

template <>
void GetValueFromEtaStruct<bool>(const EtaStruct* eta_struct, bool* value) {
  *value = eta_struct->GetBoolValue();
}

template <typename T>
bool MaybeReadFromStruct(const EtaStruct* eta_struct, T* value) {
  if (eta_struct && eta_struct->has_value()) {
    GetValueFromEtaStruct(eta_struct, value);
    return true;
  }
  return false;
}

// Assigns value of template type from child with given name in parent
// structure.
template <typename T>
bool MaybeReadFromNamedChild(const EtaStruct& parent,
                             const char* field,
                             T* value) {
  const EtaStruct* child = parent.GetChildByName(field);
  return MaybeReadFromStruct(child, value);
}


string MaybeReadStringFromChild(const EtaStruct* parent, int child_index) {
  string value;
  if (parent->num_children() > child_index) {
    value = parent->child(child_index)->value();
  }
  return value;
}

bool MaybeReadBoolFromChild(const EtaStruct* parent, int child_index) {
  bool value = false;
  if (parent->num_children() > child_index) {
    value = parent->child(child_index)->GetBoolValue();
  }
  return value;
}

std::int32_t MaybeReadInt32FromChild(const EtaStruct* parent, int child_index) {
  std::int32_t value = 0;
  if (parent->num_children() > child_index) {
    value = parent->child(child_index)->GetInt32Value();
  }
  return value;
}

double MaybeReadDoubleFromChild(const EtaStruct* parent, int child_index) {
  double value = 0.0;
  if (parent->num_children() > child_index) {
    value = parent->child(child_index)->GetDoubleValue();
  }
  return value;
}

// Extracts list of values from document. Searches for a toplevel variable with
// given in the document, then extracts all structures with given type from that
// variable. Returns whether or not list could be found.
bool GetListFromDocument(const EtaDocument& eta_doc,
                         const char* list_name,
                         const char* type,
                         vector<EtaStruct*>* output) {
  CHECK(eta_doc.root());

  const EtaStruct* toplevel_struct =
      eta_doc.root()->GetChildByName(list_name, kEtStructType);
  if (!toplevel_struct) {
    // No top-level struct with given name defined, bail out.
    return false;
  }

  toplevel_struct->GetChildrenByType(type, output);
  return true;
}

// Add string (only if non-empty) from named child in root structure to given
// StringIdOrValueProto. Returns true if given child_name contains a non-empty
// string, false if child_name is not present or set to empty
// If this function returns false, it's probably best to clear the string_proto
// field instead of leaving it empty.
bool MaybeAddStringFromEta(const EtaStruct& root,
                           const char* child_name,
                           dbroot::StringIdOrValueProto* string_proto) {
  string value;
  MaybeReadFromNamedChild(root, child_name, &value);
  if (!value.empty()) {
    AddStringToProto(value, string_proto);
    return true;
  }
  return false;
}

// Assigns a URL to output proto based on the name of a structure that contains
// host/port/path/secure information.
bool MaybeSetUrlFromEta(const EtaStruct& root, const char* struct_name,
                        dbroot::StringIdOrValueProto* output) {
  QUrl url;

  string field_name = string(struct_name) + ".host";
  string host;
  MaybeReadFromNamedChild(root, field_name.c_str(), &host);
  url.setHost(host.c_str());

  if (!url.isValid()) {
    return false;
  }

  field_name = string(struct_name) + ".port";
  std::int32_t port = 0;
  MaybeReadFromNamedChild(root, field_name.c_str(), &port);
  url.setPort(port);

  field_name = string(struct_name) + ".path";
  string path;
  MaybeReadFromNamedChild(root, field_name.c_str(), &path);
  url.setPath(host.c_str());

  field_name = string(struct_name) + ".secureSS";
  bool secure = false;
  MaybeReadFromNamedChild(root, field_name.c_str(), &secure);
  url.setProtocol(secure ? "https" : "http");

  AddStringToProto(url.toString().toStdString(), output);

  return true;
}

// Utility function that translates an MFE supported domain string into
// a list of MfeDomainFeaturesProto messages.
// Input string is a comma-separated list of "XX domain FEATURES", where:
//   XX is a 2-char country code
//   domain is the MFE domain to use for MFE requests
//   FEATURES is a string where each char corresponds to a feature available
//   on that domain:
//     'G': geocoding
//     'L': localsearch
//     'D': driving directions
//
// "US maps.google.com GLD,JP maps.google.co.jp GD" means:
// * for US users, talk to maps.google.com, all features enabled.
// * for Japan users, talk to maps.google.co.jp, only geocoding and directions
//   are enabled, localsearch is disabled.
// * Countries that are not specified in that list get a default value.
//
// In current implementations of Google Earth, default == US.
void TranslateSupportedDomains(const string& mfe_supported_domains,
                               dbroot::EndSnippetProto* end_snippet) {
  using keyhole::dbroot::MfeDomainFeaturesProto;
  vector<string> country_specs;
  TokenizeString(mfe_supported_domains, country_specs, ",");
  for (vector<string>::iterator it = country_specs.begin();
       it != country_specs.end(); ++it) {
    vector<string> fields;
    TokenizeString(*it, fields, " ");
    if (fields.size() != 3) {
      notify(NFY_WARN, "malformed mfe supported domain entry %s", it->c_str());
      continue;
    }
    MfeDomainFeaturesProto* domain_features = end_snippet->add_mfe_domains();
    domain_features->set_country_code(fields[0]);
    domain_features->set_domain_name(fields[1]);
    if (fields[2].find('G') != string::npos)
      domain_features->add_supported_features(
          MfeDomainFeaturesProto::GEOCODING);
    if (fields[2].find('L') != string::npos)
      domain_features->add_supported_features(
          MfeDomainFeaturesProto::LOCAL_SEARCH);
    if (fields[2].find('D') != string::npos)
      domain_features->add_supported_features(
          MfeDomainFeaturesProto::DRIVING_DIRECTIONS);
  }
}

//------------------------------------------------------------------------------

// Converts draw flags from ETA format (int) into dbroot proto enumerated types.
void ParseDrawFlags(int flags, dbroot::StyleAttributeProto* style) {
  using dbroot::DrawFlagProto;
  if (flags > 0) {
    if ((flags & 0x01) && (flags & 0x02)) {
      DrawFlagProto* draw_flag = style->add_draw_flag();
      draw_flag->set_draw_flag_type(DrawFlagProto::TYPE_FILL_AND_OUTLINE);
    } else if (flags & 0x02) {
      DrawFlagProto* draw_flag = style->add_draw_flag();
      draw_flag->set_draw_flag_type(DrawFlagProto::TYPE_OUTLINE_ONLY);
    } else if (flags & 0x01) {
      DrawFlagProto* draw_flag = style->add_draw_flag();
      draw_flag->set_draw_flag_type(DrawFlagProto::TYPE_FILL_ONLY);
    }
    // Center label is used with landmark packets only.
    if (flags & 0x200) {
      DrawFlagProto* draw_flag = style->add_draw_flag();
      draw_flag->set_draw_flag_type(DrawFlagProto::TYPE_CENTER_LABEL);
    }
  }
}

// Parses collection of style attributes from [export.style.attr] and stores
// them in dbroot.
void ParseStyleAttributes(const EtaDocument& eta_doc,
                          dbroot::DbRootProto* dbroot) {
  vector<EtaStruct*> style_attr_list;
  if (!GetListFromDocument(eta_doc, "export.style.attr", "etStyleAttr",
                           &style_attr_list)) {
    notify(NFY_WARN, "missing export.style.attr in dbroot");
    return;
  }

  for (vector<EtaStruct*>::const_iterator it = style_attr_list.begin();
       it != style_attr_list.end(); ++it) {
    const EtaStruct* eta_style = (*it);
    if (eta_style->num_children() >= 14) {
      //  Expected structure format is
      //  <etStyleAttr> [name]
      //  {
      //    <etString>  [providerName]  ""  // parsed as integer.
      //    <etInt>     [polyColor]     0xffffffff
      //    <etInt>     [lineColor]     0xffffffff
      //    <etInt>     [iconColor]     0xffffffff
      //    <etInt>     [labelColor]    0xffffffff
      //    <etFloat>   [linewidth]     1.0
      //    <etFloat>   [iconScale]    1.0
      //    <etFloat>   [labelScale]   1.0
      //    <etInt>     [drawFlags]     0x00000000
      //    <etString>  [iconHref]      ""
      //    <etInt>     [iconX]         0
      //    <etInt>     [iconY]         0
      //    <etInt>     [iconW]         32
      //    <etInt>     [iconH]         32
      //  # Extra fields for newer clients:
      //    <etString>  [balloonText]       ""
      //    <etInt>     [balloonColor]      0xffffffff
      //    <etInt>     [balloonTextColor]  0xff000000
      //  }
      EtaReader reader(eta_style);
      dbroot::StyleAttributeProto* style_attribute =
          dbroot->add_style_attribute();
      style_attribute->set_style_id(eta_style->name());
      std::int32_t provider_id = 0;
      // Use MaybeReadFromNamedChild(), which automatically converts an
      // etString into in32.
      MaybeReadFromNamedChild(*eta_style, "providerName", &provider_id);
      style_attribute->set_provider_id(provider_id);
      style_attribute->set_poly_color_abgr(
          reader.GetUInt("polyColor", 0xffffffff));
      style_attribute->set_line_color_abgr(
          reader.GetUInt("lineColor", 0xffffffff));
      style_attribute->set_placemark_icon_color_abgr(
          reader.GetUInt("iconColor", 0xffffffff));
      style_attribute->set_label_color_abgr(
          reader.GetUInt("labelColor", 0xffffffff));
      style_attribute->set_line_width(reader.GetFloat("linewidth", 1.0f));
      style_attribute->set_placemark_icon_scale(
          reader.GetFloat("iconScale", 1.0f));
      style_attribute->set_label_scale(reader.GetFloat("labelScale", 1.0));
      ParseDrawFlags(reader.GetInt("drawFlags", 0), style_attribute);
      AddStringToProto(reader.GetString("iconHref", ""),
                       style_attribute->mutable_placemark_icon_path());
      style_attribute->set_placemark_icon_x(
          reader.GetInt("iconX", 0));
      style_attribute->set_placemark_icon_y(
          reader.GetInt("iconY", 0));
      style_attribute->set_placemark_icon_width(
          reader.GetInt("iconW", 32));
      style_attribute->set_placemark_icon_height(
          reader.GetInt("iconH", 32));

      if (eta_style->GetChildByName("balloonText", "etString")) {
        // For newer dbroots, style attributes can also contain balloon text,
        // balloon color and balloon text color.
        dbroot::PopUpProto* pop_up = style_attribute->mutable_pop_up();
        const string& text = reader.GetString("balloonText", "");
        pop_up->set_is_balloon_style(!text.empty());
        AddStringToProto(text, pop_up->mutable_text());
        pop_up->set_background_color_abgr(
            reader.GetUInt("balloonColor", 0xffffffff));
        pop_up->set_text_color_abgr(
            reader.GetUInt("balloonTextColor", 0xff000000));
      }
    }
  }
}

// Helper function that finds a style attribute by name in the list of styles
// stored in dbroot.
int FindStyleAttrFromName(const string& name,
                          const dbroot::DbRootProto& dbroot) {
  int num_style_attrs = dbroot.style_attribute_size();
  for (int i = 0; i < num_style_attrs; ++i) {
    const dbroot::StyleAttributeProto& style_attribute =
        dbroot.style_attribute(i);
    if (style_attribute.style_id() == name) {
      return i;
    }
  }
  return -1;
}

// Parses collection of style maps from [export.style.map] and stores them
// as StyleMapProto in dbroot. Style attributes need to be parsed before parsing
// maps, since style maps reference attributes.
void ParseStyleMaps(const EtaDocument& eta_doc, dbroot::DbRootProto* dbroot) {
  CHECK(dbroot);
  vector<EtaStruct*> style_map_list;
  if (!GetListFromDocument(eta_doc, "export.style.map", "etStyleMap",
                           &style_map_list)) {
    notify(NFY_WARN, "missing export.style.map in dbroot");
    return;
  }

  for (vector<EtaStruct*>::const_iterator it = style_map_list.begin();
       it != style_map_list.end(); ++it) {
    const EtaStruct* eta_style_map = (*it);
    EtaReader reader(eta_style_map);
    if (eta_style_map->num_children() == 2) {
      std::int32_t style_map_id = ParseLeadingInt32Value(eta_style_map->name().c_str(),
                                                  0);
      if (style_map_id <= 0) {
        notify(NFY_WARN, "non-numerical style map id: %s",
               eta_style_map->name().c_str());
         continue;
      }
      const string normal_name = reader.GetString("normalStyleId", "");
      const string highlight_name = reader.GetString("highlightStyleId", "");

      // Find two style attributes from their id.
      int normal_id = FindStyleAttrFromName(normal_name, *dbroot);
      int highlight_id = FindStyleAttrFromName(highlight_name, *dbroot);
      // Only create style map if both ids can be found
      if (normal_id >= 0 && highlight_id >= 0) {
        dbroot::StyleMapProto* style_map = dbroot->add_style_map();
        style_map->set_style_map_id(style_map_id);
        style_map->set_normal_style_attribute(normal_id);
        style_map->set_highlight_style_attribute(highlight_id);
      } else {
        notify(NFY_WARN, "Could not find one of style attributes: %s and %s",
               normal_name.c_str(), highlight_name.c_str());
      }
    }
  }
}

const string ConvertToValidUTF8(const string& input) {
  return std::string(QString(input.c_str()).utf8());
}


// Extracts list of provider infos (from [export.provider.info] structure)
// in dbroot proto.
void ParseProviders(const EtaDocument& eta_doc, dbroot::DbRootProto* dbroot) {
  vector<EtaStruct*> providers;
  if (!GetListFromDocument(eta_doc, "export.provider.info", "etProviderInfo",
                           &providers)) {
    notify(NFY_WARN, "missing export.provider.info in dbroot");
    return;
  }

  vector<EtaStruct*>::const_iterator it;
  for (it = providers.begin(); it != providers.end(); ++it) {
    const EtaStruct* provider_struct = *it;
    EtaReader reader(provider_struct);
    // Each struct is supposed to contain 5 children:
    //   <etInt>      id
    //   <etString>   copyright
    //   <etInt>      copyrightY       // maybe omitted
    //   <etBool>     report           // ignored
    //   <etInt>      copyrightPri     // ignored
    if (provider_struct->num_children() >= 2) {
      std::int32_t provider_id = reader.GetInt("id", 0);
      const string input_copyright = reader.GetString("copyright", "");

      dbroot::ProviderInfoProto* provider_info = dbroot->add_provider_info();
      provider_info->set_provider_id(provider_id);
      // Special code for provider infos: replace '\aaa' literals in strings
      // with their UTF-8 equivalent, since all text data in protocol-buffers
      // must be UTF-8 encoded.
      // Providers copyright strings are the only strings in dbroot that can
      // contain such literals, we don't do that replacement anywhere else.
      string output_copyright = ConvertToValidUTF8(input_copyright);
      AddStringToProto(output_copyright,
                       provider_info->mutable_copyright_string());
      // Optional: pixel offset. Unfortunately ETA definitions declare the
      // default value for this variable as -1 instead of 0.
      std::int32_t pixel_offset = reader.GetInt("copyrightY", -1);
      provider_info->set_vertical_pixel_offset(pixel_offset);
    }
  }
}

// Utility functions for ParseETANestedFeatures.
namespace {

// Converts string to float.
inline float ToFloat(const string& str) {
  return strtod(str.c_str(), NULL);
}

}  // namespace

void ParseLookAtString(const string& look_at_string,
                       dbroot::NestedFeatureProto* nested_feature) {
  // Split and skip empty string.
  vector<string> look_at;
  TokenizeString(look_at_string, look_at, "|");

  // Valid look_at strings will have at least a latitude and longitude.
  if (look_at.size() < 2) {
    return;
  }

  dbroot::LookAtProto* look_at_proto = nested_feature->mutable_look_at();
  look_at_proto->set_longitude(ToFloat(look_at[0]));
  look_at_proto->set_latitude(ToFloat(look_at[1]));

  if (look_at.size() >= 3)
    look_at_proto->set_range(ToFloat(look_at[2]));
  if (look_at.size() >= 4)
    look_at_proto->set_tilt(ToFloat(look_at[3]));
  if (look_at.size() >= 5)
    look_at_proto->set_heading(ToFloat(look_at[4]));
}

void ParseEndSnippet(const EtaDocument& doc, dbroot::DbRootProto* dbroot) {
  CHECK(doc.root());
  const EtaStruct& root = *doc.root();

  dbroot::EndSnippetProto* snippet = dbroot->mutable_end_snippet();

  // PlanetModelProto model = 1;
  ParseModel(root, snippet);

  // StringIdOrValueProto auth_server_url = 2;
  MaybeSetUrlFromEta(root, "authServer", snippet->mutable_auth_server_url());

  const EtaStruct* child = NULL;

  // bool disable_authentication = 3;
  child = root.GetChildByName("authInfo.disableAuthKey");
  if (child && child->has_value()) {
    snippet->set_disable_authentication(true);
  }

  // MfeDomainFeaturesProto mfe_domains = 4;
  child = root.GetChildByName("googleMFEServer.supportedDomains");
  if (child && child->has_value()) {
    TranslateSupportedDomains(child->value(), snippet);
  }

  // string mfe_lang_param = 5;
  child = root.GetChildByName("mfeLangParam");
  if (child && child->has_value()) {
    snippet->set_mfe_lang_param(child->value());
  }

  // string ads_url_patterns = 6;
  child = root.GetChildByName("export.adsUrlPatterns");
  if (child && child->has_value()) {
    snippet->set_ads_url_patterns(child->value());
  }

  // StringIdOrValueProto reverse_geocoder_url = 7;
  if (!MaybeAddStringFromEta(root, "googleMFEReverseGeocoder",
                             snippet->mutable_reverse_geocoder_url())) {
    snippet->clear_reverse_geocoder_url();
  }

  // int32 reverse_geocoder_protocol_version = 8;
  child = root.GetChildByName("reverseGeocodingServerVersion");
  if (child && child->has_value()) {
    snippet->set_reverse_geocoder_protocol_version(child->GetInt32Value());
  }

  // bool sky_database_is_available = 9 [default = true];
  child = root.GetChildByName("export.skyDatabase.isAvailable");
  if (child && child->has_value()) {
    snippet->set_sky_database_is_available(child->GetBoolValue());
  }

  // StringIdOrValueProto sky_database_url = 10;
  if (!MaybeAddStringFromEta(root, "export.skyDatabase.URL",
                             snippet->mutable_sky_database_url())) {
    snippet->clear_sky_database_url();
  }

  // StringIdOrValueProto default_web_page_intl_url = 11;
  if (!MaybeAddStringFromEta(root, "defaultWebPageIntlURL",
                             snippet->mutable_default_web_page_intl_url())) {
    snippet->clear_default_web_page_intl_url();
  }

  // int32 num_start_up_tips = 12;
  child = root.GetChildByName("numStartupTips");
  if (child && child->has_value()) {
    snippet->set_num_start_up_tips(child->GetInt32Value());
  }

  // StringIdOrValueProto start_up_tips_url = 13;
  if (!MaybeAddStringFromEta(root, "startupTipsURL",
                             snippet->mutable_start_up_tips_url())) {
    snippet->clear_start_up_tips_url();
  }

  // StringIdOrValueProto user_guide_intl_url = 14;
  if (!MaybeAddStringFromEta(root, "userGuideIntlURL",
                             snippet->mutable_user_guide_intl_url())) {
    snippet->clear_user_guide_intl_url();
  }

  // StringIdOrValueProto support_center_intl_url = 15;
  if (!MaybeAddStringFromEta(root, "supportCenterIntlURL",
                             snippet->mutable_support_center_intl_url())) {
    snippet->clear_support_center_intl_url();
  }

  // StringIdOrValueProto business_listing_intl_url = 16;
  if (!MaybeAddStringFromEta(root, "businessListingIntlURL",
                             snippet->mutable_business_listing_intl_url())) {
    snippet->clear_business_listing_intl_url();
  }

  // StringIdOrValueProto support_answer_intl_url = 17;
  if (!MaybeAddStringFromEta(root, "supportAnswerIntlURL",
                             snippet->mutable_support_answer_intl_url())) {
    snippet->clear_support_answer_intl_url();
  }

  // StringIdOrValueProto support_topic_intl_url = 18;
  if (!MaybeAddStringFromEta(root, "supportTopicIntlURL",
                             snippet->mutable_support_topic_intl_url())) {
    snippet->clear_support_topic_intl_url();
  }

  // StringIdOrValueProto support_request_intl_url = 19;
  if (!MaybeAddStringFromEta(root, "supportRequestIntlURL",
                             snippet->mutable_support_request_intl_url())) {
    snippet->clear_support_request_intl_url();
  }

  // StringIdOrValueProto earth_intl_url = 20;
  if (!MaybeAddStringFromEta(root, "earthIntlURL",
                             snippet->mutable_earth_intl_url())) {
    snippet->clear_earth_intl_url();
  }

  // StringIdOrValueProto add_content_url = 21;
  if (!MaybeAddStringFromEta(root, "addContentURL",
                             snippet->mutable_add_content_url())) {
    snippet->clear_add_content_url();
  }

  // StringIdOrValueProto sketchup_not_installed_url = 22;
  if (!MaybeAddStringFromEta(root, "sketchupNotInstalledURL",
                             snippet->mutable_sketchup_not_installed_url())) {
    snippet->clear_sketchup_not_installed_url();
  }

  // StringIdOrValueProto sketchup_error_url = 23;
  if (!MaybeAddStringFromEta(root, "sketchupErrorURL",
                             snippet->mutable_sketchup_error_url())) {
    snippet->clear_sketchup_error_url();
  }

  // StringIdOrValueProto free_license_url = 24;
  if (!MaybeAddStringFromEta(root, "plusLicenseURL",
                             snippet->mutable_free_license_url())) {
    snippet->clear_free_license_url();
  }

  // StringIdOrValueProto pro_license_url = 25;
  if (!MaybeAddStringFromEta(root, "proLicenseURL",
                             snippet->mutable_pro_license_url())) {
    snippet->clear_pro_license_url();
  }

  // StringIdOrValueProto tutorial_url = 48;
  if (!MaybeAddStringFromEta(root, "tutorialURL",
                             snippet->mutable_tutorial_url())) {
    snippet->clear_tutorial_url();
  }

  // StringIdOrValueProto keyboard_shortcuts_url = 49;
  if (!MaybeAddStringFromEta(root, "keyboardShortcutsURL",
                             snippet->mutable_keyboard_shortcuts_url())) {
    snippet->clear_keyboard_shortcuts_url();
  }

  // StringIdOrValueProto release_notes_url = 50;
  if (!MaybeAddStringFromEta(root, "releaseNotesURL",
                             snippet->mutable_release_notes_url())) {
    snippet->clear_release_notes_url();
  }

  // bool hide_user_data = 26 [default = false];
  child = root.GetChildByName("hideUserData");
  if (child && child->has_value()) {
    snippet->set_hide_user_data(child->GetBoolValue());
  }

  // bool use_ge_logo = 27 [default = true];
  child = root.GetChildByName("useGELogo");
  if (child && child->has_value()) {
    snippet->set_use_ge_logo(child->GetBoolValue());
  }

  // StringIdOrValueProto diorama_description_url_base = 28;
  if (!MaybeAddStringFromEta(root,
                             "export.netOptions.dioramaDescriptionURLBase",
                             snippet->mutable_diorama_description_url_base())) {
    snippet->clear_diorama_description_url_base();
  }

  // uint32 diorama_default_color = 29 [default = 0xffc7c2c7];
  child = root.GetChildByName("export.style.dioramaDefaultColor");
  if (child && child->has_value()) {
    snippet->set_diorama_default_color(child->GetUInt32Value());
  }

  // StringIdOrValueProto diorama_blacklist_url = 53;
  if (!MaybeAddStringFromEta(root, "export.netOptions.dioramaBlacklistUrl",
                             snippet->mutable_diorama_blacklist_url())) {
    snippet->clear_diorama_blacklist_url();
  }

  // ClientOptionsProto client_options = 30;
  ParseClientOptions(root, snippet);

  // FetchingOptionsProto fetching_options = 31;
  ParseFetchingOptions(root, snippet);

  // TimeMachineOptionsProto time_machine_options = 32;
  ParseTimeMachineOptions(root, snippet);

  // CSIOptionsProto csi_options = 33;
  ParseCSIOptions(root, snippet);

  // SearchTabProto search_tab = 34;
  ParseSearchTabs(doc, snippet);

  // CobrandProto cobrand_info = 35;
  ParseCobrandInfo(doc, snippet);

  // DatabaseDescriptionProto valid_database = 36;
  ParseValidDatabases(doc, snippet);

  // ConfigScriptProto config_script = 37;
  ParseClientConfigScripts(doc, snippet);

  // StringIdOrValueProto deauth_server_url = 38;
  MaybeSetUrlFromEta(root, "deauthServer",
                     snippet->mutable_deauth_server_url());

  // SwoopParamsProto swoop_parameters = 39;
  ParseSwoopParams(root, snippet);

  // PostingServerProto bbs_server_info = 40;
  if (!ParsePostingServer(root, "bbsServer",
                          snippet->mutable_bbs_server_info())) {
    snippet->clear_bbs_server_info();
  }

  // PostingServerProto data_error_server_info = 41;
  if (!ParsePostingServer(root, "dataErrorServer",
                          snippet->mutable_data_error_server_info())) {
    snippet->clear_data_error_server_info();
  }

  // PlanetaryDatabaseProto planetary_database = 42;
  ParsePlanetaryDatabases(doc, snippet);

  // LogServerProto log_server = 43;
  ParseLogServer(root, snippet);
}

// Extracts info from [model] in ETA into ModelProto.
void ParseModel(const EtaStruct& root, dbroot::EndSnippetProto* snippet) {
  CHECK(snippet);

  const EtaStruct* child = root.GetChildByName("model.radius");
  if (child && child->has_value()) {
    snippet->mutable_model()->set_radius(child->GetDoubleValue());
  }

  child = root.GetChildByName("model.flattening");
  if (child && child->has_value()) {
    snippet->mutable_model()->set_flattening(child->GetDoubleValue());
  }

  child = root.GetChildByName("model.elevationBias");
  if (child && child->has_value()) {
    snippet->mutable_model()->set_elevation_bias(child->GetDoubleValue());
  }

  child = root.GetChildByName("model.negativeAltitudeExponentBias");
  if (child && child->has_value()) {
    snippet->mutable_model()->set_negative_altitude_exponent_bias(
        child->GetInt32Value());
  }

  child = root.GetChildByName("model.compressedNegativeAltitudeThreshold");
  if (child && child->has_value()) {
    snippet->mutable_model()->set_compressed_negative_altitude_threshold(
        child->GetDoubleValue());
  }
}

void ParseClientOptions(const EtaStruct& root,
                        dbroot::EndSnippetProto* snippet) {
  CHECK(snippet);
  const EtaStruct* child;
  child = root.GetChildByName("export.options.disableDiskCache");
  if (child && child->has_value()) {
    snippet->mutable_client_options()->set_disable_disk_cache(
        child->GetBoolValue());
  }

  child = root.GetChildByName("export.options.disableEmbeddedBrowserVista");
  if (child && child->has_value()) {
    snippet->mutable_client_options()->set_disable_embedded_browser_vista(
        child->GetBoolValue());
  }

  child = root.GetChildByName("export.options.Atmosphere");
  if (child && child->has_value()) {
    snippet->mutable_client_options()->set_draw_atmosphere(
        child->GetBoolValue());
  }

  child = root.GetChildByName("export.options.stars");
  if (child && child->has_value()) {
    snippet->mutable_client_options()->set_draw_stars(child->GetBoolValue());
  }

  child = root.GetChildByName("export.options.shaderFilePrefix");
  if (child && child->has_value()) {
    snippet->mutable_client_options()->set_shader_file_prefix(child->value());
  }

  child = root.GetChildByName("export.options.useProtoBufferQuadtreePackets");
  if (child && child->has_value()) {
    snippet->mutable_client_options()->set_use_protobuf_quadtree_packets(
        child->GetBoolValue());
  }

  child = root.GetChildByName("export.options.extendedCopyrightIDs");
  if (child && child->has_value()) {
    snippet->mutable_client_options()->set_use_extended_copyright_ids(
        child->GetBoolValue());
  }

  // Precipitations options
  child = root.GetChildByName("precipitationParameters.imageURL");
  if (child && child->has_value()) {
    dbroot::ClientOptionsProto::PrecipitationsOptions* precipitation =
        snippet->mutable_client_options()->mutable_precipitations_options();
    precipitation->set_image_url(child->value());
  }

  child = root.GetChildByName("precipitationParameters.imageExpireTime");
  if (child && child->has_value()) {
    dbroot::ClientOptionsProto::PrecipitationsOptions* precipitation =
        snippet->mutable_client_options()->mutable_precipitations_options();
    precipitation->set_image_expire_time(child->GetInt32Value());
  }

  child = root.GetChildByName("precipitationParameters.imageLevel");
  if (child && child->has_value()) {
    dbroot::ClientOptionsProto::PrecipitationsOptions* precipitation =
        snippet->mutable_client_options()->mutable_precipitations_options();
    precipitation->set_image_level(child->GetInt32Value());
  }

  child = root.GetChildByName("precipitationParameters.maxColorDistance");
  if (child && child->has_value()) {
    dbroot::ClientOptionsProto::PrecipitationsOptions* precipitation =
        snippet->mutable_client_options()->mutable_precipitations_options();
    precipitation->set_max_color_distance(child->GetInt32Value());
  }

  // Capture options
  const EtaStruct* eta_capture = root.GetChildByName("captureOptions");
  if (eta_capture) {
    dbroot::ClientOptionsProto::CaptureOptions* capture =
        snippet->mutable_client_options()->mutable_capture_options();
    child = eta_capture->GetChildByName("allowSaveAsImage");
    if (child) {
      capture->set_allow_save_as_image(child->GetBoolValue());
    }
    child = eta_capture->GetChildByName("maxFreeCaptureRes");
    if (child) {
      capture->set_max_free_capture_res(child->GetInt32Value());
    }
    child = eta_capture->GetChildByName("premiumCaptureRes");
    if (child) {
      capture->set_max_premium_capture_res(child->GetInt32Value());
    }
  }

  child = root.GetChildByName("show2DMapsIcon");
  if (child && child->has_value()) {
    snippet->mutable_client_options()->set_show_2d_maps_icon(
        child->GetBoolValue());
  }

  child = root.GetChildByName("export.options.polarTileMergingLevel");
  if (child && child->has_value()) {
    snippet->mutable_client_options()->set_polar_tile_merging_level(
        child->GetInt32Value());
  }
}

void ParseFetchingOptions(const EtaStruct& root,
                          dbroot::EndSnippetProto* snippet) {
  CHECK(snippet);
  const EtaStruct* child;
  child = root.GetChildByName("export.netOptions.maxRequestsPerQuery");
  if (child && child->has_value()) {
    snippet->mutable_fetching_options()->set_max_requests_per_query(
        child->GetInt32Value());
  }

  child = root.GetChildByName("export.netOptions.maxDrawable");
  if (child && child->has_value()) {
    snippet->mutable_fetching_options()->set_max_drawable(
        child->GetInt32Value());
  }

  child = root.GetChildByName("export.netOptions.maxImagery");
  if (child && child->has_value()) {
    snippet->mutable_fetching_options()->set_max_imagery(
        child->GetInt32Value());
  }

  child = root.GetChildByName("export.netOptions.maxTerrain");
  if (child && child->has_value()) {
    snippet->mutable_fetching_options()->set_max_terrain(
        child->GetInt32Value());
  }

  child = root.GetChildByName("export.netOptions.maxQuadNode");
  if (child && child->has_value()) {
    snippet->mutable_fetching_options()->set_max_quadtree(
        child->GetInt32Value());
  }

  child = root.GetChildByName("export.netOptions.maxDioramaMetadata");
  if (child && child->has_value()) {
    snippet->mutable_fetching_options()->set_max_diorama_metadata(
        child->GetInt32Value());
  }

  child = root.GetChildByName("export.netOptions.maxDioramaData");
  if (child && child->has_value()) {
    snippet->mutable_fetching_options()->set_max_diorama_data(
        child->GetInt32Value());
  }

  child = root.GetChildByName("export.maxFetchRatio");

  if (child && child->has_value()) {
    snippet->mutable_fetching_options()->set_max_consumer_fetch_ratio(
        child->GetFloatValue());
  }

  child = root.GetChildByName("export.maxProECFetchRatio");
  if (child && child->has_value()) {
    snippet->mutable_fetching_options()->set_max_pro_ec_fetch_ratio(
        child->GetFloatValue());
  }

  child = root.GetChildByName("safeOverallQps");
  if (child && child->has_value()) {
    snippet->mutable_fetching_options()->set_safe_overall_qps(
        child->GetFloatValue());
  }

  child = root.GetChildByName("safeImageryQps");
  if (child && child->has_value()) {
    snippet->mutable_fetching_options()->set_safe_imagery_qps(
        child->GetFloatValue());
  }
}

void ParseTimeMachineOptions(const EtaStruct& root,
                             dbroot::EndSnippetProto* snippet) {
  CHECK(snippet);

  const EtaStruct* child;
  child = root.GetChildByName("export.timemachineDatabase.timeMachineUrl");
  if (child && child->has_value()) {
    snippet->mutable_time_machine_options()->set_server_url(child->value());
  }

  child = root.GetChildByName("export.timemachineDatabase.isTimeMachine");
  if (child && child->has_value()) {
    snippet->mutable_time_machine_options()->set_is_timemachine(
        child->GetBoolValue());
  }

  child = root.GetChildByName("export.timemachineDatabase.dwellTime");
  if (child && child->has_value()) {
    snippet->mutable_time_machine_options()->set_dwell_time_ms(
        child->GetInt32Value());
  }
}

// Extracts [export.csiLogServer] parameters from dbroot into CSIOptionsProto.
void ParseCSIOptions(const EtaStruct& root,
                     dbroot::EndSnippetProto* snippet) {
  CHECK(snippet);

  const EtaStruct* child;
  child = root.GetChildByName("export.csiLogServer.samplingPercentage");
  if (child && child->has_value()) {
    snippet->mutable_csi_options()->set_sampling_percentage(
        child->GetInt32Value());
  }

  child = root.GetChildByName("export.csiLogServer.experimentId");
  if (child && child->has_value()) {
    snippet->mutable_csi_options()->set_experiment_id(child->value());
  }
}

// Helper function for search tabs - based on label, query and query_prepend,
// adds an input box to given search tab (assume non-empty label means an input
// box is needed).
void MaybeAddInputBox(dbroot::SearchTabProto* search_tab,
                      const string& label, const string& query,
                      const string& query_prepend) {
  CHECK(search_tab);
  if (!label.empty()) {
    dbroot::SearchTabProto::InputBoxInfo* box = search_tab->add_input_box();
    AddStringToProto(label, box->mutable_label());
    box->set_query_verb(query);
    box->set_query_prepend(query_prepend);
  }
}

// Extracts [export.searchTabs] parameters from dbroot into SearchTabProto.
void ParseSearchTabs(const EtaDocument& eta_doc,
                     dbroot::EndSnippetProto* snippet) {
  CHECK(snippet);

  vector<EtaStruct*> tabs_list;
  if (!GetListFromDocument(eta_doc, "export.searchTabs", "etSearchTab",
                           &tabs_list)) {
    return;
  }

  for (vector<EtaStruct*>::const_iterator it = tabs_list.begin();
       it != tabs_list.end(); ++it) {
    const EtaStruct* eta_tab = *it;
    EtaReader reader(eta_tab);
    int num_children = eta_tab->num_children();
    if (num_children < 1) {
      notify(NFY_WARN, "invalid search tab %s", eta_tab->name().c_str());
      continue;
    }
    dbroot::SearchTabProto* search_tab = snippet->add_search_tab();
    search_tab->set_is_visible(reader.GetBool("visible", true));

    string label = reader.GetString("label", "");
    if (!label.empty()) {
      AddStringToProto(label, search_tab->mutable_tab_label());
    }

    // Read URL fragments and build URL.
    QUrl url;
    url.setHost(reader.GetString("host", "").c_str());
    url.setPort(reader.GetInt("port", 0));
    url.setPath(reader.GetString("path", "").c_str());
    url.setProtocol(reader.GetBool("secure", false) ? "https" : "http");
    if (url.isValid()) {
      search_tab->set_base_url(url.toString());
    } else {
      notify(NFY_WARN, "Invalid URL info in search tab: %s\n    host: %s"
                        "    port: %d    secure: %s", eta_tab->name().c_str(),
                        string(url.host()).c_str(), url.port(),
                        string(url.protocol()).c_str() );
    }

    string viewport_prefix = reader.GetString("viewportPrefix", "").c_str();
    if (!viewport_prefix.empty()) {
      search_tab->set_viewport_prefix(viewport_prefix);
    }

    // First input box.
    label = reader.GetString("inputLabel1", "").c_str();
    string query = reader.GetString("inputQueryVerb1", "").c_str();
    string query_prepend = reader.GetString("inputQueryPrepend1", "").c_str();

    MaybeAddInputBox(search_tab, label, query, query_prepend);

    // Second input box.
    label = reader.GetString("inputLabel2", "").c_str();
    query = reader.GetString("inputQueryVerb2", "").c_str();
    query_prepend = reader.GetString("inputQueryPrepend2", "").c_str();

    MaybeAddInputBox(search_tab, label, query, query_prepend);
  }
}

// Extracts list of custom logos in [export.cobrandInfo] into CobrandProto.
void ParseCobrandInfo(const EtaDocument& eta_doc,
                      dbroot::EndSnippetProto* snippet) {
  CHECK(snippet);

  vector<EtaStruct*> cobrand_list;
  if (!GetListFromDocument(eta_doc, "export.cobrandInfo", "etCobrandInfo",
                           &cobrand_list)) {
    return;
  }

  for (vector<EtaStruct*>::const_iterator it = cobrand_list.begin();
       it != cobrand_list.end(); ++it) {
    const EtaStruct* cobrand_struct = *it;
    if (cobrand_struct->num_children() == 0) {
      continue;
    }
    string url = MaybeReadStringFromChild(cobrand_struct, 0);
    if (url.empty()) {
      continue;
    }
    dbroot::CobrandProto* cobrand = snippet->add_cobrand_info();
    cobrand->set_logo_url(url);
    double x_coord = MaybeReadDoubleFromChild(cobrand_struct, 1);
    double y_coord = MaybeReadDoubleFromChild(cobrand_struct, 2);
    bool x_relative = MaybeReadBoolFromChild(cobrand_struct, 3);
    bool y_relative = MaybeReadBoolFromChild(cobrand_struct, 4);
    double screen_size = MaybeReadDoubleFromChild(cobrand_struct, 6);
    cobrand->mutable_x_coord()->set_value(x_coord);
    cobrand->mutable_x_coord()->set_is_relative(x_relative);
    cobrand->mutable_y_coord()->set_value(y_coord);
    cobrand->mutable_y_coord()->set_is_relative(y_relative);
    cobrand->set_screen_size(screen_size);

    // Map tie-point from string into enum values.
    using dbroot::CobrandProto;
    static map<string, CobrandProto::TiePoint> tie_point_map;
    if (tie_point_map.empty()) {
      tie_point_map["top-left"] = CobrandProto::TOP_LEFT;
      tie_point_map["top-center"] = CobrandProto::TOP_CENTER;
      tie_point_map["top-right"] = CobrandProto::TOP_RIGHT;
      tie_point_map["center-left"] = CobrandProto::MID_LEFT;
      tie_point_map["center-center"] = CobrandProto::MID_CENTER;
      tie_point_map["center-right"] = CobrandProto::MID_RIGHT;
      tie_point_map["bottom-left"] = CobrandProto::BOTTOM_LEFT;
      tie_point_map["bottom-center"] = CobrandProto::BOTTOM_CENTER;
      tie_point_map["bottom-right"] = CobrandProto::BOTTOM_RIGHT;
    }

    string tie_point = MaybeReadStringFromChild(cobrand_struct, 5);
    if (!tie_point.empty()) {
      map<string, CobrandProto::TiePoint>::const_iterator it =
          tie_point_map.find(tie_point);
      if (it == tie_point_map.end()) {
        notify(NFY_WARN, "invalid tie-point string in dbroot: %s",
               tie_point.c_str());
        continue;
      }
      cobrand->set_tie_point(it->second);
    }
  }
}

// Extracts list of valid databases from [export.validservers] into
// DatabaseDescriptionProto.
void ParseValidDatabases(const EtaDocument& eta_doc,
                         dbroot::EndSnippetProto* snippet) {
  CHECK(snippet);
  vector<EtaStruct*> database_list;
  if (!GetListFromDocument(eta_doc, "export.validservers", kEtStructType,
                           &database_list)) {
    return;
  }

  for (vector<EtaStruct*>::const_iterator it = database_list.begin();
       it != database_list.end(); ++it) {
    const EtaStruct* database_struct = *it;
    const string& name = database_struct->name();

    string host;
    MaybeReadFromNamedChild(*database_struct, "host", &host);
    std::int32_t port = 0;
    MaybeReadFromNamedChild(*database_struct, "port", &port);

    if (!name.empty() && !host.empty()) {
      dbroot::DatabaseDescriptionProto* database =
          snippet->add_valid_database();
      AddStringToProto(name, database->mutable_database_name());

      // Build url from host (which can be a full url) and port.
      QUrl url;
      url.setHost(host.c_str());
      if (port != 0) {
        url.setPort(port);
      }
      database->set_database_url(url.toString());
    }
  }
}

// Extracts list of config scripts from [export.clientConfigScripts] into
// ConfigScriptProto.
void ParseClientConfigScripts(const EtaDocument& eta_doc,
                              dbroot::EndSnippetProto* snippet) {
  vector<EtaStruct*> config_script_list;
  if (!GetListFromDocument(eta_doc, "export.clientConfigScripts",
                           kEtStructType, &config_script_list)) {
    return;
  }

  for (vector<EtaStruct*>::const_iterator it = config_script_list.begin();
       it != config_script_list.end(); ++it) {
    const EtaStruct* eta_name = (*it)->GetChildByName("name");
    const EtaStruct* eta_text = (*it)->GetChildByName("text");
    if (eta_name && eta_text &&
        eta_name->has_value() && eta_text->has_value()) {
      dbroot::ConfigScriptProto* script = snippet->add_config_script();
      script->set_script_name(eta_name->value());
      script->set_script_data(google::protobuf::StringReplace(
          eta_text->value(), "\\n", "\n", true));
    }
  }
}

// Extracts swoop parameters in [swoopParams] in SwoopParamsProto.
void ParseSwoopParams(const EtaStruct& root, dbroot::EndSnippetProto* snippet) {
  CHECK(snippet);

  const EtaStruct* child;
  child = root.GetChildByName("swoopParams.startDistInMeters");
  if (child && child->has_value()) {
    snippet->mutable_swoop_parameters()->set_start_dist_in_meters(
        child->GetDoubleValue());
  }
}

bool ParsePostingServer(const EtaStruct& root,
                        const char* server_base_name,
                        dbroot::PostingServerProto* posting_server) {
  MaybeSetUrlFromEta(root, server_base_name,
                     posting_server->mutable_base_url());

  string field_name = string(server_base_name) + ".postWizardPath";
  if (!MaybeAddStringFromEta(root, field_name.c_str(),
                             posting_server->mutable_post_wizard_path())) {
    posting_server->clear_post_wizard_path();
  }

  field_name = string(server_base_name) + ".fileSubmitPath";
  if (!MaybeAddStringFromEta(root, field_name.c_str(),
                             posting_server->mutable_file_submit_path())) {
    posting_server->clear_file_submit_path();
  }

  // Name not set on purpose (default value in client good enough).
  return true;
}

// Extracts list of databases in [export.planetaryDatabases] into
// PlanetaryDatabaseProto.
void ParsePlanetaryDatabases(const EtaDocument& eta_doc,
                             dbroot::EndSnippetProto* snippet) {
  vector<EtaStruct*> planets;
  if (!GetListFromDocument(eta_doc, "export.planetaryDatabases", kEtStructType,
                           &planets)) {
    return;
  }

  for (vector<EtaStruct*>::const_iterator it = planets.begin();
       it != planets.end(); ++it) {
    const EtaStruct* name = (*it)->GetChildByName("name");
    const EtaStruct* url = (*it)->GetChildByName("url");
    if (name && url && name->has_value() && url->has_value()) {
      dbroot::PlanetaryDatabaseProto* planet =
          snippet->add_planetary_database();
      AddStringToProto(name->value(), planet->mutable_name());
      AddStringToProto(url->value(), planet->mutable_url());
      CHECK(planet->IsInitialized());
    }
  }
}

// Extracts [logServer] parameters into LogServerProto.
void ParseLogServer(const EtaStruct& root, dbroot::EndSnippetProto* snippet) {
  if (!MaybeSetUrlFromEta(root, "logServer",
                          snippet->mutable_log_server()->mutable_url())) {
    snippet->clear_log_server();
  }
  bool enable;
  if (MaybeReadFromNamedChild(root, "logServer.enableLog", &enable)) {
    snippet->mutable_log_server()->set_enable(enable);
  }

  std::int32_t factor;
  if (MaybeReadFromNamedChild(root, "logServer.throttleFactor", &factor)) {
    snippet->mutable_log_server()->set_throttling_factor(factor);
  }
}

}  // namespace eta_utils
}  // namespace keyhole
