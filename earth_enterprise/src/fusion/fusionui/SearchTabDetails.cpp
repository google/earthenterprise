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

#include <Qt/q3table.h>
#include <Qt/qlabel.h>
#include <Qt/qmessagebox.h>
#include <Qt/qinputdialog.h>
#include <Qt/qradiobutton.h>
#include "SearchTabDetails.h"

SearchTabDetails::SearchTabDetails(QWidget* parent,
                                   const SearchTabReference& ref)
  : SearchTabDetailsBase(parent, 0, 0), ref_(ref) {

  
  try {
    SearchTabDefinition config = ref_.Bind();

    // Populate the widget from the search tab definition.
    if (config.fields.size() == 0)
      return;

    label1_text->setText(config.fields[0].label);

    if (config.fields.size() > 1) {
      label2_text->setText(config.fields[1].label);
    } else {
      label2_text->hide();
      label2_edit->hide();
    }
  } catch (const std::exception &e) {
    label1_text->setText(
        QObject::tr("<font color=\"#ff0000\">%1</font>")
        .arg(QString::fromUtf8(e.what())));
    label1_edit->hide();
    label2_text->hide();
    label2_edit->hide();
  }
}
