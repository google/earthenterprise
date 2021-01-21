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


#include "fusion/fusionui/ThematicFilter.h"

#include <Qt/q3progressdialog.h>
#include <Qt/qapplication.h>
#include <Qt/q3listbox.h>
#include <Qt/qcolordialog.h>
#include <Qt/qpushbutton.h>
#include <Qt/qcombobox.h>
#include <Qt/q3table.h>
#include <Qt/q3header.h>
#include <Qt/q3widgetstack.h>
#include <qmessagebox.h>
#include "khException.h"
#include "fusion/fusionui/PixmapManager.h"
#include "fusion/gst/gstFormat.h"
#include "fusion/gst/gstLayer.h"
#include "fusion/gst/gstSource.h"
#include "fusion/fusionui/.idl/thematicstyles.h"
#include "fusion/autoingest/.idl/storage/MapLayerConfig.h"

using QTable = Q3Table;
using QTableItem = Q3TableItem;
using QProgressDialog = Q3ProgressDialog;
class ColorItem : public QTableItem {
 public:
  ColorItem(QTable* table, const QColor& color);

  void SetColor(const QColor& color);
  QColor GetColor() const;

 private:
  QColor color_;
};

ColorItem::ColorItem(QTable* table, const QColor& color)
    : QTableItem(table, QTableItem::Never) {
  SetColor(color);
}

void ColorItem::SetColor(const QColor& color) {
  color_ = color;
  setPixmap(ColorBox::Pixmap(
      color.red(), color.green(), color.blue(),
      color.red(), color.green(), color.blue()));
  setText(QString(
      "%1,%2,%3").arg(color.red()).arg(color.green()).arg(color.blue()));
}

QColor ColorItem::GetColor() const {
  return color_;
}

// -----------------------------------------------------------------------------

typedef std::map<gstValue, int> HistMap;
typedef std::map<gstValue, int>::iterator HistMapIterator;

class Histogram {
 public:
  class ValueCount {
   public:
    ValueCount(const char* c = nullptr) : value(QString(c)), count(0) {}
    QString value;
    int count;
  };

  Histogram() : num_elements_(0) {}

  void Insert(const gstValue& val);
  std::vector<ValueCount> ClassBreaks(int breaks);

 private:
  HistMap hist_map_;
  int num_elements_;
};

void Histogram::Insert(const gstValue& val) {
  HistMapIterator found = hist_map_.find(val);
  if (found == hist_map_.end()) {
    hist_map_.insert(std::make_pair(val, 1));
  } else {
    ++found->second;
  }
  ++num_elements_;
}

std::vector<Histogram::ValueCount> Histogram::ClassBreaks(int num_breaks) {
  std::vector<ValueCount> class_breaks;
  class_breaks.resize(num_breaks);

  // Handle the case when the number of classes is not more than the specified
  // number of breaks - just report all classes.
  if (static_cast<int>(hist_map_.size()) <= num_breaks) {
    int curr_break = 0;
    for (HistMapIterator it = hist_map_.begin();
         it != hist_map_.end(); ++it, ++curr_break) {
      class_breaks[curr_break].value = it->first.ValueAsUnicode();
      class_breaks[curr_break].count = it->second;
    }
    return class_breaks;
  }

  // TODO: improve algorithm of classification.
  // Handle the case when the number of classes is more than the specified
  // number of breaks.
  float items_per_break = static_cast<float>(num_elements_) / num_breaks;
  float total_count = 0.0;
  int current_count = 0;
  int curr_break = 1;
  for (HistMapIterator it = hist_map_.begin(); it != hist_map_.end(); ++it) {
    if ((curr_break != num_breaks) &&
        ((total_count + it->second) > (items_per_break * curr_break))) {
      class_breaks[curr_break - 1].value = it->first.ValueAsUnicode();
      class_breaks[curr_break - 1].count = current_count;
      current_count = 0;
      ++curr_break;
    }
    current_count += it->second;
    total_count += it->second;
  }
  class_breaks[num_breaks - 1].count = current_count;
  return class_breaks;
}

// -----------------------------------------------------------------------------

ThematicFilter::ThematicFilter(QWidget* parent, gstLayer* layer)
    : ThematicFilterBase(parent, 0, false, 0),
      record_header_(layer->GetSourceAttr()),
      source_(layer->GetSource()),
      src_layer_num_(layer->GetSourceLayerNum()) {
  Init();
}

ThematicFilter::ThematicFilter(QWidget* parent,
                               gstSource* source,
                               int src_layer_num)
               : ThematicFilterBase(parent, 0, false, 0),
                 record_header_(source->GetAttrDefs(src_layer_num)),
                 source_(source),
                 src_layer_num_(src_layer_num) {
  Init();
}

