// Copyright 2020 the Open GEE Contributors.
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


#include "fusion/fusionui/FeatureEditor.h"
#include <Qt3Support/Q3CheckListItem>
#include <Qt/qevent.h>
#include <Qt/qmessagebox.h>
#include <Qt/qpixmap.h>
#include <Qt/qmime.h>
#include <Qt/qlabel.h>
#include <Qt/q3filedialog.h>
#include <Qt/q3popupmenu.h>
using QPopupMenu = Q3PopupMenu;
#include <Qt/qmenubar.h>
#include <Qt/qspinbox.h>
#include <Qt/qcombobox.h>
#include <Qt/qlineedit.h>
#include <Qt/qgroupbox.h>
#include <Qt/qlayout.h>
#include <Qt/qbuttongroup.h>
#include <Qt/q3mimefactory.h>
using QMimeSourceFactory = Q3MimeSourceFactory;
#include <Qt/qmime.h>
#include <Qt/q3dragobject.h>
using QImageDrag = Q3ImageDrag;

#include "fusion/fusionui/ProjectManager.h"
#include "fusion/fusionui/AssetDrag.h"
#include "fusion/fusionui/GfxView.h"
#include "fusion/fusionui/Preferences.h"

#include "fusion/autoingest/Asset.h"
#include "fusion/autoingest/AssetVersion.h"
#include "fusion/autoingest/khAssetManagerProxy.h"

#include "fusion/gst/gstGeode.h"
#include "fusion/gst/gstSource.h"
#include "fusion/gst/gstSelector.h"
#include "fusion/gst/gstKVPFile.h"
#include "fusion/gst/gstKVPTable.h"
#include "fusion/gst/gstGridUtils.h"
#include "fusion/gst/gstTypes.h"
#include "fusion/gst/gstBoxCutter.h"
#include "fusion/gst/vectorprep/PolylineJoiner.h"

#include "common/khFileUtils.h"
#include "common/notify.h"
#include "khException.h"
#include "newfeaturebase.h"

using QCheckListItem = Q3CheckListItem;
static const char* const folder_closed_xpm[] = {
  "16 16 9 1",
  "g c #808080",
  "b c #c0c000",
  "e c #c0c0c0",
  "# c #000000",
  "c c #ffff00",
  ". c None",
  "a c #585858",
  "f c #a0a0a4",
  "d c #ffffff",
  "..###...........",
  ".#abc##.........",
  ".#daabc#####....",
  ".#ddeaabbccc#...",
  ".#dedeeabbbba...",
  ".#edeeeeaaaab#..",
  ".#deeeeeeefe#ba.",
  ".#eeeeeeefef#ba.",
  ".#eeeeeefeff#ba.",
  ".#eeeeefefff#ba.",
  ".##geefeffff#ba.",
  "...##gefffff#ba.",
  ".....##fffff#ba.",
  ".......##fff#b##",
  ".........##f#b##",
  "...........####." };

static const char* const folder_open_xpm[] = {
  "16 16 11 1",
  "# c #000000",
  "g c #c0c0c0",
  "e c #303030",
  "a c #ffa858",
  "b c #808080",
  "d c #a0a0a4",
  "f c #585858",
  "c c #ffdca8",
  "h c #dcdcdc",
  "i c #ffffff",
  ". c None",
  "....###.........",
  "....#ab##.......",
  "....#acab####...",
  "###.#acccccca#..",
  "#ddefaaaccccca#.",
  "#bdddbaaaacccab#",
  ".eddddbbaaaacab#",
  ".#bddggdbbaaaab#",
  "..edgdggggbbaab#",
  "..#bgggghghdaab#",
  "...ebhggghicfab#",
  "....#edhhiiidab#",
  "......#egiiicfb#",
  "........#egiibb#",
  "..........#egib#",
  "............#ee#" };

static QPixmap* folderClosed = 0;
static QPixmap* folderOpen = 0;
static QPixmap* primtype_point;
static QPixmap* primtype_line;
static QPixmap* primtype_polygon;

int FeatureItem::count = 0;

// ----------------------------------------------------------------------------

static QPixmap uic_load_pixmap(const QString& name) {
  const QMimeSource* m = QMimeSourceFactory::defaultFactory()->data(name);
  if (!m)
    return QPixmap();
  QPixmap pix;
  QImageDrag::decode(m, pix);
  return pix;
}

// ----------------------------------------------------------------------------

class NewFeatureDialog : public NewFeatureBase {
 public:
  NewFeatureDialog(QWidget* parent)
      : NewFeatureBase(parent, 0, false, 0),
        type_(gstPolyLine) {}

  gstPrimType GetType() const { return type_; }

 private:
  virtual void ButtonReleased(int id) {
    switch (id) {
      case 0:
        type_ = gstPoint;
        break;
      case 1:
        type_ = gstPolyLine;
        break;
      case 2:
        type_ = gstPolygon;
        break;
    }
  }

  gstPrimType type_;
};

// ----------------------------------------------------------------------------

FeatureItem::FeatureItem(Q3ListView* parent, int id, gstGeodeHandle geode,
                         gstRecordHandle attrib)
    : QCheckListItem(parent, QString::number(id), QCheckListItem::CheckBox),
      id_(id),
      geode_(geode) {
  ++count;

  setExpandable(!geode->IsEmpty());
  UpdatePixmap();

  setRenameEnabled(0, false);

  if (attrib) {
    for (unsigned int f = 0; f < attrib->NumFields(); ++f) {
      setText(f + 1, attrib->Field(f)->ValueAsUnicode());
      setRenameEnabled(f + 1, true);
    }
  }

  setOn(true);
}

FeatureItem::~FeatureItem() {
  --count;
}

void FeatureItem::SetId(int id) {
  id_ = id;
  setText(0, QString::number(id));
}

int FeatureItem::compare(QListViewItem* item, int, bool) const {
  FeatureItem* feature_item = dynamic_cast<FeatureItem*>(item);
  assert(feature_item != NULL);

  return id_ - feature_item->Id();
}

