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


#include <Qt/qcombobox.h>
#include <Qt/q3combobox.h>
#include <Qt/qstringlist.h>
#include <Qt/q3groupbox.h>
#include <Qt/qlineedit.h>
#include <Qt/q3ptrlist.h>
#include <Qt/qmessagebox.h>
#include <Qt/q3objectdict.h>
#include <Qt/q3scrollview.h>
using QScrollView = Q3ScrollView;
#include <gstSelectRule.h>
#include "khException.h"
#include "QueryRules.h"
#include <Qt/qevent.h>

QueryRules::QueryRules(QWidget* parent, const char* name, Qt::WFlags f)
  : QScrollView(parent, name, f) {
  rule_count_ = rule_modified_ = 0;
  setVScrollBarMode(QScrollView::AlwaysOn);
  viewport()->setPaletteBackgroundColor(parent->paletteBackgroundColor());

  rb_item_height_ = 0;
  field_descriptors_ = NULL;
}

void QueryRules::init(const FilterConfig& config) {
  // clear out any previous children
  const QObjectList c = viewport()->children();
  if (c.size()) {
    auto it = c.begin();
    QObject* obj;
    while ((obj = *it) != 0) {
      ++it;
      Q3GroupBox* box = reinterpret_cast<Q3GroupBox*>(obj);
      delete box;
    }
  }

  rule_count_ = 0;

  // now add the new ones back in
  for (const auto& it : config.selectRules) {
    int id = BuildRule() - 1;
    field_[id]->setCurrentItem(it.fieldNum);
    oper_[id]->setCurrentItem(static_cast<int>(it.op));
    rval_[id]->setText(it.rvalue);
  }

  rule_modified_ = 0;
}

void QueryRules::ruleModified() {
  notify(NFY_VERBOSE, "QueryRules::ruleModified()");
  ++rule_modified_;
}

FilterConfig QueryRules::getConfig() const {
  FilterConfig cfg;
  for (int id = 0; id < rule_count_; ++id) {
    cfg.selectRules.push_back(SelectRuleConfig(
      (SelectRuleConfig::Operator)oper_[id]->currentItem(),
      uint(field_[id]->currentItem()), rval_[id]->text()));
  }

  return cfg;
}

const int kRuleBoxMargin = 10;

int QueryRules::BuildRule() {
  //
  // create a box to hold our new rule in
  //
  Q3GroupBox* box = new Q3GroupBox(viewport());
  box->setOrientation(Qt::Horizontal);
  box->setColumns(3);
  box->setInsideSpacing(kRuleBoxMargin);
  box->setInsideMargin(kRuleBoxMargin);
  box->setFrameShape(static_cast<Q3GroupBox::DummyFrame>(0));
  Q3ComboBox* field_box = new Q3ComboBox(box);
  if (field_descriptors_) {
    field_box->insertStringList(*field_descriptors_);
    connect(field_box, SIGNAL(activated(int)), this, SLOT(ruleModified()));
  }

  QStringList opsList;
  opsList << kh::tr("equals")
          << kh::tr("is less than or equal to")
          << kh::tr("is greater than or equal to")
          << kh::tr("is less than")
          << kh::tr("is greater than")
          << kh::tr("is not equal to")
          << kh::tr("matches")
          << kh::tr("does not match");

  Q3ComboBox* ops_box = new Q3ComboBox(box);
  ops_box->insertStringList(opsList);

  connect(ops_box, SIGNAL(activated(int)), this, SLOT(ruleModified()));

  QLineEdit* user_val = new QLineEdit(box);

  connect(user_val, SIGNAL(textChanged(const QString&)),
          this, SLOT(ruleModified()));

  // Get the correct height after the first item is created.
  // This should never change again during the apps life.
  // This height now includes the margins.
  box->adjustSize();
  if (rb_item_height_ == 0)
    rb_item_height_ = box->height() - kRuleBoxMargin;

  // Resize to remove the margin on the bottom side of the box.
  box->resize(viewport()->width(), rb_item_height_);

  addChild(box, 0, (rb_item_height_ * rule_count_));
  resizeContents(viewport()->width(),
                 (rb_item_height_ * (rule_count_ + 1)) + kRuleBoxMargin);
  showChild(box);

  ensureVisible(0, rb_item_height_ * (rule_count_ + 1) + kRuleBoxMargin);

  field_[rule_count_] = field_box;
  oper_[rule_count_] = ops_box;
  rval_[rule_count_] = user_val;

  ++rule_count_;

  return rule_count_;
}

void QueryRules::moreRules() {
  //
  // we put an arbitrary maximum number of rules just to prevent users
  // from getting carried away.  if more are needed, probably a different
  // approach should be taken
  //
  if (rule_count_ == kMaxRuleCount) {
    QMessageBox::warning(
      this, "Maximum rules",
      QString(kh::trUtf8("You now have %1 rules, which is the maximum supported."))
      .arg(kMaxRuleCount),
      kh::trUtf8("OK"), 0, 0, 0);
    return;
  }

  BuildRule();

  ++rule_modified_;
}

void QueryRules::fewerRules() {
  if (rule_count_ == 0)
    return;

  const QObjectList c = viewport()->children();
  Q3GroupBox* lastbox = reinterpret_cast<Q3GroupBox*>(c.last());
  removeChild(lastbox);
  delete lastbox;

  rule_count_--;

  resizeContents(viewport()->width(), rb_item_height_ * rule_count_);

  ensureVisible(0, rb_item_height_ * rule_count_);

  ++rule_modified_;
}

//
// need to resize horizontally all children
// vertical is solved by the scroll bar
//
void QueryRules::viewportResizeEvent(QResizeEvent* event) {
  QSize sz = event->size();

  const QObjectList c = viewport()->children();
  if (c.size()) {
    auto it = c.begin();
    QObject* obj;
    while ((obj = *it) != 0) {
      ++it;
      QSize wsz = reinterpret_cast<QWidget*>(obj)->size();
      wsz.setWidth(sz.width());
      reinterpret_cast<QWidget*>(obj)->resize(wsz);
    }
  }

  Q3ScrollView::viewportResizeEvent(event);
}

void QueryRules::setFieldDesc(const QStringList& from) {
  delete field_descriptors_;
  field_descriptors_ = new QStringList(from);
}
