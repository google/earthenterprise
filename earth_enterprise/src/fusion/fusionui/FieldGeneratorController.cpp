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

#include "fusion/fusionui/FieldGeneratorController.h"
#include "autoingest/.idl/storage/FieldGenerator.h"
#include "fusion/fusionui/LabelFormat.h"
#include "fusion/fusionui/ScriptEditor.h"

#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>

#include "fusion/gst/gstRecordJSContext.h"
#include "common/khException.h"


void FieldGeneratorController::Create(WidgetControllerManager &manager,
                                      QComboBox *combo_,
                                      QLineEdit *lineEdit_,
                                      bool lineEditEmptyOk_,
                                      QPushButton *button_,
                                      FieldGenerator *config_,
                                      const gstSharedSource &source_,
                                      const QStringList &contextScripts_,
                                      const QString &errorContext) {
  (void) new FieldGeneratorController(manager, combo_, lineEdit_,
                                      lineEditEmptyOk_, button_,
                                      config_, source_, contextScripts_,
                                      errorContext);
}


void FieldGeneratorController::modeChanged(void) {
  lineEdit->setText(QString(""));
  EmitChanged();
}

void FieldGeneratorController::moreButtonClicked(void) {
  switch ((FieldGenerator::FieldGenerationMode)combo->currentItem()) {
    case FieldGenerator::RecordFormatter: {
      LabelFormat label_format(PopupParent(),
                               source ? source->GetAttrDefs(0)
                               : gstHeaderHandle(),
                               lineEdit->text(), LabelFormat::SingleLine);
      if (label_format.exec() == QDialog::Accepted) {
        if (lineEdit->text() != label_format.GetText()) {
          lineEdit->setText(label_format.GetText());
          EmitChanged();
        }
      }
      break;
    }
    case FieldGenerator::JSExpression: {
      QString script = lineEdit->text();
      if (ScriptEditor::Run(PopupParent(), source,
                            script, ScriptEditor::Expression,
                            contextScripts)) {
        if (lineEdit->text() != script) {
          lineEdit->setText(script);
          EmitChanged();
        }
      }
      break;
    }
    default:
      assert(0);
  }
}

void FieldGeneratorController::SyncToConfig(void) {
  config->mode = (FieldGenerator::FieldGenerationMode)combo->currentItem();
  config->value = lineEdit->text();
  if (!lineEditEmptyOk && config->empty()) {
    throw khException(
        kh::tr("The '%1' is enabled, but empty.").arg(errorContext));
  }
  if (config->mode == FieldGenerator::JSExpression) {
    QString compilationError;
    if (!gstRecordJSContextImpl::CompileCheck(
            source ? source->GetAttrDefs(0) : gstHeaderHandle(),
            contextScripts, config->value, compilationError)) {
      throw khException(kh::tr("JavaScript Error (%1):\n%2")
                        .arg(errorContext).arg(compilationError));
    }
  }
}

void FieldGeneratorController::SyncToWidgetsImpl(void) {
  MySyncToWidgetsImpl();
}

void FieldGeneratorController::MySyncToWidgetsImpl(void) {
  combo->setCurrentItem(static_cast<int>(config->mode));
  lineEdit->setText(config->value);
}

FieldGeneratorController::FieldGeneratorController(
    WidgetControllerManager &manager,
    QComboBox *combo_,
    QLineEdit *lineEdit_,
    bool lineEditEmptyOk_,
    QPushButton *button_,
    FieldGenerator *config_,
    const gstSharedSource &source_,
    const QStringList &contextScripts_,
    const QString &errorContext_) :
    WidgetController(manager),
    combo(combo_),
    lineEdit(lineEdit_),
    lineEditEmptyOk(lineEditEmptyOk_),
    button(button_),
    config(config_),
    source(source_),
    contextScripts(contextScripts_),
    errorContext(errorContext_) {
  combo->clear();
  combo->insertItem("Text");
  combo->insertItem("JS Text");

  MySyncToWidgetsImpl();

  // connect all the widgets to me
  connect(combo, SIGNAL(activated(int)), this, SLOT(modeChanged()));
  connect(button, SIGNAL(clicked()), this, SLOT(moreButtonClicked()));
  connect(lineEdit, SIGNAL(textChanged(const QString &)),
          this, SLOT(EmitTextChanged()));
}