void FeatureItem::setOpen(bool o) {
  if (o) {
    // setPixmap(0, *folderOpen);
    switch (geode_->PrimType()) {
      case gstUnknown:
        break;

      case gstPoint:
      case gstPoint25D:
      case gstPolyLine:
      case gstPolyLine25D:
      case gstStreet:
      case gstStreet25D:
      case gstPolygon:
      case gstPolygon25D:
      case gstPolygon3D:
        for (unsigned int i = 0;  i < geode_->NumParts(); ++i) {
          new SubpartItem(this, i, geode_);
        }
        break;

      case gstMultiPolygon:
      case gstMultiPolygon25D:
      case gstMultiPolygon3D:
        // TODO: Support multipolygon features.
        notify(NFY_WARN,
               "%s: multi-polygon features are not supported.", __func__);
        break;
    }
  } else {
    // setPixmap(0, *folderClosed);
    QListViewItem* item = firstChild();
    while (item) {
      takeItem(item);
      delete item;
      item = firstChild();
    }
  }
  QListViewItem::setOpen(o);
}

void FeatureItem::EnsureBackup() {
  setText(0, QString::number(id_) + " *");
  if (!orig_geode_)
    orig_geode_ = geode_->Duplicate();
}

void FeatureItem::RevertEdits() {
  setText(0, QString::number(id_));
  if (orig_geode_)
    geode_ = orig_geode_;
}

bool FeatureItem::HasBackup() {
  return orig_geode_;
}

void FeatureItem::UpdatePixmap() {
  if (geode_->PrimType() == gstPoint)
    setPixmap(0, *primtype_point);
  else if (geode_->PrimType() == gstPolyLine || geode_->PrimType() == gstStreet)
    setPixmap(0, *primtype_line);
  else
    setPixmap(0, *primtype_polygon);
}

// ----------------------------------------------------------------------------

int SubpartItem::count = 0;

SubpartItem::SubpartItem(QListViewItem* parent, int id, gstGeodeHandle geode)
    : QListViewItem(parent),
      id_(id),
      geode_(geode) {
  switch (geode_->PrimType()) {
    case gstUnknown:
      break;

    case gstPoint:
    case gstPoint25D:
    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
    case gstPolygon:
    case gstPolygon25D:
    case gstPolygon3D:
      setText(0, QString("Subpart: %1 (%2)").arg(id).arg(
          (static_cast<const gstGeode*>(&(*geode)))->VertexCount(id)));
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      notify(NFY_WARN,
             "%s: multi-polygon features are not supported.", __func__);
      break;
  }

  //setExpandable(geode->vertexCount(id) != 0);
  setPixmap(0, *folderClosed);
  ++count;
}

SubpartItem::~SubpartItem() {
  --count;
}

int SubpartItem::compare(QListViewItem* item, int, bool) const {
  SubpartItem* subpart_item = dynamic_cast<SubpartItem*>(item);
  assert(subpart_item != NULL);

  return id_ - subpart_item->Id();
}

void SubpartItem::setOpen(bool o) {
#if 0
  if (o) {
    setPixmap(0, *folderOpen);
    for (unsigned int v = 0; v < geode_->vertexCount(id_); ++v)
      new VertexItem(this, v, geode_);
  } else {
    setPixmap(0, *folderClosed);
    QListViewItem *item = firstChild();
    while (item) {
      takeItem(item);
      delete item;
      item = firstChild();
    }
  }
#endif
  QListViewItem::setOpen(o);
}

// ----------------------------------------------------------------------------

int VertexItem::count = 0;

VertexItem::VertexItem(QListViewItem* parent, int id, gstGeodeHandle geode)
    : QListViewItem(parent),
      id_(id),
      geode_(geode) {
  ++count;

  const gstVertex& v = Vertex();
  setText(0, QString("Vertex: %1, %2, %3").
          arg(v.y * 360 - 180, 0, 'f', 10).
          arg(v.x * 360 - 180, 0, 'f', 10).
          arg(v.z));
}

VertexItem::~VertexItem() {
  --count;
}

int VertexItem::compare(QListViewItem* item, int, bool) const {
  VertexItem* vertex_item = dynamic_cast<VertexItem*>(item);
  assert(vertex_item != NULL);

  return id_ - vertex_item->Id();
}

gstVertex VertexItem::Vertex() const {
  if (geode_->PrimType() == gstMultiPolygon ||
      geode_->PrimType() == gstMultiPolygon25D ||
      geode_->PrimType() == gstMultiPolygon3D) {
    notify(NFY_WARN,
           "%s: multi-polygon features are not supported.", __func__);
    return gstVertex();
  }

  SubpartItem* subpart_item = dynamic_cast<SubpartItem*>(parent());
  assert(subpart_item != NULL);
  return  (static_cast<const gstGeode*>(&(*geode_)))->GetVertex(
      subpart_item->Id(), id_);
}

// ----------------------------------------------------------------------------

const char* kLayoutFilename = "featureeditor.layout";

