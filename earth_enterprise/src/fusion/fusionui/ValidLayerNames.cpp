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

//

#include "ValidLayerNames.h"
#include "khException.h"
#include <qvalidator.h>
#include <qobject.h>


// A valid Layer name cannot have '|', '[', or ']'
// ']' is disallowed because it breaks the dbRoot
// '[' is disallowed for symetry with ']'
// '|' is disallowed because we use when flattening layer paths for old clients
static QRegExp ValidLayerNameRegExp("[^|\\[\\]]+");

QString InvalidLayerNameMessage
(kh::tr("Layer name cannot contain the following characters: [ ] | #"));

const QValidator* NewLayerNameValidator(void) {
  return new QRegExpValidator(ValidLayerNameRegExp, 0);
}

bool ValidLayerName(const QString &str)
{
  return ValidLayerNameRegExp.exactMatch(str);
}

QString CleanInvalidLayerName(const QString &str)
{
  QString copy = str;
  copy.replace(QChar('|'), "_");
  copy.replace(QChar('['), "_");
  copy.replace(QChar(']'), "_");
  return copy;
}
