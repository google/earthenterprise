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


#include <Qt/qpainter.h>
#include <Qt/qmessagebox.h>

#include <gstSelector.h>
#include <gstSourceManager.h>

#include "DataViewTable.h"
#include "Preferences.h"


DataViewTable::DataViewTable(QWidget* parent, const char* name)
    : QTable(parent, name), sort_column_(-1), selector_(NULL) {
  setNumRows(0);
  setNumCols(0);
  setShowGrid(true);
  setSelectionMode(QTable::MultiRow);
  setSorting(true);
}

DataViewTable::~DataViewTable() {
}

void DataViewTable::setSelector(gstSelector* sel) {
  selector_ = sel;

  const SelectList &flist = selector_->pickList();
  setNumRows(flist.size());
}


void DataViewTable::columnClicked(int col) {
  if (col == sort_column_) {
    sort_ascending_ = !sort_ascending_;
  } else {
    sort_ascending_ = true;
    sort_column_ = col;
  }

  horizontalHeader()->setSortIndicator(col, sort_ascending_);
  sortColumn(sort_column_, sort_ascending_, true);
}


void DataViewTable::sortColumn(int col, bool ascending, bool wholeRows) {
  if (selector_ == NULL)
    return;

  try {
    const SelectList& flist = selector_->sortColumnOrThrow(col, ascending);

    clearSelection();

    //
    // this seems to be the only way to force a redraw
    // update() or repaint() should work?
    //
    // probably because the cells have no widgets, but
    // instead are drawn with paintCell()
    //
    setNumRows(0);
    setNumRows(flist.size());
  } catch (const khException &e) {
    QMessageBox::warning(this, "Warning",
                         tr("Error sorting column:\n") +
                         e.qwhat(),
                         QObject::tr("OK"), 0, 0, 0);
  } catch (const std::exception &e) {
    QMessageBox::warning(this, "Warning",
                         tr("Error sorting column:\n") +
                         e.what(),
                         QObject::tr("OK"), 0, 0, 0);
  } catch (...) {
    QMessageBox::warning(this, "Warning",
                         tr("Error sorting column:\n") +
                         "Unknown error",
                         QObject::tr("OK"), 0, 0, 0);
  }
}


void DataViewTable::paintCell(QPainter* painter, int row, int col,
                              const QRect& cr, bool selected, const QColorGroup& cg) {
  if (selector_ == NULL || row < 0 || col < 0)
    return;

  QRect rect(0, 0, cr.width(), cr.height());
  if (selected) {
    painter->fillRect(rect, cg.highlight());
    painter->setPen(cg.highlightedText());
  } else {
    if (row % 2) {
      painter->fillRect(rect, cg.base());
    } else {
      painter->fillRect(rect, cg.background().light(135));
    }
    painter->setPen(cg.text());
  }

  const SelectList &flist = selector_->pickList();

  // Check for row out of bounds.
  int max_row_id = flist.size() - 1;
  if (row > max_row_id)
    return;

  // determine if table has optional col for the feature id
  bool show_id = Preferences::getConfig().dataViewShowFID;

  if (show_id && col == 0) {
    painter->drawText(rect, Qt::AlignRight, QString::number(flist[row]));
    return;
  }

  if (!selector_->HasAttrib()) {
    // we don't have anny attribs, bail now
    return;
  }

  // we don't have to worry about fails attribute fetches since we are anly
  // asking for records that are already in our select list and they cannot
  // get into the select list w/o a valid record.
  gstRecordHandle rec;
  try {
    rec = theSourceManager->GetAttributeOrThrow(
        UniqueFeatureId(selector_->getSourceID(), selector_->layer(),
                        flist[row]));
  } catch (...) {
    // silently ignore. see comment before 'try'
    return;
  }

  int num_fields = static_cast<int>(rec->NumFields());
  // Check for column out of bounds.
  int max_column_id = show_id ? num_fields : num_fields - 1;
  if (col > max_column_id)
    return;

  gstValue* val = rec->Field(show_id ? col - 1 : col);
  int tf = val->IsNumber() ? Qt::AlignRight : Qt::AlignLeft;
  int rowHeight = this->rowHeight(row);
  int colWidth  = this->columnWidth(col);
  QRect bounding_rect = fontMetrics().boundingRect(val->ValueAsUnicode());

  // fontMetrics should account for the font descenders, but it
  // seems to be broken.
  // I accounted for the descenders by adding a margin of 4 around the cell
  // contents to account for errors in the fontMetrics
  // height estimation for font descenders, e.g., the tail of
  // the "y" or "p") . This seems to work for even large fonts and
  // is readable for smaller fonts.
  int added_cell_margin = 4;
  int cellWidth = bounding_rect.width() + added_cell_margin * 2;
  int cellHeight = bounding_rect.height() + added_cell_margin * 2;
  if (cellHeight > rowHeight) {
    setRowHeight(row, cellHeight);
  }
  if (cellWidth > colWidth) {
    setColumnWidth(col, cellWidth);
  }

  // When drawing, we're going to force the horizontal margin here.
  QRect text_rect(added_cell_margin, 0, cr.width() - added_cell_margin, cr.height());

  painter->drawText(text_rect, tf, val->ValueAsUnicode());
}
