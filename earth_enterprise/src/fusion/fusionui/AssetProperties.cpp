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

#include <cstdlib>
#include <Qt/qframe.h>
#include <Qt/qlayout.h>
#include <Qt/qdatetimeedit.h>
#include <Qt/qlabel.h>
#include <Qt/qmessagebox.h>

#include "AssetProperties.h"
#include "AssetVersionProperties.h"
#include "AssetManager.h"
#include "AssetVersionActions.h"
#include <autoingest/AssetVersion.h>
#include <autoingest/khAssetManagerProxy.h>

#define COL_VER 0
#define COL_CREATETIME 1
#define COL_STATE 2
AssetVersionItem::AssetVersionItem( Q3ListView *parent, const AssetVersion &ver )
    : Q3ListViewItem( parent, QString( "%1" ).arg( ver->version ),
                     ver->meta.GetValue("createdtime"),
                     ver->PrettyState().c_str() ),
      AssetWatcher(ver->GetRef().toString())
{
}

int AssetVersionItem::compare( Q3ListViewItem *item, int, bool ) const
{
  int lhs = std::stoi(std::string(text(0).toUtf8().constData()));
  int rhs = std::stoi(std::string(item->text(0).toUtf8().constData()));
  return lhs - rhs;
}

void AssetVersionItem::paintCell( QPainter *p, const QColorGroup &cg,
                                  int column, int width, int alignment )
{
  QColorGroup ncg = cg;

  if ( column == 2 )
    ncg = AssetManager::GetStateDrawStyle(text(column).toUtf8().constData(), p, cg);

  Q3ListViewItem::paintCell( p, ncg, column, width, alignment );
}

void
AssetVersionItem::changed(void)
{
  AssetVersion ver(ref);
  setText( COL_STATE, ver->PrettyState().c_str() );
}

// ------------------------------------------------------------------------

AssetProperties::AssetProperties( QWidget* parent, const gstAssetHandle &handle )
    : AssetPropertiesBase( parent, 0, false, Qt::WDestructiveClose ), assetHandle( handle )
{
  versionsList->setSorting( 0, false );

  Asset asset = handle->getAsset();
  std::string san = shortAssetName( handle->getName());
  nameLabel->setText( san.c_str() );
  typeLabel->setText( ToString( asset->type ).c_str() );
  subTypeLabel->setText( asset->PrettySubtype().c_str() );

  connect( versionsList, SIGNAL( contextMenuRequested( Q3ListViewItem *, const QPoint &, int ) ),
           this, SLOT( rmbClicked( Q3ListViewItem *, const QPoint &, int ) ) );


  refresh();
}

AssetProperties::~AssetProperties()
{
}

void AssetProperties::refresh()
{
  versionsList->clear();

  Asset asset = assetHandle->getAsset();
  for (AssetStorage::VersionList::const_iterator it = asset->versions.begin();
       it != asset->versions.end(); ++it ) {
    new AssetVersionItem( versionsList, *it );
  }

  for ( int col = 0; col < versionsList->columns(); col++ ) {
    versionsList->adjustColumn( col );
    int w = versionsList->columnWidth( col );
    versionsList->setColumnWidth( col, w + 10 );
  }

}


void AssetProperties::selectVersion( Q3ListViewItem *item )
{
  if ( item ) {
    AssetVersionItem *ver = ( AssetVersionItem* )item;
    AssetVersionProperties::Open(ver->getVersionRef());
  }
}


void
AssetProperties::rmbClicked( Q3ListViewItem *item,
                             const QPoint &pos, int )
{
  AssetVersionItem *verItem = dynamic_cast<AssetVersionItem*>(item);
  if (!verItem) return;

  enum { VERSION_PROPERTIES = 0 };
  AssetVersionActions actions(this, verItem->ref);
  QPopupMenu menu( this );
  if (actions.Contribute(&menu))
    menu.insertSeparator();
  menu.insertItem( QObject::tr( "Version Properties" ), VERSION_PROPERTIES );

  int selection = menu.exec(pos);
  switch (selection) {
    case VERSION_PROPERTIES:
      AssetVersionProperties::Open(verItem->ref);
      break;
    default:
      actions.handle(selection);
  }
}
