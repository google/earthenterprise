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


#include "fusion/dbroot/raster_dbroot_generator.h"
#include "common/khConstants.h"
#include "common/khSimpleException.h"
#include "fusion/dbroot/raster_dbroot_context.h"


RasterDbrootGenerator::RasterDbrootGenerator(
    RasterDbrootContext *raster_context,
    const QString &locale,
    const std::string &outfile) :
    ProtoDbrootGenerator(*raster_context, locale, outfile,
                         false /* map_strings_to_ids */),
    raster_context_(raster_context) {
  // My config has the list of used providers. Add them to the baseclass.
  for (std::set<std::uint32_t>::const_iterator p =
           raster_context_->config_.used_provider_ids_.begin();
       p != raster_context_->config_.used_provider_ids_.end(); ++p) {
    AddProvider(*p);
  }
}

#ifdef SEND_RASTER_LAYERS
void RasterDbrootGenerator::EmitNestedLayers(void) {
  if (raster_context_->config_.layers_.size() != 1) {
    // The config was designed to someday support multiple imagery layers.
    // But we changed our minds and now use the "meta-dbroot" to accomplish
    // the same thing.
    // Therefore we only support a single layer here.
    throw khSimpleException("Unexpected number of raster layers: ")
        << raster_context_->config_.layers_.size();
  }

  const RasterDBRootGenConfig::Layer &layer_config =
      raster_context_->config_.layers_[0];

  // get localized strings for our current locale
  LegendLocale legend_locale = layer_config.legend_.GetLegendLocale(locale_);
  std::string display_name((const char*)legend_locale.name.GetValue().utf8());
  std::string description(
      (const char*)legend_locale.desc.GetValue().section('\n', 0, 0).utf8());
  std::string look_at((const char*)legend_locale.lookAt.GetValue().utf8());

  keyhole::dbroot::NestedFeatureProto* nested = outproto_.add_nested_feature();
  nested->set_channel_id(layer_config.channel_id_);
  nested->set_asset_uuid(layer_config.asset_uuid_.c_str());
  SetProtoStringId(nested->mutable_display_name(), display_name);
  nested->set_is_visible(true);
  nested->set_is_enabled(true);
  nested->set_is_checked(legend_locale.isChecked.GetValue());
  IconReference icon_ref = legend_locale.icon.GetValue();
  nested->set_layer_menu_icon_path("icons/" + icon_ref.LegendHref());
  raster_context_->AddLegendIcon(icon_ref);  // tell context about this icon
  if (!description.empty()) {
    SetProtoStringId(nested->mutable_description(), description);
  }
  if (!SetLookAtString(nested, look_at)) {
    throw khSimpleException("Invalid LookAt string for layer '")
        << display_name << "'";
  }

  // GE clients have a feature that allows a user to right click on a
  // "layer" in the left hand pane of the client and save it to disk so they
  // can then view the content even if they are not connected to the
  // database. It was intended for KML, but can actually work for some
  // streamed layers too. A decision was made years ago, to simply disallow
  // saving of any streamed layers.
  nested->set_is_save_locked(true);
}
#endif  /* ifdef SEND_RASTER_LAYERS */

void RasterDbrootGenerator::Emit(geProtoDbroot::FileFormat output_format) {
#ifdef SEND_RASTER_LAYERS
  EmitNestedLayers();
#endif
  if (raster_context_->is_imagery_) {
    outproto_.set_proto_imagery(true);

    // NOTE: imagery_present set by gedbgen since it also needs to set if
    // false if there is no imagery
  } else {
    // NOTE: terrain_present set by gedbgen since it also needs to set if
    // false if there is no imagery

    keyhole::dbroot::PlanetModelProto* model =
        outproto_.mutable_end_snippet()->mutable_model();
    model->set_radius(khEarthRadius/1000);

    // Tell the client which values were used to encode negative elevations.
    // The terrain packets are encoded using unsigned shorts which
    // intrinsically cannot represent negative values. A bias and threshhold
    // method was devised to be able to encode negative elevations. The
    // client has to know which values fusion used to encode them. These
    // constants are the same ones used by the terrain routines.
    model->set_negative_altitude_exponent_bias(kNegativeElevationExponentBias);
    model->set_compressed_negative_altitude_threshold(
        kCompressedNegativeAltitudeThreshold);
  }

  WriteProtobuf(output_format);
}