FeatureEditor::FeatureEditor(QWidget* parent)
    : FeatureEditorBase(parent, 0, 0),
      current_header_(gstHeaderImpl::Create()),
      editing_vertex_(false),
      modified_(false) {
  connect(GfxView::instance, SIGNAL(drawVectors(const gstDrawState&)),
          this, SLOT(DrawFeatures(const gstDrawState&)));
  connect(GfxView::instance, SIGNAL(KeyPress(QKeyEvent*)),
          this, SLOT(KeyPress(QKeyEvent*)));

  connect(this, SIGNAL(RedrawPreview()), GfxView::instance, SLOT(updateGL()));

  connect(GfxView::instance, SIGNAL(MousePress(const gstBBox&, Qt::KeyboardModifier)),
          this, SLOT(MousePress(const gstBBox&, Qt::KeyboardModifier)));
  connect(GfxView::instance, SIGNAL(MouseMove(const gstVertex&)),
          this, SLOT(MouseMove(const gstVertex&)));
  connect(GfxView::instance, SIGNAL(MouseRelease()),
          this, SLOT(MouseRelease()));
  connect(this, SIGNAL(ZoomToBox(const gstBBox&)),
          GfxView::instance, SLOT(zoomToBox(const gstBBox&)));

  connect(GfxView::instance, SIGNAL(selectBox(const gstDrawState &, Qt::KeyboardModifier)),
          this, SLOT(SelectBox(const gstDrawState &, Qt::KeyboardModifier)));

  setAcceptDrops(true);

  if (folderOpen == 0) {
    folderOpen = new QPixmap(folder_open_xpm);
    folderClosed = new QPixmap(folder_closed_xpm);
    primtype_point = new QPixmap(uic_load_pixmap("primtype_point.png").
                                 convertToImage().smoothScale(16, 15));
    primtype_line = new QPixmap(uic_load_pixmap("primtype_line.png").
                                convertToImage().smoothScale(16, 15));
    primtype_polygon = new QPixmap(uic_load_pixmap("primtype_polygon.png").
                                   convertToImage().smoothScale(16, 15));
  }

  if (!Preferences::ExperimentalMode)
    experimental_box->hide();

  if (khExists(Preferences::filepath(kLayoutFilename).toUtf8().constData())) {
    if (layout_persist_.Load(Preferences::filepath(kLayoutFilename).toUtf8().constData())) {
      // update position
      resize(layout_persist_.width, layout_persist_.height);
      move(layout_persist_.xpos, layout_persist_.ypos);

      // update visibility
      if (layout_persist_.showme)
        show();
    }
  }

  QPopupMenu* file = new QPopupMenu(this);
  Q_CHECK_PTR(file);
  file->insertItem("&Open", this, SLOT(Open()));
  file->insertItem("&Save", this, SLOT(Save()));
  file->insertItem("&Close", this, SLOT(Close()));

  QPopupMenu* edit = new QPopupMenu(this);
  Q_CHECK_PTR(edit);
  edit->insertItem("&New Feature", this, SLOT(NewFeature()));
  edit->insertSeparator();
  //edit->insertItem("&Copy", this, SLOT(EditCopy()));
  //edit->insertItem("&Paste", this, SLOT(EditPaste()));
  edit->insertItem("&Delete", this, SLOT(EditDelete()));

  QMenuBar* menubar = new QMenuBar(this);
  menubar->insertItem("File", file);
  menubar->insertItem("Edit", edit);
  menubar->setSeparator(QMenuBar::InWindowsStyle);
  QLayout* main_layout = layout();
  Q_CHECK_PTR(main_layout);
  main_layout->setMenuBar(menubar);
  menubar->show();
}

FeatureEditor::~FeatureEditor() {
  // catch any unsaved changes
  // it is impossible to prevent the shutdown process here
  // so this test should be done by having the quit process
  // ask this object if it is ok to quit
  Close();

  layout_persist_.showme = isShown();
  layout_persist_.Save(Preferences::filepath(kLayoutFilename).toUtf8().constData());
}

void FeatureEditor::moveEvent(QMoveEvent* event) {
  const QPoint& pt = event->pos();

  // XXX only update if this is a valid event
  // XXX this is clearly a QT bug!
  if (x() != 0 && y() != 0) {
    layout_persist_.xpos = pt.x();
    layout_persist_.ypos = pt.y();
  }
}

void FeatureEditor::resizeEvent(QResizeEvent* event) {
  const QSize& sz = event->size();
  layout_persist_.width = sz.width();
  layout_persist_.height = sz.height();
}

void FeatureEditor::ContextMenu(QListViewItem* item, const QPoint& pos, int) {
  if (item == NULL)
    return;

  enum { mZoomToFeature,
         mCopyFeature, mPasteFeature, mDeleteFeature, mRevertFeature,
         mCopySubpart, mPasteSubpart, mDeleteSubpart };

  FeatureItem* feature_item = dynamic_cast<FeatureItem*>(item);
  SubpartItem* subpart_item = dynamic_cast<SubpartItem*>(item);

  QPopupMenu menu(this);

  int id;

  menu.insertItem(kh::tr("Zoom to Feature"), mZoomToFeature);
  menu.insertSeparator();
  if (feature_item) {
    menu.insertItem(kh::tr("Copy Feature"), mCopyFeature);
    id = menu.insertItem(kh::tr("Paste Feature"), mPasteFeature);
    if (!geode_copy_buffer_)
      menu.setItemEnabled(id, false);
    menu.insertItem(kh::tr("Delete Feature"), mDeleteFeature);
    menu.insertSeparator();
    id = menu.insertItem(kh::tr("Revert Feature"), mRevertFeature);
    if (!feature_item->HasBackup())
      menu.setItemEnabled(id, false);
  } else if (subpart_item) {
#if 0
    menu.insertItem(kh::tr("Copy Subpart"), mCopySubpart);
    id = menu.insertItem(kh::tr("Paste Subpart"), mPasteSubpart);
    if (!geode_copy_buffer_)
      menu.setItemEnabled(id, false);
    menu.insertItem(kh::tr("Delete Subpart"), mDeleteSubpart);
#endif
  }

  switch (menu.exec(pos)) {
    case mZoomToFeature:
      if (feature_item) {
        emit ZoomToBox(feature_item->Geode()->BoundingBox());
      } else if (subpart_item) {
        emit ZoomToBox(subpart_item->Geode()->BoundingBox());
      }
      break;
    case mCopyFeature:
      geode_copy_buffer_ = feature_item->Geode()->Duplicate();
      break;
    case mPasteFeature:
      {
        gstGeodeHandle geodeh = feature_item->Geode();
        if (geodeh->PrimType() == gstMultiPolygon ||
            geodeh->PrimType() == gstMultiPolygon25D ||
            geodeh->PrimType() == gstMultiPolygon3D ||
            geode_copy_buffer_->PrimType() == gstMultiPolygon ||
            geode_copy_buffer_->PrimType() == gstMultiPolygon25D ||
            geode_copy_buffer_->PrimType() == gstMultiPolygon3D) {
          notify(NFY_WARN,
                 "%s: multi-polygon features are not supported.", __func__);
        } else {
          gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
          const gstGeode *geode_copy_buffer =
              static_cast<const gstGeode*>(&(*geode_copy_buffer_));

          for (unsigned int p = 0; p < geode_copy_buffer->NumParts(); ++p) {
            geode->AddPart(geode_copy_buffer->VertexCount(p));
            for (unsigned int v = 0; v < geode_copy_buffer->VertexCount(p); ++v) {
              geode->AddVertex(geode_copy_buffer->GetVertex(p, v));
            }
          }
        }

        emit RedrawPreview();
      }
      break;
    case mDeleteFeature:
      DeleteFeature(feature_item);
      break;
    case mRevertFeature:
      feature_item->RevertEdits();
      emit RedrawPreview();
      break;
    case mCopySubpart:
      break;
    case mPasteSubpart:
      break;
    case mDeleteSubpart:
      if (editing_vertex_) {
        editing_vertex_ = false;
      }
      break;
  }
}

