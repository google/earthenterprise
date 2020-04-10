/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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

#ifndef _KHSRC_FUSION_FUSIONUI_SCRIPTEDITOR_H__
#define _KHSRC_FUSION_FUSIONUI_SCRIPTEDITOR_H__

#include <vector>
#include <map>
#include <Qt/qobjectdefs.h>
#include <Qt/qstring.h>
#include <Qt/qstringlist.h>
#include "scripteditorbase.h"
#include <gstRecord.h>
#include <gstSourceManager.h>
#include <Qt/q3listbox.h>
using QListBoxItem = Q3ListBoxItem;

class gstFormat;

class ScriptEditor : public ScriptEditorBase {
 public:
  enum Type {StatementBlock, Expression};
  static bool Run(QWidget *parent,
                  const gstSharedSource &source_,
                  QString& script, Type type,
                  const QStringList &contextScripts);

  // From ScriptEditorBase
  virtual void fieldHighlighted(int);
  virtual void insertField(int);
  virtual void insertValue(QListBoxItem*);
  virtual void getValues();
  virtual void insertButtonClicked(int);
  virtual void compileAndAccept();

 protected:
  ScriptEditor(QWidget* parent,
               const gstSharedSource &source_,
               const QString &script, Type type,
               const QStringList &contextScripts);
  virtual ~ScriptEditor(void);
  bool CurrFieldIsReal(void) const;
  bool WantGetValues(void) const;

 private:
  gstSharedSource source;
  gstHeaderHandle recordHeader;
  int currField;
  QStringList contextScripts;
  typedef std::map<unsigned int, QStringList*> ValueCache;
  ValueCache cachedValues;
  std::vector<QString> insertFieldText;
};



#endif // _KHSRC_FUSION_FUSIONUI_SCRIPTEDITOR_H__
