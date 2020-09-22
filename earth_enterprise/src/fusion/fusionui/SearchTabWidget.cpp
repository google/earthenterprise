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


#include <Qt/qcursor.h>
#include <Qt/q3popupmenu.h>
#include <Qt/qinputdialog.h>
#include <Qt/qmessagebox.h>
#include "SearchTabWidget.h"
#include "SearchTabDetails.h"
#include <autoingest/.idl/SearchTabSet.h>
#include <notify.h>

using QPopupMenu = Q3PopupMenu;

SearchTabWidget::SearchTabWidget(QWidget* parent,
                                 const char* name)
  : QTabWidget(parent, name), is_ge_db_(false), UnhandledErrorCount(0) {
}


void SearchTabWidget::Set(const std::vector<SearchTabReference> &refs) {
  // clear all existing pages
  while (count()) {
    QWidget *w = page(0);
    removePage(w);
    delete w;
  }

  for (unsigned int i = 0; i < refs.size(); ++i) {
    try {
      SearchTabDefinition def = refs[i].Bind();
      InsertTab(def.label, refs[i]);
    } catch (...) {
      // add a broken tab - so user can fix it
      InsertTab(QString("-- broken --"), refs[i]);
    }
  }
}

void SearchTabWidget::ProjectsChanged(
    const std::vector<std::string> &projects) {
  projects_ = projects;

  // Traverse all pages and prune any that come from projects that I no longer
  // have
  for (int i = 0; i < count(); ) {
    SearchTabDetails *details = dynamic_cast<SearchTabDetails*>(page(i));
    if (!details->ref_.project_ref_.empty()) {
      bool found = false;
      for (unsigned int p = 0; p < projects_.size(); ++p) {
        if (details->ref_.project_ref_ == projects_[p]) {
          found = true;
          break;
        }
      }
      if (found) {
        ++i;
        continue;
      } else {
        QString str = label(i);
        QWidget *w = page(i);
        removePage(w);
        delete w;

        QMessageBox::information(
            this,
            tr("Search tab removed"),
            tr("The search tab '%1' belonged to the project you just\n"
               "changed (or removed). The tab has been removed from your\n"
               "list of tabs.").arg(str),
            tr("OK"));
        // do not increment i
      }
    } else {
      ++i;
    }
  }
}

std::vector<SearchTabReference> SearchTabWidget::GetSearchTabRefs(void) const {
  std::vector<SearchTabReference> tmp;
  for (int i = 0; i < count(); ++i) {
    SearchTabDetails *details = dynamic_cast<SearchTabDetails*>(page(i));
    tmp.push_back(details->ref_);
  }
  return tmp;
}


void SearchTabWidget::AddTab() {
  std::vector<std::pair<QString, SearchTabReference> > tab_refs;

  // add all the global search tabs to the popup menu
  {
    SearchTabSet search_tab_set;
    search_tab_set.Load();
    for (unsigned int i = 0; i < search_tab_set.tabs_.size(); ++i) {
      const SearchTabSet::Tab& tab = search_tab_set.tabs_[i];
      tab_refs.push_back(
          std::make_pair(tab.definition_.label,
                         SearchTabReference(std::string(), tab.id_)));
    }
  }

  // Add the ones that come from my projects - but only those that
  // actually are valid to specify
  for (unsigned int i = 0; i < projects_.size(); ++i) {
    try {
      SearchTabReference ref(projects_[i]);
      SearchTabDefinition def = ref.Bind();
      tab_refs.push_back(std::make_pair(def.label, ref));
    } catch (...) {
      // no op
      // this count is for future code instrumentation
      UnhandledErrorCount++;
    }
  }

  QPopupMenu menu;
  for (unsigned int i = 0; i < tab_refs.size(); ++i) {
    menu.insertItem(tab_refs[i].first, i);
  }

  int select = menu.exec(QCursor::pos());
  if (select == -1)
    return;

  InsertTab(tab_refs[select].first, tab_refs[select].second);
  if (is_ge_db_ && count() > static_cast<int>(kMaxEarthClientTabCount)) {
    QMessageBox::information(
        this,
        tr("Search Tab Warning"),
        tr("The Google Earth Client only supports %1 search tabs.\n"
           "Only the first %2 search tabs will be displayed in the\n"
           "Google Earth Client when viewing this database.\n"
           "The Google Earth Plugin has no limit and will show\n"
           "all added search tabs.").arg(kMaxEarthClientTabCount)
           .arg(kMaxEarthClientTabCount),
        tr("OK"));
  }
}

void SearchTabWidget::InsertTab(const QString &tab_label,
                                const SearchTabReference &ref) {
  SearchTabDetails* search_tab = new SearchTabDetails(this, ref);
  addTab(search_tab, tab_label);
  showPage(search_tab);
  emit currentChanged(currentPage());
}

void SearchTabWidget::DeleteTab() {
  if (QMessageBox::question(this, tr("Delete Search Tab"),
        tr("Are you sure you want to delete this search tab:\n\n    ") +
           "\"" + label(currentPageIndex()) + "\"\n",
        QMessageBox::Yes, QMessageBox::No, 0) != QMessageBox::Yes)
    return;
  QWidget* w = currentPage();
  removePage(w);
  delete w;
  emit currentChanged(currentPage());
}

void SearchTabWidget::MoveTabToPos(QWidget* w, int pos) {
  QString label = tabLabel(w);
  removePage(w);
  insertTab(w, label, pos);
  setCurrentPage(pos);
}

void SearchTabWidget::MoveTabLeft() {
  QWidget* w = currentPage();
  MoveTabToPos(w, indexOf(w)-1);
}

void SearchTabWidget::MoveTabRight() {
  QWidget* w = currentPage();
  MoveTabToPos(w, indexOf(w)+1);
}
