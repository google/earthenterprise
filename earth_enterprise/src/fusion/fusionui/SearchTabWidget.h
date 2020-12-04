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


#ifndef KHSRC_FUSION_FUSIONUI_SEARCHTABWIDGET_H__
#define KHSRC_FUSION_FUSIONUI_SEARCHTABWIDGET_H__
#include <Qt/qobjectdefs.h>
#include <Qt/qtabwidget.h>
#include <vector>
#include <autoingest/.idl/storage/SearchTabReference.h>


class SearchTabWidget : public QTabWidget {
  Q_OBJECT

 public:
  SearchTabWidget(QWidget* parent = 0, const char* name = 0);

  void InsertTab(const QString &tab_label, const SearchTabReference& ref);
  void Set(const std::vector<SearchTabReference> &refs);
  void ProjectsChanged(const std::vector<std::string> &projects);
  std::vector<SearchTabReference> GetSearchTabRefs(void) const;
  // Set a flag to indicate that this is a google earth database
  // as opposed to a maps database.
  void SetIsGEDatabase(bool is_ge_db) { is_ge_db_ = is_ge_db; }
 public slots:
  void AddTab();
  void DeleteTab();
  void MoveTabLeft();
  void MoveTabRight();

 private:
  void MoveTabToPos(QWidget* w, int pos);

  bool is_ge_db_;  // True if we're editing a GE database.
                   // Necessary because of search tab limit of the earth client.
  std::vector<std::string> projects_;
  unsigned int UnhandledErrorCount;
};


#endif  // !KHSRC_FUSION_FUSIONUI_SEARCHTABWIDGET_H__