void FeatureEditor::EditCopy() {
}

void FeatureEditor::EditPaste() {
}

void FeatureEditor::EditDelete() {
  int id = 0;
  QListViewItem* item = feature_listview->firstChild();
  while (item) {
    QListViewItem* next = item->nextSibling();
    FeatureItem* feature_item = dynamic_cast<FeatureItem*>(item);
    assert(feature_item != NULL);
    if (feature_item->isSelected()) {
      feature_listview->takeItem(item);
      delete item;
    } else {
      feature_item->SetId(id++);
    }

    item = next;
  }
  modified_ = true;
  emit RedrawPreview();
}

void FeatureEditor::DeleteFeature(FeatureItem* feature_item) {
  editing_vertex_ = false;

  int id = feature_item->Id();
  QListViewItem* item = feature_item->nextSibling();
  // renumber all remaining items
  while (item) {
    FeatureItem* f_item = dynamic_cast<FeatureItem*>(item);
    assert(f_item != NULL);
    f_item->SetId(id);
    item = item->nextSibling();
    ++id;
  }

  feature_listview->takeItem(feature_item);
  delete feature_item;

  feature_listview->update();

  modified_ = true;
  emit RedrawPreview();
}

bool FeatureEditor::Save() {
  Q3FileDialog fd(this);
  fd.setMode(Q3FileDialog::AnyFile);
  fd.addFilter("Keyhole Geometry (*.kvgeom)");
  if (fd.exec() != QDialog::Accepted)
    return false;

  QString fname(fd.selectedFile());

  // TODO: Ideally the save() option will support all OGR formats so the
  // check for existing file(s) will need to be reworked then.

  std::string kvgeom_name = khEnsureExtension(fname.toUtf8().constData(), ".kvgeom");
  std::string kvattr_name = khReplaceExtension(kvgeom_name, ".kvattr");
  std::string kvindx_name = khReplaceExtension(kvgeom_name, ".kvindx");
  if (khExists(kvgeom_name) || khExists(kvattr_name) || khExists(kvindx_name)) {
    if (QMessageBox::critical(this, "File Already Exists",
                              kh::tr("The file already exists.  "
                                 "Do you wish to overwrite it?"),
                              kh::tr("Yes"), kh::tr("No"), 0, 1) == 0) {
      khUnlink(kvgeom_name);
      khUnlink(kvattr_name);
      khUnlink(kvindx_name);
    } else {
      return false;
    }
  }

  if (ExportKVP(fname) == false) {
    QMessageBox::critical(this, "Error",
                          kh::tr("Unable to save file:") + fname ,
                          kh::tr("OK"), 0, 0, 0);
    return false;
  }

  modified_ = false;
  return true;
}

bool FeatureEditor::ExportKVP(const QString& fname) {
  std::string kvp_name = khEnsureExtension(fname.toUtf8().constData(), ".kvgeom");
  gstKVPFile kvp(kvp_name.c_str());
  if (kvp.OpenForWrite() != GST_OKAY) {
    notify(NFY_WARN, "Unable to open feature file %s", kvp_name.c_str());
    return false;
  }

  std::string kdb_name = khReplaceExtension(kvp_name, ".kvattr");
  gstKVPTable kdb(kdb_name.c_str());
  if (kdb.Open(GST_WRITEONLY) != GST_OKAY) {
    notify(NFY_WARN, "Unable to open attribute file %s", kdb_name.c_str());
    return false;
  }
  kdb.SetHeader(current_header_);
  gstRecordHandle rec = current_header_->NewRecord();

  QListViewItem* item = feature_listview->firstChild();
  while (item) {
    FeatureItem* feature_item = dynamic_cast<FeatureItem*>(item);
    assert(feature_item != NULL);
    if (kvp.AddGeode(feature_item->Geode()) != GST_OKAY) {
      notify(NFY_WARN, "Unable to add feature geometry");
      return false;
    }
    for (unsigned int c = 0; c < current_header_->numColumns(); ++c) {
      rec->Field(c)->set(item->text(c + 1));
    }
    if (kdb.AddRecord(rec) != GST_OKAY) {
      notify(NFY_WARN, "Unable to add attribute record");
      return false;
    }
    item = item->nextSibling();
  }

  return true;
}


void FeatureEditor::GetSelectList(std::vector<FeatureItem*>* select_list) {
  QListViewItem* item = feature_listview->firstChild();
  while (item) {
    FeatureItem* feature_item = dynamic_cast<FeatureItem*>(item);
    assert(feature_item != NULL);
    if (feature_item->isSelected()) {
      select_list->push_back(feature_item);
    }
    item = item->nextSibling();
  }
}


