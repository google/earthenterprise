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


#include "fusion/fusionui/ProjectManager.h"

#include <unistd.h>
#include <pthread.h>
#include <vector>
#include <assert.h>
#include <functional>

#include <Qt/qapplication.h>
#include <Qt/qpushbutton.h>
#include <Qt/qcursor.h>
#include <Qt/qpainter.h>
#include <Qt/q3popupmenu.h>
using QPopupMenu = Q3PopupMenu;
#include <Qt/qprogressdialog.h>
#include <Qt/qmessagebox.h>
#include <Qt/q3header.h>
#include <Qt/qinputdialog.h>
#include <Qt/qthread.h>
#include <Qt/qstringlist.h>
#include <Qt/qevent.h>
#include <Qt/qlayout.h>
#include <Qt/q3vgroupbox.h>
using QVGroupBox = Q3VGroupBox;
#include <Qt/q3hgroupbox.h>
using QHGroupBox = Q3HGroupBox;
#include <Qt/qtooltip.h>
#include <Qt/q3mimefactory.h>
using QMimeSourceFactory = Q3MimeSourceFactory;
#include <Qt/q3listview.h>
#include <khConstants.h>
#include <khGuard.h>
#include <autoingest/Asset.h>
#include <autoingest/AssetVersion.h>
#include <autoingest/khAssetManagerProxy.h>

#include <gstRecord.h>
#include <gstFormatManager.h>
#include <gstSite.h>
#include <gstLayer.h>
#include <gstProgress.h>
#include <gstSource.h>
#include <gstFilter.h>
#include <gstSelector.h>
#include <gstFeature.h>
#include <gstSourceManager.h>
#include <gstIconManager.h>
#include <gstMisc.h>
#include <gstAssetGroup.h>
#include <gstVectorProject.h>

#include "PixmapManager.h"
#include "MainWindow.h"
#include "LayerProperties.h"
#include "LayerGroupProperties.h"
#include "SelectionRules.h"
#include "SourceFileDialog.h"
#include "GfxView.h"
#include "SiteIcons.h"
#include "AssetDrag.h"
#include "AssetChooser.h"
#include "SpatialReferenceSystem.h"
#include <fusionui/.idl/layoutpersist.h>
#include "Preferences.h"
#include "ValidLayerNames.h"
#include <Qt/q3dragobject.h>
#include <autoingest/.idl/storage/VectorProjectConfig.h>
#include <third_party/rfc_uuid/uuid.h>

using QImageDrag = Q3ImageDrag;
using QScrollView = Q3ScrollView;

namespace {
const int ConfigDispRuleEventId  = (int)QEvent::User;
const int RemoveLayerEventId     = (int)QEvent::User + 1;
const int RemoveAllLayersEventId = (int)QEvent::User + 2;
}

class ConfigDispRuleEvent : public QCustomEvent {
 public:
  ConfigDispRuleEvent(Q3ListViewItem *listItem_, int filterId_)
      : QCustomEvent(ConfigDispRuleEventId),
        listItem(listItem_), filterId(filterId_) { }

  Q3ListViewItem *listItem;
  int filterId;
};

class RemoveLayerEvent : public QCustomEvent {
 public:
  RemoveLayerEvent(Q3ListViewItem *listItem_)
      : QCustomEvent(RemoveLayerEventId),
        listItem(listItem_) { }

  Q3ListViewItem *listItem;
};

class RemoveAllLayersEvent : public QCustomEvent {
 public:
  RemoveAllLayersEvent(void) : QCustomEvent(RemoveAllLayersEventId) { }
};


static QPixmap LoadPixmap(const QString& name) {
  const QMimeSource* m = QMimeSourceFactory::defaultFactory()->data(name);
  if (!m)
    return QPixmap();
  QPixmap pix;
  QImageDrag::decode(m, pix);
  return pix;
}

// -----------------------------------------------------------------------------

class FilterItem : public Q3ListViewItem {
 public:
  FilterItem(Q3ListViewItem* parent, gstFilter* filter);
  ~FilterItem();

  virtual QString text(int) const;
  virtual int rtti() const { return FILTER; }
  virtual int compare(Q3ListViewItem* i, int col, bool ascending) const;

  int FilterId() const { return filter_id_; }

 private:
  gstFilter* GetFilter() const;
  unsigned int filter_id_;
};

// -----------------------------------------------------------------------------
using QCheckListItem = Q3CheckListItem;
class LayerItem : public QCheckListItem {
 public:
  LayerItem(Q3ListView* parent, gstLayer* l);
  LayerItem(Q3ListViewItem* parent, gstLayer* l);
  ~LayerItem();

  virtual int rtti() const { return LAYER; }
  virtual QString text(int n) const;
  virtual int compare(Q3ListViewItem* i, int col, bool ascending) const;
  virtual void stateChange(bool s);
  virtual int width(const QFontMetrics& fm, const Q3ListView* lv, int c) const;
  virtual void paintCell(QPainter* p, const QColorGroup& cg,
                         int col, int width, int align);

  void orphan();
  gstLayer* layer() const { return layer_; }
  void update();
  void UpdateFilters();

 private:
  ProjectManager* myProjectManager() const;

  gstLayer* layer_;
};

// -----------------------------------------------------------------------------

class LayerGroupItem : public QCheckListItem {
 public:
  LayerGroupItem(Q3ListView* parent, gstLayer* l);
  LayerGroupItem(Q3ListViewItem* parent, gstLayer* l);
  ~LayerGroupItem();

  virtual int rtti() const { return GROUP; }
  virtual QString text(int col) const;
  virtual int compare(Q3ListViewItem* i, int col, bool ascending) const;
  virtual void stateChange(bool s);
  virtual void paintCell(QPainter* p, const QColorGroup& cg,
                         int col, int width, int align);

  void orphan();
  gstLayer* layer() const { return layer_; }

  void update();
  bool isChildOf(Q3ListViewItem* item);

 private:
  ProjectManager* myProjectManager() const;
  gstLayer* layer_;
};



FilterItem::FilterItem(Q3ListViewItem* parent, gstFilter* filter)
    : Q3ListViewItem(parent), filter_id_(filter->Id()) {
  std::vector< unsigned int> fill_rgba, outline_rgba;
  fill_rgba.resize(4, 255);
  outline_rgba.resize(4, 255);

  if (filter->FeatureConfigs().config.enabled()) {
    fill_rgba = filter->FeatureConfigs().preview_config.poly_color_;
    outline_rgba = filter->FeatureConfigs().preview_config.line_color_;
  } else if (filter->Site().config.enabled) {
    fill_rgba = filter->Site().preview_config.normal_color_;
    outline_rgba = filter->Site().preview_config.highlight_color_;
  }

  setPixmap(0, ColorBox::Pixmap(
      fill_rgba[0], fill_rgba[1], fill_rgba[2],
      outline_rgba[0], outline_rgba[1], outline_rgba[2]));
  setDragEnabled(true);
}

FilterItem::~FilterItem() {
}

gstFilter* FilterItem::GetFilter() const {
  return static_cast<LayerItem*>(parent())->layer()->GetFilterById(filter_id_);
}

QString FilterItem::text(int col) const {
  QString display_text;
  gstFilter* filter = GetFilter();
  if (!filter) {
    // just in case we still have some races between the gstLayer's filters and
    // the GUI's FilterItems
    display_text = "-- deleted --";
  } else if (col == 0) {
    display_text = filter->Name() +
      QString("  (%1)").arg(filter->GetSelectListSize());
  } else if (col == 1 && Preferences::ExpertMode) {
    if (filter->FeatureConfigs().config.enabled())
      display_text =
          QString("F:%1 ").arg(filter->FeatureConfigs().config.style.id);
    if (filter->Site().config.enabled)
      display_text += QString("L:%1").arg(filter->Site().config.style.id);
  }
  return display_text;
}

int FilterItem::compare(Q3ListViewItem* item, int, bool) const {
  FilterItem* pi = static_cast<FilterItem*>(item);
  return (FilterId() - pi->FilterId());
}

// -----------------------------------------------------------------------------

//
// convenience routine to extract layer from LayerItem and LayerGroupItem
//
gstLayer* extractLayer(Q3ListViewItem* item) {
  int rtti = item->rtti();
  if (rtti == FILTER)
    item = item->parent();

  if (rtti == LAYER)
    return static_cast<LayerItem*>(item)->layer();

  if (rtti == GROUP)
    return static_cast<LayerGroupItem*>(item)->layer();

  assert(0);

  return NULL;
}

// -----------------------------------------------------------------------------

void LayerItem::paintCell(QPainter* p, const QColorGroup& cg,
                          int col, int width, int align)

{
  QFont f = p->font();
  f.setStrikeOut(extractLayer(this)->GetConfig().skipLayer);
  p->setFont(f);
  QCheckListItem::paintCell(p, cg, col, width, align);
}

LayerItem::LayerItem(Q3ListView* p, gstLayer* l)
    : QCheckListItem(p, l->GetShortName(), QCheckListItem::CheckBox),
      layer_(l) {
  update();
}

LayerItem::LayerItem(Q3ListViewItem* p, gstLayer* l)
    : QCheckListItem(p, l->GetShortName(), QCheckListItem::CheckBox),
      layer_(l) {
  update();
}

LayerItem::~LayerItem() {
}

void LayerItem::update() {
  QImage img;
  img = thePixmapManager->GetPixmap(gstIcon(layer_->Icon(), layer_->IconType()),
                                    PixmapManager::LayerIcon);
  setPixmap(0, QPixmap(img));
}

ProjectManager* LayerItem::myProjectManager() const {
  return dynamic_cast<ProjectManager*>(listView());
}

QString LayerItem::text(int col) const {
  if (col == 0) {
    if (Preferences::GlobalEnableAll) {
      QString id = QString("  SortID=%1 ").arg(layer_->SortId());
      QString legend = layer_->Legend().isEmpty() ? QString::null :
                       QString("Legend=%1").arg(layer_->Legend());
      return layer_->GetShortName() + id + legend;
    } else {
      return layer_->GetShortName();
    }
  } else if (col == 1 && Preferences::ExpertMode) {
    return QString().setNum(layer_->Id());
  }
  return QString::null;
}

