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


#include <qframe.h>
#include <qlayout.h>
#include <qdatetimeedit.h>
#include <qpopupmenu.h>
#include <qlabel.h>
#include <qmessagebox.h>

#include "AssetProperties.h"
#include "AssetVersionProperties.h"
#include "AssetManager.h"
#include "AssetVersionActions.h"
#include <autoingest/AssetVersion.h>
#include <autoingest/khAssetManagerProxy.h>


#define COL_VER 0
#define COL_CREATETIME 1
#define COL_STATE 2
AssetVersionItem::AssetVersionItem( QListView *parent, const AssetVersion &ver )
    : QListViewItem( parent, QString( "%1" ).arg( ver->version ),
                     ver->meta.GetValue("createdtime"),
                     ver->PrettyState() ),
      AssetWatcher(ver->GetRef())
{
}

int AssetVersionItem::compare( QListViewItem *item, int, bool ) const
{
  return atoi( text( 0 ).latin1() ) - atoi( item->text( 0 ).latin1() );
}

void AssetVersionItem::paintCell( QPainter *p, const QColorGroup &cg,
                                  int column, int width, int alignment )
{
  QColorGroup ncg = cg;

  if ( column == 2 )
    ncg = AssetManager::GetStateDrawStyle( text( column ), p, cg );

  QListViewItem::paintCell( p, ncg, column, width, alignment );
}

void
AssetVersionItem::changed(void)
{
  AssetVersion ver(ref);
  setText( COL_STATE, ver->PrettyState() );
}

// ------------------------------------------------------------------------

AssetProperties::AssetProperties( QWidget* parent, const gstAssetHandle &handle )
    : AssetPropertiesBase( parent, 0, false, Qt::WDestructiveClose ), assetHandle( handle )
{
  versionsList->setSorting( 0, false );
  //aquisitionDate->setSeparator( "-" );

  Asset asset = handle->getAsset();
  nameLabel->setText( shortAssetName( handle->getName() ) );
  typeLabel->setText( ToString( asset->type ) );
  subTypeLabel->setText( asset->PrettySubtype() );

  connect( versionsList, SIGNAL( contextMenuRequested( QListViewItem *, const QPoint &, int ) ),
           this, SLOT( rmbClicked( QListViewItem *, const QPoint &, int ) ) );


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


void AssetProperties::selectVersion( QListViewItem *item )
{
  if ( item ) {
    AssetVersionItem *ver = ( AssetVersionItem* )item;
    AssetVersionProperties::Open(ver->getVersionRef());
  }
}


void
AssetProperties::rmbClicked( QListViewItem *item,
                             const QPoint &pos, int )
{
  AssetVersionItem *verItem = dynamic_cast<AssetVersionItem*>(item);
  if (!verItem) return;

  enum { VERSION_PROPERTIES = 0 };
  AssetVersionActions actions(this, verItem->ref);
  QPopupMenu menu( this );
  if (actions.Contribute(&menu))
    menu.insertSeparator();
  menu.insertItem( tr( "Version Properties" ), VERSION_PROPERTIES );

  int selection = menu.exec(pos);
  switch (selection) {
    case VERSION_PROPERTIES:
      AssetVersionProperties::Open(verItem->ref);
      break;
    default:
      actions.handle(selection);
  }
}