void FeatureEditor::MousePress(const gstBBox& box_point, Qt::KeyboardModifier state) {
  // cancel any editing that's currently underway
  editing_vertex_ = false;

  // always need to know which items are currently selected
  std::vector<FeatureItem*> select_list;
  GetSelectList(&select_list);
  if (select_list.size() == 0)
    return;


  // make a short list of features that might be clicked
  // based on bounding box and selected state
  // TODO: use std::remove_if here...
  std::vector<FeatureItem*> edit_list;
  for (std::vector<FeatureItem*>::iterator it = select_list.begin();
       it != select_list.end(); ++it) {
    if ((*it)->Geode()->Intersect(box_point))
      edit_list.push_back(*it);
  }

  int match = 0;
  for (std::vector<FeatureItem*>::iterator it = edit_list.begin();
       it != edit_list.end() && match == 0;
       ++it) {
    gstGeodeHandle geodeh = (*it)->Geode();
    if (geodeh->PrimType() == gstMultiPolygon ||
        geodeh->PrimType() == gstMultiPolygon25D ||
        geodeh->PrimType() == gstMultiPolygon3D) {
      notify(NFY_WARN,
             "%s: multi-polygon features are not supported(skipped).",
             __func__);
    } else {
      gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
      for (unsigned int p = 0; p < geode->NumParts() && match == 0; ++p) {
        for (unsigned int v = 0; v < geode->VertexCount(p) && match == 0; ++v) {
          if (box_point.Contains(geode->GetVertex(p, v))) {
            if (state & Qt::ControlButton) {
              (*it)->EnsureBackup();
              geode->DeleteVertex(p, v);
              emit RedrawPreview();
              modified_ = true;
              return;
            }
            ++match;
            editing_vertex_ = true;
            current_geode_ = geodeh;
            current_geode_subpart_ = p;
            current_geode_vertex_ = v;
            current_feature_item_ = (*it);
            modified_vertex_ = geode->GetVertex(p, v);
          }
        }
      }
    }
  }

  // ctrl-clicking to delete should abort here if the above
  // didn't result in any intersection with a point
  if (state & Qt::ControlButton)
    return;

  // if there is no match, intersect a line and add a vertex
  if (match == 0) {
    for (std::vector<FeatureItem*>::iterator it = edit_list.begin();
         it != edit_list.end() && match == 0;
         ++it) {
      gstGeodeHandle geodeh = (*it)->Geode();
      if (geodeh->PrimType() == gstMultiPolygon ||
          geodeh->PrimType() == gstMultiPolygon25D ||
          geodeh->PrimType() == gstMultiPolygon3D) {
        notify(NFY_WARN,
               "%s: multi-polygon features are not supported(skipped).",
               __func__);
      } else {
        gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
        unsigned int part;
        if (geode->Intersect(box_point, &part)) {
          for (unsigned int v = 0; v < geode->VertexCount(part) - 1; ++v) {
            // find the segment that is intersected
            gstVertex v0 = geode->GetVertex(part, v);
            gstVertex v1 = geode->GetVertex(part, v + 1);

            // make sure this segment is cut by point
            if (box_point.Intersect(v0, v1)) {
              (*it)->EnsureBackup();
              modified_ = true;

              geode->InsertVertex(part, v + 1, box_point.Center());

              editing_vertex_ = true;
              current_geode_ = geodeh;
              current_geode_subpart_ = part;
              current_geode_vertex_ = v + 1;
              current_feature_item_ = (*it);
              modified_vertex_ = geode->GetVertex(part, v + 1);
              break;
            }
          }
          ++match;
        }
      }
    }
  }

  // if there is still no intersection, add a new point
  if (!editing_vertex_) {
    current_feature_item_ = select_list[0];
    current_feature_item_->EnsureBackup();
    modified_ = true;
    editing_vertex_ = true;
    current_geode_ = current_feature_item_->Geode();

    if (current_geode_->PrimType() == gstMultiPolygon ||
        current_geode_->PrimType() == gstMultiPolygon25D ||
        current_geode_->PrimType() == gstMultiPolygon3D) {
      notify(NFY_WARN,
             "%s: multi-polygon features are not supported(skipped).",
             __func__);
    } else {
      gstGeode *current_g = static_cast<gstGeode*>(&(*current_geode_));
      current_g->AddVertex(box_point.Center());
      current_geode_subpart_ = current_g->NumParts() - 1;
      current_geode_vertex_ =
          current_g->VertexCount(current_geode_subpart_) - 1;
      modified_vertex_ = current_g->GetVertex(
          current_geode_subpart_, current_geode_vertex_);
    }
  }

  emit RedrawPreview();
}

void FeatureEditor::MouseMove(const gstVertex& point) {
  if (editing_vertex_) {
    // make sure geode has been backed up first
    current_feature_item_->EnsureBackup();
    modified_ = true;
    modified_vertex_.set(point.x, point.y, 0);

    if (current_geode_->PrimType() == gstMultiPolygon ||
        current_geode_->PrimType() == gstMultiPolygon25D ||
        current_geode_->PrimType() == gstMultiPolygon3D) {
      notify(NFY_WARN,
             "%s: multi-polygon features are not supported(skipped).",
             __func__);
    } else {
      gstGeode *current_g = static_cast<gstGeode*>(&(*current_geode_));
      current_g->ModifyVertex(current_geode_subpart_,
                              current_geode_vertex_, modified_vertex_);
    }
    emit RedrawPreview();
  }
}

void FeatureEditor::MouseRelease() {
}

void FeatureEditor::dragEnterEvent(QDragEnterEvent* event) {
  if (AssetDrag::canDecode(event, AssetDefs::Vector, kProductSubtype) ||
      AssetDrag::canDecode(event, AssetDefs::Imagery, kProductSubtype) ||
      AssetDrag::canDecode(event, AssetDefs::Terrain, kProductSubtype)) {
    event->accept(true);
  }
}

void FeatureEditor::dropEvent(QDropEvent* event) {
  if (!Close())
    return;

  if (AssetDrag::canDecode(event, AssetDefs::Vector, kProductSubtype) ||
      AssetDrag::canDecode(event, AssetDefs::Imagery, kProductSubtype) ||
      AssetDrag::canDecode(event, AssetDefs::Terrain, kProductSubtype)) {

    QString text;
    AssetDrag::decode(event, text);

    Asset asset(AssetDefs::FilenameToAssetPath(text.toUtf8().constData()));

    //
    // Handle assets (products)
    //
    notify(NFY_DEBUG, "Got asset %s", asset->GetRef().toString().c_str());
    AssetVersion ver(asset->GetLastGoodVersionRef());
    if (ver) {
      notify(NFY_DEBUG, "Last good kvp is %s",
             ver->GetOutputFilename(0).c_str());
      bool isasset = (ver->type == AssetDefs::Imagery ||
                      ver->type == AssetDefs::Terrain);
      gstSource* new_source = OpenSource(isasset ?
                                         ver->GetRef().toString().c_str() :
                                         ver->GetOutputFilename(0).c_str(),
                                         0,
                                         isasset);
      if (new_source)
        AddFeaturesFromSource(new_source);
    } else {
      if (QMessageBox::warning(this, kh::tr("Error"),
                               kh::tr("Cannot open asset.\n"
                                  "Asset does not have a good version.\n"
                                  "Do you want to schedule a build now?"),
                               kh::tr("Yes"), kh::tr("No"), 0, 1) == 0) {
        QString error;
        bool needed;
        if (!khAssetManagerProxy::BuildAsset(asset->GetRef(),
                                             needed, error)) {
          QMessageBox::critical(this, "Error",
                                kh::tr("Unable to schedule build. ") +
                                error,
                                kh::tr("OK"), 0, 0, 0);
        } else if (!needed) {
          QMessageBox::critical(this, "Error" ,
                                kh::tr("Unable to schedule build. Already up to date"),
                                kh::tr("OK"), 0, 0, 0);
        }
      }
    }
  } else if (event->provides("text/plain")) {

    QString text;
    QTextDrag::decode(event, text);

    QStringList files = QStringList::split('\n', text);

    for (QStringList::Iterator it = files.begin(); it != files.end(); ++it) {
      QString fname = ProjectManager::CleanupDropText(*it);
      notify(NFY_DEBUG, "Got file: %s", fname.toUtf8().constData());
    }
  }
  emit RedrawPreview();
}