int LayerItem::compare(Q3ListViewItem* item, int, bool) const {
  return layer()->SortId() - extractLayer(item)->SortId();
}

// make this item parent-less
void LayerItem::orphan() {
  Q3ListViewItem* p = parent();
  // reset the sortId
  this->layer_->SetSortId(myProjectManager()->numLayers());

  if (p) {
    p->takeItem(this);
    // close parent if it is now empty
    if (p->childCount() == 0)
      p->setOpen(false);
  } else {
    listView()->takeItem(this);
  }
}

int LayerItem::width(const QFontMetrics& fm, const Q3ListView* lv, int c) const {
  if (!parent()) {
    return QCheckListItem::width(fm, lv, c);
  } else {
    return Q3ListViewItem::width(fm, lv, c);
  }
}

void LayerItem::stateChange(bool state) {
  myProjectManager()->cancelDrag();

  if (state) {
    // make sure we have a valid source
    gstSource* src = layer_->GetSource();
    if (src == NULL) {
      QMessageBox::critical(myProjectManager(), "Error" ,
        kh::tr("Unable to preview this layer due to invalid source"),
        kh::tr("OK"), 0, 0, 0);
      setOn(false);
      return;
    }

    // verify that queries are complete
    if (!layer_->QueryComplete()) {
      if (!myProjectManager()->applyQueries(layer_)) {
        setOn(false);
        return;
      }
    }
  }

  layer_->SetEnabled(state);
  //MainWindow::self->updateGfx();
  //emit redrawPreview();
  myProjectManager()->forcePreviewRedraw();
  setOn(state);
  listView()->repaint();
}

void LayerItem::UpdateFilters() {
  // clear out any children
  while (firstChild() != NULL) {
    delete firstChild();
  }

  if (!layer_->isGroup()) {
    for (unsigned int kk = 0; kk < layer_->NumFilters(); ++kk) {
      gstFilter* filter = layer_->GetFilterById(kk);
      new FilterItem(this, filter);
    }
  }
}

// -----------------------------------------------------------------------------

void LayerGroupItem::paintCell(QPainter* p, const QColorGroup& cg,
                          int col, int width, int align)

{
  QFont f = p->font();
  f.setStrikeOut(extractLayer(this)->GetConfig().skipLayer);
  p->setFont(f);
  QCheckListItem::paintCell(p, cg, col, width, align);
}

LayerGroupItem::LayerGroupItem(Q3ListView* parent, gstLayer* l)
  : QCheckListItem(parent, l->GetShortName(), QCheckListItem::CheckBox),
    layer_(l) {
  setOpen(false);
  setExpandable(false);
  setDragEnabled(true);
  QImage img;
  img = thePixmapManager->GetPixmap(gstIcon(layer_->Icon(), layer_->IconType()),
                                    PixmapManager::LayerIcon);
  setPixmap(0, QPixmap(img));
}

LayerGroupItem::LayerGroupItem(Q3ListViewItem* parent, gstLayer* l)
  : QCheckListItem(parent, l->GetShortName(), QCheckListItem::CheckBox),
    layer_(l) {
  setOpen(false);
  setExpandable(false);
  setDragEnabled(true);
  QImage img;
  img = thePixmapManager->GetPixmap(gstIcon(layer_->Icon(), layer_->IconType()),
                                    PixmapManager::LayerIcon);
  setPixmap(0, QPixmap(img));
}

LayerGroupItem::~LayerGroupItem() {
}


// make this item parent-less
void LayerGroupItem::orphan() {
  Q3ListViewItem* p = parent();
  // reset the sortId
  this->layer_->SetSortId(myProjectManager()->numLayers());

  if (p) {
    p->takeItem(this);
    // close parent if it is now empty
    if (p->childCount() == 0)
      p->setOpen(false);
  } else {
    listView()->takeItem(this);
  }
}

void LayerGroupItem::update() {
  QImage img;
  img = thePixmapManager->GetPixmap(gstIcon(layer_->Icon(), layer_->IconType()),
                                    PixmapManager::LayerIcon);
  setPixmap(0, QPixmap(img));
}

bool LayerGroupItem::isChildOf(Q3ListViewItem* item) {
  Q3ListViewItem* p = parent();
  while (p) {
    if (item == p)
      return true;
    p = p->parent();
  }
  return false;
}

QString LayerGroupItem::text(int col) const {
  if (col == 0) {
    if (Preferences::GlobalEnableAll) {
      QString id = QString("  SortID=%1 ").arg(layer_->SortId());
      QString legend = layer_->Legend().isEmpty() ? QString::null :
                       QString("Legend=%1").arg(layer_->Legend());
      return layer_->GetShortName() + id + legend;
    } else {
      return layer_->GetShortName();
    }
  } else if (col == 1 && Preferences::ExpertMode) {
    return QString().setNum(layer_->Id());
  }

  return QString::null;
}

int LayerGroupItem::compare(Q3ListViewItem* item, int, bool) const {
  return layer()->SortId() - extractLayer(item)->SortId();
}

ProjectManager* LayerGroupItem::myProjectManager() const {
  return dynamic_cast<ProjectManager*>(listView());
}

void LayerGroupItem::stateChange(bool state) {
  myProjectManager()->cancelDrag();
  LayerItem* layerItem;
  LayerGroupItem* layerGroupItem;
  Q3ListViewItem* item = firstChild();
  while (item) {
    if (item->rtti() == LAYER) {
      layerItem = static_cast<LayerItem*>(item);
      layerItem->stateChange(state);
    } else if (item->rtti() == GROUP) {
      layerGroupItem = static_cast<LayerGroupItem*>(item);
      layerGroupItem->stateChange(state);
    }
    item = item->nextSibling();
  }
  setOn(state);
  listView()->repaint();
}


// -----------------------------------------------------------------------------

class LayerDrag : public QTextDrag {
 public:
  LayerDrag(const QString& text, QWidget* parent);
  static bool canDecode(QMimeSource* e);
};

LayerDrag::LayerDrag(const QString& str, QWidget* parent)
    : QTextDrag(str, parent, 0) {
  setSubtype("KeyholeLayer");
}

bool LayerDrag::canDecode(QMimeSource* e) {
  return e->provides("text/KeyholeLayer");
}

// -----------------------------------------------------------------------------


ProjectManager::ProjectManager(QWidget* parent, const char* name, Type t)
  : Q3ListView(parent, name, 0),
      type_(t),
      mouse_pressed_(false),
      drag_layer_(0),
      old_current_(0),
      show_max_count_reached_message_(true)
{
  setAcceptDrops(true);
  viewport()->setAcceptDrops(true);
  setResizePolicy(QScrollView::Manual);

  if (Preferences::ExpertMode) {
    addColumn("Name");
    addColumn("ID");
    setColumnWidthMode(1, Q3ListView::Maximum);
    header()->setStretchEnabled(true, 0);
    header()->setStretchEnabled(false, 1);
    header()->show();
  } else {
    addColumn(QString::null);
    header()->setStretchEnabled(true, 0);
    header()->hide();
  }

  connect(this, SIGNAL(doubleClicked(Q3ListViewItem*)),
          this, SLOT(itemDoubleClicked(Q3ListViewItem *)));

  connect(this, SIGNAL(contextMenuRequested(Q3ListViewItem*, const QPoint&, int)),
          this, SLOT(contextMenu(Q3ListViewItem*, const QPoint&, int)));

  connect(this, SIGNAL(pressed(Q3ListViewItem*)),
          this, SLOT(pressed(Q3ListViewItem*)));
  connect(this, SIGNAL(selectionChanged(Q3ListViewItem*)),
          this, SLOT(selectionChanged(Q3ListViewItem*)));

  connect(GfxView::instance, SIGNAL(drawVectors(const gstDrawState&)),
          this, SLOT(DrawFeatures(const gstDrawState&)));

  connect(GfxView::instance, SIGNAL(drawLabels(QPainter*, const gstDrawState&)),
          this, SLOT(DrawLabels(QPainter*, const gstDrawState&)));

  connect(this, SIGNAL(redrawPreview()),
          GfxView::instance, SLOT(updateGL()));

  connect(this, SIGNAL(zoomToSource(const gstBBox&)),
          GfxView::instance, SLOT(zoomToBox(const gstBBox&)));

  project_ = new gstVectorProject(std::string());
}

ProjectManager::~ProjectManager() {
  if (project_)
    delete project_;
}

void ProjectManager::forcePreviewRedraw() {
  // allow the 'max features drawn' message be drawn again
  show_max_count_reached_message_ = true;
  emit redrawPreview();
}

unsigned int ProjectManager::numLayers() {
  return project_->layers_.size();
}

// this signal is only needed for de-select
// selectionChanged() below will get called with valid items
// but pressed() won't get called with keyboard actions (arrows)
void ProjectManager::pressed(Q3ListViewItem* item) {
  if (item == NULL)
    selectItem(NULL);
}

void ProjectManager::selectionChanged(Q3ListViewItem* item) {
  selectItem(item);
}


void ProjectManager::selectItem(Q3ListViewItem* item) {
  assert(project_ != NULL);

  //
  // clicks update the selection view
  //
  gstLayer* layer = getSelectedLayer();
  // ignore layer groups
  if (layer && !layer->isGroup()) {
    emit selectionView(layer->GetSelector());
    emit selectLayer(layer);
  } else {
    emit selectionView(NULL);
    emit selectLayer(NULL);
  }

  if (item == NULL || item->rtti() == FILTER) {
    updateButtons(NULL);
  } else {
    updateButtons(item);
  }

  emit redrawPreview();
}


bool ProjectManager::canRaise(Q3ListViewItem* item) {
  if (item->rtti() == FILTER)
    item = item->parent();

  Q3ListViewItem* firstChild;
  if (item->parent() == 0) {
    firstChild = this->firstChild();
  } else {
    firstChild = item->parent()->firstChild();
  }

  return (item != firstChild);
}


bool ProjectManager::canLower(Q3ListViewItem* item) {
  if (item->rtti() == FILTER)
    item = item->parent();

  return (item->nextSibling() != 0);
}

