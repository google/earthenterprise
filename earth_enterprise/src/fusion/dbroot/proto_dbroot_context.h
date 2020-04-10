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

//  Base class for RasterDbrootContext and VectorDbrootContext
//  Dbroot contexts are the objects that remember state across multiple
//  locale-specialized dbroot generators for a single project. The
//  ProviderMap can be loaded only once and reused for all the local-specific
//  dbroots. And we want to gather the set of all unique icons that are used
//  by all the dbroots for a given project.
//  This class will only ever be used when specialized for Vector or Raster


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_PROTO_DBROOT_CONTEXT_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_PROTO_DBROOT_CONTEXT_H_

#include <set>
#include <map>
//#include "common/khTypes.h"
#include <cstdint>
#include "fusion/autoingest/.idl/gstProvider.h"
#include "fusion/autoingest/.idl/Locale.h"
#include "fusion/autoingest/.idl/storage/IconReference.h"

class ProtoDbrootContext {
  friend class Test_dbrootgen;

 private:
  typedef std::map<std::uint32_t, gstProvider> ProviderMap;
  typedef std::set<IconReference>       UsedIconSet;

 public:
  enum TestingFlagType { kForTesting };

  ProtoDbrootContext(void);
  explicit ProtoDbrootContext(TestingFlagType testing_flag);
  const gstProvider* GetProvider(std::uint32_t id) const;

  // Add icon references to the various context sets.
  // An IconReference in fusion references an image stack (single image
  // of known size with multiple glyphs at known locations)
  // The context the icon is referenced from (e.g. legend, normal, etc),
  // tells this class which of the subglyphs to extract and how to name the
  // filename (e.g 776_l.png, 776_h.png)
  void AddLegendIcon(const IconReference &icon) {
    used_legend_icons_.insert(icon);
  }
  void AddNormalIcon(const IconReference &icon) {
    used_normal_icons_.insert(icon);
  }
  void AddHighlightIcon(const IconReference &icon) {
    used_highlight_icons_.insert(icon);
  }
  void AddNormalHighlightIcon(const IconReference &icon) {
    used_normalhighlight_icons_.insert(icon);
  }

  // Walks all the icon context sets, extracts the appropriate glyphs from
  // the image stack and writes them into <outdir>/icons/*.png
  void InstallIcons(const std::string &outdir);

 public:
  LocaleSet   locale_set_;

 protected:
  bool        testing_only_;

 private:
  ProviderMap provider_map_;
  UsedIconSet used_legend_icons_;
  UsedIconSet used_normal_icons_;
  UsedIconSet used_highlight_icons_;
  UsedIconSet used_normalhighlight_icons_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_PROTO_DBROOT_CONTEXT_H_