void FeatureEditor::SelectionChanged() {
  std::vector<FeatureItem*> select_list;
  GetSelectList(&select_list);
  if (select_list.size() > 0)
    feature_listview->ensureItemVisible(select_list[0]);
  emit RedrawPreview();
}

void FeatureEditor::Open() {
  if (!Close())
    return;

  Q3FileDialog file_dialog(this);
  file_dialog.setMode(Q3FileDialog::ExistingFile);

  file_dialog.addFilter("US Census Tiger Line Files (*.rt1 *.RT1)");
  file_dialog.addFilter("OpenGIS GML (*.gml *.GML)");
  file_dialog.addFilter("Generic Text (*.txt *.csv *.TXT *.CSV)");
  file_dialog.addFilter("MicroStation Design (*.dgn *.DGN)");
  file_dialog.addFilter("MapInfo Table (*.tab *.TAB)");
  file_dialog.addFilter("ESRI Shapefile (*.shp *.SHP)");
  file_dialog.addFilter("Keyhole Geometry (*.kvgeom)");

  // make the first filter current, which is the default "All Files (*)"
  file_dialog.setSelectedFilter(0);

  if (file_dialog.exec() != QDialog::Accepted)
    return;

  QString fname(file_dialog.selectedFile());

  gstSource* new_source = OpenSource(fname.toUtf8().constData(), 0, false);
  if (new_source) {
    AddFeaturesFromSource(new_source);
    delete new_source;
  }
}

bool FeatureEditor::Close() {
  if (modified_) {
    // any modification must be saved, or the user
    // must acknowledge their changes will be lost
    switch (QMessageBox::warning(this, "Save Changes...",
                                 "You have changes that have not been saved.\n"
                                 "Do you want to save these changes now?",
                                 "&Yes", "&No", "&Cancel", 0, 2)) {
      case 0:
        // if the save failed or was cancelled, abort now
        if (!Save())
          return false;
        break;
      case 1:
        // user agrees that his changes will be lost
        break;
      case 2:
        // cancel
        return false;
        break;
    }
  }

  modified_ = false;
  editing_vertex_ = false;

  feature_listview->clear();
  while (feature_listview->columns() != 1)
    feature_listview->removeColumn(1);

  current_header_ = gstHeaderImpl::Create();
  current_feature_item_ = NULL;

  //mobile_blocks_.clear();

  emit RedrawPreview();
  return true;
}

void FeatureEditor::KeyPress(QKeyEvent* event) {
#if 0
  if ((event->state() & Qt::ControlButton) && event->key() == Qt::Key_Z) {
    // undo (ctrl-z)
    printf("undo\n");

  } else if (event->key() == Qt::Key_Delete) {
    // delete
    printf("delete\n");
  } else {
    printf("key:%d, ascii:%d\n", event->key(), event->ascii());
  }
#endif
}

void FeatureEditor::DrawFeatures(const gstDrawState& state) {
  if (feature_listview->childCount() == 0)
    return;

  gstFeaturePreviewConfig style;
  style.line_color_[0] = 255;
  style.line_color_[1] = 128;
  style.line_color_[2] = 0;
  style.line_color_[3] = 255;

  gstFeaturePreviewConfig white_style;
  white_style.line_color_[0] = 255;
  white_style.line_color_[1] = 255;
  white_style.line_color_[2] = 255;
  white_style.line_color_[3] = 255;

  gstFeaturePreviewConfig red_style;
  red_style.line_color_[0] = 255;
  red_style.line_color_[1] = 0;
  red_style.line_color_[2] = 0;
  red_style.line_color_[3] = 255;

  gstDrawState select_state = state;
  select_state.mode |= DrawSelected;
  select_state.mode |= DrawBBoxes;

  std::vector<FeatureItem*> selected;

  QListViewItem* item = feature_listview->firstChild();
  while (item) {
    FeatureItem* feature_item = dynamic_cast<FeatureItem*>(item);
    assert(feature_item != NULL);
    if (feature_item->isOn()) {
      if (feature_item->isSelected()) {
        selected.push_back(feature_item);
      } else {
        feature_item->Geode()->Draw(state, style);
      }
    }
    item = item->nextSibling();
  }

  // draw selected features with special 'select_state'
  for (std::vector<FeatureItem*>::iterator it = selected.begin();
       it != selected.end();
       ++it) {
    if ((*it)->BackupGeode())
      (*it)->BackupGeode()->Draw(state, white_style);
    (*it)->Geode()->Draw(select_state, style);
  }

  if (editing_vertex_) {
    gstGeodeImpl::DrawSelectedVertex(state, modified_vertex_);
  }

  //DrawMobileBlocks(state);

  if (join_.size()) {
    for (GeodeListIterator it = join_.begin();
         it != join_.end(); ++it) {
      (*it)->Draw(state, red_style);
    }
  }
}