//
// update moveup, movedown, and delete buttons
//
void ProjectManager::updateButtons(Q3ListViewItem* item) {
  if (item ==  NULL) {
    emit layerStateChange(0, 0, 0);
  } else {
    emit layerStateChange(canRaise(item), canLower(item), 1);
  }
}

QColor getRandomColor() {
  // try to get a semi-random color that doesn't repeat very often
  static int s0 = static_cast<int>(drand48() * 255.0);
  s0 += 35;
  double hue = static_cast<double>(s0 % 256) / 256.0;
  return QColor(static_cast<int>(hue * 255), 255, 255, QColor::Hsv);
}

LayerConfig
ProjectManager::MakeDefaultLayerConfig(const QString &name,
                                       gstPrimType primType,
                                       const std::string &assetRef) const {
  LayerConfig cfg;

  cfg.assetRef = assetRef;
  cfg.defaultLocale.ClearDefaultFlags();
  cfg.defaultLocale.name_ = name;
  cfg.defaultLocale.icon_ = IconReference(IconReference::Internal,
                                          kDefaultIconName.c_str());

  // Add a default display rule and get some refs to the internal pieces
  // so we can poke in some override values below
  cfg.displayRules.push_back(DisplayRuleConfig(tr("default select all")));
  DisplayRuleConfig &dispRule = cfg.displayRules[0];
  FeatureConfig &feature      = dispRule.feature;
  SiteConfig &site            = dispRule.site;

  // preview project type overrides the default min level
  // so that features can be seen from the global view
  if (getType() == Preview) {
    feature.minLevel = 0;
  }

  // Initialize feature config based on primitive type expected for this layer
  feature.featureType = VectorDefs::SuggestedFeatureType(primType);
  if (primType == gstStreet) {
    feature.drawAsRoads = true;
  }
  if (primType == gstPolygon25D) {
    feature.style.extrude = true;
    feature.style.altitudeMode = StyleConfig::Relative;
  }
  QColor rnd = getRandomColor();
  feature.style.lineColor = makevec4< unsigned int> (rnd.red(), rnd.green(),
                                           rnd.blue(), 255);

  // Initialize site config based on primitive type expected for this layer
  if (primType == gstPoint) {
    site.enabled = true;
    site.enablePopup = true;
  }

  return cfg;
}

LayerConfig
ProjectManager::MakeDefaultGroupConfig(const QString &name) const {
  LayerConfig cfg;

  cfg.defaultLocale.ClearDefaultFlags();
  cfg.defaultLocale.name_ = name;
  cfg.defaultLocale.icon_ = IconReference(IconReference::Internal,
                                          kDefaultIconName.c_str());

  return cfg;
}

void ProjectManager::addLayerGroup() {
  // loop over the dialog until we get a valid layer name,
  // or cancel is pressed
  QString name;
  while (true) {
    bool ok = false;
    name = QInputDialog::getText(tr("New Layer Group"),
                                 tr("Name:"),
                                 QLineEdit::Normal, name,
                                 &ok, this);

    if (!ok || name.isEmpty())
      break;

    if (!ValidLayerName(name)) {
      QMessageBox::critical(
        this, tr("Error"),
        InvalidLayerNameMessage,
        tr("OK"), 0, 0, 0);
      continue;
    }

    // build a config for this new LayerGroup
    LayerConfig grpConfig =
      MakeDefaultGroupConfig(name);

    gstLayer* grp = new gstLayer(project_,
                                 0 /*src*/, 0 /*srcLayerNum*/,
                                 grpConfig);
    grp->SetSortId(project_->layers_.size());

    LayerGroupItem* item = new LayerGroupItem(this, grp);
    // Check that the current name is unique. Do it again if it isn't.
    if (!LayerNameSanityCheck(item, name)) {
      delete item;
      grp->unref();
      continue;
    }
    item->setOpen(false);
    RefreshLayerList(true, true);
    break;
  }
}

void ProjectManager::itemDoubleClicked(Q3ListViewItem* item) {
  if (item == NULL || item->rtti() != FILTER)
    return;
  int idx = static_cast<FilterItem*>(item)->FilterId();

  // we can't call configureDisplayRules directly since it may delete this
  // FilterItem. And the Qt routine that called us will access it again
  // after we return. Post an event back to myself with the layerItem
  // (as Q3ListViewItem) and the filter id
  QApplication::postEvent(this,
                          new ConfigDispRuleEvent(item->parent(), idx));
}

void ProjectManager::configureDisplayRules(LayerItem* item, int id) {
  gstLayer* layer = item->layer();

  gstHeaderHandle hdr = layer->GetSourceAttr();

  LayerConfig oldcfg = layer->GetConfig();
  SelectionRules selection_rules(this, oldcfg, layer, hdr, id);

  if (selection_rules.exec() == QDialog::Accepted) {
    bool origOnState = item->isOn();
    LayerConfig newcfg = selection_rules.getConfig();
    if (newcfg != oldcfg) {
      layer->SetConfig(newcfg);
      item->UpdateFilters();
      if (origOnState) {
        item->setOn(false);
        if (applyQueries(layer))
          item->setOn(true);
      }
    }
  }
}

void SetSkipLayer(gstLayer *layer, bool skip) {
  LayerConfig config = layer->GetConfig();
  config.skipLayer = skip;
  layer->SetConfig(config);
}

template <class Func>
void ProcessGroupAndChildren(Q3ListViewItem *item, const Func &func) {
  if (item->rtti() == LAYER) {
    func(extractLayer(item));
    item->repaint();
  } else if (item->rtti() == GROUP) {
    func(extractLayer(item));
    item->repaint();
    Q3ListViewItem *child = item->firstChild();
    while (child) {
      ProcessGroupAndChildren(child, func);
      child = child->nextSibling();
    }
  }
}

// Do a check for unique layer name in the current project.
// If it is not unique, warn the user and return false.
bool ProjectManager::LayerNameSanityCheck(Q3ListViewItem *item, const QString& name) {
  bool ok_to_proceed_with_name = true;

  if (FindLayerNameAmongSiblings(item, name)) {
    QMessageBox::critical(
      this, tr("Error"),
      QString(tr("This layer name already exists in the current level.\n"
                 "Please choose a different name.")),
      tr("OK"), 0, 0, 0);
    ok_to_proceed_with_name = false;
  }
  return ok_to_proceed_with_name;
}

// For the release of the Google Earth Javascript API (Tumbler),
// internal vector production will be using UUIDs for
// client API purposes (selecting layers by UUID).
// These UUIDs are immutable and universally unique to
// the specific layer. The UUIDs are not localized.
// This API capability is not yet planned for the enterprise client and
// is hidden from enterprise Fusion customers.
// It wouldn't be harmful for them to maintain unique/immutable layer names.
// These layer names must be unique and within the DbRoot.
// We do basic checks for non-empty UUIDs, and uniqueness within this project.
// This checks the old and new UUID, if the new UUID is not unique, it will present the
// user with options.
// Return true if it's ok, false if the user needs to try again to enter a valid UUID.
// The new_uuid is modified on return if deemed necessary
// (possibly reverting it or creating a new one) and should be used as the valid value of the uuid.
bool ProjectManager::UuidSanityCheck(Q3ListViewItem* item,
                                         const std::string& old_uuid, std::string& new_uuid)
{
  bool ok_to_proceed_with_uuid = true;

  if (Preferences::GoogleInternal) {
    if (new_uuid == "") {
      if (old_uuid != "") {
        // Revert the previous setting (they didn't mean to change it).
        new_uuid = old_uuid;
        QMessageBox::critical(
            this, tr("Error"),
            tr("The Universal Unique ID for this layer is empty.\n"
               "Please choose a different ID."),
            tr("OK"), 0, 0, 0);
        ok_to_proceed_with_uuid = false;
      } else {
        QMessageBox::critical(
            this, tr("Error"),
            tr("The Universal Unique ID for this layer is empty.\n"
               "A UUID has been created for you automatically"),
            tr("OK"), 0, 0, 0);
        new_uuid = create_uuid_string();
      }
    } else if (old_uuid != "" && old_uuid != new_uuid) {
      if (FindUuid(firstChild(), item, new_uuid.c_str())) {
        QMessageBox::critical(
          this, tr("Error"),
          tr("This Universal Unique ID  already exists for another asset.\n"
             "Copying the UUID of an existing layer is not allowed.\n"
             "The UUID has been reverted to its prior value."),
          tr("OK"), 0, 0, 0);
        // Revert the previous setting (they didn't mean to change it).
        new_uuid = old_uuid;
        ok_to_proceed_with_uuid = false;
      } else {
        int result = QMessageBox::warning(
            this, tr("Warning"),
            tr("You have changed the Universal Unique ID for this layer.\n"
               "This name is intended to be permanent for all future versions\n"
               "of this layer.\n"
               "Are you sure you wish to make this change?\n"
               "Press \"No\" to revert to the original ID."),
            tr("Yes"), tr("No"), QString::null, 1, 1);
        if (result == 1) {
          // Revert the previous setting (they didn't mean to change it).
          new_uuid = old_uuid;
        }
      }
    } else if (FindUuid(firstChild(), item, new_uuid.c_str())) {
      QMessageBox::critical(
          this, tr("Error"),
          tr("This Universal Unique ID  already exists for another asset.\n"
             "Please choose a different ID."),
          tr("OK"), 0, 0, 0);
      ok_to_proceed_with_uuid = false;
    }
  }

  return ok_to_proceed_with_uuid;
}