void ThematicFilter::Init() {
  if (record_header_->numColumns() != 0) {
    for (unsigned int col = 0; col < record_header_->numColumns(); ++col)
      field_names_box->insertItem(record_header_->Name(col));
  }

  start_color_btn->setPaletteBackgroundColor(QColor(255, 255, 255));
  end_color_btn->setPaletteBackgroundColor(QColor(255, 0, 0));

  results_table->verticalHeader()->hide();
  results_table->setLeftMargin(0);
  results_table->setNumCols(kNumCols);
  results_table->horizontalHeader()->adjustHeaderSize();

  ChangeNumClasses();
}

ThematicFilter::~ThematicFilter() {
}

bool ThematicFilter::DefineNewFilters(std::vector<DisplayRuleConfig>* configs) {
  if (exec() == QDialog::Rejected)
    return false;

  int field_num = field_names_box->currentItem();
  QString field_name = field_names_box->text(field_num);

  // fill vector with default configs
  configs->resize(0);
  configs->reserve(results_table->numRows());
  for (int row = 0; row < results_table->numRows(); ++row) {
    configs->push_back(DisplayRuleConfig());
    DisplayRuleConfig& cfg = configs->back();
    cfg.name = QString("Class # %1").arg(row + 1);
    cfg.feature.featureType = VectorDefs::PolygonZ;
    ColorItem* color_item =
        dynamic_cast<ColorItem*>(results_table->item(row, 0));
    assert(color_item != NULL);
    QRgb rgb = color_item->GetColor().rgb();
    cfg.feature.style.polyColor[0] = qRed(rgb);
    cfg.feature.style.polyColor[1] = qGreen(rgb);
    cfg.feature.style.polyColor[2] = qBlue(rgb);
    cfg.feature.style.polyColor[3] = qAlpha(rgb);
    cfg.feature.style.lineColor[0] = qRed(rgb);
    cfg.feature.style.lineColor[1] = qGreen(rgb);
    cfg.feature.style.lineColor[2] = qBlue(rgb);
    cfg.feature.style.lineColor[3] = qAlpha(rgb);

    // Assemble the filter, except for the last one that is caught as first
    // hidden row or last row.
    // The DisplayRules for a thematic filter are designed so that the last rule
    // is a "Catch All" filter, applying to all the features that were not
    // filtered by previous rules. This last filter is empty.
    if (!results_table->isRowHidden(row) &&
        row != results_table->numRows() - 1) {
      cfg.filter.match = FilterConfig::MatchAll;
      cfg.filter.selectRules.resize(1);
      cfg.filter.selectRules[0].op = SelectRuleConfig::Equal;
      cfg.filter.selectRules[0].fieldNum = field_num;
      cfg.filter.selectRules[0].rvalue = results_table->text(row, kBreakValCol);
    } else {
      // last added filter is default: match all of the following.
      break;
    }
  }

  return true;
}

bool ThematicFilter::DefineNewFilters(
  std::vector<MapDisplayRuleConfig>* configs) {
  if (exec() == QDialog::Rejected)
    return false;

  int field_num = field_names_box->currentItem();
  QString field_name = field_names_box->text(field_num);

  // fill vector with default configs
  configs->resize(0);
  configs->reserve(results_table->numRows());
  for (int row = 0; row < results_table->numRows(); ++row) {
    configs->push_back(MapDisplayRuleConfig());
    MapDisplayRuleConfig& cfg = configs->back();
    cfg.name = QString("Class # %1").arg(row + 1);
    cfg.feature.displayType = VectorDefs::PolygonZ;
    ColorItem* color_item =
        dynamic_cast<ColorItem*>(results_table->item(row, 0));
    assert(color_item != NULL);
    QRgb rgb = color_item->GetColor().rgb();
    cfg.feature.fill_color = QColor(rgb);
    cfg.feature.stroke_color = QColor(rgb);

    // Assemble the filter, except for the last one that is caught as first
    // hidden row or last row.
    // The DisplayRules for a thematic filter are designed so that the last rule
    // is a "Catch All" filter, applying to all the features that were not
    // filtered by previous rules. This last filter is empty.
    if (!results_table->isRowHidden(row) &&
        row != results_table->numRows() - 1) {
      cfg.filter.match = FilterConfig::MatchAll;
      cfg.filter.selectRules.resize(1);
      cfg.filter.selectRules[0].op = SelectRuleConfig::Equal;
      cfg.filter.selectRules[0].fieldNum = field_num;
      cfg.filter.selectRules[0].rvalue = results_table->text(row, kBreakValCol);
    } else {
      // last added filter is default: match all of the following.
      break;
    }
  }

  return true;
}

