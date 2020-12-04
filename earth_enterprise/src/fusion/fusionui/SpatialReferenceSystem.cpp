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

#include <Qt/qobjectdefs.h>
#include <Qt/qcombobox.h>
#include <Qt/q3listbox.h>
#include <Qt/qtextedit.h>
#include <Qt/qdialog.h>

#include "Preferences.h"
#include "SpatialReferenceSystem.h"
#include "SRSDetails.h"

#include <ogr_spatialref.h>
#include <cpl_conv.h>
#include <cpl_string.h>
#include <Qt/qobjectdefs.h>
#include <gstSRSDefs.h>

SpatialReferenceSystem::SpatialReferenceSystem( QWidget *parent )
    : SpatialReferenceSystemBase( parent ), savePrj( false )
{
  for ( int c = 0; c < theSRSDefs.getCategoryCount(); ++c )
    categoriesCombo->insertItem( theSRSDefs.getCategoryName( c ).c_str() );

  categoriesCombo->setCurrentItem( 0 );
  selectCategory( 0 );
}

void SpatialReferenceSystem::selectCategory( int cat )
{
  projectionList->clear();

  for ( int p = 0; p < theSRSDefs.getProjectionCount( cat ); ++p )
    projectionList->insertItem( theSRSDefs.getProjectionName( cat, p ).c_str() );

  projectionList->setCurrentItem( 0 );
}

void SpatialReferenceSystem::selectProjection( int proj )
{
  wktText = theSRSDefs.getWKT( categoriesCombo->currentItem(), proj ).c_str();
}

QString SpatialReferenceSystem::getWKT() const
{
  return wktText;
}

bool SpatialReferenceSystem::getSavePrj() const
{
  return savePrj;
}

void SpatialReferenceSystem::advanced()
{
  SRSDetails details( this, wktText, savePrj );
  if ( details.exec() == QDialog::Accepted )
    savePrj = details.savePrj();
}
