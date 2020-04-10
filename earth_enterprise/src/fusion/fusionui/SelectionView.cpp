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


#include <vector>


#include <Qt/q3table.h>
#include <Qt/q3popupmenu.h>
#include <Qt/qcursor.h>
#include <Qt/qmessagebox.h>
#include <Qt/qfiledialog.h>
#include <Qt/qapplication.h>
#include <Qt/qclipboard.h>
#include <Qt/q3header.h>
#include <khFileUtils.h>
#include <gstFilter.h>
#include <gstSelector.h>
#include <gstSource.h>
#include <gstRecord.h>
#include <gstGeode.h>
#include <gstSourceManager.h>
#include <gstKVPFile.h>
#include <gstKVPTable.h>
#include <Qt/qiodevice.h>
#include "Preferences.h"
#include "SelectionView.h"
#include "DataViewTable.h"
#include "ObjectDetail.h"
#include "GfxView.h"
#include <Qt/qtextstream.h>
using QHeader = Q3Header;
using QPopupMenu = Q3PopupMenu;

SelectionView::SelectionView(QWidget* parent, const char* name, Qt::WFlags fl)
    : SelectionViewBase(parent, name, fl) {
  connect(selectionTable, SIGNAL(contextMenuRequested(int, int, const QPoint&)),
          this, SLOT(openContextMenu(int, int, const QPoint&)));
  connect(this, SIGNAL(zoomToBox(const gstBBox&)),
          GfxView::instance, SLOT(zoomToBox(const gstBBox&)));
}

void SelectionView::configure(gstSelector* selector) {
  //
  // always clear out any previous entries
  //
  selectionTable->setNumCols(0);
  selectionTable->setNumRows(0);

  //
  // empty selector means no selector is active
  // so just leave empty selector view
  //
  if (selector == NULL)
    return;

  selector_ = selector;

  gstSource* src = selector->getSource();
  if (src == NULL)
    return;

  if (selector->pickList().size() == 0)
    return;

  selectionTable->setSelector(selector);

  //
  // set header
  //
  QHeader* header = selectionTable->horizontalHeader();

  gstHeaderHandle attrib = src->GetAttrDefs(selector->layer());

  int col = 0;
  if (Preferences::getConfig().dataViewShowFID) {
    selectionTable->setNumCols(attrib->numColumns() + 1);
    header->setLabel(col++, kh::tr("Feature ID"));
  } else {
    selectionTable->setNumCols(attrib->numColumns());
  }

  for (unsigned int ii = 0; ii < attrib->numColumns(); ++ii)
    header->setLabel(col++, attrib->Name(ii));

  // adjust column widths
  for (int ii = 0; ii < selectionTable->numCols(); ++ii)
    selectionTable->adjustColumn(ii);

  if (Preferences::getConfig().dataViewAutoRaise) {
    show();
    parentWidget()->show();
  }
}

void SelectionView::openContextMenu(int row, int col, const QPoint &pos) {
  QHeader* header = selectionTable->horizontalHeader();

  enum { SORT_ASCENDING, SORT_DESCENDING,
         EXPORT_COLUMN, EXPORT_ALL,
         EXPORT_SELECTED_FEATURES,   // experimental only
         COPY_CELL, ITEM_DETAILS, ZOOM_TO
  };

  QPopupMenu menu(this);

  if (col >= 0) {
    menu.insertItem(QString("Sort by %1 ascending").arg(header->label(col)),
                    SORT_ASCENDING);
    menu.insertItem(QString("Sort by %1 descending").arg(header->label(col)),
                    SORT_DESCENDING);
    menu.insertSeparator();
    menu.insertItem(QString("Export %1 column").arg(header->label(col)),
                    EXPORT_COLUMN);
    menu.insertItem(QString("Export all columns"), EXPORT_ALL);
    if (Preferences::ExperimentalMode) {
      menu.insertItem(QString("Export selected features"), EXPORT_SELECTED_FEATURES);
    }
    menu.insertSeparator();
    menu.insertItem("Feature Details", ITEM_DETAILS);
    menu.insertItem("Zoom to Feature", ZOOM_TO);
    menu.insertSeparator();
    menu.insertItem("Copy cell contents", COPY_CELL);
  } else {
    return;
  }

  // We used to have a boolean flag to indicate whether something was selected.
  // This flag was never used so removed it. (wolfb@ 2013-12-16)
  switch (menu.exec(QCursor::pos())) {
    case SORT_ASCENDING:
      selectionTable->sortColumn(col, true, true);
      break;

    case SORT_DESCENDING:
      selectionTable->sortColumn(col, false, true);
      break;

    case EXPORT_COLUMN:
      SaveColumns(col);
      break;

    case EXPORT_ALL:
      SaveColumns(-1);
      break;

    case EXPORT_SELECTED_FEATURES:
      ExportSelectedFeatures();
      break;

    case COPY_CELL: {
      if (selector_->HasAttrib()) {
        bool showid = Preferences::getConfig().dataViewShowFID;
        gstRecordHandle rec = selector_->getPickRecord(row);
        QString text;
        if (showid && col == 0) {
          text = QString::number(selector_->pickList()[row]);
        } else {
          text = rec->Field(showid ? col - 1 : col)->ValueAsUnicode();
        }
        QApplication::clipboard()->setText(text, QClipboard::Clipboard);
        QApplication::clipboard()->setText(text, QClipboard::Selection);
      }
      break;
    }
    case ITEM_DETAILS: {
      const SelectList& flist = selector_->pickList();
      gstRecordHandle rec;
      if (selector_->HasAttrib()) {
        rec = selector_->getPickRecord(row);
      }
      gstGeodeHandle geode = selector_->getPickGeode(row);
      ObjectDetail* detail = new ObjectDetail(this, flist[row], geode, rec);
      detail->show();
      break;
    }
    case ZOOM_TO: {
      gstGeodeHandle geode = selector_->getPickGeode(row);
      emit zoomToBox(geode->BoundingBox());
    }
  }
}