void ProjectManager::contextMenu(Q3ListViewItem* item, const QPoint& pos, int) {
  if (item && !item->isEnabled())
    return;

  enum { ZOOM_TO_LAYER,
         MOVE_LAYER_UP, MOVE_LAYER_DOWN, MAKE_TOP_LEVEL,
         CONFIG_RULES, EXPORT_TEMPLATE, IMPORT_TEMPLATE,
         LAYER_PROPERTIES, REMOVE_LAYER, REMOVE_LAYER_GROUP,
         REMOVE_ALL_LAYERS, LAYER_GROUP_PROPERTIES,
         SKIP_LAYER, NO_SKIP_LAYER, FILE_OPEN
  };

  // It's our resonsibility to delete these, even after we add them to a menu
  // but we want to delete them _after_ the menu has been deleted. So put the
  // objects before the menu object.
  khDeleteGuard<QPopupMenu> addMenu;
  khDeleteGuard<QPopupMenu> removeMenu;

  QPopupMenu menu(this);

  int item_type = (item == NULL) ? 0 : item->rtti();
  int id;
  switch (item_type) {
    case 0:
      menu.insertItem(QIconSet(LoadPixmap("fileopen_1.png")),
                                          tr("&Open..."), FILE_OPEN);
      menu.insertSeparator();
      id = menu.insertItem(tr("Remove All Layers"), REMOVE_ALL_LAYERS);
      if (childCount() == 0)
        menu.setItemEnabled(id, false);
      break;
    case LAYER:
      menu.insertItem(tr("&Zoom to Layer"), ZOOM_TO_LAYER);
      if (getType() == Project) {
        id = menu.insertItem(tr("Move Layer &Up"), MOVE_LAYER_UP);
        if (!canRaise(item))
          menu.setItemEnabled(id, false);
        id = menu.insertItem(tr("Move Layer &Down"), MOVE_LAYER_DOWN);
        if (!canLower(item))
          menu.setItemEnabled(id, false);
        menu.insertSeparator();
      }
      if (!extractLayer(item)->isTopLevel()) {
        menu.insertItem(tr("&Take Layer out of Group"), MAKE_TOP_LEVEL);
        menu.insertSeparator();
      }
      menu.insertItem(tr("&Configure Display Rules..."), CONFIG_RULES);
      menu.insertItem(tr("&Export Configuration as Template..."),
                      EXPORT_TEMPLATE);
      menu.insertItem(tr("&Import Configuration from Template..."),
                      IMPORT_TEMPLATE);
      menu.insertSeparator();
      if (getType() == Project) {
        if (extractLayer(item)->GetConfig().skipLayer) {
          menu.insertItem(tr("&Don't Skip Layer"), NO_SKIP_LAYER);
        } else {
          menu.insertItem(tr("&Skip Layer"), SKIP_LAYER);
        }
      }
      menu.insertItem(tr("&Remove Layer"), REMOVE_LAYER);
      if (getType() == Preview)
        menu.insertItem(tr("Remove All Layers"), REMOVE_ALL_LAYERS);
      if (getType() == Project) {
        menu.insertSeparator();
        menu.insertItem(tr("Layer &Properties"), LAYER_PROPERTIES);
      }
      break;

    case FILTER:
      menu.insertItem(tr("&Configure Display Rules..."), CONFIG_RULES);
      break;

    case GROUP:
      id = menu.insertItem(tr("Move Layer Group &Up"), MOVE_LAYER_UP);
      if (!canRaise(item))
        menu.setItemEnabled(id, false);
      id = menu.insertItem(tr("Move Layer Group &Down"), MOVE_LAYER_DOWN);
      if (!canLower(item))
        menu.setItemEnabled(id, false);
      menu.insertSeparator();
      menu.insertItem(tr("&Export Configuration as Template..."), EXPORT_TEMPLATE);
      menu.insertItem(tr("&Import Configuration from Template..."), IMPORT_TEMPLATE);
      menu.insertSeparator();
      if (extractLayer(item)->GetConfig().skipLayer) {
        menu.insertItem(tr("&Don't Skip Group"), NO_SKIP_LAYER);
      } else {
        menu.insertItem(tr("&Skip Group"), SKIP_LAYER);
      }
      menu.insertItem(tr("&Remove Layer Group"), REMOVE_LAYER_GROUP);
      menu.insertSeparator();
      menu.insertItem(tr("Layer Group &Properties"), LAYER_GROUP_PROPERTIES);
      break;
  }

  int menuId = menu.exec(pos);
  if (menuId == -1) return;

  switch (menuId) {
    case EXPORT_TEMPLATE:
      exportDisplayTemplate(extractLayer(item));
      break;

    case IMPORT_TEMPLATE:
      if (item->rtti() == FILTER)
        item = item->parent();
      importDisplayTemplate(static_cast<QCheckListItem*>(item));
      break;

    case MOVE_LAYER_UP:
      moveLayerUp(item);
      break;

    case MOVE_LAYER_DOWN:
      moveLayerDown(item);
      break;

    case MAKE_TOP_LEVEL:
      makeTopLevel(static_cast<LayerItem*>(item));
      break;

    case LAYER_PROPERTIES:
      {
        gstLayer* layer = (extractLayer(item));

        LayerProperties prop(this, layer->GetConfig(), layer);
        bool verified = false;

        while (!verified) {
          if (prop.exec() != QDialog::Accepted)
            break;

          // get the old a new config for comparison purposes
          LayerConfig oldcfg = layer->GetConfig();
          LayerConfig newcfg = prop.GetConfig();

          // Check that the current name is unique.
          if (!LayerNameSanityCheck(item, newcfg.defaultLocale.name_)) {
            continue;
          }

          // Check for a unique UUID. May need to query user.
          std::string new_uuid = newcfg.asset_uuid_;
          bool ok_to_proceed = UuidSanityCheck(item, oldcfg.asset_uuid_, new_uuid);
          // Set the uuidEdit (in case it's changed).
          newcfg.asset_uuid_ = new_uuid;
          prop.uuidEdit->setText(new_uuid.c_str());
          // If it's not unique, we need to continue and let the user try again.
          if (!ok_to_proceed) continue;

          LayerItem* layer_item = static_cast<LayerItem*>(item);

          if (oldcfg != newcfg) {
            bool origOnState = layer_item->isOn();

            // update the config
            layer->SetConfig(newcfg);

            if (QueryConfig(oldcfg, layer->GetExternalContextScripts()) !=
                QueryConfig(newcfg, layer->GetExternalContextScripts())) {
              // the filters have changed - due to a script change
              // we need to recreate all my children
              layer_item->UpdateFilters();
              if (origOnState) {
                layer_item->setOn(false);
                if (applyQueries(layer)) {
                  layer_item->setOn(true);
                }
              }
            } else {
              // Only the non-filter stuff changed, just update this LayerItem
              layer_item->update();
            }
          }

          verified = true;
        }

      }
      break;

    case LAYER_GROUP_PROPERTIES:
      {
        gstLayer* layer = extractLayer(item);

        LayerGroupProperties prop(this, layer->GetConfig());
        bool verified = false;
        while (!verified) {
          if (prop.exec() != QDialog::Accepted)
            break;

          // Get the old a new config for comparison purposes.
          LayerConfig oldcfg = layer->GetConfig();
          LayerConfig newcfg = prop.GetConfig();

          // Check that the current name is unique.
          if (!LayerNameSanityCheck(item, newcfg.defaultLocale.name_)) {
            continue;
          }

          // Check for a unique UUID. May need to query user.
          std::string new_uuid = newcfg.asset_uuid_;
          bool ok_to_proceed = UuidSanityCheck(item, oldcfg.asset_uuid_, new_uuid);
          // Set the uuidEdit (in case it's changed).
          prop.uuidEdit->setText(new_uuid.c_str());
          newcfg.asset_uuid_ = new_uuid;
          // If it's not unique, we need to continue and let the user try again.
          if (!ok_to_proceed) continue;

          layer->SetConfig(newcfg);

          verified = true;
          static_cast<LayerGroupItem*>(item)->update();
          RefreshLayerList(true, false);
        }
      }
      break;

    case REMOVE_LAYER:
    case REMOVE_LAYER_GROUP:
      {
        // we can't call removeLayer directly since it will
        // delete this item. The Qt routine that called us will
        // access it again after we return. Post an event back to myself
        // with the item
        QApplication::postEvent(this, new RemoveLayerEvent(item));
      }
      break;

    case SKIP_LAYER:
      ProcessGroupAndChildren(item,
                              std::bind2nd(std::ptr_fun(SetSkipLayer),
                                           true));
      break;

    case NO_SKIP_LAYER:
      ProcessGroupAndChildren(item,
                              std::bind2nd(std::ptr_fun(SetSkipLayer),
                                           false));
      break;

    case REMOVE_ALL_LAYERS:
      {
        // we can't call removeAllLayers directly since it will
        // delete this item. The Qt routine that called us will
        // access it again after we return. Post an event back to myself
        // with the item
        QApplication::postEvent(this, new RemoveAllLayersEvent());
      }
      break;

    case CONFIG_RULES:
      {
        int idx = 0;

        if (item->rtti() == FILTER) {
          idx = static_cast<FilterItem*>(item)->FilterId();

          // we can't call configureDisplayRules directly since it may
          // delete this FilterItem. And the Qt routine that called us will
          // access it again after we return. Post an event back to myself
          // with the layerItem (as Q3ListViewItem) and the filter id
          QApplication::postEvent(this,
                                  new ConfigDispRuleEvent(item->parent(),idx));
          break;
        }
        if (item->rtti() == LAYER)
          configureDisplayRules(static_cast<LayerItem*>(item), idx);
      }
      break;

    case ZOOM_TO_LAYER:
      if (!extractLayer(item)->isGroup()) {
        gstSource* gst_source = extractLayer(item)->GetSource();
        // in case of previewing vector features need renormalization.
        if (GfxView::instance->state()->IsMercatorPreview()) {
          gstBBox box = gst_source->BoundingBox(0);
          MercatorProjection::NormalizeToMercatorFromFlat(&box.n, &box.s);
          emit zoomToSource(box);
        } else {
          emit zoomToSource(gst_source->BoundingBox(0));
        }
      }
      break;

    case FILE_OPEN:
      FileOpen();
      break;
  }
}

