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


#ifndef KHSRC_FUSION_FUSIONUI_PLACEMARKMANAGER_H__
#define KHSRC_FUSION_FUSIONUI_PLACEMARKMANAGER_H__

#include "placemarkmanagerbase.h"

#include <gst/.idl/gstPlacemark.h>
#include <qstringlist.h>


class PlacemarkManager : public PlacemarkManagerBase {
 public:
  PlacemarkManager();

  // inherited from QDialog
  virtual void accept();
  virtual void reject();

  // external interface
  void Add(const gstPlacemark& pm);
  gstPlacemark Get(int id) const;
  QStringList GetList();
  int Manage();

 private:
  // inherited from PlacemarkManagerBase
  virtual void deletePlacemark();
  virtual void modifyPlacemark();
  virtual void moveDown();
  virtual void moveUp();
  virtual void newPlacemark();
  virtual void selectionChanged(int, int);

  void SetRow(int row, const gstPlacemark& pm);
  void Save();

  gstPlacemarkSet placemarks_;
  gstPlacemarkSet restore_placemarks_;
};

#endif  // !KHSRC_FUSION_FUSIONUI_PLACEMARKMANAGER_H__
