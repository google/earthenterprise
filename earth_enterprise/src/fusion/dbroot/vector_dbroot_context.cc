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


#include "fusion/dbroot/vector_dbroot_context.h"
#include "common/khSimpleException.h"
#include "common/khFileUtils.h"
#include "fusion/khgdal/khgdal.h"
#include "fusion/dbroot/vector_dbroot_generator.h"

void VectorDbrootContext::Init(const std::string &config_filename) {
  if (!config_.Load(config_filename)) {
    throw khSimpleException("Unable to load config file ") <<
        config_filename;
  }

  // the test cases have specially modified config files with relative paths
  // for the LOD files. This is where we need to turn them into absolute paths.
  std::string config_dir = khDirname(config_filename);
  for (uint i = 0; i < config_.layerLODFiles.size(); ++i) {
    if (!config_.layerLODFiles[i].empty() &&
        !khIsAbspath(config_.layerLODFiles[i])) {
      config_.layerLODFiles[i] = khComposePath(config_dir,
                                               config_.layerLODFiles[i]);
    }
  }

  // populate the group map
  for (uint i = 0; i < config_.layers.size(); ++i) {
    const LayerConfig *layer = &config_.layers[i];
    if (layer->IsFolder()) {
      QString fullPath = layer->DefaultNameWithPath();
      if (groupMap.find(fullPath) == groupMap.end()) {
        groupMap[fullPath] = i;
      }
    }
  }

  // populate used_layer_ids_ (skipping empty folders)
  for (uint i = 0; i < config_.layers.size(); ++i) {
    const LayerConfig *layer = &config_.layers[i];
    if (!layer->IsFolder()) {
      // Add this layer's id
      used_layer_ids_.insert(i);

      // add all my parent groups back up to the top
      QString legend = layer->legend;
      while (!legend.isEmpty()) {
        // my group should be here, but check just in case
        // some of the funky "!" legend names get through to here
        GroupMapConstIterator found = groupMap.find(legend);
        if (found != groupMap.end()) {
          used_layer_ids_.insert(found->second);
          legend = config_.layers[found->second].legend;
        } else {
          break;
        }
      }
    }
  }
}


VectorDbrootContext::VectorDbrootContext(const std::string &config_filename)
    : ProtoDbrootContext() {
  Init(config_filename);
}

VectorDbrootContext::VectorDbrootContext(const std::string &config_filename,
                                         TestingFlagType testing_flag)
    : ProtoDbrootContext(testing_flag) {
  Init(config_filename);
}


void VectorDbrootContext::EmitAll(const std::string &out_dir,
                                  geProtoDbroot::FileFormat output_format) {
  // scan through my config and gather a set of all referenced locales
  std::set<QString> used_locales;
  for (std::set<uint>::const_iterator it = used_layer_ids_.begin();
       it != used_layer_ids_.end(); ++it) {
    const LayerConfig &layer = config_.layers[*it];
    typedef std::map<QString, LocaleConfig>::const_iterator MapIterator;
    for (MapIterator m = layer.locales.begin(); m != layer.locales.end(); ++m) {
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
    VectorDbrootGenerator dbrootgen(this, kDefaultLocaleSuffix.c_str(), outfile);
    dbrootgen.Emit(output_format);
  }

  // emit localized dbRoots
  {
    typedef std::set<QString>::const_iterator SetIterator;
    for (SetIterator i = used_locales.begin(); i != used_locales.end(); ++i) {
      std::string outfile = out_dir + "/" + kPostamblePrefix +
          "." + i->latin1();
      VectorDbrootGenerator dbrootgen(this, *i /* locale */, outfile);
      dbrootgen.Emit(output_format);
    }
  }

  // now install the icons that were used by any of the dbroots
  InstallIcons(out_dir);
}

int VectorDbrootContext::GetIconWidth(const IconReference &icon_ref) const {
  // in testing environments we don't have the installed icons
  // so we can't actually check their width.
  // All testing icons are 32pixels wide.
  if (testing_only_) {
    return 32;
  }


  // see if we already have the width for this one
  IconWidthMap::const_iterator found = icon_width_map_.find(icon_ref);
  if (found != icon_width_map_.end()) {
    return found->second;
  }

  // make sure GDAL is initialized - safe to call multiple times
  khGDALInit();

  // load the icon, extract the width and close it again
  GDALDataset *srcDataset =
      static_cast<GDALDataset*>(GDALOpen(icon_ref.SourcePath().c_str(),
                                         GA_ReadOnly));
  if (!srcDataset) {
    throw khException(kh::tr("Unable to open %1").arg(icon_ref.SourcePath().c_str()));
  }
  int srcXSize = srcDataset->GetRasterXSize();
  delete srcDataset;

  // remember and return the width
  icon_width_map_[icon_ref] = srcXSize;
  return srcXSize;
}