int BlendModes[] = {
  GL_ZERO,
  GL_ONE,
  GL_SRC_COLOR,
  GL_ONE_MINUS_SRC_COLOR,
  GL_DST_COLOR,
  GL_ONE_MINUS_DST_COLOR,
  GL_SRC_ALPHA,
  GL_ONE_MINUS_SRC_ALPHA,
  GL_DST_ALPHA,
  GL_ONE_MINUS_DST_ALPHA,
  GL_CONSTANT_COLOR,
  GL_ONE_MINUS_CONSTANT_COLOR,
  GL_CONSTANT_ALPHA,
  GL_ONE_MINUS_CONSTANT_ALPHA,
  GL_SRC_ALPHA_SATURATE,
};

int BlendEquations[] = {
  GL_FUNC_ADD,
  GL_FUNC_SUBTRACT,
  GL_FUNC_REVERSE_SUBTRACT,
  GL_MIN,
  GL_MAX
};


gstSource* FeatureEditor::OpenSource(const char* src, const char* codec,
                                     bool nofileok) {
  gstSource* new_source = new gstSource(src);
  khDeleteGuard<gstSource> srcGuard(TransferOwnership(new_source));
  if (codec != NULL)
    new_source->SetCodec(QString(codec));
  new_source->SetNoFile(nofileok);


  if (new_source->Open() != GST_OKAY) {
    QMessageBox::critical(this, kh::tr("Error"),
                          QString(kh::tr("File has unknown problems:\n\n%1")).arg(new_source->name()),
                          kh::tr("OK"), 0, 0, 0);
    return NULL;
  }

  if (new_source->NumLayers() == 0) {
    QMessageBox::critical(this, kh::tr("Error"),
                          QString(kh::tr("File doesn't have any valid layers:\n\n%1")).
                          arg(new_source->name()),
                          kh::tr("OK"), 0, 0, 0);
    return NULL;
  }

  return srcGuard.take();
}


void FeatureEditor::AddFeaturesFromSource(gstSource* source) {
  const int kMaxBadFeatures = 20;

  // configure columns for attribute data
  current_header_ = source->GetAttrDefs(0);
  for (unsigned int col = 0; col < current_header_->numColumns(); ++col)
    feature_listview->addColumn(current_header_->Name(col));

  // only adjust columns after all have been added
  feature_listview->setResizeMode(Q3ListView::AllColumns);

  SoftErrorPolicy soft_errors(kMaxBadFeatures);
  try {
    // iterate over all source features from layer 0
    for (std::uint32_t f = 0; f < source->NumFeatures(0); ++f) {
      try {
        // retrieve feature & attribute
        const bool is_mercator_preview = false;  // This call chain is from
        // FeatureEditor functionality; No way related to mercator preview.
        gstGeodeHandle geode = source->GetFeatureOrThrow(
            0, f, is_mercator_preview);
        gstRecordHandle rec;
        if (current_header_->HasAttrib()) {
          rec = source->GetAttributeOrThrow(0, f);
        }

        // add it to my list
        (void) new FeatureItem(feature_listview, f, geode, rec);
      } catch (const khSoftException &e) {
        soft_errors.HandleSoftError(e.qwhat());
      }
    }
    if (soft_errors.NumSoftErrors() > 0) {
      QString error = kh::tr("Encountered %1 error(s) during query:")
                      .arg(soft_errors.NumSoftErrors());
      for (unsigned int i = 0; i < soft_errors.Errors().size(); ++i) {
        error += "\n";
        error += QString::fromUtf8(soft_errors.Errors()[i].c_str());
      }
      QMessageBox::warning(
          this, kh::tr("Warning"),
          error,
          kh::tr("OK"), 0, 0, 0);
    }

  } catch (const SoftErrorPolicy::TooManyException &e) {
    QString error(kh::tr("Too many bad features"));
    for (unsigned int i = 0; i < e.errors_.size(); ++i) {
      error += "\n";
      error += e.errors_[i].c_str();
    }
    QMessageBox::critical(this, kh::tr("Error"),
                          kh::tr("Error while importing") +
                          error,
                          kh::tr("OK"), 0, 0, 0);
  } catch (const khException &e) {
    QMessageBox::critical(this, kh::tr("Error"),
                          kh::tr("Error while importing") +
                          e.qwhat(),
                          kh::tr("OK"), 0, 0, 0);
  } catch (const std::exception &e) {
    QMessageBox::critical(this, kh::tr("Error"),
                          kh::tr("Error while importing") +
                          e.what(),
                          kh::tr("OK"), 0, 0, 0);
  } catch (...) {
    QMessageBox::critical(this, kh::tr("Error"),
                          kh::tr("Error while importing") +
                          "Unknown error",
                          kh::tr("OK"), 0, 0, 0);
  }

  emit RedrawPreview();
}

void FeatureEditor::SelectBox(const gstDrawState& state,
                              Qt::KeyboardModifier btn_state) {
  // three possible states:
  //   clear & add - no keyboard modifiers
  //   add         - shift
  //   subtract    - ctrl-shift
  bool pick_clear = true;
  bool pick_add = true;

  if (btn_state & Qt::ShiftModifier) {
    if (btn_state & Qt::ControlModifier) {
      pick_clear = false;
      pick_add = false;
    } else {
      pick_clear = false;
      pick_add = true;
    }
  }

  // update gui selection
  QListViewItem* top_item = NULL;
  feature_listview->blockSignals(true);
  QListViewItem* item = feature_listview->firstChild();
  while (item) {
    FeatureItem* feature_item = dynamic_cast<FeatureItem*>(item);
    assert(feature_item != NULL);
    if (feature_item->Geode()->Intersect(state.select)) {
      feature_listview->setSelected(feature_item, pick_add);
      if (!top_item)
        top_item = item;
    } else {
      if (pick_clear)
        feature_listview->setSelected(feature_item, false);
    }
    item = item->nextSibling();
  }
  feature_listview->blockSignals(false);

  editing_vertex_ = false;

  if (top_item)
    feature_listview->ensureItemVisible(top_item);
  emit RedrawPreview();
}


