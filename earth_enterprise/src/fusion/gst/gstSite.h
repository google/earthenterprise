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

#ifndef KHSRC_FUSION_GST_GSTSITE_H__
#define KHSRC_FUSION_GST_GSTSITE_H__

#include <vector>
#include <qstring.h>
#include <gstVertex.h>
#include <gstRecord.h>
#include <autoingest/.idl/storage/LayerConfig.h>
#include <gstGeode.h>
#include <gstIconManager.h>

class gstSelector;
class gstFilter;
class JSDisplayBundle;

// ****************************************************************************
// ***  gstSitePreviewConfig
// ***
// ***  Holds the members with the styling information needed for previewing
// ***  in the fusion GUI.
// ****************************************************************************
class gstSitePreviewConfig {
 public:
  IconConfig icon_;
  std::vector< unsigned int>  normal_color_;
  std::vector< unsigned int>  highlight_color_;

  // used when building a gstSite when you only have a FuseSiteConfig
  // the values don't matter, since they will never be used
  gstSitePreviewConfig(void) :
      icon_(),
      normal_color_(std::vector< unsigned int> (4, 255)),
      highlight_color_(std::vector< unsigned int> (4, 255))
  { }

  explicit gstSitePreviewConfig(const SiteConfig &cfg) :
      icon_(cfg.style.icon),
      normal_color_(cfg.style.label.normalColor),
      highlight_color_(cfg.style.label.highlightColor)
  { }
};



class gstSite {
 public:
  gstSite(const FuseSiteConfig& cfg, const gstSitePreviewConfig &preview_cfg);
  virtual ~gstSite();

  // some convenience wrappers around config
  gstIcon icon() const { return gstIcon(preview_config.icon_); }
  QString label() const { return config.style.label.label; }

  // the real workers
  gstVertex Location(const gstGeodeHandle &feature) const;
  gstRecordHandle Expand(gstRecordHandle record,
                         bool label_only,
                         JSDisplayBundle &jsbundle,
                         unsigned int filterId) const;

  // This is now const. No more setting here. No more config extraction either!
  const FuseSiteConfig config;
  const gstSitePreviewConfig preview_config;


 private:
  double offset_x_, offset_y_;

  mutable gstHeaderHandle site_header_;
  mutable gstRecordFormatter* label_format_;
  mutable gstRecordFormatter* popup_text_format_;
  mutable gstRecordFormatter* custom_height_format_;
};


class gstSiteSet {
 public:
  gstSiteSet() : site(0) { }

  VertexList vlist;
  RecordList rlist;
  const gstSite* site;
};

#endif  // !KHSRC_FUSION_GST_GSTSITE_H__
