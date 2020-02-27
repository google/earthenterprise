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

#include <Qt/qlabel.h>
#include <Qt/q3popupmenu.h>
#include <Qt/q3dragobject.h>
#include <Qt/qfont.h>
#include <Qt/qmessagebox.h>
#include <autoingest/AssetVersion.h>
#include <autoingest/khAssetManagerProxy.h>
#include <Qt/qpalette.h>

#include "AssetManager.h"
#include "AssetVersionProperties.h"
#include "AssetLog.h"
#include <Qt/q3mimefactory.h>

using QMimeSourceFactory = Q3MimeSourceFactory;
using QPopupWindow = Q3PopupMenu;
using QImageDrag = Q3ImageDrag;

static QPixmap uic_load_pixmap( const QString &name )
{
  const QMimeSource *m = QMimeSourceFactory::defaultFactory()->data( name );
  if ( !m )
    return QPixmap();
  QPixmap pix;
  QImageDrag::decode( m, pix );
  return pix;
}

// -----------------------------------------------------------------------------

#define COL_SUBTYPE 0
#define COL_STATE 1
#define COL_LOG 2
#define COL_REF 3

AssetChildItem::AssetChildItem( Q3ListView *parent, const AssetVersion &ver)
    : Q3ListViewItem( parent ), AssetWatcher(ver->GetRef())
{
  configureWidgets(ver);
}

AssetChildItem::AssetChildItem( Q3ListViewItem *parent, const AssetVersion &ver, const std::string &msg )
    : Q3ListViewItem( parent ), AssetWatcher(ver->GetRef())
{
  configureWidgets(ver, msg);
}

void
AssetChildItem::configureWidgets(const AssetVersion &ver,
                                 const std::string &msg)
{
  std::string tmpMsg(msg);
  tmpMsg += ver->PrettySubtype();
  setText( COL_SUBTYPE, tmpMsg.c_str());
  setText( COL_STATE, ver->PrettyState().c_str()) ;
  if ( ver->Logfile().size() != 0 )
    setPixmap( COL_LOG, uic_load_pixmap( "history.png" ) );
  setText( COL_REF, ver->GetRef().toString().c_str() );

  setExpandable(ver->inputs.size() ||
                (!ver->IsLeaf() && ver->children.size()));
}


void
AssetChildItem::changed(void)
{
  AssetVersion ver(ref);
  setText( COL_STATE, ver->PrettyState().c_str() );
}

void
AssetChildItem::setOpen(bool open)
{
  if (open) {
    // add my children
    AssetVersion ver(ref);
    if ( !ver->IsLeaf() ) {
      for ( auto it =
              ver->children.begin();
            it != ver->children.end(); ++it ) {
        AssetVersion child(*it);
        // AssetChildItem *item =
        new AssetChildItem( this, child, "CHILD: " );
        //              item->setOpen( false );
      }
    }

    for ( auto it = ver->inputs.begin();
          it != ver->inputs.end(); ++it ) {
      AssetVersion input(*it);
      // AssetChildItem *item =
      new AssetChildItem( this, input, "INPUT: " );
      //      item->setOpen( false );
    }


    // open second
    Q3ListViewItem::setOpen(open);
  } else {
    // close first
    Q3ListViewItem::setOpen(open);

    // delete all my children
    Q3ListViewItem *tokill;
    while ((tokill = firstChild())) {
      delete tokill;
    }
  }
}

void AssetChildItem::paintCell( QPainter *p, const QColorGroup &cg, int col, int width, int align )
{
  QColorGroup ngrp = cg;

  if ( col == COL_STATE )
    ngrp = AssetManager::GetStateDrawStyle( text( col ).toUtf8().constData(), p, cg );

  Q3ListViewItem::paintCell( p, ngrp, col, width, align );
}

std::string
AssetChildItem::getVersionRef() const
{
  return ref;
}


// ---------------------------------------------------------------------------------------------------

AssetVersionProperties::VerPropMap AssetVersionProperties::openverprops;

AssetVersionProperties::AssetVersionProperties( const std::string &verref_ )
    : AssetVersionPropertiesBase( 0, 0, false, Qt::WDestructiveClose ), verref( verref_ )
{
  depListView->setColumnWidthMode( 0, Q3ListView::Maximum );
  depListView->setColumnWidthMode( 1, Q3ListView::Maximum );
  depListView->setSorting( -1 );
  depListView->setRootIsDecorated( true );

  connect( depListView, SIGNAL( contextMenuRequested( Q3ListViewItem *, const QPoint &, int ) ),
           this, SLOT( rmbClicked( Q3ListViewItem *, const QPoint &, int ) ) );

  openverprops.insert(std::make_pair(verref, this));

  refresh();
  show();
}

AssetVersionProperties::~AssetVersionProperties()
{
  openverprops.erase(verref);
}

void AssetVersionProperties::refresh()
{
  depListView->clear();


  AssetVersion ver(verref);
  Q3ListViewItem *item = new AssetChildItem( depListView, ver );

  // automatically open the first level
  item->setOpen( true );

  // auto size the window?
  for ( int col = 0; col < depListView->columns(); col++ ) {
    int w = depListView->columnWidth( col );
    depListView->setColumnWidth( col, w + 20 );
  }

}

void AssetVersionProperties::clicked( Q3ListViewItem *item, const QPoint & pos, int col )
{
  if ( item == NULL || col != COL_LOG )
    return;

  AssetChildItem *child = ( AssetChildItem* )item;
  std::string logfile = AssetVersion(child->getVersionRef())->Logfile();

  if ( !logfile.empty() ) {
    AssetLog::Open(logfile);
  }
}

void
AssetVersionProperties::Open(const std::string &verref)
{
  VerPropMap::const_iterator found = openverprops.find(verref);
  if (found != openverprops.end()) {
    found->second->showNormal();
    found->second->raise();
  } else {
    (void)new AssetVersionProperties(verref);
  }
}

void
AssetVersionProperties::rmbClicked( Q3ListViewItem *item,
                                    const QPoint &pos, int )
{
  AssetChildItem *childItem = dynamic_cast<AssetChildItem*>(item);
  if (!childItem) return;

  AssetVersionActions actions(this, childItem->getVersionRef());
  QPopupMenu menu( this );
  if (actions.Contribute(&menu)) {
    int item = menu.exec(pos);
    actions.handle(item);
  }
}
