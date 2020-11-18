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

//
// Contains different validators (qt extensions).
//

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_QVALIDATOREXT_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_QVALIDATOREXT_H_

#include <Qt/qvalidator.h>

namespace qt_fusion {

// EvenQIntValidator class provides a validator that ensures that string
// contains even integer with specified range.
class EvenQIntValidator : public QIntValidator {
 public:
  EvenQIntValidator(QObject *parent = 0);

  EvenQIntValidator(int minimum, int maximum, QObject *parent = 0);

  virtual QValidator::State validate(QString &input, int &pos) const;
};

}  // namespace qt_fusion

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_QVALIDATOREXT_H_