void FeatureEditor::BoxCut() {
  std::vector<FeatureItem*> select_list;
  GetSelectList(&select_list);
  if (select_list.size() == 0) {
    QMessageBox::critical(this, "Error",
                          kh::tr("Nothing selected!"),
                          kh::tr("OK"), 0, 0, 0);
    return;
  }

  // Create BoxCutter once. Reuse by resetting the bounding box;
  fusion_gst::BoxCutter box_cutter(false);  // cut_holes is false;

  int g = 0;
  double grid_step = Grid::Step(box_cut_spin->value());
  for (std::vector<FeatureItem*>::iterator it = select_list.begin();
       it != select_list.end(); ++it, ++g) {
    const gstGeodeHandle& geode = (*it)->Geode();;
    const gstBBox& box = geode->BoundingBox();
    // Assumptions: FeatureEditor feature is no way related to Mercator
    khLevelCoverage lc(ClientImageryTilespaceFlat,
                       khTilespace::NormToDegExtents(khExtents<double>(
                           NSEWOrder, box.n, box.s, box.e, box.w)),
                       box_cut_spin->value(),
                       box_cut_spin->value());

    for (unsigned int row = lc.extents.beginY(); row < lc.extents.endY(); ++row) {
      for (unsigned int col = lc.extents.beginX(); col < lc.extents.endX(); ++col) {
        double w = static_cast<double>(col) * grid_step;
        double e = w + grid_step;
        double s = static_cast<double>(row) * grid_step;
        double n = s + grid_step;
        gstBBox cut_box(w, e, s, n);

        // Initialize BoxCutter with new cut_box;
        box_cutter.SetClipRect(cut_box);

        // Run BoxCut for current geode;
        GeodeList pieces;
        box_cutter.Run(geode, pieces);

        for (GeodeListIterator p = pieces.begin(); p != pieces.end(); ++p) {
          gstRecordHandle rec = current_header_->NewRecord();
          (void) new FeatureItem(feature_listview,
                                 feature_listview->childCount(),
                                 (*p), rec);
        }
      }
    }
  }
}

#if 0
void FeatureEditor::MobileConvert() {
  mobile_blocks_.clear();

  std::vector<FeatureItem*> select_list;
  GetSelectList(&select_list);
  if (select_list.size() == 0)
    return;

  for (std::vector<FeatureItem*>::iterator it = select_list.begin();
       it != select_list.end(); ++it) {
    const gstGeodeHandle& geode = (*it)->Geode();;
    const gstBBox& box = geode->BoundingBox();

    khLevelCoverage lc(ClientImageryTilespace,
                       NormToDegExtents(khExtents<double>(NSEWOrder, box.n, box.s,
                                                          box.e, box.w)),
                       mobile_level_spin->value(),
                       mobile_level_spin->value());

    // cut up geode for each block that it intersects
    for (unsigned int row = lc.extents.beginY(); row < lc.extents.endY(); ++row) {
      for (unsigned int col = lc.extents.beginX(); col < lc.extents.endX(); ++col) {
        MobileBlockHandle eb = MobileBlockImpl::Create(
            khTileAddr(mobile_level_spin->value(), row, col),
            snap_grid_spin->value(),
            width_edit->text().toDouble());
        if (geode->TotalVertexCount() < 2) {
          continue;
        }
        GeodeList pieces;
        geode->BoxCut(eb->GetBoundingBox(), &pieces);
        gstSelector::JoinSegments(&pieces);

        if (pieces.size() > 0) {
          bool success = false;
          for (GeodeList it = pieces.begin();
               it != pieces.end(); ++it) {
            // gstSelector::joinSegments will leave zero-length geodes
            // for all that were merged into other segments, skip these
            if (((*it)->TotalVertexCount()) != 0) {
              if (eb->AddGeometry(*it))
                success = true;
            }
          }
          if (success)
            mobile_blocks_.push_back(eb);
        }
      }
    }
  }
  emit RedrawPreview();
}
#endif

void FeatureEditor::CheckAll() {
  QCheckListItem* item = static_cast<QCheckListItem*>(feature_listview->firstChild());
  while (item) {
    item->setOn(true);
    item = static_cast<QCheckListItem*>(item->nextSibling());
  }
  emit RedrawPreview();
}

void FeatureEditor::CheckNone() {
  QCheckListItem* item = static_cast<QCheckListItem*>(feature_listview->firstChild());
  while (item) {
    item->setOn(false);
    item = static_cast<QCheckListItem*>(item->nextSibling());
  }
  emit RedrawPreview();
}

void FeatureEditor::ChangePrimType() {
  gstPrimType new_type = prim_type_combo->currentItem() == 0 ? gstPoint :
      (prim_type_combo->currentItem() == 1 ? gstPolyLine : gstPolygon);
  FeatureItem* feature_item = dynamic_cast<FeatureItem*>(
      feature_listview->firstChild());
  while (feature_item) {
    feature_item->Geode()->ChangePrimType(new_type);
    feature_item->UpdatePixmap();
    feature_item = dynamic_cast<FeatureItem*>(feature_item->nextSibling());
  }
  emit RedrawPreview();
  feature_listview->update();
}

void FeatureEditor::NewFeature() {
  NewFeatureDialog new_feature_dialog(this);
  if (new_feature_dialog.exec() != QDialog::Accepted)
    return;

  gstRecordHandle rec = current_header_->NewRecord();
  gstGeodeHandle geode = gstGeodeImpl::Create(new_feature_dialog.GetType());

  FeatureItem* item = new FeatureItem(feature_listview,
                                      feature_listview->childCount(), geode, rec);

  feature_listview->selectAll(false);
  feature_listview->setSelected(item, true);

  emit RedrawPreview();
}

void FeatureEditor::Join() {
  std::vector<FeatureItem*> select_list;
  GetSelectList(&select_list);
  if (select_list.size() == 0) {
    QMessageBox::critical(this, "Error",
                          kh::tr("Nothing selected!"),
                          kh::tr("OK"), 0, 0, 0);
    return;
  }

  GeodeList glist;
  for (std::vector<FeatureItem*>::iterator it = select_list.begin();
       it != select_list.end(); ++it) {
    glist.push_back((*it)->Geode());
  }

  //  int before_count = glist.size();
  std::uint64_t num_duplicates = 0;
  std::uint64_t num_joined = 0;
  vectorprep::PolylineJoiner<GeodeList>::
      RemoveDuplicatesAndJoinNeighborsAtDegreeTwoVertices(
          glist, &num_duplicates, &num_joined);
}

void FeatureEditor::Simplify() {
}