void ProjectManager::exportDisplayTemplate(gstLayer* layer) {
  Q3FileDialog fd(this);
  fd.setCaption(tr("Export Template"));
  fd.setMode(Q3FileDialog::AnyFile);
  fd.addFilter(tr("Fusion Template File (*.khdsp)"));

  //
  // restore previous layout
  //
  TemplateImportLayout layout;
  if (khExists(Preferences::filepath("templateimport.layout").latin1())) {
    if (layout.Load(Preferences::filepath("templateimport.layout").latin1())) {
      fd.resize(layout.width, layout.height);
      fd.setDir(layout.lastDirectory);
    }
  }

  if (fd.exec() != QDialog::Accepted)
    return;

  // make sure this layer's legend has been updated before writing the
  // template file
  RefreshLayerList(true, true);

  //
  // save layout
  //
  layout.width = fd.width();
  layout.height = fd.height();
  layout.lastDirectory = fd.dirPath();
  layout.Save(Preferences::filepath("templateimport.layout").latin1());

  QString khdsp(fd.selectedFile());
  if (!khdsp.endsWith(".khdsp"))
    khdsp += QString(".khdsp");

  LayerConfig cfg = layer->GetConfig();
  if (!cfg.Save(khdsp.latin1())) {
    QMessageBox::critical(this, "Error",
                          tr("Unable to save file:") + khdsp ,
                          tr("OK"), 0, 0, 0);
  }
}

class LayerTemplateLoadDialog : public OpenWithHistoryDialog {
 public:
  LayerTemplateLoadDialog(QWidget *parent, bool groupMode)
    : OpenWithHistoryDialog(parent, tr("Import Template"),
                            "vectorlayertemplatehistory.xml") {
    addFilter(tr("Fusion Template File (*.khdsp)"));

    // provide checkboxes to specify what to apply
    apply_display_rules_check = new QCheckBox(tr("Apply display rules"), this);
    apply_display_rules_check->setChecked(!groupMode);
    apply_legend_check = new QCheckBox(tr("Apply legend settings"), this);
    apply_legend_check->setChecked(groupMode);

    addWidgets(NULL, apply_display_rules_check, NULL);
    addWidgets(NULL, apply_legend_check, NULL);

    if (groupMode) {
      apply_display_rules_check->hide();
    }
  }

  bool ApplyDisplayRules(void) const {
    return apply_display_rules_check->isChecked();
  }
  bool ApplyLegend(void) const {
    return apply_legend_check->isChecked();
  }

 private:
  QCheckBox* apply_display_rules_check;
  QCheckBox* apply_legend_check;
};

void ProjectManager::importDisplayTemplate(QCheckListItem* item) {
  LayerTemplateLoadDialog fd(this, (item->rtti() == GROUP));

  //
  // restore previous layout
  //
  TemplateImportLayout layout;
  if (khExists(Preferences::filepath("templateimport.layout").latin1())) {
    if (layout.Load(Preferences::filepath("templateimport.layout").latin1())) {
      fd.resize(layout.width, layout.height);
      fd.setDir(layout.lastDirectory);
    }
  }

  if (fd.exec() != QDialog::Accepted)
    return;

  //
  // save layout
  //
  layout.width = fd.width();
  layout.height = fd.height();
  layout.lastDirectory = fd.dirPath();
  layout.Save(Preferences::filepath("templateimport.layout").latin1());

  LayerConfig template_cfg;
  if (!template_cfg.Load(fd.selectedFile().latin1())) {
    QMessageBox::critical(this, tr("Error"),
                          QString(tr("Unable to open Display Rule Template file:\n\n%1")).
                          arg(fd.selectedFile()),
                          tr("OK"), 0, 0, 0);
    return;
  }

  QString error_message;
  bool status = template_cfg.ValidateIconPresence(&error_message);
  if (!status) {
    QMessageBox::critical(this, tr("Error"),
                          tr("The following problems were encountered when applying template:\n") +
                          error_message +
                          tr("\nThis layer will fail to build until these problems are fixed."),
                          tr("OK"), 0, 0, 0);
  }

  // force this layer off.  user will have to turn back on manually
  // we don't want to trigger a lengthy query automatically
  item->setOn(false);
  repaint();

  // apply the template to a copy and save it back into the gstLayer
  // make a copy of the config that we can modify
  gstLayer *layer = extractLayer(item);
  LayerConfig tmp_config = layer->GetConfig();
  tmp_config.ApplyTemplate(template_cfg,
                          fd.ApplyDisplayRules(),
                          fd.ApplyLegend());

  layer->SetConfig(tmp_config);

  if (item->rtti() == LAYER) {
    static_cast<LayerItem*>(item)->UpdateFilters();
  }
}


void ProjectManager::moveLayerUp(Q3ListViewItem* item) {
  if (item == NULL)
    item = selectedItem();
  if (item == NULL)
    return;

  Q3ListViewItem* next;
  if (item->parent() != 0)
    next = item->parent()->firstChild();
  else
    next = this->firstChild();

  while ((next->nextSibling() != item) &&
         (next->nextSibling() != 0))
    next = next->nextSibling();

  if (next != 0) {
    gstLayer* nextLayer = extractLayer(next);
    gstLayer* thisLayer = extractLayer(item);
    unsigned int tempSortId = nextLayer->SortId();
    nextLayer->SetSortId(thisLayer->SortId());
    thisLayer->SetSortId(tempSortId);

    next->moveItem(item);
  }

  RefreshLayerList(false, true);
  updateButtons(item);
}

void ProjectManager::moveLayerDown(Q3ListViewItem* item) {
  if (item == NULL)
    item = selectedItem();
  if (item == NULL)
    return;

  if (item->nextSibling() != 0) {
    gstLayer* nextLayer = extractLayer(item->nextSibling());
    gstLayer* thisLayer = extractLayer(item);
    unsigned int tempSortId = nextLayer->SortId();
    nextLayer->SetSortId(thisLayer->SortId());
    thisLayer->SetSortId(tempSortId);
    item->moveItem(item->nextSibling());
  }

  RefreshLayerList(false, true);
  updateButtons(item);
}

void ProjectManager::makeTopLevel(LayerItem* item) {
  item->orphan();
  insertItem(item);
  item->update();
  RefreshLayerList(true, true);
  updateButtons(item);
}


void ProjectManager::FileOpen() {
  SourceFileDialog* dialog = SourceFileDialog::self();

  if (dialog->exec() != QDialog::Accepted)
    return;

  QStringList files = dialog->selectedFiles();
  QString codec = dialog->getCodec();
  QStringList::Iterator it;

  QProgressDialog progress(tr("Please wait while loading..."),
                           tr("Abort load"), 0, 100, this);
  progress.setCaption(tr("Loading"));

  for (it = files.begin(); it != files.end(); ++it) {
    QFileInfo fi(*it);

    QString name = (fi.fileName() == kHeaderXmlFile.c_str()) ?
                   fi.dirPath(true) : fi.absFilePath();

    addLayers(name.latin1(), codec.latin1());
  }

  progress.setValue(100);

  emit redrawPreview();
}


void ProjectManager::addLayers(const char* src, const char* codec) {
  gstSource* new_source = openSource(src, codec, false);

  if (new_source == NULL)
    return;

  unsigned int numLayers = new_source->NumLayers();
  for (unsigned int i = 0; i < numLayers; ++i) {
    gstFileInfo finfo(new_source->name());
    QString layername = finfo.baseName();
    if (numLayers != 1)
      layername += QString(":%1").arg(i);
    (void)CreateNewLayer(layername, new_source, i,
                         std::string() /* asset name */);
  }

  parentWidget()->parentWidget()->show();
  qApp->processEvents();

  new_source->unref(); // selector now own this and has incremented ref
}


gstSource* ProjectManager::openSource(const char* src, const char* codec,
                                      bool nofileok) {
  gstSource* new_source = new gstSource(src);
  khDeleteGuard<gstSource> srcGuard(TransferOwnership(new_source));
  new_source->SetNoFile(nofileok);


  if (new_source->Open() != GST_OKAY) {
    QMessageBox::critical(this, tr("Error"),
                          QString(tr("File has unknown problems:\n\n%1")).arg(new_source->name()),
                          tr("OK"), 0, 0, 0);
    return NULL;
  }
  if (codec != NULL)
    new_source->SetCodec(QString(codec));

  if (new_source->NumLayers() == 0) {
    QMessageBox::critical(this, tr("Error"),
                          QString(tr("File doesn't have any valid layers:\n\n%1")).
                          arg(new_source->name()),
                          tr("OK"), 0, 0, 0);
    return NULL;
  }

  if (!validateSrs(new_source))
    return NULL;

  return srcGuard.take();
}

bool ProjectManager::validateSrs(gstSource* src) {
  const gstBBox maxbbox(-0.1, 1.1, -0.1, 1.1);
  while (true) {
    gstBBox bb = src->BoundingBox(0);
    if (bb.Valid() && maxbbox.Contains(bb))
      return true;

    QString coords = QString("    ( S:%1 N:%2 W:%3 E:%4 )\n\n").
      arg(khTilespace::Denormalize(bb.s)).arg(khTilespace::Denormalize(bb.n)).
      arg(khTilespace::Denormalize(bb.w)).arg(khTilespace::Denormalize(bb.e));

    if (QMessageBox::critical(this, tr("Error"),
                              tr("Geographical extents of file %1 are not valid.\n").arg(src->name()) +
                              coords +
                              tr("Perhaps this is due to missing or invalid projection parameters.\n\n") +
                              tr("Would you like to define this now?"),
                              tr("OK"), tr("Cancel"), QString::null, 0) != 0)
      return false;

    SpatialReferenceSystem srs(this);
    if (srs.exec() == QDialog::Accepted) {
      // flush out any geometry
      src->Close();
      src->Open();

      src->DefineSrs(srs.getWKT(), srs.getSavePrj());
    }
  }
}

// GUI Add Layer btn triggers this
void ProjectManager::addLayer() {
  AssetChooser chooser(this, AssetChooser::Open, AssetDefs::Vector, kProductSubtype);
  if (chooser.exec() != QDialog::Accepted)
    return;

  QString newpath;
  if (!chooser.getFullPath(newpath))
    return;

  AddAssetLayer(newpath.latin1());
}


