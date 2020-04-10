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


#include <qspinbox.h>
#include <qlabel.h>
#include <khTileAddr.h>
#include <autoingest/Misc.h>
#include <gstMisc.h>

#include "Preferences.h"
#include "RasterLayerProperties.h"

RasterLayerProperties::RasterLayerProperties( QWidget* parent, const InsetStackItem &cfg, AssetDefs::Type type )
    : RasterLayerPropertiesBase( parent, 0, false, 0 )
{
  unsigned int startlevel = 0;
  if ( type == AssetDefs::Imagery ) {
    fromPixel = ProductToImageryLevel;
    toPixel = ImageryToProductLevel;
    startlevel = StartImageryLevel;
  } else {
    fromPixel = ProductToTmeshLevel;
    toPixel = TmeshToProductLevel;
    startlevel = StartTmeshLevel;
  }

  assetNameLabel->setText( cfg.dataAsset.c_str() );

  // in the InsetStackItem:
  // 'maxlevel' is the level to use for the blend (not the inset's max)
  // 'override' max is the level we want to use regardless of the
  //   inset's max level

  unsigned int insetmin = 0;
  unsigned int insetmax = 0;
  GetMinMaxLevels(cfg.dataAsset, insetmin, insetmax);

  maxLevelLabel->setText( QString( "%1" ).arg( fromPixel( insetmax ) ) );
  if ( cfg.overridemax == 0 )
    overrideMaxSpin->setValue( fromPixel( insetmax ) );
  else
    overrideMaxSpin->setValue( fromPixel( cfg.overridemax ) );
  overrideMaxSpin->setMinValue( std::max<int>( startlevel, fromPixel( insetmin ) ) );
  overrideMaxSpin->setMaxValue( fromPixel( insetmax ) );

#if 0
  peerGroupSpin->setValue( cfg.peergroup );

  if ( Preferences::ExpertMode ) {
    peerGroupSpin->show();
    peerGroupLabel->show();
  } else {
    peerGroupSpin->hide();
    peerGroupLabel->hide();
  }
#endif

}

InsetStackItem RasterLayerProperties::getConfig() const
{
  InsetStackItem cfg;

  // in the InsetStackItem:
  // 'maxlevel' is the level to use for the blend (not the inset's max)
  // 'override' max is the level we want to use regardless of the
  //   inset's max level
  // GetInsetLevels() extracts the inset's min/max level from the inset

  cfg.dataAsset = assetNameLabel->text().latin1();

  unsigned int insetmin = 0;
  unsigned int insetmax = 0;
  GetMinMaxLevels(cfg.dataAsset, insetmin, insetmax);

  cfg.overridemax = toPixel( overrideMaxSpin->value() );
  if ( cfg.overridemax == insetmax )
    cfg.overridemax = 0;

  //cfg.peergroup = peerGroupSpin->value();

  cfg.maxlevel = cfg.overridemax;
  if (!cfg.maxlevel) {
    cfg.maxlevel = insetmax;
  } else if (cfg.maxlevel > insetmax) {
    cfg.maxlevel = insetmax;
  } else if (cfg.maxlevel < insetmin) {
    cfg.maxlevel = insetmin;
  }

  return cfg;
}

