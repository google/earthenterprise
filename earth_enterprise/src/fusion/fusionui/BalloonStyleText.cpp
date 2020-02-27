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

#include <Qt/qtextedit.h>
#include <Qt/qregexp.h>
#include <Qt/qmessagebox.h>

#include "BalloonStyleText.h"

// client v4 default balloon style
static const char kDefaultBalloonStyleText[] =
    "<h3>$[name]</h3>$[description]<br><br>$[geDirections]";

BalloonStyleText::BalloonStyleText(QWidget* parent, const QString& text)
    : BalloonStyleTextBase(parent, 0, false, 0) {
  text_edit->setText(text);
}

void BalloonStyleText::accept() {
  // test for the only invalid character
  if (text_edit->text().find('"') != -1) {
      QMessageBox::critical(
        this, tr("Error"),
        tr("Using the double-quote character: \" is not allowed.") +
        tr("\nPlease correct this before continuing."),
        tr("OK"), 0, 0, 0);
    return;
  }
  text_ = text_edit->text();
  BalloonStyleTextBase::accept();
}

void BalloonStyleText::InsertDefault() {
  text_edit->insert(kDefaultBalloonStyleText);
}
