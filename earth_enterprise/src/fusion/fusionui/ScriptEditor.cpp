// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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

#include <set>
#include <Qt/qmessagebox.h>
#include <Qt/q3textedit.h>
#include <Qt/q3listbox.h>
#include <Qt/qstringlist.h>
#include <Qt/qpushbutton.h>
#include <Qt/q3buttongroup.h>
#include <Qt/q3progressdialog.h>
#include <Qt/qapplication.h>
#include <Qt/q3button.h>
#include "ScriptEditor.h"
#include <gstFormat.h>
#include <gstSource.h>
#include <gstSourceManager.h>
#include <gstRecordJSContext.h>
#include <khException.h>

using QTextEdit = Q3TextEdit;
using QButton = Q3Button;
using QProgressDialog = Q3ProgressDialog;

ScriptEditor::ScriptEditor(QWidget* parent,
                           const gstSharedSource &source_,
                           const QString &script, Type type,
                           const QStringList &contextScripts_) :
    ScriptEditorBase(parent),
    source(source_),
    recordHeader(source ? source->GetAttrDefs(0) : gstHeaderHandle()),
    currField(-1),
    contextScripts(contextScripts_)
{
  gstRecordJSContext cx = gstRecordJSContextImpl::Create(recordHeader,
                                                         contextScripts);

  // Walk the recordHeader, add each field name to the field listBox and
  // figure out what test we're going to paste for each one.
  for (unsigned int f = 0; f < recordHeader->numColumns(); ++f) {
    QString name = recordHeader->Name(f);
    fieldsListBox->insertItem(name);
    if ((f < gstRecordJSContextImpl::MaxNumProperties) &&
        cx->IsIdentifier(name)) {
      insertFieldText.push_back(QString("fields.")+name);
    } else {
      insertFieldText.push_back(QString("fields[%1 /* %2 */]")
                                .arg(f).arg(name));
    }
  }

  // Walk the global functions registered by the context scripts and add
  // them to the fields listBox too.
  QStringList globalFuncs;
  cx->FindGlobalFunctionNames(globalFuncs);
  for (QStringList::const_iterator gf = globalFuncs.begin();
       gf != globalFuncs.end(); ++gf) {
    QString withParens = *gf + "()";
    fieldsListBox->insertItem(withParens);
    insertFieldText.push_back(withParens);
  }

  // misc widgetry setup
  getValuesButton->setEnabled(false);
  scriptEdit->setText(script);
  scriptEdit->moveCursor(QTextEdit::MoveEnd, false);
  scriptEdit->setFocus();
  if (type == Expression) {
    semicolonButton->setEnabled(false);
  }
}

ScriptEditor::~ScriptEditor(void)
{

  for (const auto& i : cachedValues) {
    if (i.second) {
      delete i.second;
    }
  }
}

bool
ScriptEditor::Run(QWidget *parent,
                  const gstSharedSource &source_,
                  QString& script, Type type,
                  const QStringList &contextScripts)
{
  if (!source_) {
    QMessageBox::critical(parent, kh::tr("Error"),
                          kh::tr("No source record is available"),
                          kh::tr("OK"), 0, 0, 0);
    return false;
  }

  QString javascriptError;
  try {
    ScriptEditor editor(parent, source_, script, type, contextScripts);

    if (editor.exec() != QDialog::Accepted) {
      return false;
    }

    script = editor.scriptEdit->text();
    return true;
  } catch (const khException &e) {
    javascriptError = e.qwhat();
  } catch (const std::exception &e) {
    javascriptError = e.what();
  } catch (...) {
    javascriptError = "Unknown error";
  }
  // This should really only trip for erros in the context scritps
  // errors in the script we're editing should be caught and handled
  // while the dialog is still up
  QMessageBox::critical(parent, kh::tr("JavaScript Error"),
                        kh::tr("JavaScript Error:\n%1")
                        .arg(javascriptError),
                        kh::tr("OK"), 0, 0, 0);
  return false;
}


bool ScriptEditor::CurrFieldIsReal(void) const {
  return ((currField >= 0) &&
          (currField < (int)recordHeader->numColumns()));
}
bool ScriptEditor::WantGetValues(void) const {
  return CurrFieldIsReal() || insertFieldText[currField].endsWith("()");
}

void ScriptEditor::fieldHighlighted(int fNum) {
  if (currField != fNum) {
    currField = fNum;
    valuesListBox->clear();
    QStringList *values = cachedValues[currField];
    if (values) {
      valuesListBox->insertStringList(*values);
    }
    getValuesButton->setEnabled(WantGetValues());
  }
}

void ScriptEditor::insertField(int num) {
  scriptEdit->insert(insertFieldText[num]);
}