// from drop event
void ProjectManager::AddAssetLayer(const char* assetname) {
  Asset asset(AssetDefs::FilenameToAssetPath(assetname));
  AssetVersion ver(asset->GetLastGoodVersionRef());
  if (ver) {
    bool isasset = (ver->type == AssetDefs::Imagery ||
                    ver->type == AssetDefs::Terrain);
    gstSource* newsource = openSource(isasset ?
                                      ver->GetRef().toString().c_str() :
                                      ver->GetOutputFilename(0).c_str(),
                                      0,
                                      isasset);
    if (newsource) {
      std::string basename = khBasename(asset->GetRef().toString());
      std::string san = shortAssetName(basename);
      QString layername(san.c_str());
      gstLayer* layer = CreateNewLayer(layername,
                                       newsource, 0 /* src layer num */,
                                       asset->GetRef());
      if (!layer) {
        // will already have reported error
        newsource->unref();    // cleanup before leaving
        return;
      }
      newsource->unref();    // project owns this now
    }
  } else {
    if (QMessageBox::warning(this, tr("Error"),
                             tr("Cannot add unbuilt resources to projects.\n"
                                "Do you want to build the selected resource now?"),
                             tr("Yes"), tr("No"), 0, 1) == 0) {
      QString error;
      bool needed;
      if (!khAssetManagerProxy::BuildAsset(asset->GetRef(),
                                           needed, error)) {
        QMessageBox::critical(this, "Error",
                              tr("Unable to schedule build. ") +
                              error,
                              tr("OK"), 0, 0, 0);
      } else if (!needed) {
        QMessageBox::critical(this, "Error" ,
                              tr("Unable to schedule build. Already up to date"),
                              tr("OK"), 0, 0, 0);
      }
    }
  }
}

// re-implement clearselection ourself since the one provided
// as Q3ListView::clearSelection() doesn't work.
void ProjectManager::ClearSelection() {
  Q3ListViewItemIterator it(this);
  while (it.current()) {
    Q3ListViewItem* i = it.current();
    i->setSelected(false);
    ++it;
  }
}

gstLayer* ProjectManager::CreateNewLayer(const QString& origLayername,
                                         gstSource* src, int lnum,
                                         const std::string &assetRef) {
  assert(src);
  ClearSelection();

  // Just in case this layer name contains invalid characters
  QString layername = CleanInvalidLayerName(origLayername);

  // build a config for this type of data
  LayerConfig layerConfig =
    MakeDefaultLayerConfig(layername, src->GetPrimType(lnum), assetRef);

  // create the "active" gstLayer
  gstLayer* layer = new gstLayer(project_, src, lnum, layerConfig);

  layer->SetSortId(project_->layers_.size());

  // We MUST turn off sorting while we add a new layer item,
  // otherwise Q3ListView can end up trying to sort our new item before it is
  // fully constructed. We've see this with drag-n-drop into the list view.
  setSorting(-1);
  LayerItem* item = new LayerItem(this, layer);
  setSorting(0);
  sort();
  item->setSelected(true);

  if (!EnsureUniqueLayerName(item)) {
    QMessageBox::critical(this, "Error",
        tr("Internal Error: Could not ensure unique layer name."),
        tr("OK"), 0, 0, 0);
    delete item;
    layer->unref();
    return 0;
  }
  item->setOpen(false);

  gstFilter* new_filter = layer->GetFilterById(0);
  assert(new_filter != NULL);
  FilterItem* fi = new FilterItem(item, new_filter);
  fi->setOpen(false);
  item->setOn(false);

  RefreshLayerList(true, true);
  return layer;
}

class QueryThread : public QThread {
  gstLayer *layer;
  struct itimerval profile_timer;
 public:
  QueryThread(gstLayer *layer_) : layer(layer_) {}
  void start() {
    khThreadBase::PrepareProfiling(&profile_timer);
    QThread::start();
  }
 protected:
  virtual void run(void) {
    khThreadBase::BeginProfiling(profile_timer);
    layer->QueryThread();
  }
};

bool ProjectManager::applyQueries(gstLayer* layer) {
  if (layer->QueryComplete())
    return true;

  QProgressDialog progress_dialog(tr("Please wait while applying queries..."),
                                  tr("Cancel"), 0, 100);
  progress_dialog.setCaption(tr("Applying Queries"));

  gstProgress query_progress;

  layer->QueryThreadPrep(&query_progress);
  QueryThread qthread(layer);
  qthread.start();

  progress_dialog.show();

  while (query_progress.GetState() == gstProgress::Busy) {
    progress_dialog.setValue(query_progress.Val());
    qApp->processEvents();
    if (progress_dialog.wasCanceled()) {
      query_progress.SetInterrupted();
    }
  }

  // join with the thread so it's safe to return (destroying QThread on stack)
  qthread.wait();

  switch (query_progress.GetState()) {
    case gstProgress::Busy:
      // this shouldn't happen, we checked for !Busy above
      // But we need the case to keep the compiler happy
      return false;
    case gstProgress::Interrupted:
      return false;
    case gstProgress::Succeeded:
      if (!query_progress.Warning().empty()) {
        QMessageBox::warning
          (this, "Warning",
           QString::fromUtf8(query_progress.Warning().c_str()),
           tr("OK"), 0, 0, 0);
      }
      return true;
    case gstProgress::Failed: {
      // compose a single QString message from (possibly) multiple
      // error strings
      QString error_message;
      std::vector<std::string> errors = query_progress.Errors();
      for (unsigned int i = 0; i < errors.size(); ++i) {
        if (i != 0) {
          error_message += "\n";
        }
        error_message += QString::fromUtf8(errors[i].c_str());
      }
      QMessageBox::critical(this, "Error", error_message,
                            tr("OK"), 0, 0, 0);
      return false;
    }
  }

  // unreached but silences warnings
  return false;
}



void ProjectManager::RefreshLayerList(bool setLegends,
                                      bool setSortIds) {
  project_->layers_.clear();
  Q3ListViewItemIterator it(this);
  while (it.current()) {
    // we dont need filters in the layer list
    if (it.current()->rtti() == FILTER) {
      ++it;
      continue;
    }
    gstLayer* layer = extractLayer(it.current());
    if (setSortIds)
      layer->SetSortId(project_->layers_.size());
    if (setLegends) {
      if (it.current()->parent() == 0)
        layer->SetLegend(QString());
      else
        layer->SetLegend(extractLayer(it.current()->parent())->GetPath());
    }
    project_->layers_.push_back(layer);
    ++it;
  }
}


// FindLayerNameAmongSiblings checks the sibling items to discover if
// a sibling other than "item" has the same name.
// It returns true, if another item with name is found.
bool ProjectManager::FindLayerNameAmongSiblings(Q3ListViewItem* item, const QString& name) {
  Q3ListViewItem* next;
  if (item->parent())
    next = item->parent()->firstChild();
  else
    next = this->firstChild();

  while (next) {
    if (next == item) {
      next = next->nextSibling();
      continue;
    }
    if (extractLayer(next)->GetShortName() == name)
      return true;
    next = next->nextSibling();
  }

  return false;
}

// FindUuid uses a depth first search of a Q3ListViewItem tree rooted at parent to discover if
// a node other than "item" has the same UUID.
// It returns true, if another item with UUID is found.
bool ProjectManager::FindUuid(Q3ListViewItem* parent, Q3ListViewItem* item, const QString& uuid) {

  // Check to see if an tree element other than item has the same uuid.
  // Return immediately if the current element matches the uuid.
  if (parent != item) {
    int rtti = parent->rtti();
    // only look into Layers and LayerGroups
    if (rtti == LAYER || rtti == GROUP) {
      if (extractLayer(parent)) {
        if (extractLayer(parent)->GetUuid() == uuid.toUtf8().constData()) {
          return true;
        }
      }
    }
  }

  // Traverse in depth first mode, down to the first child if it exists.
  if (parent->firstChild()) {
    if (FindUuid(parent->firstChild(), item, uuid)) {
      return true;
    }
  }

  // Traverse to the parent sibling if it exists.
  if (parent->nextSibling()) {
    if (FindUuid(parent->nextSibling(), item, uuid)) {
      return true;
    }
  }

  return false;
}

bool ProjectManager::EnsureUniqueLayerName(Q3ListViewItem* item) {
  gstLayer* layer = extractLayer(item);
  if (!layer)
    return false;
  if (!FindLayerNameAmongSiblings(item, layer->GetShortName()))
    return true;

  int index = 2;
  while (true) {
    QString uname = layer->GetShortName() + QString(" (%1)").arg(index);
    if (!FindLayerNameAmongSiblings(item, uname)) {
      layer->SetName(uname);
      return true;
    }
    ++index;
  }
}

//
// draw labels on OpenGL window after buffers have swapped
//
void ProjectManager::DrawLabels(QPainter* painter, const gstDrawState& state) {
  QFont label_font("Times", 16);
  QFontMetrics label_font_metrics(label_font);

  for (unsigned int ii = 0; ii < project_->layers_.size(); ++ii) {
    gstLayer* layer = project_->layers_[ii];
    if (!layer->Enabled())
      continue;

    std::vector<gstSiteSet> site_sets;
    layer->GetSiteLabels(state, &site_sets);
    for (unsigned int s = 0; s < site_sets.size(); ++s) {
      const gstSite* site = site_sets[s].site;

      if (!site->config.enabled)
        continue;

      const double effective_level = state.level + state.offset;
      if (effective_level < site->config.minLevel ||
          effective_level > site->config.maxLevel) {
        continue;
      }

      for (unsigned int n = 0; n < site_sets[s].vlist.size(); ++n) {
        gstValue* label = site_sets[s].rlist[n]->Field(0);

        double x = (site_sets[s].vlist[n].x - state.frust.w) /
                   state.Scale();
        double y = (site_sets[s].vlist[n].y - state.frust.s) /
                   state.Scale();

        if (!label->IsEmpty()) {
          const std::vector< unsigned int> & rgba = site->preview_config.normal_color_;
          QColor color(qRgba(rgba[0], rgba[1], rgba[2], rgba[3]));
          painter->setPen(color);
          painter->setFont(label_font);
          painter->drawText(static_cast<int>(x), painter->viewport().height() -
                            static_cast<int>(y), label->ValueAsUnicode());
        }

        if (site->config.enablePopup) {

          const QPixmap& pixmap = thePixmapManager->GetPixmap(
                                      site->icon(), PixmapManager::NormalIcon);
          if (!pixmap.isNull()) {
            // draw the icon to the left of the text (with a 2 pixel spacing)
            int xpix = static_cast<int>(x) - pixmap.width() - 2;

            // Draw the icon centered vertically with the ascent text
            // y represents the baseline of the text
            // the +1 compensates for the baseline itself and noticably
            // improves the perceived alignment.
            int ypix = painter->viewport().height() - static_cast<int>(y) -
                       pixmap.height()/2 - label_font_metrics.ascent()/2 + 1;

            painter->drawPixmap(xpix, ypix, pixmap);
          }
        }
      }
    }
  }
}

