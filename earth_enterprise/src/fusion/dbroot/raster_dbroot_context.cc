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


#include "fusion/dbroot/raster_dbroot_context.h"
#include "common/khSimpleException.h"
#include "fusion/dbroot/raster_dbroot_generator.h"


void RasterDbrootContext::Init(const std::string &config_filename) {
  if (!config_.Load(config_filename)) {
    throw khSimpleException("Unable to load config file ") <<
        config_filename;
  }
}


RasterDbrootContext::RasterDbrootContext(const std::string &config_filename,
                                         bool is_imagery)
    : ProtoDbrootContext(), is_imagery_(is_imagery) {
  Init(config_filename);
}

RasterDbrootContext::RasterDbrootContext(const std::string &config_filename,
                                         bool is_imagery,
                                         TestingFlagType testing_flag)
    : ProtoDbrootContext(testing_flag), is_imagery_(is_imagery) {
  Init(config_filename);
}


void RasterDbrootContext::EmitAll(const std::string &out_dir,
                                  geProtoDbroot::FileFormat output_format) {
  // scan through my config and gather a set of all referenced locales
  std::set<QString> used_locales;
  for (uint i = 0; i < config_.layers_.size(); ++i) {
    typedef LayerLegend::LocaleMap::const_iterator MapIterator;
    for (MapIterator m = config_.layers_[i].legend_.locales.begin();
         m != config_.layers_[i].legend_.locales.end(); ++m) {
      // it's possible that a locale has been removed from the global locale
      // set after this config was saved. This implies that the user no
      // longer wished to generate anything for that locale.
      // If we find one of those locales, silently skip it
      if (locale_set_.validLocale(m->first)) {
        used_locales.insert(m->first);
      }
    }
  }

  // emit normal dbRoot
  {
    std::string outfile =
        out_dir + "/" + kPostamblePrefix + "." + kDefaultLocaleSuffix;
    // no filtering right now for raster dbroots, always emit all layers
    RasterDbrootGenerator dbrootgen(this, kDefaultLocaleSuffix.c_str(), outfile);
    dbrootgen.Emit(output_format);
  }

  // emit localized dbRoots
  {
    typedef std::set<QString>::const_iterator SetIterator;
    for (SetIterator i = used_locales.begin(); i != used_locales.end(); ++i) {
      std::string outfile = out_dir + "/" + kPostamblePrefix +
          "." + i->latin1();
      // no filtering right now for raster dbroots, always emit all layers
      RasterDbrootGenerator dbrootgen(this, *i /* locale */, outfile);
      dbrootgen.Emit(output_format);
    }
  }

  // now install the icons that were used by any of the dbroots
  InstallIcons(out_dir);
}
