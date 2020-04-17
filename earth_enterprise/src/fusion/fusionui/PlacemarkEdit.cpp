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


#include <Qt/qlineedit.h>
#include <Qt/qspinbox.h>
#include <Qt/qpushbutton.h>

#include "PlacemarkEdit.h"
#include "khException.h"


PlacemarkEdit::PlacemarkEdit( QWidget* parent, const gstPlacemark &pm )
    : PlacemarkEditBase( parent, 0, false, 0 )
{
  if ( !pm.name.isEmpty() ) {
    nameEdit->setText( pm.name );
    latitudeEdit->setText( QString( "%1" ).arg( pm.latitude, 0, 'f', 8 ) );
    longitudeEdit->setText( QString( "%1" ).arg( pm.longitude, 0, 'f', 8 ) );
    levelEdit->setText( QString( "%1" ).arg( pm.level ) );
    okBtn->setEnabled( true );
  } else {
    nameEdit->clear();
    latitudeEdit->setText( "0" );
    longitudeEdit->setText( "0" );
    levelEdit->setText( "0" );
    okBtn->setEnabled( false );
  }
}

gstPlacemark PlacemarkEdit::getPlacemark() const
{
  gstPlacemark placemark;

  placemark.name = nameEdit->text();
  placemark.latitude = latitudeEdit->text().toDouble();
  placemark.longitude = longitudeEdit->text().toDouble();
  placemark.level = levelEdit->text().toDouble();

  return placemark;
}

void PlacemarkEdit::nameChanged( const QString &txt )
{
  bool m = !txt.isEmpty();
  okBtn->setEnabled( m );
  okBtn->setDefault( m );
}

