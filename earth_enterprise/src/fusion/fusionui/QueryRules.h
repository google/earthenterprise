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


#ifndef KHSRC_FUSION_FUSIONUI_QUERYRULES_H__
#define KHSRC_FUSION_FUSIONUI_QUERYRULES_H__

#include <khArray.h>
#include <Qt/q3scrollview.h>
#include <Qt/qwidget.h>
#include <Qt/qobjectdefs.h>

#include <autoingest/.idl/storage/FilterConfig.h>

class Q3ComboBox;
class QLineEdit;

class QueryRules : public Q3ScrollView {
  Q_OBJECT

 public:
  QueryRules(QWidget* parent = 0, const char* name = 0, Qt::WFlags f = 0);

  void init(const FilterConfig& cfg);
  FilterConfig getConfig() const;

  void setFieldDesc(const QStringList& desc);
  int getNumRules() const { return rule_count_; }
  int isModified() const { return rule_modified_; }

  enum { kMaxRuleCount = 20 };

 public slots:
  void moreRules();
  void fewerRules();
  void ruleModified();

 protected:
  int BuildRule();
  // from QScrollView
  virtual void viewportResizeEvent(QResizeEvent* event);

 private:
  int rule_count_;
  int rule_modified_;
  int rb_item_height_;
  QStringList* field_descriptors_;
  Q3ComboBox* field_[kMaxRuleCount];
  Q3ComboBox* oper_[kMaxRuleCount];
  QLineEdit* rval_[kMaxRuleCount];
};

#endif  // !KHSRC_FUSION_FUSIONUI_QUERYRULES_H__