void ProjectManager::DrawFeatures(const gstDrawState& state) {
  const int kMaxCount = 200000;
  int max_count = kMaxCount;

  for (unsigned int ii = 0; ii < project_->layers_.size(); ++ii) {
    gstLayer* layer = project_->layers_[ii];
    if (layer->Enabled()) {
      layer->DrawFeatures(&max_count, state, false);
      if (max_count <= 0)
        break;
    }
  }


  gstLayer* layer = getSelectedLayer();
  if (layer) {
    layer->DrawFeatures(&max_count, state, true);
  }

  if (max_count <= 0 && show_max_count_reached_message_) {
    QMessageBox::warning(this, "Warning",
                         tr("Some of your features have not been draw.\n\n"
                            "Fusion limits the preview drawing to a maximum of\n"
                            "200000 features for any given view."),
                         tr("OK"));
    show_max_count_reached_message_ = false;
  }


  emit statusMessage(QString("f=%1/g=%2/h=%3/r=%4/v=%5(i=%6,d=%7,s=%8)")
                     .arg(kMaxCount - max_count)
                     .arg(gstGeodeImpl::gcount)
                     .arg(gstHeaderImpl::hcount).arg(gstRecordImpl::rcount)
                     .arg(gstValue::vcount).arg(gstValue::icount)
                     .arg(gstValue::dcount).arg(gstValue::scount));

  if (edit_buffer_.size() != 0)
    DrawEditBuffer(state);
}

void ProjectManager::DrawEditBuffer(const gstDrawState& state) {
  // assemble simple red style
  gstFeaturePreviewConfig style;
  style.line_color_[0] = 255;
  style.line_color_[1] = 0;
  style.line_color_[2] = 0;
  style.line_color_[3] = 255;

  for (GeodeListIterator g = edit_buffer_.begin();
       g != edit_buffer_.end(); ++g) {
    (*g)->Draw(state, style);
  }
}

gstLayer* ProjectManager::getSelectedLayer() {
  Q3ListViewItem* item = selectedItem();
  if (item == NULL)
    return NULL;

  if (item->rtti() == FILTER)
    item = item->parent();

  if (item->rtti() == LAYER || item->rtti() == GROUP) {
    return extractLayer(item);
  }

  return NULL;
}


void ProjectManager::selectBox(const gstDrawState& drawState,
                               Qt::ButtonState btnState) {
  // quick exit if nothing selected
  gstLayer* layer = getSelectedLayer();
  if (layer == NULL)
    return;
  if (layer->isGroup())
    return;

  // special cut tool
  if (Preferences::GlobalEnableAll) {
    if ((btnState & Qt::AltButton) && (btnState & Qt::ShiftButton)
        && (!(btnState & Qt::ControlButton))) {
      edit_buffer_.clear();
      layer->ApplyBoxCutter(drawState, edit_buffer_);
      emit redrawPreview();
      return;
    }
  }

  int mode;
  if (btnState & Qt::ShiftModifier) {
    if (btnState & Qt::ControlModifier) {
      mode = gstSelector::PICK_SUBTRACT;
    } else {
      mode = gstSelector::PICK_ADD;
    }
  } else {
    mode = gstSelector::PICK_CLEAR_ADD;
  }

  layer->ApplyBox(drawState, mode);

  emit selectionView(layer->GetSelector());
  //MainWindow::self->updateGfx();
  emit redrawPreview();
}


void ProjectManager::removeLayer(Q3ListViewItem *item) {
  assert(project_ != NULL);

  if (item == 0) {
    item = selectedItem();
    if (item == NULL)
      return;
  }

  if (item->rtti() == LAYER) {
    LayerItem* layer_item = (LayerItem*) item;
    gstLayer* layer = layer_item->layer();

    if (QMessageBox::warning(this, "Warning",
                 QString(tr("Are you sure you want to remove layer: %1\n\n"))
                 .arg(layer->GetShortName()),
                 tr("OK"), tr("Cancel"), 0, 1) == 1)
      return;

    // take out of list before deleting anything
    // to prevent drawing something invalid
    layer_item->orphan();

    // clear out selectionview
    selectItem(NULL);
    delete layer_item;

    layer->unref();
  } else if (item->rtti() == GROUP) {
    LayerGroupItem *groupItem = (LayerGroupItem*)item;
    gstLayer* layer = groupItem->layer();

    if (groupItem->childCount() != 0) {
      if (QMessageBox::warning(
          this, "Warning",
          QString(tr("Are you sure you want to remove layer group: %1\n\n"))
          .arg(layer->GetShortName()),
          tr("OK"), tr("Cancel"), 0, 1) == 1)
        return;
    }

    // take out of list before deleting anything
    // to prevent drawing something invalid
    groupItem->orphan();

    // clear out selectionview
    selectItem(NULL);

    delete groupItem;

    layer->unref();
  }

  RefreshLayerList(true, true);
}

void ProjectManager::paintEvent(QPaintEvent* e) {
  Q3ListView::paintEvent(e);
}

void ProjectManager::UpdateWidgets() {
  clear();

  QMap<QString, LayerGroupItem*> groupMap;

  for (unsigned int ii = 0; ii < project_->layers_.size(); ++ii) {
    gstLayer* layer = project_->layers_[ii];

    if (layer->isGroup()) {
      LayerGroupItem* parent = groupMap[layer->Legend()];
      LayerGroupItem* group;
      if (parent) {
        group = new LayerGroupItem(parent, layer);
        parent->setOpen(true);
      } else {
        group = new LayerGroupItem(this, layer);
      }
      groupMap[layer->GetPath()] = group;
    } else {
      LayerGroupItem* parent = groupMap[layer->Legend()];
      LayerItem* layer_item;
      if (parent) {
        layer_item = new LayerItem(parent, layer);
        parent->setOpen(true);
      } else {
        layer_item = new LayerItem(this, layer);
      }
      layer_item->UpdateFilters();
    }
  }

  parentWidget()->parentWidget()->show();
}

void ProjectManager::enableAllLayers(bool state) {
  LayerItem* item = static_cast<LayerItem*>(firstChild());
  while (item) {
    item->setOn(state);
    item = static_cast<LayerItem*>(item->nextSibling());
  }
}

void ProjectManager::openAllLayers(bool state) {
  LayerItem* item = static_cast<LayerItem*>(firstChild());
  while (item) {
    item->setOpen(state);
    item = static_cast<LayerItem*>(item->nextSibling());
  }
}


void ProjectManager::contentsMousePressEvent(QMouseEvent* e) {
  if (e->button() == Qt::LeftButton) {
    QPoint p(contentsToViewport(e->pos()));
    Q3ListViewItem* i = itemAt(p);
    if (i && (i->rtti() == LAYER || i->rtti() == GROUP)) {
      // if the user clicked into the root decoration of the item,
      // don't try to start a drag!
      if (p.x() > header()->cellPos(header()->mapToActual(0)) +
          treeStepSize() * (i->depth() + (rootIsDecorated() ? 1 : 0)) +
          itemMargin() ||
          p.x() < header()->cellPos(header()->mapToActual(0))) {
        press_pos_ = e->pos();
        mouse_pressed_ = true;
      }
    }
  }

  Q3ListView::contentsMousePressEvent(e);
}

void ProjectManager::contentsMouseMoveEvent(QMouseEvent* e) {
  if (mouse_pressed_ && (press_pos_ - e->pos()).manhattanLength() >
      QApplication::startDragDistance()) {
    mouse_pressed_ = false;
    Q3ListViewItem* item = itemAt(contentsToViewport(press_pos_));
    if (item) {
      drag_layer_ = static_cast<LayerItem*>(item);
      gstLayer* layer = extractLayer(item);
      LayerDrag* ld = new LayerDrag(layer->GetShortName(), viewport());
      ld->drag();
    }
  }
}

void ProjectManager::contentsMouseReleaseEvent(QMouseEvent* e) {
  mouse_pressed_ = false;
}

void ProjectManager::cancelDrag() {
  mouse_pressed_ = false;
}

void ProjectManager::contentsDragMoveEvent(QDragMoveEvent* e) {
  if (getType() == Project &&
      AssetDrag::canDecode(e, AssetDefs::Vector, kProductSubtype)) {
    e->accept(true);
  } else if (getType() == Preview &&
        (AssetDrag::canDecode(e, AssetDefs::Vector, kProductSubtype) ||
         AssetDrag::canDecode(e, AssetDefs::Imagery,
                              Preferences::getConfig().isMercatorPreview ?
                              kMercatorProductSubtype : kProductSubtype) ||
         AssetDrag::canDecode(e, AssetDefs::Terrain, kProductSubtype))) {
    e->accept(true);
  } else if (LayerDrag::canDecode(e)) {
    QPoint vp = contentsToViewport(e->pos());
    Q3ListViewItem* i = itemAt(vp);
    if (!i) {
      e->accept();
      clearSelection();
    } else if (i && i->rtti() == GROUP) {
      LayerGroupItem* grp = static_cast<LayerGroupItem*>(i);
      // don't allow drop on children
      if (!(grp->isChildOf(drag_layer_))) {
        e->accept();
        setSelected(i, true);
      } else {
        e->ignore();
        setSelected(drag_layer_, true);
      }
    } else {
      e->ignore();
      setSelected(drag_layer_, true);
    }
    // allow window manager drag/drop in preview only
  } else if (getType() == Preview && e->provides("text/plain")) {
    e->accept();
  } else {
    e->ignore();
  }
}