void ThematicFilter::ChooseStartColor() {
  QRgb init_color = start_color_btn->paletteBackgroundColor().rgb();
  QRgb new_color = QColorDialog::getRgba(init_color);
  if (new_color != init_color) {
    QPalette palette;
    palette.setColor(QPalette::Button, QColor(new_color));

    start_color_btn->setAutoFillBackground(true);
    start_color_btn->setPalette(palette);
    start_color_btn->setFlat(true);
    start_color_btn->update();

    RedrawColors();
  }
}

void ThematicFilter::ChooseEndColor() {
  QRgb init_color = end_color_btn->paletteBackgroundColor().rgb();
  QRgb new_color = QColorDialog::getRgba(init_color);
  if (new_color != init_color) {
    QPalette palette;
    palette.setColor(QPalette::Button, QColor(new_color));

    end_color_btn->setAutoFillBackground(true);
    end_color_btn->setPalette(palette);
    end_color_btn->setFlat(true);
    end_color_btn->update();

    RedrawColors();
  }
}

void ThematicFilter::ChangeNumClasses() {
  // setting num rows to zero clears out all the contents
  results_table->setNumRows(0);
  results_table->setNumRows(classes_combo->currentText().toInt());

  RedrawColors();
  ComputeStatistics(field_names_box->currentItem());
}

void ThematicFilter::RedrawColors() {
  int num_classes = classes_combo->currentText().toInt();

  // linear interpolation
  QColor start_color = start_color_btn->paletteBackgroundColor().rgb();
  QColor end_color = end_color_btn->paletteBackgroundColor().rgb();

  int r_step = (end_color.red() - start_color.red()) / (num_classes - 1);
  int g_step = (end_color.green() - start_color.green()) / (num_classes - 1);
  int b_step = (end_color.blue() - start_color.blue()) / (num_classes - 1);

  for (int c = 0; c < num_classes; ++c) {
    QColor clr(start_color.red() + (c * r_step),
               start_color.green() + (c * g_step),
               start_color.blue() + (c * b_step));
    results_table->setItem(c, kFieldNameCol, new ColorItem(results_table, clr));
  }
}

void ThematicFilter::SelectAttribute(QListBoxItem* item) {
  ComputeStatistics(field_names_box->currentItem());
}

void ThematicFilter::ComputeStatistics(int field) {
  // it's possible nothing has been selected yet
  if (field == -1)
    return;

  try {
    QProgressDialog progress(this, 0, true);
    progress.setCaption(kh::tr("Gathering Statistics"));
    progress.setTotalSteps(source_->NumFeatures(src_layer_num_));
    progress.setMinimumDuration(2000);

    Histogram histogram;
    unsigned int count = 0;
    for (source_->ResetReadingOrThrow(src_layer_num_);
         !source_->IsReadingDone();
         source_->IncrementReadingOrThrow()) {
      // make sure we're listening to the dialog
      if (progress.wasCanceled()) {
        break;
      }
      qApp->processEvents();

      ++count;
      gstRecordHandle curr_rec = source_->GetCurrentAttributeOrThrow();

      gstValue* val = curr_rec->Field(field);
      histogram.Insert(*val);

      progress.setProgress(count);
    }
    progress.setProgress(source_->NumFeatures(src_layer_num_));

    int num_classes = classes_combo->currentText().toInt();
    std::vector<Histogram::ValueCount> val_breaks =
        histogram.ClassBreaks(num_classes);

    QString field_name = field_names_box->text(field_names_box->currentItem());
    for (int row = 0; row < num_classes; ++row) {
      if (val_breaks[row].count) {
        if (results_table->isRowHidden(row))
          results_table->showRow(row);
      } else {
        results_table->hideRow(row);
      }
      results_table->setText(row, kBreakValCol, val_breaks[row].value);
      results_table->setText(row, kClassCountCol,
                             QString::number(val_breaks[row].count));
      results_table->adjustRow(row);
    }

    for (int c = 0; c < results_table->numCols(); ++c)
      results_table->adjustColumn(c);
  } catch(const khException &e) {
    QMessageBox::warning(this, "Warning",
                         kh::tr("Error gathering statistics:\n") +
                         e.qwhat(),
                         kh::tr("OK"), 0, 0, 0);
  } catch(const std::exception &e) {
    QMessageBox::warning(this, "Warning",
                         kh::tr("Error gathering statistics:\n") +
                         e.what(),
                         kh::tr("OK"), 0, 0, 0);
  } catch(...) {
    QMessageBox::warning(this, "Warning",
                         kh::tr("Error gathering statistics:\n") +
                         "Unknown error",
                         kh::tr("OK"), 0, 0, 0);
  }
}
