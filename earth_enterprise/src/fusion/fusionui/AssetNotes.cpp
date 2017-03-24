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


#include <qtextedit.h>
#include <qdatetime.h>

#include "AssetNotes.h"

AssetNotes::AssetNotes(QWidget* parent, const QString& text)
  : AssetNotesBase(parent) {
  notes_edit->setText(text);
  notes_edit->setFocus();
  notes_edit->setCursorPosition(0, 0);
}

QString AssetNotes::GetText() const {
  return notes_edit->text();
}

void AssetNotes::Timestamp() {
  QDateTime now = QDateTime::currentDateTime();
  notes_edit->insert(now.toString("yyyy-MM-dd hh:mm"));
}
