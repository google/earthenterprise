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


#include "fusion/gst/gstSource.h"

#include <qfile.h>
#include <qtextcodec.h>
#include <qtextstream.h>

#include "common/khException.h"
#include "common/khFileUtils.h"
#include "common/notify.h"
#include "fusion/gst/gstRecord.h"
#include "fusion/gst/gstSpatialRef.h"


gstSource::gstSource(const QString &fname)
    : gstMemory(fname.latin1()),
      opened_(false),
      format_(NULL),
      status_(GST_UNKNOWN),
      no_file_(false) {
}

gstSource::~gstSource() {
  Close();
}

void gstSource::SetEnabled(bool enabled) {
  if (format_)
    format_->setEnabled(enabled);
}

void gstSource::DefineSrs(const QString &wkt, bool save_prj) {
  if (format_ && !wkt.isEmpty()) {
    const std::string wktbuf(wkt.latin1());
    OGRSpatialReference srs(wktbuf.c_str());
    if (srs.Validate() == OGRERR_NONE) {
      format_->SetSpatialRef(new gstSpatialRef(&srs));
      if (save_prj)
        SavePrj(wkt);
    } else {
      notify(NFY_WARN, "Spatial Reference failed! wkt=%s", wktbuf.c_str());
    }
  } else {
    notify(NFY_WARN, "Unable to define Spatial Reference! wkt=%s",
           wkt.latin1());
  }
}

gstStatus gstSource::Open() {
  if (Opened())
    return NoFile() ? GST_OKAY : status_;

  format_ = gstFormat::Open(name());

  if (format_ == NULL)
    return (status_ = GST_OPEN_FAIL);

  layer_defs_.clear();

  opened_ = true;

  return GST_OKAY;
}


void gstSource::Close() {
  if (!opened_)
    return;

  opened_ = false;

  delete format_;
  format_ = NULL;

  layer_defs_.clear();
}

void gstSource::SavePrj(const QString& wkt) {
  std::string prjname = khReplaceExtension(name(), ".prj");
  QFile prjfile(prjname.c_str());
  if (prjfile.open(IO_WriteOnly)) {
    QTextStream stream(&prjfile);
    stream << wkt << "\n";
    prjfile.close();
  } else {
    notify(NFY_WARN, "Unable to open prj file: %s", prjname.c_str());
  }
}

std::vector<gstLayerDef>& gstSource::LayerDefs() const {
  if (layer_defs_.size() == 0) {
    layer_defs_ = format_->GetNormLayerDefs();
  }

  return layer_defs_;
}


bool gstSource::SetCodec(const QString& codec) {
  if (!opened_) {
    notify(NFY_WARN, "Source not opened, cannot apply codec");
    return false;
  }

  QTextCodec* qcodec = QTextCodec::codecForName(codec.latin1());
  if (qcodec == NULL) {
    notify(NFY_WARN, "Unable to find codec named %s", codec.latin1());
    return false;
  }

  format_->SetCodec(qcodec);

  return true;
}


 std::uint32_t gstSource::NumLayers() const {
  return static_cast<std::uint32_t>(LayerDefs().size());
}

 std::uint32_t gstSource::NumFeatures(std::uint32_t layer) const {
  if (!Opened()) {
    notify(NFY_WARN, "source must be opened first!");
    return 0;
  }

  if (layer >= NumLayers()) {
    notify(NFY_WARN, "asking for unavailable layer id %u ( max = %u )",
           layer, NumLayers() - 1);
    return 0;
  }

  return LayerDefs()[layer].NumFeatures();
}

double gstSource::AverageFeatureDiameter(std::uint32_t layer) const {
  if (!Opened()) {
    notify(NFY_WARN, "source must be opened first!");
    return 0;
  }

  if (layer >= NumLayers()) {
    notify(NFY_WARN, "asking for unavailable layer id %u ( max = %u )",
           layer, NumLayers() - 1);
    return 0;
  }
  return LayerDefs()[layer].AverageFeatureDiameter();
}

 std::uint32_t gstSource::RecommendedMaxResolutionLevel() const {
  if (!Opened()) {
    notify(NFY_WARN, "source must be opened first!");
    return 0;
  }
  std::uint32_t maxResolutionLevel = 0;
  for (std::uint32_t i = 0; i < NumLayers(); i++) {
    maxResolutionLevel = std::max(
      maxResolutionLevel, LayerDefs()[i].MaxResolutionLevel());
  }
  return maxResolutionLevel;
}

 std::uint32_t gstSource::RecommendedMinResolutionLevel() const {
  if (!Opened()) {
    notify(NFY_WARN, "source must be opened first!");
    return 0;
  }
  std::uint32_t minResolutionLevel = 0;
  for (std::uint32_t i = 0; i < NumLayers(); i++) {
    minResolutionLevel = std::min(
      minResolutionLevel, LayerDefs()[i].MinResolutionLevel());
  }
  return minResolutionLevel;
}

 std::uint32_t gstSource::RecommendedEfficientResolutionLevel() const {
  if (!Opened()) {
    notify(NFY_WARN, "source must be opened first!");
    return 0;
  }
  if (!NumLayers()) {
    return 0;
  }
  std::uint32_t efficientResolutionLevel = 0;
  for (std::uint32_t i = 0; i < NumLayers(); i++) {
    efficientResolutionLevel += LayerDefs()[i].EfficientResolutionLevel();
  }
  return static_cast<std::uint32_t>(static_cast<double>(efficientResolutionLevel)
                             / NumLayers() + 0.5);
}


