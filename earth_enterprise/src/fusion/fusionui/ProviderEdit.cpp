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
#include <Qt/qpushbutton.h>

#include "ProviderEdit.h"

ProviderEdit::ProviderEdit(QWidget* parent)
  : ProviderEditBase(parent, 0, false, 0) {
}

int ProviderEdit::configure(const gstProvider &provider) {
  orig_ = provider;
  if (provider.name.length() != 0) {
    nameEdit->setText(provider.name);
    keyEdit->setText(QString::fromUtf8(provider.key.c_str()));
    copyrightEdit->setText(provider.copyright);
    okBtn->setEnabled(true);
  } else {
    nameEdit->clear();
    keyEdit->clear();
    copyrightEdit->clear();
    okBtn->setEnabled(false);
  }

  nameEdit->setFocus();

  return exec();
}

gstProvider ProviderEdit::getProvider() const {
  gstProvider provider = orig_;
  provider.name = nameEdit->text();
  provider.key = keyEdit->text().toUtf8().constData();
  provider.copyright = copyrightEdit->text();
  return provider;
}


void ProviderEdit::nameChanged(const QString& txt) {
  if (txt.isEmpty()) {
    okBtn->setEnabled(false);
  } else {
    okBtn->setEnabled(true);
  }
}
