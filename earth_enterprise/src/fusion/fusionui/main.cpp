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


#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <qapplication.h>
#include <qpainter.h>
#include <qstylefactory.h>
#include <gstRegistry.h>
#include <qtranslator.h>
#include <qtextcodec.h>
#include <qgl.h>
#include <qwidget.h>
#include <qimage.h>
#include <qfile.h>
#include <qeventloop.h>

#include <builddate.h>
#include "fusion/fusionversion.h"
#include "fusion/fusionui/MainWindow.h"
#include "fusion/fusionui/GlobalFusion.h"
#include "fusion/fusionui/Preferences.h"
#include "fusion/fusionui/SystemListener.h"
#include "fusion/autoingest/khVolumeManager.h"
#include "fusion/autoingest/MiscConfig.h"
#include "fusion/fusionui/GfxView.h"
#include "common/geFilePool.h"
#include "common/geInstallPaths.h"
#include "common/khFileUtils.h"
#include "common/khGetopt.h"

// pre-qt3.1 workaround
// #ifndef WStyle_Splash
// #define WStyle_Splash WStyle_NoBorder | WStyle_StaysOnTop | WStyle_Tool | WWinOwnDC | WX11BypassWM
// #endif

#if defined(Q_WS_X11)
void qt_wait_for_window_manager(QWidget* widget);
#endif

class SplashScreen : public QWidget {
 public:
  explicit SplashScreen(const QPixmap& pix);
  void setStatus(const QString &message, int alignment = AlignLeft,
                 const QColor &color = white);
  void finish(QWidget* main_win);
  void repaint();

 private:
  QPixmap pix_;
};

SplashScreen::SplashScreen(const QPixmap& pix)
    : QWidget(0, 0, WStyle_Customize | WStyle_Splash),
      pix_(pix) {
  resize(pix_.size());
  QRect scr = QApplication::desktop()->screenGeometry();
  move(scr.center() - rect().center());

  QString versionText = QString("%1 v%2")
                        .arg(GetFusionProductShortName())
                        .arg(GEE_VERSION);

  setFont(QFont("arial", 13));
  QPainter painter(&pix_, this);
  QColor dark_green(3, 158, 40);
  painter.setPen(dark_green);
  QRect r = rect();
  painter.boundingRect(r, AlignLeft | AlignTop, versionText);
  painter.drawText(5, r.height() - 16, versionText);

  setErasePixmap(pix_);

  show();
  repaint();
}

void SplashScreen::repaint() {
  QWidget::repaint();
  QApplication::flush();
}

void SplashScreen::finish(QWidget* main_win) {
#if defined(Q_WS_X11)
  qt_wait_for_window_manager(main_win);
#endif

  close();
}

void SplashScreen::setStatus(const QString& message, int alignment,
                             const QColor& color) {
  setFont(QFont("Times", 10, QFont::Bold));
  QPixmap text_pix = pix_;
  QPainter painter(&text_pix, this);
  painter.setPen(color);
  QRect r = rect();
  r.setRect(r.x() + 10, r.y() + 10, r.width() - 20, r.height() - 20);
  painter.drawText(r, alignment, message);
  setErasePixmap(text_pix);
  repaint();
}

// -----------------------------------------------------------------------------

int main(int argc, char** argv) {
  int argn;
  FusionProductType type = GetFusionProductType();
  khGetopt options;
  options.choiceOpt("mode", type,
                    makemap(std::string("LT"),       FusionLT,
                            std::string("Pro"),      FusionPro,
                            std::string("Internal"), FusionInternal));
  if (!options.processAll(argc, argv, argn)) {
    notify(NFY_FATAL, "Error processing commandline options");
  }
  SetFusionProductType(type);


  //
  // must always create QApplication before initializing gst library
  //
  QApplication a(argc, argv);

  //
  // confirm opengl support
  //
  if (!QGLFormat::hasOpenGL()) {
    qWarning("This system has no OpenGL support.  Exiting.");
    return EXIT_FAILURE;
  }

  //
  // Pre-initialize the volume manager so it will find any problems
  // before we really do any work
  //
  theVolumeManager.Init();

  // Turn off any site-configured delay before accessing small files.
  // This just causes the GUI to look like it's hung. If the file really
  // isn't written yet, the low level routines will report an arror that
  // the GUI will report. The user can then retry it later.
  MiscConfig::Instance().NFSVisibilityDelay = 0;

  //
  // display splash screen
  //
  QString pixname("fusion_splash.png");
  if (!QFile::exists(pixname)) {
    pixname = khComposePath(kGESharePath, pixname);
    if (!QFile::exists(pixname))
      pixname.setLength(0);
  }

  SplashScreen* splash = NULL;
  if (!pixname.isEmpty()) {
    QPixmap pixmap(pixname);
    splash = new SplashScreen(pixmap);
    a.processEvents();
  }

  //
  // initialize the gst library
  //
  geFilePool file_pool;
  gstInit(&file_pool);

  //
  // initialize all fusion globals
  //
  Preferences::init();
  GlobalFusion::init();

  //
  // qglwidget defaults to no alpha channel, turn this on
  //
  QGLFormat f;
  f.setAlpha(true);
  QGLFormat::setDefaultFormat(f);

  //
  // install translator
  //
  QTranslator translator(0);
  if (translator.load(QString("fusion_") + QTextCodec::locale(), ".")) {
    a.installTranslator(&translator);
  } else if (translator.load(QString("fusion_") + QTextCodec::locale(),
                             kGESharePath)) {
    a.installTranslator(&translator);
  }

  //
  // configure qt style
  //
  a.setStyle(QStyleFactory::create("platinum"));

  try {
    //
    // launch the system listener
    //
    SystemListener listener;
    AssetWatcherManager watcherManager;

    //
    // launch gui now
    //
    MainWindow* w = new MainWindow();

    // check if system has valid GL context.
    if (!w->gfxview->isValid()) {
      if (splash) {
        splash->finish(w);
        delete splash;
      }
      delete w;

      qWarning("Sorry, this system doesn't have sufficient OpenGL resources.");
      qWarning("Try setting LIBGL_ALWAYS_INDIRECT=yes in your environment.");
      return EXIT_FAILURE;
    }

    // initialize main window.
    w->Init();

    // first event is really slow for some unknown reason
    // so process it now before taking the splash screen away
    a.eventLoop()->processEvents(QEventLoop::ExcludeUserInput, 10);

    w->show();

    if (splash) {
      splash->finish(w);
      delete splash;
    }

    a.connect(&a, SIGNAL(lastWindowClosed()), w, SLOT(fileExit()));

    a.setMainWidget(w);

    int status = a.exec();

    delete w;

    return status;
  } catch(const std::exception& e) {
    notify(NFY_FATAL, "Caught exception: %s", e.what());
  } catch(...) {
    notify(NFY_FATAL, "Caught unknown exception");
  }
  return 1;
}
