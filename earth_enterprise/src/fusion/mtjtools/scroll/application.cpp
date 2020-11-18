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

#include <memory>
#include <qimage.h>
#include <qpixmap.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qmenu.h>
#include <qmenubar.h>
#include <qtextedit.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qstatusbar.h>
#include <qmessagebox.h>
#include <qapplication.h>
#include <Qt/q3accel.h>
#include <qtextstream.h>
#include <qpainter.h>
#include <Qt/q3paintdevicemetrics.h>
#include <Qt/q3simplerichtext.h>
#include <Qt/q3simplerichtext.h>
#include <Qt/q3grid.h>
#include <Qt/q3canvas.h>
#include <Qt/qobject.h>
#include  <Qt/q3filedialog.h>
#include "fileopen.xpm"
#include <Qt/q3mimefactory.h>
#include "application.h"
#include "imageview.h"
#include "gdal.h"
#include <Qt/q3popupmenu.h>

using QPopupMenu = Q3PopupMenu;
using QMimeSourceFactory = Q3MimeSourceFactory;

ApplicationWindow::ApplicationWindow()
    : QMainWindow( 0, "scroll", Qt::WDestructiveClose | Qt::WGroupLeader )
{
  QPixmap openIcon;

  QToolBar * fileTools = new QToolBar( this, "file operations" );
  fileTools->setLabel( "File Operations" );

  openIcon = QPixmap( fileopen );
  (void) new QToolButton( openIcon, "Open", QString::null,
                          this, SLOT(choose()), fileTools, "open" );
  QMimeSourceFactory::defaultFactory()->setPixmap( "fileopen", openIcon );

  QPopupMenu * file = new QPopupMenu( this );
  menuBar()->insertItem( "&File", file );

  file->insertItem( openIcon, "&Open...",
                         this, SLOT(choose()), Qt::CTRL+Qt::Key_O);
  file->insertItem( "&Save", this, SLOT(save()));

  file->insertSeparator();

  file->insertItem( "&Quit", qApp, SLOT( closeAllWindows() ), Qt::CTRL+Qt::Key_Q );

  QPopupMenu * help = new QPopupMenu( this );
  menuBar()->insertItem( "&Help", help );
  help->insertItem( "&Commands", this,
                    SLOT(keyhelps()), Qt::CTRL+Qt::Key_H);



  // setup GDAL once per execution
  GDALSetCacheMax(512*1024*1024);
  GDALAllRegister();

  viewer = new ImageView(this, "viewer");
  viewer->setStatusBar(statusBar());
  viewer->setFocus();

  setCentralWidget(viewer);
  statusBar()->message( "Please open an image", 0 );

  resize(800, 600);
}

ApplicationWindow::~ApplicationWindow()
{
}

void ApplicationWindow::loadInitImage(const std::string &image_file_path) {
  QString abs_file_path;
  // Set prevDir per fileName (using non executed QFileDialog setting.)

  std::unique_ptr<Q3FileDialog> fdlg
  {
      new Q3FileDialog(QString::null, QString::null, this)
  };

  fdlg->setSelection(image_file_path.c_str());
  setCaption(fdlg->selectedFile()); // using selectedFile() to get full path
  prevDir = fdlg->dirPath();

  viewer->loadInitImage(image_file_path);
}


void ApplicationWindow::choose()
{
  std::unique_ptr<Q3FileDialog> fdlg
  {
      new Q3FileDialog(prevDir, QString::null, this)
  };
  std::string filter { "Images (*.pre *.img *.tif *.tiff *.jp2 *.sid *.jpg *.IMG *.TIF *.TIFF *.JP2 *.SID *.JPG)" };
  fdlg->addFilter(filter.c_str());
  if (fdlg->exec() == QDialog::Accepted) {
    prevDir = fdlg->dirPath();
    load(fdlg->selectedFile());
  }
}

void ApplicationWindow::save()
{
  viewer->SaveLutWork();
}

void ApplicationWindow::keyhelps()
{
  QMessageBox::about
    (this, "Command Help",
     "<table>"
     "<tr><td align='center' colspan='2'><b>Keyboard Commands</b></td></tr>"
     "<tr><td width='30%'>0 - 7</td><td width='70%'>set mouse radius</td></tr>"
     "<tr><td>up,down,left,right</td><td>move mouse</td></tr>"
     "<tr><td>pageup, pagedown, shift-up, shift-down, shift-left, shift-right, ctrl-shift-up, ctrl-shift-down, ctrl-shift-left, ctrl-shift-right</td><td>scroll image</td></tr>"
     "<tr><td>=</td><td>reset on file open</td></tr>"
     "<tr><td>-</td><td>toggle \"swap red and blue\" mode</td></tr>"
     "<tr><td>u, U</td><td>lower/increase row limit</td></tr>"
     "<tr><td>l, L</td><td>less/more left tail clip during histogram</td></tr>"
     "<tr><td>r, R</td><td>less/more right tail clip during histogram</td></tr>"
     "<tr><td>p, P</td><td>less/more pre-gamma</td></tr>"
     "<tr><td>g, G</td><td>less/more post-gamma</td></tr>"
     "<tr><td>z, Z</td><td>less/more zoom</td></tr>"
     "<tr><td>ALT-z, ALT-Z</td><td>less/more zoom (4 levels at a time)</td></tr>"
     "<tr><td>s, S</td><td>less/more contrast</td></tr>"
     "<tr><td>h</td><td>compute histogram</td></tr>"
     "<tr><td>c</td><td>toggle correct mode</td></tr>"
     "<tr><td>del, backspace</td><td>remove sample(s)</td></tr>"
     "<tr><td>Alt-w, Alt-W</td><td>write .lut file</td></tr>"
     "<tr><td>w, W</td><td>write .p,.lut, & .info files</td></tr>"
     "<tr><td>A</td><td>apply lut and write new image</td></tr>"
     "<tr><td>Home</td><td>Return to zoom & position right before last right mouse press</td></tr>"

     "<tr><td align='center' colspan='2'><b>Mouse Commands</b></td></tr>"
     "<tr><td>left mouse press</td><td>add sample(s)</td></tr>"
     "<tr><td>right mouse press</td><td>recenter and zoom just beyond fullres</td></tr>"

     "</table>"
     );
}

void ApplicationWindow::load( const QString &fileName )
{
  viewer->setFilename((char *)fileName.ascii());
  setCaption(fileName);
}

void ApplicationWindow::closeEvent( QCloseEvent* ce )
{
  ce->accept();
}
