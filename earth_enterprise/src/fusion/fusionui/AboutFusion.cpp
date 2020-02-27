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


#include <qlabel.h>

#include <builddate.h>
#include <fusionversion.h>
#include "GfxView.h"

#include "AboutFusion.h"

AboutFusion::AboutFusion(QWidget* parent)
    : AboutFusionBase(parent, 0, false, 0) {
  setCaption(GetFusionProductName().c_str());

  fusionNameLabel->setText(GetFusionProductName().c_str());
  versionLabel->setText(QString("%1").arg(GEE_LONG_VERSION));

  buildDateLabel->setText(QString("%1").arg(BUILD_DATE));

  maxTextureLabel->setText(GfxView::MaxTexSize);

  bitsPerCompLabel->setText(GfxView::BitsPerComp);
}
