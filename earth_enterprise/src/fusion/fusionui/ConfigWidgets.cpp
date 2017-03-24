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


#include <qlineedit.h>
#include <qslider.h>
#include <qlayout.h>

#include "ConfigWidgets.h"

LevelSlider::LevelSlider(int min_val, int max_val, int val, QWidget* parent)
  : QWidget(parent) {
  QHBoxLayout* layout = new QHBoxLayout(this, 0, 0);
  slider_ = new QSlider(min_val, max_val, 1, val, Qt::Horizontal, this);
  layout->addWidget(slider_);

  line_edit_ = new QLineEdit(QString::number(val), this);
  line_edit_->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)0,
                               (QSizePolicy::SizeType)0, 0, 0,
                               line_edit_->sizePolicy().hasHeightForWidth()));
  line_edit_->setMaximumSize(QSize(28, 32767));
  line_edit_->setReadOnly(true);
  layout->addWidget(line_edit_);
  resize(QSize(124, 26).expandedTo(minimumSizeHint()));

  connect(slider_, SIGNAL(valueChanged(int)),
          this, SLOT(ValueChanged(int)));
}

#if 0
void LevelSlider::SyncToConfig() {
  *value_ = slider_->value();
}

void LevelSlider::SyncToWidget() {
  slider_->setValue(*value_);
}
#endif

void LevelSlider::ValueChanged(int val) {
  line_edit_->setText(QString::number(val));
}

int LevelSlider::Value() const {
  return slider_->value();
}