//
// SaveColumns() dumps current selection view to a file, chosen from file dialog
// passing -1 will output all columns
//
void SelectionView::SaveColumns(int pick_col) {
  //
  // must have a valid source
  //
  if (selector_ == NULL)
    return;
  gstSource* src = selector_->getSource();
  if (src == NULL)
    return;

  //
  // get a filename from the user
  //
  QString fname = QFileDialog::getSaveFileName(QString::null,
                                               "Comma Separated Files (*.csv)", this);

  //
  // iterate through all selected rows, outputing to named file
  //
  if (!fname.isEmpty()) {
    QFile f(fname);
    if (!f.open(QIODevice::WriteOnly)) {
      QMessageBox::critical(this, kh::tr("Fusion"), kh::tr("Unable to open file"));
      return;
    }

    const SelectList& flist = selector_->pickList();

    // if the feature ID is being shown in the selection view,
    // and the range is all columns, save the feature ID
    bool showid = Preferences::getConfig().dataViewShowFID;

    QTextStream out (&f);

    //
    // single column which is the feature id
    //
    if (pick_col == 0 && showid) {
      for (unsigned int row = 0; row < flist.size(); ++row) {
        out << flist[row] << "\n";
      }

      //
      // single column, not the feature id
      //
    } else if (pick_col != -1) {
      if (selector_->HasAttrib()) {
        int col = showid ? pick_col - 1 : pick_col;
        for (unsigned int row = 0; row < flist.size(); ++row) {
          gstRecordHandle rec = selector_->getPickRecord(row);
          out << rec->Field(col)->ValueAsCSV().c_str() << "\n";
        }
      }

      //
      // multiple columns
      //
    } else {
      // determine column range
      int startcol = 0;
      int endcol = src->GetAttrDefs(selector_->layer())->numColumns();

      for (unsigned int row = 0; row < flist.size(); ++row) {
        bool need_sep = false;

        if (showid) {
          out << flist[row];
          need_sep = true;
        }

        if (selector_->HasAttrib()) {
          gstRecordHandle rec = selector_->getPickRecord(row);
          for (int col = startcol; col < endcol; ++col) {
            if (need_sep)
              out << ",";
            out << rec->Field(col)->ValueAsCSV().c_str();
            need_sep = true;
          }
        }
        out << "\n";
      }
    }

    f.close();
  }
}

void SelectionView::ExportSelectedFeatures() {
  // must have a valid source
  if (selector_ == NULL)
    return;
  gstSource* src = selector_->getSource();
  if (src == NULL)
    return;

  // get a filename from the user
  QString fname = QFileDialog::getSaveFileName(QString::null,
                                               "Keyhole Geometry (*.kvgeom)", this);
  if (fname.isEmpty())
    return;

  std::string kvp_name = khEnsureExtension(fname.toUtf8().constData(), ".kvgeom");
  gstKVPFile kvp(kvp_name.c_str());
  if (kvp.OpenForWrite() != GST_OKAY) {
    notify(NFY_WARN, "Unable to open feature file %s", kvp_name.c_str());
    return;
  }

  std::string kdb_name = khReplaceExtension(kvp_name, ".kvattr");
  gstKVPTable kdb(kdb_name.c_str());
  if (kdb.Open(GST_WRITEONLY) != GST_OKAY) {
    notify(NFY_WARN, "Unable to open attribute file %s", kdb_name.c_str());
    return;
  }
  gstHeaderHandle attrib = src->GetAttrDefs(selector_->layer());
  kdb.SetHeader(attrib);

  const SelectList& flist = selector_->pickList();
  for (unsigned int row = 0; row < flist.size(); ++row) {
    gstGeodeHandle geode = selector_->getPickGeode(row);
    if (kvp.AddGeode(geode) != GST_OKAY) {
      notify(NFY_WARN, "Unable to add feature geometry %d", row);
      return;
    }

    if (selector_->HasAttrib()) {
      gstRecordHandle rec = selector_->getPickRecord(row);
      if (kdb.AddRecord(rec) != GST_OKAY) {
        notify(NFY_WARN, "Unable to add attribute record %d", row);
        return;
      }
    }
  }
}

SelectionViewDocker::SelectionViewDocker(Place p, QWidget* parent,
                                         const char* n, Qt::WFlags f, bool)
    : QDockWindow(p, parent, n, f) {
  setResizeEnabled(true);
  setCloseMode(QDockWindow::Always);
  setCaption(n);

  selection_view_ = new SelectionView(this);
  setWidget(selection_view_);
}


SelectionViewDocker::~SelectionViewDocker() {
  delete selection_view_;
}
