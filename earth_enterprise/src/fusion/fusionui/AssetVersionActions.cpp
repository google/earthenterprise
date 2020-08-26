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


/******************************************************************************
File:        AssetVersionActions.cpp
Description: 

-------------------------------------------------------------------------------
For history see CVS log (cvs log AssetVersionActions.cpp -or- Emacs Ctrl-xvl).
******************************************************************************/
#include "AssetVersionActions.h"
#include <qmessagebox.h>
#include <autoingest/AssetVersion.h>
#include <autoingest/khAssetManagerProxy.h>


AssetVersionActions::AssetVersionActions(QWidget *parent_,
                                         const std::string &ref) :
    parent(parent_),
    verref(ref)
{
  connect(parent, SIGNAL(destroyed()), this, SLOT(parentDestroyed()));
}

void AssetVersionActions::parentDestroyed(void) {
  parent = 0;
}


void AssetVersionActions::handle(int item) {
  switch (item) {
    case REBUILD_ACTION:
      rebuild();
      break;
    case CANCEL_ACTION:
      cancel();
      break;
    case CLEAN_ACTION:
      clean();
      break;
    case SETBAD_ACTION:
      setbad();
      break;
    case CLEARBAD_ACTION:
      clearbad();
      break;
  }
}


bool
AssetVersionActions::Contribute(QPopupMenu *menu)
{
  AssetVersion version(verref);
  if (!version)
    return false;

  bool added = false;
  if (version->CanRebuild()) {
    menu->insertItem( tr("Resume"), REBUILD_ACTION );
    added = true;
  }
  if (version->CanCancel()) {
    menu->insertItem( tr("Cancel"), CANCEL_ACTION );
    added = true;
  }
  if (version->CanSetBad()) {
    menu->insertItem( tr("Mark as bad"), SETBAD_ACTION );
    added = true;
  }
  if (version->CanClearBad()) {
    menu->insertItem( tr("Mark as good"), CLEARBAD_ACTION );
    added = true;
  }
  if (version->CanClean()) {
    menu->insertItem( tr("Clean"), CLEAN_ACTION );
    added = true;
  }

  return added;
}


void
AssetVersionActions::rebuild(void)
{
  QString error;
  if (!khAssetManagerProxy::RebuildVersion(verref, error)) {
    QMessageBox::critical( parent, QMessageBox::tr( "Error" ),
                           error,
                           QMessageBox::tr( "OK" ), 0, 0, 0 );
  }
}

void
AssetVersionActions::cancel(void)
{
  QString error;
  if (!khAssetManagerProxy::CancelVersion(verref, error)) {
    QMessageBox::critical( parent, QMessageBox::tr( "Error" ),
                           error,
                           QMessageBox::tr( "OK" ), 0, 0, 0 );
  }
}

void
AssetVersionActions::clean(void)
{
  if ( QMessageBox::question
       ( parent, "Confirm Clean",
         QMessageBox::tr( "Cleaning an asset version will remove all of its output files.\n"
                          "It will also remove all of its intermediate files that are not shared with\n"
                          "other asset versions.\n\n"
                          "This action is not reversable.\n\n"
                          "Do you wish to proceed?" ),
         QMessageBox::tr("Clean"), QMessageBox::tr("Cancel"), QString::null,
         1, 1 ) == 0 ) {
        
    QString error;
    if (!khAssetManagerProxy::CleanVersion(verref, error)) {
      QMessageBox::critical( parent, QMessageBox::tr( "Error" ),
                             error,
                             QMessageBox::tr( "OK" ), 0, 0, 0 );
    }
  }
}

void
AssetVersionActions::setbad(void)
{
  QString error;
  if (!khAssetManagerProxy::SetBadVersion(verref, error)) {
    QMessageBox::critical( parent, QMessageBox::tr( "Error" ),
                           error,
                           QMessageBox::tr( "OK" ), 0, 0, 0 );
  }
}

void
AssetVersionActions::clearbad(void)
{
  QString error;
  if (!khAssetManagerProxy::ClearBadVersion(verref, error)) {
    QMessageBox::critical( parent, QMessageBox::tr( "Error" ),
                           error,
                           QMessageBox::tr( "OK" ), 0, 0, 0 );
  }
}

