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


#include <ctype.h>
#include <gstSite.h>
#include <gstRecord.h>
#include <JSDisplayBundle.h>

gstSite::gstSite(const FuseSiteConfig& cfg,
                 const gstSitePreviewConfig &preview_cfg)
    : config(cfg),
      preview_config(preview_cfg) {
  offset_x_ = offset_y_ = 0.0;

  label_format_ = NULL;
  popup_text_format_ = NULL;
  custom_height_format_ = NULL;

  site_header_ = gstHeaderImpl::Create();
  site_header_->addSpec("label", gstTagUnicode);
  site_header_->addSpec("outline_label", gstTagUnicode);
  site_header_->addSpec("address", gstTagUnicode);
  site_header_->addSpec("customHeight", gstTagDouble);
}

gstSite::~gstSite() {
  delete label_format_;
  delete popup_text_format_;
  delete custom_height_format_;
}

gstVertex gstSite::Location(const gstGeodeHandle &feature) const {
  if (feature->IsDegenerate()) {
    notify(NFY_WARN,
           "Can't build a site without a feature to get coords from!");
    return gstVertex();
  }

  // find the vertex coordinate
  if (feature->PrimType() == gstPoint) {
    return feature->Center();
  } else {
    switch (config.position) {
      case VectorDefs::AreaCenter:
        {
          gstVertex center = feature->Center();
          if (center != gstVertex()) {
            return center;
          } else {
            return gstVertex(feature->BoundingBox().CenterX() + offset_x_,
                             feature->BoundingBox().CenterY() + offset_y_);
          }
        }
        break;

      case VectorDefs::LineCenter: {
        // TODO: feature can contain no one polyline here!
        // Where should be the line center in this case?
        return feature->Center();
      }
        break;
    }
  }

  return gstVertex();
}


gstRecordHandle gstSite::Expand(gstRecordHandle srcrec,
                                bool label_only,
                                JSDisplayBundle &jsbundle,
                                unsigned int filterId) const {
  gstHeaderHandle srchdr = srcrec->Header();
  gstRecordHandle new_rec = site_header_->NewRecord();
  bool setJSRec = false;

  if (label().length()) {
    QString outval;
    if (jsbundle.displayJS[filterId].labelScript) {
      jsbundle.cx->SetRecord(srcrec);
      setJSRec = true;
      QString error;
      if (!jsbundle.cx->TryExecuteScript
          (jsbundle.displayJS[filterId].labelScript, outval, error)) {
        notify(NFY_WARN, "Error executing JS: %s", (const char *)error.utf8());
        outval = QString();
        return gstRecordHandle();
      }
    } else {
      if (label_format_ == NULL)
        label_format_ = new gstRecordFormatter(label(), srchdr);
      outval = label_format_->out(srcrec);
    }
    if (!outval.isEmpty())
      new_rec->Field(0)->set(outval);
  }

  if (label_only)
    return new_rec;

  if (config.popupText.length()) {
    QString outval;
    if (jsbundle.displayJS[filterId].popupTextScript) {
      if (!setJSRec) {
        jsbundle.cx->SetRecord(srcrec);
        setJSRec = true;
      }
      QString error;
      if (!jsbundle.cx->TryExecuteScript
          (jsbundle.displayJS[filterId].popupTextScript, outval, error)) {
        notify(NFY_WARN, "Error executing JS: %s", (const char *)error.utf8());
        outval = QString();
        return gstRecordHandle();
      }
    } else {
      if (popup_text_format_ == NULL)
        popup_text_format_ = new gstRecordFormatter(config.popupText, srchdr);
      outval = popup_text_format_->out(srcrec);
    }
    if (!outval.isEmpty())
      new_rec->Field(1)->set(outval);
  }


  // get the custom height variable field
  if (config.style.altitudeMode != StyleConfig::ClampToGround &&
      config.style.enableCustomHeight &&
      config.style.customHeightVariableName.length()) {
    if (custom_height_format_ == NULL)
      custom_height_format_ =
        new gstRecordFormatter(config.style.customHeightVariableName,
                               srchdr);
    QString outval = custom_height_format_->out(srcrec);
    if (outval.isEmpty()) {
      new_rec->Field(2)->set(QString("0"));
    } else {
      new_rec->Field(2)->set(outval);
    }
  } else {
    new_rec->Field(2)->set(QString("0"));
  }

  return new_rec;
}
