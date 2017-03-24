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


#include "fusion/fusionui/QValidatorExt.h"
#include <assert.h>
#include "common/notify.h"

namespace qt_fusion {

EvenQIntValidator::EvenQIntValidator(QObject *parent)
    : QIntValidator(parent) {
}

EvenQIntValidator::EvenQIntValidator(int minimum, int maximum, QObject *parent)
    : QIntValidator(minimum, maximum, parent) {
  assert(!((minimum & 0x1) || (maximum & 0x1)));
}

QValidator::State EvenQIntValidator::validate(
    QString &input, int &pos) const {
  QValidator::State state = QIntValidator::validate(input, pos);

  if (QValidator::Acceptable == state) {
    // Check if value is even.
    bool ok;
    int value = input.toInt(&ok, 10);  // 10 is base.
    if (value & 0x1) {  // not even - return state "Invalid".
      return QValidator::Invalid;  // do not allow to input odd value.
    }
  }
  return state;
}

}  // namespace qt_fusion
