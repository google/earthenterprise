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


#ifndef KHSRC_FUSION_FUSIONUI_ASSETBASE_H__
#define KHSRC_FUSION_FUSIONUI_ASSETBASE_H__

#include <QtGui/qmenudata.h>
#include <Qt/qaction.h>
#include <Qt/qvariant.h>
#include <Qt/qmainwindow.h>
#include <khMetaData.h>
#include <Qt/q3popupmenu.h>
#include <autoingest/.idl/storage/AssetDefs.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QToolBar;
using QPopupMenu = Q3PopupMenu;
class QLabel;
class QFrame;

class AssetBase : public QMainWindow {
  Q_OBJECT

 public:
  AssetBase(QWidget* parent);
  virtual ~AssetBase();

  static QString untitled_name;

  virtual AssetDefs::Type AssetType() const = 0;
  virtual std::string AssetSubtype() const = 0;
  virtual QString AssetPrettyName() const = 0;

  QString Name() const;
  bool OkToQuit();
  void RestoreLayout();
  void SetErrorMsg(const QString& text, bool red);
  void SetSaveError(bool state);
  void SetLastSaveError(bool state);

 signals:
  void NameChanged(const QString& text);

 protected:
  virtual QWidget* BuildMainWidget(QWidget* parent) = 0;
  virtual bool IsModified() = 0;
  virtual bool SubmitEditRequest(QString* error_msg, bool save_error) = 0;

  void InstallMainWidget();
  void SetName(const QString& text);
  void SetMeta(const khMetaData& meta);

  khMetaData Meta() const;

  QGridLayout* main_frame_layout_;

  QFrame* main_frame_;
  QMenuBar* menu_bar_;
  QPopupMenu* file_menu_;
  QPopupMenu* edit_menu_;
  QLabel* error_msg_label_;

  QAction* save_action_;
  QAction* saveas_action_;
  QAction* close_action_;
  QAction* hidden_action_;
  QAction* notes_action_;
  QAction* build_action_;
  QAction* savebuild_action_;

 protected slots:
  virtual void languageChange();

 private slots:
  virtual bool Save();
  virtual bool SaveAs();
  virtual void Close();
  virtual void EditNotes();
  virtual void Build();
  virtual void SaveAndBuild();
  virtual void SetHidden(bool on);
  virtual void AboutToShowFileMenu();
  virtual void AboutToHideFileMenu();

 private:
  // inherited from qwidget
  virtual void resizeEvent(QResizeEvent* e);
  virtual void closeEvent(QCloseEvent* event);

  bool EnsureNameValid();
  khMetaData meta_;
  QString asset_path_;
  bool save_error_;
  bool last_save_error_;
};

#endif // !KHSRC_FUSION_FUSIONUI_ASSETBASE_H__