#if 0
gstBBox gstSource::BoundingBox() {
  gstBBox all;
  for (unsigned int layer = 0; layer < NumLayers(); ++layer)
    all.Grow(BoundingBox(layer));
  return all;
}
#endif


gstBBox gstSource::BoundingBox(std::uint32_t layer) {
  if (!Opened()) {
    notify(NFY_WARN, "can't retrieve box from un-opened source %s", name());
    return gstBBox::empty;
  }

  if (layer >= NumLayers()) {
    notify(NFY_WARN, "gstSource::bbox() illegal call!\n");
    return gstBBox::empty;
  }

  //
  // usually the format will provide the bbox
  // but if not, calculate it right now
  //
  gstLayerDef& ldef = LayerDefs()[layer];
  if (!ldef.BoundingBox().Valid()) {
    gstBBox newbox;
    try {
      format_->ResetReadingImpl(layer);
      for (; !format_->IsReadingDoneImpl(); format_->IncrementReadingImpl()) {
        try {
          gstGeodeHandle geode = format_->GetNormCurrFeatureImpl();
          if (geode && !geode->IsEmpty()) {
            newbox.Grow(geode->BoundingBox());
          } else {
            // silently ignore missing/empty geodes that didn't throw
            // this routine is only used for "Snap To Layer" in GUI. We just
            // need to be close. If they turn on the layer they will see the
            // full errors from the filter.
          }
        } catch(const khSoftException &e) {
          // silently ignore missing/empty geodes that didn't throw
          // this routine is only used for "Snap To Layer" in GUI. We just
          // need to be close. If they turn on the layer they will see the
          // full errors from the filter.
        }
      }
      ldef.SetBoundingBox(newbox);
    } catch(const std::exception &e) {
      notify(NFY_WARN,
             "Unable to fetch features to calculate bounding box:\n%s",
             e.what());
      return gstBBox::empty;
    } catch(...) {
      notify(NFY_WARN,
             "Unable to fetch features to calculate bounding box:\n%s",
             "Unknown error");
      return gstBBox::empty;
    }
  }

  return ldef.BoundingBox();
}

void gstSource::ResetReadingOrThrow(std::uint32_t layer) {
  if (!Opened()) {
    throw khException(kh::tr("Source not open: %1").arg(name()));
  } else if (layer >= NumLayers()) {
    throw khException(kh::tr("Invalid layer (%1) for %2")
                      .arg(layer).arg(name()));
  }

  try {
    format_->ResetReadingImpl(layer);
  } catch(const khSoftException &e) {
    throw khSoftException(kh::tr("Cannot reset reading for: %1\n"
                                 "error: %2")
                          .arg(name()).arg(e.qwhat()));
  } catch(const khException &e) {
    throw khException(kh::tr("Cannot reset reading for: %1\n"
                             "error: %2")
                      .arg(name()).arg(e.qwhat()));
  } catch(const std::exception &e) {
    throw khException(kh::tr("Cannot reset reading for: %1\n"
                             "error: %2")
                      .arg(name()).arg(e.what()));
  } catch(...) {
    throw khException(kh::tr("Cannot reset reading for: %1\n"
                             "error: %2")
                      .arg(name()).arg("Unknown error"));
  }
}

