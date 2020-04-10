/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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


#ifndef _featureEditor_h_
#define _featureEditor_h_

#include <Qt/q3dockwindow.h>
using QDockWindow = Q3DockWindow;
#include <Qt/qlistview.h>
#include <Qt/q3listview.h>
#include <khGuard.h>
#include <gstVertex.h>
#include <gstGeode.h>
#include <gstRecord.h>
#include <khRefCounter.h>
#include <gstBBox.h>
//#include <gstMobile.h>

#include "featureeditorbase.h"
#include <fusionui/.idl/layoutpersist.h>

class gstDrawState;
class QDropEvent;
class gstSource;
class FeatureEditor;
using QListViewItem = Q3ListViewItem;
using QCheckListItem = Q3CheckListItem;

// ----------------------------------------------------------------------------

QString PrimTypeToString(int type);

// ----------------------------------------------------------------------------

class FeatureItem : public QCheckListItem {
 public:
  FeatureItem(Q3ListView* parent, int id, gstGeodeHandle g, gstRecordHandle a);
  ~FeatureItem();

  virtual int compare(QListViewItem* item, int, bool) const;
  virtual void setOpen(bool o);

  const gstGeodeHandle Geode() const { return geode_; }
  int Id() const { return id_; }
  void SetId(int);

  void EnsureBackup();
  void RevertEdits();
  bool HasBackup();
  void UpdatePixmap();

  gstGeodeHandle BackupGeode() { return orig_geode_; }

  static int count;

 private:
  friend class FeatureEditor;

  int id_;
  gstGeodeHandle geode_;
  gstGeodeHandle orig_geode_;
};

// ----------------------------------------------------------------------------

class SubpartItem : public QListViewItem {
 public:
  SubpartItem(QListViewItem* parent, int id, gstGeodeHandle geode);
  ~SubpartItem();

  virtual int compare(QListViewItem* item, int, bool) const;
  virtual void setOpen(bool o);

  gstGeodeHandle Geode() { return geode_; }
  int Id() const { return id_; }
  
  static int count;

 private:
  int id_;
  gstGeodeHandle geode_;
};

// ----------------------------------------------------------------------------

class VertexItem : public QListViewItem {
 public:
  VertexItem(QListViewItem* parent, int id, gstGeodeHandle geode);
  ~VertexItem();

  virtual int compare(QListViewItem* item, int, bool) const;

  gstGeodeHandle Geode() const { return geode_; }
  gstVertex Vertex() const;
  int Id() const { return id_; }
  
  static int count;

 private:
  int id_;
  gstGeodeHandle geode_;
};

// ----------------------------------------------------------------------------

class FeatureEditor : public FeatureEditorBase {
  Q_OBJECT

 public:
  FeatureEditor(QWidget* parent);
  ~FeatureEditor();

  void DeleteFeature(FeatureItem* feature_item);
  //void DrawMobileBlocks(const gstDrawState& state);

 public slots:
  bool Close();

 signals:
  void ZoomToBox(const gstBBox& b);
  void RedrawPreview();

 protected slots:
  void DrawFeatures(const gstDrawState& s);
  void MousePress(const gstBBox& b, Qt::KeyboardModifier s);
  void MouseMove(const gstVertex& v);
  void MouseRelease();
  void SelectBox(const gstDrawState& state, Qt::KeyboardModifier btn_state);
  void KeyPress(QKeyEvent* e);
  
  // file menu
  void Open();
  bool Save();

  // edit menu
  void NewFeature();
  void EditCopy();
  void EditPaste();
  void EditDelete();


 private:
  // inherited from qwidget
  virtual void resizeEvent(QResizeEvent* e);
  virtual void moveEvent(QMoveEvent* e);

  virtual void BoxCut();
  //virtual void MobileConvert();

  virtual void CheckAll();
  virtual void CheckNone();
#if 0
  virtual void SelectAll();
  virtual void SelectNone();
  virtual void ShowPrevious();
  virtual void ShowNext();
#endif

  virtual void ChangePrimType();
  virtual void Join();
  virtual void Simplify();

  virtual void dragEnterEvent(QDragEnterEvent* event);
  virtual void dropEvent(QDropEvent* event);
  virtual void ContextMenu(QListViewItem* item, const QPoint& pos, int);
  virtual void SelectionChanged();
  bool ExportKVP(const QString& fname);

  void UpdateListview();
  void GetSelectList(std::vector<FeatureItem*>* edit_list);

  gstSource* OpenSource(const char *src, const char *codec, bool nofileok);
  void AddFeaturesFromSource(gstSource* source);

  gstHeaderHandle current_header_;

  bool editing_vertex_;
  FeatureItem* current_feature_item_;
  gstGeodeHandle current_geode_;
  unsigned int current_geode_subpart_;
  unsigned int current_geode_vertex_;
  gstVertex modified_vertex_;

  gstGeodeHandle geode_copy_buffer_;

  //std::vector<MobileBlockHandle> mobile_blocks_;
  GeodeList join_;

  bool modified_;
  bool agree_to_lose_changes_;
  FeatureEditorLayout layout_persist_;
};

#endif  // !_featureEditor_h_
