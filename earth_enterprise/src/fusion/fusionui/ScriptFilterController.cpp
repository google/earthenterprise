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

#include "ScriptFilterController.h"
#include "LabelFormat.h"
#include "ScriptEditor.h"
#include <qlineedit.h>
#include <qpushbutton.h>
#include <gst/gstRecordJSContext.h>
#include <khException.h>


void
ScriptFilterController::Create(WidgetControllerManager &manager,
                               QLineEdit *lineEdit_,
                               QPushButton *button_,
                               QString *config_,
                               const gstSharedSource &source_,
                               const QStringList &contextScripts_,
                               const QString &errorContext)
{
  (void) new ScriptFilterController(manager, lineEdit_, button_,
                                    config_, source_, contextScripts_,
                                    errorContext);
}


void
ScriptFilterController::moreButtonClicked(void)
{
  QString script = lineEdit->text();
  if (ScriptEditor::Run(PopupParent(), source,
                        script, ScriptEditor::Expression,
                        contextScripts)) {
    if (lineEdit->text() != script) {
      lineEdit->setText(script);
      EmitChanged();
    }
  }
}

void
ScriptFilterController::SyncToConfig(void)
{
  *config = lineEdit->text();
  if (!config->isEmpty()) {
    QString compilationError;
    if (!gstRecordJSContextImpl::CompileCheck(
            source ? source->GetAttrDefs(0) : gstHeaderHandle(),
            contextScripts, *config, compilationError)) {
      throw khException(kh::tr("JavaScript Error (%1):\n%2")
                        .arg(errorContext).arg(compilationError));
    }    
  }
}

void
ScriptFilterController::SyncToWidgetsImpl(void)
{
  MySyncToWidgetsImpl();
}

void
ScriptFilterController::MySyncToWidgetsImpl(void)
{
  lineEdit->setText(*config);
}

ScriptFilterController::ScriptFilterController(
    WidgetControllerManager &manager,
    QLineEdit *lineEdit_,
    QPushButton *button_,
    QString *config_,
    const gstSharedSource &source_,
    const QStringList &contextScripts_,
    const QString &errorContext_) :
    WidgetController(manager),
    lineEdit(lineEdit_),
    button(button_),
    config(config_),
    source(source_),
    contextScripts(contextScripts_),
    errorContext(errorContext_)
{
  MySyncToWidgetsImpl();

  // connect all the widgets to me
  connect(button, SIGNAL(clicked()), this, SLOT(moreButtonClicked()));
  connect(lineEdit, SIGNAL(textChanged(const QString &)),
          this, SLOT(EmitTextChanged()));
}