gstGeodeHandle gstSource::GetNormCurrFeatureOrThrow(void) {
  if (!Opened()) {
    throw khException(kh::tr("Source not open: %1").arg(name()));
  }

  try {
    gstGeodeHandle new_geode = format_->GetNormCurrFeatureImpl();
    if (!new_geode) {
      throw khSoftException("Unable to get feature");
    }
    if (new_geode->IsEmpty()) {
      throw khSoftException(kh::tr("Empty feature"));
    }
    // features must fit entirely within the valid domain
    static const gstBBox kMaxDomain(0.0, 1.0, 0.25, 0.75);
    const gstBBox& box = new_geode->BoundingBox();
    if (!kMaxDomain.Contains(box)) {
      throw khSoftException(QString().sprintf(
                "Feature has invalid extents"
                "(n:%6.6lg s:%6.6lg w:%6.6lg e:%6.6lg)",
                khTilespace::Denormalize(box.n),
                khTilespace::Denormalize(box.s),
                khTilespace::Denormalize(box.w),
                khTilespace::Denormalize(box.e)));
    }
    return new_geode;
  } catch(const khSoftException &e) {
    throw khSoftException(kh::tr("Cannot get current feature for: %1\n"
                                 "    Error: %2")
                          .arg(name()).arg(e.qwhat()));
  } catch(const khException &e) {
    throw khException(kh::tr("Cannot get current feature for: %1\n"
                             "    Error: %2")
                      .arg(name()).arg(e.qwhat()));
  } catch(const std::exception &e) {
    throw khException(kh::tr("Cannot get current feature for: %1\n"
                             "    Error: %2")
                      .arg(name()).arg(e.what()));
  } catch(...) {
    throw khException(kh::tr("Cannot get current feature for: %1\n"
                             "    Error: %2")
                      .arg(name()).arg("Unknown error"));
  }

  // unreached, but silences warnings
  return gstGeodeHandle();
}

gstRecordHandle gstSource::GetCurrentAttributeOrThrow(void) {
  if (!Opened()) {
    throw khException(kh::tr("Source not open: %1").arg(name()));
  }

  try {
    gstRecordHandle new_record = format_->GetCurrentAttributeImpl();
    if (!new_record) {
      throw khSoftException("Unable to get attribute");
    }
    return new_record;
  } catch(const khSoftException &e) {
    throw khSoftException(kh::tr("Cannot get current attribute for: %1\n"
                                 "error: %2")
                          .arg(name()).arg(e.qwhat()));
  } catch(const khException &e) {
    throw khException(kh::tr("Cannot get current attribute for: %1\n"
                             "error: %2")
                      .arg(name()).arg(e.qwhat()));
  } catch(const std::exception &e) {
    throw khException(kh::tr("Cannot get current attribute for: %1\n"
                             "error: %2")
                      .arg(name()).arg(e.what()));
  } catch(...) {
    throw khException(kh::tr("Cannot get current attribute for: %1\n"
                             "error: %2")
                      .arg(name()).arg("Unknown error"));
  }

  // unreached, but silences warnings
  return gstRecordHandle();
}

void gstSource::IncrementReadingOrThrow(void) {
  if (!Opened()) {
    throw khException(kh::tr("Source not open: %1").arg(name()));
  }

  try {
    format_->IncrementReadingImpl();
  } catch(const khSoftException &e) {
    throw khSoftException(kh::tr("Cannot get next feature for: %1\n"
                                 "error: %2")
                          .arg(name()).arg(e.qwhat()));
  } catch(const khException &e) {
    throw khException(kh::tr("Cannot get next feature for: %1\n"
                             "error: %2")
                      .arg(name()).arg(e.qwhat()));
  } catch(const std::exception &e) {
    throw khException(kh::tr("Cannot get next feature for: %1\n"
                             "error: %2")
                      .arg(name()).arg(e.what()));
  } catch(...) {
    throw khException(kh::tr("Cannot get next feature for: %1\n"
                             "error: %2")
                      .arg(name()).arg("Unknown error"));
  }
}

bool gstSource::IsReadingDone(void) {
  return format_->IsReadingDoneImpl();
}


gstGeodeHandle gstSource::GetFeatureOrThrow(std::uint32_t layer, std::uint32_t id,
                                            bool is_mercator_preview) {
  if (!Opened()) {
    throw khException(kh::tr("Source not open: %1").arg(name()));
  } else if (layer >= NumLayers()) {
    throw khException(kh::tr("Invalid layer (%1) for %2")
                      .arg(layer).arg(name()));
  } else if (id >= LayerDefs()[layer].NumFeatures()) {
    throw khException(kh::tr("Invalid feature id (%1) for layer %2 in %3")
                      .arg(id).arg(layer).arg(name()));
  }

  try {
    gstGeodeHandle new_geode = format_->GetNormFeatureImpl(layer, id,
                                                           is_mercator_preview);
    if (!new_geode) {
      throw khSoftException("Unable to get");
    }
    if (new_geode->IsEmpty()) {
      throw khSoftException(kh::tr("Empty feature"));
    }

    return new_geode;
  } catch(const khSoftException &e) {
    throw khSoftException(kh::tr("Cannot get feature: %4\n"
                                 "id: %1, layer: %2, source: %3")
                          .arg(id).arg(layer).arg(name()).arg(e.qwhat()));
  } catch(const khException &e) {
    throw khException(kh::tr("Cannot get feature: %4\n"
                             "id: %1, layer: %2, source: %3")
                      .arg(id).arg(layer).arg(name()).arg(e.qwhat()));
  } catch(const std::exception &e) {
    throw khException(kh::tr("Cannot get feature: %4\n"
                             "id: %1, layer: %2, source: %3")
                      .arg(id).arg(layer).arg(name()).arg(e.what()));
  } catch(...) {
    throw khException(kh::tr("Cannot get feature: %4\n"
                             "id: %1, layer: %2, source: %3")
                      .arg(id).arg(layer).arg(name()).arg("Unknown error"));
  }

  // not reached, but silences warnings
  return gstGeodeHandle();
}

