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

// Base class for RasterDbrootContext and VectorDbrootContext
// See header file for more complete description

#include "fusion/dbroot/proto_dbroot_context.h"
#include "common/khException.h"
#include "common/khFileUtils.h"
#include "fusion/iconutil/iconutil.h"

ProtoDbrootContext::ProtoDbrootContext(TestingFlagType testing_flag)
    : testing_only_(true) {
  // Nothing to do when loading the context for testing.
  // The test case will populate providers and locales to a known state
}

ProtoDbrootContext::ProtoDbrootContext(void) : testing_only_(false) {
  gstProviderSet providers;
  if (!providers.Load()) {
    throw khException(kh::tr("Unable to load providers"));
  }
  for (std::vector<gstProvider>::const_iterator p =
           providers.items.begin();
       p != providers.items.end(); ++p) {
    provider_map_[p->id] = *p;
  }

  // Read the supported locales
  if (!locale_set_.Load()) {
    throw khException(kh::tr("Unable to load locales"));
  }
}

const gstProvider* ProtoDbrootContext::GetProvider(std::uint32_t id) const {
  ProviderMap::const_iterator found = provider_map_.find(id);
  return (found == provider_map_.end()) ? 0 : &found->second;
}

void ProtoDbrootContext::InstallIcons(const std::string &outdir) {
  if (testing_only_) {
    // skip the installation of the icons when testing
    // Many of our test enviroments don't have an installed version of fusion
    // so we can't get the icon files to extract from.
    return;
  }

  // make the outdir for all the icons
  std::string icondir = outdir + "/icons";
  if (!khMakeDir(icondir)) {
    throw khException(kh::tr("Unable to create icon directory"));
  }

  // - legend -
  for (UsedIconSet::const_iterator icon = used_legend_icons_.begin();
       icon != used_legend_icons_.end(); ++icon) {
    std::string src = icon->SourcePath();
    std::string destbase = icondir + "/" + icon->LegendHref();

    iconutil::Extract(src, iconutil::Legend, destbase);
  }

  // - normal -
  for (UsedIconSet::const_iterator icon = used_normal_icons_.begin();
       icon != used_normal_icons_.end(); ++icon) {
    std::string src = icon->SourcePath();
    std::string destbase = icondir + "/" + icon->NormalHref();

    iconutil::Extract(src, iconutil::Normal, destbase);
  }

  // - highlight -
  for (UsedIconSet::const_iterator icon = used_highlight_icons_.begin();
       icon != used_highlight_icons_.end(); ++icon) {
    std::string src = icon->SourcePath();
    std::string destbase = icondir + "/" + icon->HighlightHref();

    iconutil::Extract(src, iconutil::Highlight, destbase);
  }

  // - normalhighlight -
  for (UsedIconSet::const_iterator icon = used_normalhighlight_icons_.begin();
       icon != used_normalhighlight_icons_.end(); ++icon) {
    std::string src = icon->SourcePath();
    std::string destbase = icondir + "/" + icon->NormalHighlightHref();

    iconutil::Extract(src, iconutil::NormalHighlight, destbase);
  }
}
