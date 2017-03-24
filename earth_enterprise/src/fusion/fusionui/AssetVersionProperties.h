/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef _AssetVersionProperties_h_
#define _AssetVersionProperties_h_

#include <qlistview.h>
#include <string>
#include <map>
#include "assetversionpropertiesbase.h"
#include <autoingest/AssetVersion.h>
#include "SystemListener.h"
#include "AssetVersionActions.h"


#define ASSET_CHILD 0x00ff0001
#define ASSET_INPUT 0x00ff0002

class AssetChildItem : public QListViewItem, public AssetWatcher
{
 public:
  AssetChildItem( QListView *parent, const AssetVersion &ver);
  AssetChildItem( QListViewItem *parent, const AssetVersion &ver, const std::string &msg);

  void paintCell( QPainter *p, const QColorGroup &cg, int col, int width, int align );

  std::string getVersionRef() const;

  void configureWidgets(const AssetVersion &ver,
                        const std::string &msg = std::string());

  virtual void changed(void);
  virtual void setOpen(bool open);
 private:
};


// ------------------------------------------------------------------------

class AssetVersionProperties : public AssetVersionPropertiesBase
{
  Q_OBJECT

  AssetVersionProperties( const std::string &verref_ );
 public:
  ~AssetVersionProperties();
 public:

  void refresh();

  void clicked( QListViewItem *item, const QPoint & pos, int col );

  static void Open(const std::string &verref);
 private:
  void startLogViewer( const std::string &path );
  std::string verref;

  typedef std::map<std::string, AssetVersionProperties*> VerPropMap;
  static VerPropMap openverprops;
public slots:
void rmbClicked( QListViewItem *item, const QPoint &pos, int );
};

#endif // !_AssetVersionProperties_h_