gstBBox gstSource::GetFeatureBoxOrThrow(std::uint32_t layer, std::uint32_t id) {
  if (!Opened()) {
    throw khException(kh::tr("Source not open: %1").arg(name()));
  } else if (layer >= NumLayers()) {
    throw khException(kh::tr("Invalid layer (%1) for %2")
                      .arg(layer).arg(name()));
  } else if (id >= LayerDefs()[layer].NumFeatures()) {
    throw khException(kh::tr("Invalid feature id (%1) for layer %2 in %3")
                      .arg(id).arg(layer).arg(name()));
  }

  try {
    return format_->GetFeatureBoxImpl(layer, id);
  } catch(const khSoftException &e) {
    throw khSoftException(kh::tr("Cannot get bounding box: %4\n"
                                 "id: %1, layer: %2, source: %3")
                          .arg(id).arg(layer).arg(name()).arg(e.qwhat()));
  } catch(const khException &e) {
    throw khException(kh::tr("Cannot get bounding box: %4\n"
                             "id: %1, layer: %2, source: %3")
                      .arg(id).arg(layer).arg(name()).arg(e.qwhat()));
  } catch(const std::exception &e) {
    throw khException(kh::tr("Cannot get bounding box: %4\n"
                             "id: %1, layer: %2, source: %3")
                      .arg(id).arg(layer).arg(name()).arg(e.what()));
  } catch(...) {
    throw khException(kh::tr("Cannot get bounding box: %4\n"
                             "id: %1, layer: %2, source: %3")
                      .arg(id).arg(layer).arg(name()).arg("Unknown error"));
  }

  // not reached, but silences warnings
  return gstBBox();
}

gstRecordHandle gstSource::GetAttributeOrThrow(std::uint32_t layer, std::uint32_t id) {
  if (!Opened()) {
    throw khException(kh::tr("Source not open: %1").arg(name()));
  } else if (layer >= NumLayers()) {
    throw khException(kh::tr("Invalid layer (%1) for %2")
                      .arg(layer).arg(name()));
  } else if (id >= LayerDefs()[layer].NumFeatures()) {
    throw khException(kh::tr("Invalid feature id (%1) for layer %2 in %3")
                      .arg(id).arg(layer).arg(name()));
  } else if (LayerDefs()[layer].AttrDefs()->numColumns() == 0) {
    throw khException(kh::tr("No attributes available for layer %1 in %2")
                      .arg(layer).arg(name()));
  }

  try {
    gstRecordHandle new_record = format_->GetAttributeImpl(layer, id);
    if (!new_record) {
      throw khSoftException("Unable to get attribute");
    }
    return new_record;
  } catch(const khSoftException &e) {
    throw khSoftException(kh::tr("Cannot get attribute: %4\n"
                                 "id: %1, layer: %2, source: %3")
                          .arg(id).arg(layer).arg(name()).arg(e.qwhat()));
  } catch(const khException &e) {
    throw khException(kh::tr("Cannot get attribute: %4\n"
                             "id: %1, layer: %2, source: %3")
                      .arg(id).arg(layer).arg(name()).arg(e.qwhat()));
  } catch(const std::exception &e) {
    throw khException(kh::tr("Cannot get attribute: %4\n"
                             "id: %1, layer: %2, source: %3")
                      .arg(id).arg(layer).arg(name()).arg(e.what()));
  } catch(...) {
    throw khException(kh::tr("Cannot get attribute: %4\n"
                             "id: %1, layer: %2, source: %3")
                      .arg(id).arg(layer).arg(name()).arg("Unknown error"));
  }

  // not reached, but silences warnings
  return gstRecordHandle();
}

const gstHeaderHandle& gstSource::GetAttrDefs(std::uint32_t layer) const {
  return LayerDefs()[layer].AttrDefs();
}

gstPrimType gstSource::GetPrimType(std::uint32_t layer) const {
  return LayerDefs()[layer].Type();
}