void ScriptEditor::insertValue(QListBoxItem* item) {
  QString text = item->text();

  if (!WantGetValues()) {
    return;
  }

  if (CurrFieldIsReal()) {
    switch (recordHeader->ftype(currField)) {
      case gstTagInt:
      case gstTagUInt:
      case gstTagInt64:
      case gstTagUInt64:
      case gstTagFloat:
      case gstTagDouble:
        scriptEdit->insert(text);
        break;
      case gstTagString:
      case gstTagUnicode:
      default:
        text.replace("\\", "\\\\");  // escape embedded '\'
        text.replace("\"", "\\\"");  // escape embedded quotes
        text.replace("\n", "\\\n");  // escape embedded newline
        scriptEdit->insert(QString("\"")+text+QString("\""));
    }
  } else {
    text.replace("\\", "\\\\");  // escape embedded '\'
    text.replace("\"", "\\\"");  // escape embedded quotes
    text.replace("\n", "\\\n");  // escape embedded newline
    scriptEdit->insert(QString("\"")+text+QString("\""));
  }
}

void ScriptEditor::getValues() {
  if (!WantGetValues()) {
    return;
  }

  if (!source) return;

  QString javascriptError;
  try {
    gstSource* raw_source = source->GetRawSource();

    QProgressDialog progress(this, 0, true);
    progress.setCaption(kh::tr("Find Unique Values"));
    progress.setLabelText(kh::tr("Found %1 unique values").arg(0));
    progress.setTotalSteps(raw_source->NumFeatures(0));
    progress.setMinimumDuration(2000);

    gstRecordJSContext cx;

    if (!CurrFieldIsReal()) {
      cx = gstRecordJSContextImpl::Create(recordHeader, contextScripts,
                                          insertFieldText[currField]);
    }

    std::set<QString> unique;
    unsigned int found = 0;
    unsigned int count = 0;
    for (raw_source->ResetReadingOrThrow(0);
         !raw_source->IsReadingDone();
         raw_source->IncrementReadingOrThrow()) {
      // make sure we're listening to the dialog
      if (progress.wasCanceled()) {
        break;
      }
      qApp->processEvents();

      ++count;
      gstRecordHandle currRec = raw_source->GetCurrentAttributeOrThrow();

      QString val;
      if (cx) {
        cx->SetRecord(currRec);
        cx->ExecutePrimaryScript(val);
      } else {
        if (currField >= (int)currRec->NumFields()) {
          // this shouldn't happen, but there are a lot of format loaders out
          // there. This just jeeps us safe.
          break;
        }
        val = currRec->Field(currField)->ValueAsUnicode();
      }

      if (unique.find(val) == unique.end()) {
        unique.insert(val);
        ++found;
        progress.setLabelText(kh::tr("Found %1 unique values").arg(found));
      }
      progress.setProgress(count);
    }
    progress.setProgress(raw_source->NumFeatures(0));

    // Now we're done. Update the list widget and our cache
    QStringList *cache = cachedValues[currField];
    if (!cache) {
      cache = cachedValues[currField] = new QStringList();
    }

    valuesListBox->clear();
    for (std::set<QString>::const_iterator i = unique.begin();
         i != unique.end(); ++i) {
      valuesListBox->insertItem(*i);
      cache->push_back(*i);
    }

    return;
  } catch (const khException &e) {
    javascriptError = e.qwhat();
  } catch (const std::exception &e) {
    javascriptError = e.what();
  } catch (...) {
    javascriptError = "Unknown error";
  }
  // This should really only trip for erros in the context scritps
  // errors in the script we're editing should be caught and handled
  // while the dialog is still up
  QMessageBox::critical(this, kh::tr("Error"),
                        kh::tr("Error getting values:\n%1")
                        .arg(javascriptError),
                        kh::tr("OK"), 0, 0, 0);
    
}

void ScriptEditor::insertButtonClicked(int id) {
  QButton *button = static_cast<QButton*>(insertButtonGroup->find(id));
  if (!button) return;
  // originally, QButton (Q3Button) is cast to QPushButton
  // this cast is not allowed in Qt4, as QButton does not exist
  // however, the text() method can be called directly on QButton now
  QString text = button->text();
  text.replace("&&", "&");  // undo double '&' needed to avoid accelerator key
  scriptEdit->insert(text);
}

void ScriptEditor::compileAndAccept() {
  QString newScript = scriptEdit->text();
  if (newScript.isEmpty()) {
    accept();
    return;
  }

  QString compilationError;
  if (gstRecordJSContextImpl::CompileCheck(recordHeader, contextScripts,
                                           newScript, compilationError)) {
    accept();
  } else {
    QMessageBox::critical(this, kh::tr("JavaScript Error"),
                          kh::tr("JavaScript Error:\n%1")
                          .arg(compilationError),
                          kh::tr("OK"), 0, 0, 0);
  }
}