void ProjectManager::contentsDragLeaveEvent(QDragLeaveEvent* e) {
  setCurrentItem(drag_layer_);
  setSelected(drag_layer_, true);
}

void ProjectManager::contentsDropEvent(QDropEvent* e) {
  // reposition layers
  if (LayerDrag::canDecode(e)) {
    if (!drag_layer_)
      return;
    QString text;
    LayerDrag::decode(e, text);

    // determine if drop location is list or group
    LayerGroupItem* group = static_cast<LayerGroupItem*>(
                               itemAt(contentsToViewport(e->pos())));
    if (group) {
      // drop on a group

      // check if the dropped item and its parent are not the same
      if (static_cast<Q3ListViewItem*>(group) !=
          static_cast<Q3ListViewItem*>(drag_layer_)) {
        // remove dragged item from parent
        drag_layer_->orphan();

        // and insert it here
        group->insertItem(drag_layer_);
        group->setOpen(true);
        drag_layer_->update();

        // force group's state so that new item shares the same state
        group->stateChange(group->isOn());
      }
    } else {
      //
      // drop on the list, add to the end
      //
      drag_layer_->orphan();
      insertItem(drag_layer_);
      drag_layer_->update();
    }

    // redo buttons since layer is now in a new location
    if (!EnsureUniqueLayerName(drag_layer_)) {
      QMessageBox::critical(this, tr("Error"),
          QString(tr("Internal Error - Could not ensure unique name.")),
          tr("OK"), 0, 0, 0);
      notify(NFY_FATAL, "Internal Error - Could not ensure unique name.");
    }
    updateButtons(drag_layer_);
    RefreshLayerList(true, true);

    setCurrentItem(drag_layer_);
    setSelected(drag_layer_, true);

  //
  // drop assets from asset manager
  //
  } else if (AssetDrag::canDecode(e, AssetDefs::Vector, kProductSubtype) ||
             AssetDrag::canDecode(e, AssetDefs::Imagery,
                              Preferences::getConfig().isMercatorPreview ?
                              kMercatorProductSubtype : kProductSubtype) ||
             AssetDrag::canDecode(e, AssetDefs::Terrain, kProductSubtype)) {
    QString nogoodversions;
    QString text;
    AssetDrag::decode(e, text);
    QStringList assets = QStringList::split(QChar(127), text);
    for (QStringList::Iterator it = assets.begin(); it != assets.end(); ++it) {
      Asset asset(AssetDefs::FilenameToAssetPath(it->toUtf8().constData()));
      AssetVersion ver(asset->GetLastGoodVersionRef());
      if (ver) {
         AddAssetLayer(it->latin1());
      } else {
        std::string san = shortAssetName(*it);
        nogoodversions += QString("   " )
                       +  QString(san.c_str())
                       +  QString("\n");
      }
    }

    if (!nogoodversions.isEmpty()) {
      QMessageBox::warning(this, tr("Error"),
                           tr("The following assets could not be added to this\n"
                              "project because they don't have good versions:\n") + nogoodversions,
                           tr("OK"));
    }
  } else if (e->provides("text/plain")) {
    QString text;
    QTextDrag::decode(e, text);

    QStringList files = QStringList::split('\n', text);

    for (QStringList::Iterator it = files.begin(); it != files.end(); ++it) {
      QString fname = CleanupDropText(*it);
      addLayers(fname.latin1(), NULL);
    }
  }
}

QString ProjectManager::CleanupDropText(const QString &text) {
  // cleanup string to be a standard file name
  QString modstr = QString(text).stripWhiteSpace();
  if (modstr.startsWith("file:"))
    modstr.remove(0, 5);
  while (modstr.startsWith("//"))
    modstr.remove(0, 1);
  return modstr;
}

void ProjectManager::customEvent(QEvent *e) {
  int filterId = 0;
  Q3ListViewItem *listItem = 0;

  // Make sure this is really an event that we sent.
  switch ((int)e->type()) {
    case ConfigDispRuleEventId: {
      ConfigDispRuleEvent* cdrEvent = dynamic_cast<ConfigDispRuleEvent*>(e);
      listItem = cdrEvent->listItem;
      filterId = cdrEvent->filterId;
      break;
    }
    case RemoveLayerEventId: {
      RemoveLayerEvent* rlEvent = dynamic_cast<RemoveLayerEvent*>(e);
      listItem = rlEvent->listItem;
      break;
    }
    case RemoveAllLayersEventId: {
      removeAllLayers();
      return;
    }
    default:
      /* not mine */
      return;
  }

  if (!listItem) {
    return;
  }

  // make sure that the listitem that we squirelled away is still valid
  Q3ListViewItemIterator it(this);
  while (it.current()) {
    if (it.current() == listItem) {
      break;
    }
    ++it;
  }
  if (!it.current()) {
    return;
  }


  // Dispatch it to the appropriate routine
  switch ((int)e->type()) {
    case ConfigDispRuleEventId:
      // make sure it's still a LAYER
      if (listItem->rtti() != LAYER) {
        return;
      }

      // great, everything double checks. Go ahead and lanuch the
      // dialog to edit the display rules
      configureDisplayRules(static_cast<LayerItem*>(listItem), filterId);
      break;
    case RemoveLayerEventId:
      removeLayer(static_cast<LayerItem*>(listItem));
      break;
    case RemoveAllLayersEventId:
      // noop - we did the work above
      break;
  }
}


void ProjectManager::removeAllLayers(void) {
  if (QMessageBox::warning
      (this, "Warning",
       QString(tr("Are you sure you want to remove all layers?\n\n")),
       tr("OK"), tr("Cancel"), 0, 1) == 1) {
    return;
  }

  clear();
  for (unsigned int ii = 0; ii < project_->layers_.size(); ++ii) {
    project_->layers_[ii]->unref();
  }
  RefreshLayerList(true, true);
  selectItem(NULL);
  emit redrawPreview();
}


// -----------------------------------------------------------------------------

ProjectManagerHolder::ProjectManagerHolder(QWidget* parent, AssetBase* base)
  : QWidget(parent), AssetWidgetBase(base) {
  QGridLayout* base_layout = new QGridLayout(this, 2, 2, 0, 4);

  project_manager_ = new ProjectManager(this, kVectorType.c_str(), ProjectManager::Project);
  base_layout->addWidget(project_manager_, 0, 0);

  QVBoxLayout* tool_layout = new QVBoxLayout(0, 0, 6);

  new_layer_ = new QPushButton(this);
  tool_layout->addWidget(new_layer_);
  new_layer_->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)0,
      (QSizePolicy::SizeType)0, 0, 0,
      new_layer_->sizePolicy().hasHeightForWidth()));
  new_layer_->setPixmap(LoadPixmap("filenew"));
  QToolTip::add(new_layer_, trUtf8("Add New Layer"));

  delete_layer_ = new QPushButton(this);
  tool_layout->addWidget(delete_layer_);
  delete_layer_->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)0,
      (QSizePolicy::SizeType)0, 0, 0,
      delete_layer_->sizePolicy().hasHeightForWidth()));
  delete_layer_->setPixmap(LoadPixmap("editdelete.png"));
  delete_layer_->setEnabled(false);
  QToolTip::add(delete_layer_, trUtf8("Delete Layer"));

  new_layer_group_ = new QPushButton(this);
  tool_layout->addWidget(new_layer_group_);
  new_layer_group_->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)0,
      (QSizePolicy::SizeType)0, 0, 0,
      new_layer_group_->sizePolicy().hasHeightForWidth()));
  new_layer_group_->setPixmap(LoadPixmap("folder_new.png"));
  QToolTip::add(new_layer_group_, trUtf8("Add New Layer Group"));

  up_layer_ = new QPushButton(this);
  tool_layout->addWidget(up_layer_);
  up_layer_->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)0,
      (QSizePolicy::SizeType)0, 0, 0,
      up_layer_->sizePolicy().hasHeightForWidth()));
  up_layer_->setPixmap(LoadPixmap("up.png"));
  up_layer_->setEnabled(false);
  QToolTip::add(up_layer_, trUtf8("Move Layer Up"));

  down_layer_ = new QPushButton(this);
  tool_layout->addWidget(down_layer_);
  down_layer_->setSizePolicy(QSizePolicy((QSizePolicy::SizeType)0,
      (QSizePolicy::SizeType)0, 0, 0,
      down_layer_->sizePolicy().hasHeightForWidth()));
  down_layer_->setPixmap(LoadPixmap("down.png"));
  down_layer_->setEnabled(false);
  QToolTip::add(down_layer_, trUtf8("Move Layer Down"));

  QSpacerItem* spacer = new QSpacerItem(30, 130, QSizePolicy::Minimum,
                                        QSizePolicy::Expanding);
  tool_layout->addItem(spacer);

  base_layout->addLayout(tool_layout, 0, 1);

  connect(project_manager_, SIGNAL(layerStateChange(int, int, int)),
          this, SLOT(UpdateLayerControls(int, int, int)));
  connect(new_layer_, SIGNAL(released()), project_manager_,
          SLOT(addLayer()));
  connect(up_layer_, SIGNAL(released()), project_manager_,
          SLOT(moveLayerUp()));
  connect(new_layer_group_, SIGNAL(released()), project_manager_,
          SLOT(addLayerGroup()));
  connect(down_layer_, SIGNAL(released()), project_manager_,
          SLOT(moveLayerDown()));
  connect(delete_layer_, SIGNAL(released()), project_manager_,
          SLOT(removeLayer()));
}

void ProjectManagerHolder::Prefill(const VectorProjectEditRequest &req) {
  project_manager_->getProject()->Prefill(req.config);
  project_manager_->UpdateWidgets();
}

void ProjectManagerHolder::AssembleEditRequest(VectorProjectEditRequest *request) {
  project_manager_->getProject()->AssembleEditRequest(request);
}

void ProjectManagerHolder::UpdateLayerControls(int up, int down, int del) {
  if (up != -1)
    up_layer_->setEnabled(up);
  if (down != -1)
    down_layer_->setEnabled(down);
  if (del != -1)
    delete_layer_->setEnabled(del);
}
