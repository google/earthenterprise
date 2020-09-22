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

#include <Qt/qglobal.h>
#include <Qt/qobject.h>
#include <Qt/qcursor.h>
#include <Qt/qtimer.h>
#include <Qt/q3dragobject.h>
#include <Qt/qmessagebox.h>
#include <Qt/qbitmap.h>
#include <Qt/qcoreapplication.h>
#include <gstTexture.h>
#include <gstTextureManager.h>
#include <notify.h>
#include <gstHistogram.h>
#include <font.h>
#include <Qt/qpainter.h>
#include <khTimer.h>
#include <gstIconManager.h>
#include <khTileAddr.h>
#include <gstGridUtils.h>
#include "khException.h"
#include "GfxView.h"
#include "Preferences.h"

using QTextDrag = Q3TextDrag;

QString GfxView::MaxTexSize;
QString GfxView::BitsPerComp;

GfxView* GfxView::instance = 0;
using QImageDrag = Q3ImageDrag;

GfxView::GfxView(QWidget* parent) : GfxView(parent,0) {}

GfxView::GfxView(QWidget* parent, const char* name)
    : QGLWidget(parent, name),
      start_scale_(-1.0),               // less than 0 -> not dragging
      state_(false),
      snap_to_level_(false)
{
  if (instance) {
    notify(NFY_FATAL,
           "Internal Error: Attempted to create a second preview window");
  }

  instance = this;

  frame_num_ = 0;

  state_.mode = 0;
  state_.level = 0;
  state_.offset = 0;
  state_.mode = DrawTextures;
  state_.mode |= DrawLabels;

  alpha_mode_ = Transparent;

  tile_size_ = 256;
  int ts = tile_size_;
  level_zero_ = 0;
  while (ts >>= 1)
    level_zero_++;

  // this is a holder for our "reset" view
  domain_box_.init(0, 1, .25, .75);

  rubberband_on_ = false;

  tool_mode_ = Select;
  save_tool_mode_ = Unknown;

  int txsz = 200;
  int mxld = 150;

  texture_manager_ = new gstTextureManager(txsz, mxld);

  tex_load_timer_ = 0;

  // accept all mouse move events
  // even if no button is pressed
  setMouseTracking(true);

  memset(tex_stat_, 0, STAT_FRAME * sizeof(double));
  memset(geom_stat_, 0, STAT_FRAME * sizeof(double));

  // this timer will draw labels after the screen has been
  // idle for a small amount of time
  label_timer_ = new QTimer(this);
  connect(label_timer_, SIGNAL(timeout()), this, SLOT(drawLabels()));

  setAcceptDrops(true);

  ReloadPrefs();
}

GfxView::~GfxView(void) {
  doneCurrent();
  delete texture_manager_;
}

void GfxView::contextMenuEvent(QContextMenuEvent* e) {
  emit contextMenu(this, QCursor::pos());
}

void GfxView::mousePressEvent(QMouseEvent* e) {
  drag_box_.start(e->x(), e->y());
  drag_box_.end(e->x(), e->y());

  if (e->button() == Qt::LeftButton) {
    switch (tool_mode_) {
      case Edit: {
        const int kPickBox = 4;
        gstVertex lower_left = ConvertScreenToNormalizedWorld(
            e->x() - kPickBox, e->y() - kPickBox);
        gstVertex upper_right = ConvertScreenToNormalizedWorld(
            e->x() + kPickBox, e->y() + kPickBox);
        emit MousePress(gstBBox(lower_left.x, upper_right.x,
                                lower_left.y, upper_right.y),
                        e->state());
      }
        break;

      case Select:
      case ZoomBox:
        rubberband_on_ = true;
        break;

      case Pan:
        start_pan_[0] = state_.frust.CenterX();
        start_pan_[1] = state_.frust.CenterY();
        break;

      case ZoomDrag:
        start_scale_ = state_.Scale();
        break;

      case Unknown:
        break;

      case ToolMask:
        break;
    }

  } else if (e->button() == Qt::MidButton) {
    save_tool_mode_ = tool_mode_;
    tool_mode_ = Pan;
    start_pan_[0] = state_.frust.CenterX();
    start_pan_[1] = state_.frust.CenterY();
  }
}

void GfxView::mouseMoveEvent(QMouseEvent* e) {
  // if anyone is watching, update the geographic coordinate of the cursor
  gstVertex point = ConvertScreenToLatLon(e->x(), e->y());
  emit latlonChanged(khTilespace::Denormalize(point.y),
                     khTilespace::Denormalize(point.x));

  // ignore if no button is pressed
  if (!(e->state() & (Qt::LeftButton | Qt::MidButton | Qt::RightButton)))
    return;

  drag_box_.end(e->x(), e->y());

  switch (tool_mode_ | (e->state() & Qt::MouseButtonMask)) {
    case Edit | Qt::LeftButton:
      emit MouseMove(ConvertScreenToNormalizedWorld(e->x(), e->y()));
      break;

    case Select | Qt::LeftButton:
    case ZoomBox | Qt::LeftButton:
      if (rubberband_on_)
        updateGL();
      break;

    case Pan | Qt::LeftButton:
    case Pan | Qt::MidButton:
      {
        double cx = start_pan_[0] - (static_cast<double>(drag_box_.dX()) *
                                     state_.Scale());
        double cy = start_pan_[1] + (static_cast<double>(drag_box_.dY()) *
                                     state_.Scale());
        double wd = state_.frust.Width() * 0.5;
        double hd = state_.frust.Height() * 0.5;
        state_.frust.init(cx - wd, cx + wd, cy - hd, cy + hd);
        updateGL();
      }
      break;

    case ZoomDrag | Qt::LeftButton:
      {
        if (snap_to_level_) {
          // For snapping to level zooming, we choose to step a level for
          // every 5% change in the Y-coordinate of the mouse being dragged.
          // Since the level changes during the drag, we need to calculate
          // the delta off the current level to the desired level relative to
          // the level at the beginning of the drag.
          // TODO: need to move this level computation into
          // utility method with unit tests.
          double start_level = log(2 / start_scale_) * M_LOG2E - level_zero_;
          double current_level = log(2 / state_.Scale()) * M_LOG2E - level_zero_;
          // Every 5% of window height is a level.
          double delta_y = static_cast<double> (drag_box_.dY()) / height();
          double desired_level = start_level + int(delta_y * 20);
          adjustLevel(int(round(desired_level - current_level)));
        } else {
          double m = static_cast<double> (drag_box_.dY()) / height() * -2;

          double new_scale;
          if (drag_box_.dY() <= 0) {
            new_scale = start_scale_ + (start_scale_ * m);
          } else {
            new_scale = start_scale_ * height() / ((drag_box_.dY() * 2)
                + height());
          }

          SetScale(new_scale);

          adjustLevel(0);
        }
        resizeEvent(NULL);
        updateGL();
      }
      break;
  }
}

void GfxView::mouseReleaseEvent(QMouseEvent* e) {
  switch (tool_mode_ | e->button()) {
    case Edit | Qt::LeftButton:
      emit MouseRelease();
      break;

    case Select | Qt::LeftButton:
      if (rubberband_on_) {
        rubberband_on_ = false;
        selectFeatures(e->state());
        updateGL();
      }
      break;

    case ZoomBox | Qt::LeftButton:
      if (rubberband_on_) {
        rubberband_on_ = false;
        SetCenter(state_.frust.w + drag_box_.cX() * state_.Scale(),
                  state_.frust.s + (state_.Height() - drag_box_.cY()) *
                  state_.Scale());

        double dx = (drag_box_.dX() >= 0) ? drag_box_.dX()
                    : drag_box_.dX() * -1.0;
        double dy = (drag_box_.dY() >= 0) ? drag_box_.dY()
                    : drag_box_.dY() * -1.0;
        SetScale((dx > dy) ? state_.Scale() * dx / width()
                 : state_.Scale() * dy / height());
        resizeGL(width(), height());
        updateGL();
      }
      break;
    case Pan | Qt::MidButton:
    case Pan | Qt::LeftButton:
      if (save_tool_mode_ != Unknown) {
        tool_mode_ = save_tool_mode_;
        save_tool_mode_ = Unknown;
      }
      break;
    case ZoomDrag | Qt::LeftButton:
      start_scale_ = -1.0;
      adjustLevel(0);
      break;
  }
}

void GfxView::selectFeatures(Qt::ButtonState btnState) {
  gstBBox box(drag_box_.SX, drag_box_.EX, height() - drag_box_.SY,
              height() - drag_box_.EY);
  box *= state_.Scale();
  box += gstBBox(state_.frust.w, state_.frust.w, state_.frust.s,
                 state_.frust.s);

  state_.select = box;

  emit selectBox(state_, btnState);
}

void GfxView::adjustLevel(int step) {
  double nl = log(2 / state_.Scale()) * M_LOG2E - level_zero_ + step;

  if (nl > 24 || std::isnan(nl)) {
    nl = 24;
  } else if (nl < 0) {
    nl = 0;
  }
  if (snap_to_level_) {
    // Round to nl nearest integer.
    nl = round(nl);
  }

  SetScale(1.0 / pow(2, nl + level_zero_ - 1));
  emit levelChanged(nl);
  state_.level = nl;
}

double GfxView::CenterLat() const {
  return khTilespace::Denormalize(state_.frust.CenterY());
}

double GfxView::CenterLon() const {
  return khTilespace::Denormalize(state_.frust.CenterX());
}

void GfxView::setPosition(double lat, double lon, double lev) {
  SetScale(1.0 / pow(2, lev + level_zero_ - 1));

  SetCenter(khTilespace::Normalize(lon), khTilespace::Normalize(lat));
  resizeGL(width(), height());
  repaint();
}

void GfxView::levelOffset(int lev) {
  state_.offset = lev;
  repaint();
}

void GfxView::SetScale(double new_scale) {
  double w = state_.frust.CenterX() - (state_.Width() * 0.5 * new_scale);
  double e = w + (state_.Width() * new_scale);
  double s = state_.frust.CenterY() - (state_.Height() * 0.5 * new_scale);
  double n = s + (state_.Height() * new_scale);
  state_.frust.init(w, e, s, n);
}

void GfxView::SetCenter(double cx, double cy) {
  double wd = state_.frust.Width() * 0.5;
  double hd = state_.frust.Height() * 0.5;
  state_.frust.init(cx - wd, cx + wd, cy - hd, cy + hd);
}

gstVertex GfxView::ConvertScreenToLatLon(int x, int y) const {
  double lng_norm = state_.frust.w + static_cast<double>(x) * state_.Scale();
  double lat_norm = state_.frust.s +
                    static_cast<double>(state_.Height() - y) * state_.Scale();
  if (state_.IsMercatorPreview()) {
    lat_norm = MercatorProjection::
        FromMercatorNormalizedLatitudeToNormalizedLatitude(lat_norm);
  }

  // clamp output to valid ranges
  if (lng_norm < 0.0) {
    lng_norm = 0.0;
  } else if (lng_norm > 1.0) {
    lng_norm = 1.0;
  }
  if (lat_norm < 0.0) {
    lat_norm = 0.0;
  } else if (lat_norm > 1.0) {
    lat_norm = 1.0;
  }

  return gstVertex(lng_norm, lat_norm);
}

gstVertex GfxView::ConvertScreenToNormalizedWorld(int x, int y) const {
  return gstVertex(
      state_.frust.w + (static_cast<double>(x) * state_.Scale()),
      state_.frust.s + (static_cast<double>(state_.Height() - y) * state_.Scale()));
}

void GfxView::SizeToFit(const gstBBox& box) {
  SetScale(std::max(box.Width() / state_.Width(),
                    box.Height() / state_.Height()));
}

void GfxView::resetView() {
  SizeToFit(domain_box_);
  SetCenter(.5, .5);

  resizeGL(width(), height());
  repaint();
}

void GfxView::zoomToBox(const gstBBox& bbox) {
  SizeToFit(bbox);
  SetCenter(bbox.CenterX(), bbox.CenterY());

  resizeGL(width(), height());
  updateGL();
}

void GfxView::toolModeZoomBox(bool state) {
  if (state == 1) {
    tool_mode_ = ZoomBox;
    QPixmap zoombox("zoombox_cursor.png");
    QBitmap mask;
    mask = QPixmap("zoombox_cursor_mask.png");
    zoombox.setMask(mask);
    setCursor(zoombox);
  }
}

void GfxView::toolModeZoomDrag(bool state) {
  if (state == 1) {
    tool_mode_ = ZoomDrag;
    setCursor(Qt::SizeVerCursor);
  }
}

void GfxView::toolModeSelect(bool state) {
  if (state == 1) {
    tool_mode_ = Select;
    setCursor(Qt::ArrowCursor);
  }
}

void GfxView::toolModePan(bool state) {
  if (state == 1) {
    tool_mode_ = Pan;
    setCursor(Qt::PointingHandCursor);
  }
}

void GfxView::toolModeEdit(bool state) {
  if (state == 1) {
    tool_mode_ = Edit;
    setCursor(Qt::CrossCursor);
  }
}


void GfxView::drawGrids(bool state) {
  if (state)
    state_.mode |= DrawGrids;
  else
    state_.mode &= ~DrawGrids;
  repaint();
}

#define TOGGLEstate_(m) \
    if (state_.mode & (m)) { \
      state_.mode &= ~(m); \
    } else {\
      state_.mode |= (m); \
    }


void GfxView::toggleTextures() {
  TOGGLEstate_(DrawTextures);
  repaint();
}

void GfxView::toggleBaseTexture() {
  texture_manager_->toggleBaseTexture();
  repaint();
}

void GfxView::toggleAlpha() {
  alpha_mode_++;
  if (alpha_mode_ > None)
    alpha_mode_ = 0;

  repaint();
}

void GfxView::toggleDebug() {
  TOGGLEstate_(DrawDebug);
  repaint();
}

void GfxView::newTexture() {
  state_.mode |= DrawTextures;
  repaint();
}


void GfxView::keyPressEvent(QKeyEvent* e) {
  if (tool_mode_ == Edit) {
    emit KeyPress(e);
    return;
  }

  switch (e->ascii()) {
    case 3:    // ctrl-c
      if (Preferences::GlobalEnableAll)  {
        TOGGLEstate_(ShowCullFrust);
        repaint();
      }
      break;
    case 12:   // ctrl-l
      if (Preferences::GlobalEnableAll) {
        TOGGLEstate_(DrawLabels);
        repaint();
      }
      break;
    case 13:   // ctrl-m
      TOGGLEstate_(DrawTileMsg);
      repaint();
      break;
    case 'v':
      TOGGLEstate_(DrawVertexes);
      repaint();
      break;
    case 24:   // ctrl-x
      if (Preferences::GlobalEnableAll) {
        TOGGLEstate_(DrawBBoxes);
        repaint();
      }
      break;
    default:
      e->ignore();
      QGLWidget::keyPressEvent(e);
  }
}

void GfxView::wheelEvent(QWheelEvent* e) {
  if (snap_to_level_) {
    adjustLevel(e->delta() <= 0 ? -1 : 1);
  } else {
    double new_scale;
    if (e->delta() > 0) {
      new_scale = state_.Scale() * 0.90000000;
    } else {
      new_scale = state_.Scale() * 1.11111111;
    }

    SetScale(new_scale);

    adjustLevel(0);
  }
  resizeEvent(NULL);
  updateGL();

  e->accept();
}

void GfxView::timerEvent(QTimerEvent* e) {
  if (texture_manager_->renderUnfinished())
    updateGL();
}

void GfxView::ReloadPrefs() {
  PrefsConfig cfg = Preferences::getConfig();

  select_outline_color_[0] =
    static_cast<float>(cfg.selectOutlineColor[0]) / 255.0;
  select_outline_color_[1] =
    static_cast<float>(cfg.selectOutlineColor[1]) / 255.0;
  select_outline_color_[2] =
    static_cast<float>(cfg.selectOutlineColor[2]) / 255.0;
  select_outline_color_[3] =
    static_cast<float>(cfg.selectOutlineColor[3]) / 255.0;

  select_fill_color_[0] = static_cast<float>(cfg.selectFillColor[0]) / 255.0;
  select_fill_color_[1] = static_cast<float>(cfg.selectFillColor[1]) / 255.0;
  select_fill_color_[2] = static_cast<float>(cfg.selectFillColor[2]) / 255.0;
  select_fill_color_[3] = static_cast<float>(cfg.selectFillColor[3]) / 255.0;

  select_outline_line_width_ = static_cast<float>(cfg.selectOutlineLineWidth);
}


void GfxView::drawLabels() {
  QPainter p(this);

  QFont font("Helvetica");
  font.setPointSize(14);
  p.setFont(font);
  emit drawLabels(&p, state_);
}

void GfxView::snapToLevel(bool enable) {
  snap_to_level_ = enable;
  adjustLevel(0);
  repaint();
}

//
// main draw() function...
//
void GfxView::paintGL() {
  khTimer_t start = khTimer::tick();

  glClear(GL_COLOR_BUFFER_BIT);

  state_.cullbox.init(state_.frust);

  double grid = Grid::Step(static_cast<int>(level()));

  if (state_.mode & ShowCullFrust) {
    double w = state_.frust.Width() * 0.25;
    double h = state_.frust.Height() * 0.25;
    state_.cullbox.init(state_.frust.CenterX() - w, state_.frust.CenterX() + w,
                        state_.frust.CenterY() - h, state_.frust.CenterY() + h);
  }

  gstBBox snapBox = gstBBox(state_.cullbox).snapOut(grid);

  if (level() < 0 || level() > 32 || static_cast<int>(level()) < 0) {
    notify(NFY_WARN, "Bogus level: %f (%d)", level(),
           static_cast<int>(level()));
    return;
  }

  drawTextures(snapBox, grid, static_cast<int>(level()));

  khTimer_t tex_check = khTimer::tick();
  tex_stat_[frame_num_ % STAT_FRAME] = khTimer::delta_m(start, tex_check);

  emit drawVectors(state_);

  khTimer_t geom_check = khTimer::tick();
  geom_stat_[frame_num_ % STAT_FRAME] = khTimer::delta_m(tex_check,
                                                           geom_check);

  // draw major grid lines (1024 tile size) as red
  // and minor lines (256 tile size) as green
  if (state_.mode & DrawGrids) {
    glLineWidth(1);

    glBegin(GL_LINES);
    // drawlongitude lines
    double line;
    for (line = snapBox.w - state_.frust.w;
         line <= snapBox.e - state_.frust.w; line += grid) {
      unsigned int col = uint((line + state_.frust.w) / grid + 0.5);
      if (col % 4 == 0)
        glColor3f(1, 0, 0);
      else
        glColor3f(0, 1, 0);
      glVertex2f(line, snapBox.s - state_.frust.s);
      glVertex2f(line, snapBox.n - state_.frust.s);
    }
    glColor3f(0, 1, 0);
    // draw latitude lines
    for (line = snapBox.s - state_.frust.s;
         line <= snapBox.n - state_.frust.s; line += grid) {
      unsigned int row = uint((line + state_.frust.s) / grid + 0.5);
      if (row % 4 == 0)
        glColor3f(1, 0, 0);
      else
        glColor3f(0, 1, 0);
      glVertex2f(snapBox.w - state_.frust.w, line);
      glVertex2f(snapBox.e - state_.frust.w, line);
    }
    glEnd();
  }

  if (state_.mode & ShowCullFrust) {
    glLineWidth(2);
    glColor3f(1.0, 0, 0);
    glBegin(GL_LINE_LOOP);
    glVertex2f(state_.cullbox.w - state_.frust.w,
               state_.cullbox.s - state_.frust.s);
    glVertex2f(state_.cullbox.w - state_.frust.w,
               state_.cullbox.n - state_.frust.s);
    glVertex2f(state_.cullbox.e - state_.frust.w,
               state_.cullbox.n - state_.frust.s);
    glVertex2f(state_.cullbox.e - state_.frust.w,
               state_.cullbox.s - state_.frust.s);
    glEnd();
  }

  if (rubberband_on_) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width(), height(), 0, -1, 1);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glColor4fv(select_fill_color_);
    glBegin(GL_QUADS);
    glVertex2f(drag_box_.SX, drag_box_.SY);
    glVertex2f(drag_box_.EX, drag_box_.SY);
    glVertex2f(drag_box_.EX, drag_box_.EY);
    glVertex2f(drag_box_.SX, drag_box_.EY);
    glEnd();

    if (select_outline_line_width_ > 0) {
      glColor4fv(select_outline_color_);
      glLineWidth(select_outline_line_width_);
      glBegin(GL_LINE_LOOP);
      glVertex2f(drag_box_.SX, drag_box_.SY);
      glVertex2f(drag_box_.EX, drag_box_.SY);
      glVertex2f(drag_box_.EX, drag_box_.EY);
      glVertex2f(drag_box_.SX, drag_box_.EY);
      glEnd();
    }

    glDisable(GL_BLEND);
    glPopMatrix();
  }

  if (state_.mode & DrawDebug) {
    static gstHistogram* texhist = texture_manager_->textureCacheHist();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);

    if (texture_manager_->renderUnfinished()) {
      int ticks = texture_manager_->renderUnfinished();
      float p = 0.1;
      float s = 0.005;
      for (int ii = 0; ii < ticks; ii++) {
        glColor3f(2, 0, 1);
        glBegin(GL_POLYGON);
        glVertex2f(p, .045);
        glVertex2f(p + s, .045);
        glVertex2f(p + s, .055);
        glVertex2f(p, .055);
        glEnd();
        p += (s * 1.5);
      }
    }

    float step = .8 / texhist->size();
    float height = .8 / 200;

    glColor3f(1, 0.2, 0.2);
    for (int ii = 0; ii < texhist->size(); ++ii) {
      glBegin(GL_POLYGON);
      glVertex2f(.1 + (step * static_cast<float>(ii)), .1);
      glVertex2f(.1 + (step * static_cast<float>(ii)), .1 +
                 (height * static_cast<float>((*texhist)[ii])));
      glVertex2f(.1 + (step * static_cast<float>(ii)) + (step * .5), .1 +
                 (height * static_cast<float>((*texhist)[ii])));
      glVertex2f(.1 + (step * static_cast<float>(ii)) + (step * .5), .1);
      glEnd();
    }

    glPopMatrix();
  }

#if 0
  if (GlobalEnableAll) {
    double textime = 0;
    double geomtime = 0;
    for (int ii =  0; ii < STAT_FRAME; ii++) {
      textime += tex_stat_[ ii ];
      geomtime += geom_stat_[ ii ];
    }
    textime /= STAT_FRAME;
    geomtime /= STAT_FRAME;
    emit drawstats(frame_num_, textime, geomtime);
  }
#else
  emit drawstats(frame_num_, tex_stat_[frame_num_ % STAT_FRAME],
                 geom_stat_[frame_num_ % STAT_FRAME]);
#endif

  // if this timer hasn't been triggerd yet, it will get reset
  // effectively making sure the labels are never drawn until
  // all textures are loaded and drawn
  label_timer_->start(200, true);

  ++frame_num_;
}  // End GfxView::paintGL()


void GfxView::initializeGL() {
  InitGLFonts();

  glClearColor(0, 0, 0, 1);
  glShadeModel(GL_FLAT);

  state_.SetWidthHeight(width(), height());
  SizeToFit(domain_box_);
  SetCenter(.5, .5);

  texture_manager_->glinit();

  adjustLevel(0);

  //
  // define a few display lists here to make drawing fancy vertexes easier
  //

  // normal vertex display list
  glNewList(VERTEX_NORMAL, GL_COMPILE);
  glBegin(GL_QUADS);
  glVertex2f(.35, 0);
  glVertex2f(0, .35);
  glVertex2f(-.35, 0);
  glVertex2f(0, -.35);
  glEnd();
  glBegin(GL_LINES);
  glVertex2f(-.5, 0);
  glVertex2f(.5, 0);
  glVertex2f(0, -.5);
  glVertex2f(0, .5);
  glEnd();
  glEndList();

  // selected vertex display list
  const GLint sg = 20;           // number of segments in this circle
  const GLfloat radius = 1.0f;   // use glScale to get desired size
  GLfloat angle;
  glNewList(VERTEX_SELECT, GL_COMPILE);
  // draw circle
  glBegin(GL_LINE_LOOP);
  for (int ii = 0; ii < sg; ++ii) {
    angle = 2 * M_PI * ii / sg;
    glVertex2f(cos(angle) * radius, sin(angle) * radius);
  }
  glEnd();
  glCallList(VERTEX_NORMAL);
  glEndList();
}


void GfxView::ValidateGfxMode() {
  // ensure OpenGL context is current
  makeCurrent();

  GLint maxtex;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtex);
  MaxTexSize = QString("%1 x %2").arg(maxtex).arg(maxtex);

  GLint red, green, blue, alpha;
  glGetIntegerv(GL_RED_BITS, &red);
  glGetIntegerv(GL_GREEN_BITS, &green);
  glGetIntegerv(GL_BLUE_BITS, &blue);
  glGetIntegerv(GL_ALPHA_BITS, &alpha);
  BitsPerComp = QString("%1 / %2 / %3 / %4").arg(red).arg(green).arg(blue).
                arg(alpha);

  //
  // provide a warning dialog if correct visual is not available
  //
  if (!Preferences::getConfig().warnOnBadGfxMode)
    return;

  if (red != 8 || green != 8 || blue != 8 || alpha != 8) {
    QString clrmode = QString("%1/%2/%3/%4").arg(red).arg(green).
                      arg(blue).arg(alpha);
    QMessageBox error(
      "Error",
      kh::tr("Unable to find appropriate graphics mode.\n") +
      kh::tr("Fusion needs 24 bits for RGB and 8 bits for alpha, or 8/8/8/8.\n") +
      kh::tr("This system is currently configured to provide: ") +
      clrmode + "\n" +
      kh::tr("Perhaps your graphics driver is not installed correctly.\n") +
      kh::tr("Fusion will attempt to continue, but graphics may perform poorly."),
      QMessageBox::Critical,                      // icon
      QMessageBox::Ok,                            // button #1 text
      QMessageBox::NoButton,                      // button #2 text
      QMessageBox::NoButton,                      // button #3 text
      this,                                       // parent
      0,                                          // name
      true,                                       // modal
      Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);   // style
    error.exec();
  }
}

void GfxView::resizeGL(int w, int h) {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glViewport(0, 0, (GLint)w, (GLint)h);

  state_.SetWidthHeight(w, h);
  SetScale(state_.Scale());

  glOrtho(0.0f, state_.frust.Width(), 0.0f, state_.frust.Height(), -1.0f, 1.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  adjustLevel(0);
}

static float tileColors[][4] = {
  {1, 0, 0, .4},
  {0, 1, 0, .4},
  {0, 0, 1, .4},
  {1, 1, 0, .4},
  {1, 0, 1, .4},
  {0, 1, 1, .4}
};

void GfxView::drawTextures(const gstBBox &box, double grid, int level) {
  if (level < 0) {
    notify(NFY_WARN,
           "Bogus level, GfxView::drawTextures() wants to draw level: %d", level);
    return;
  }

  //
  // swap the front and back texture read queues,
  // and clear the back queue so we can start stuffing
  // it all over again.
  // This ensures that we don't leave any non-relevant
  // tiles in our read queue beyond one frame
  //
  //texture_manager_->resetFrame();

  // draw longitude lines
  for (double xx = box.w; xx < box.e; xx += grid) {
    // draw latitude lines
    for (double yy = box.s; yy < box.n; yy += grid) {
      int lev = 0;     // actual level of supplied texture

      TexTile tile(xx, yy, grid, level);

      // check bounds first
      bool validBounds = tile.isValid();
      if (!validBounds) {
        drawInvalidTile(xx, yy, grid);
      } else if (!(state_.mode & DrawTextures)) {
        drawSolidTile(xx, yy, grid);
      } else {
        lev = texture_manager_->prepareBaseTexture(tile);
        drawTextureTile(tile);
        if (lev >= 0)
          texture_manager_->recycleTexture(tile.addr());

        // get a list of all overlay textures for this tile
        TexTile tile(xx, yy, grid, level, 0);
        std::vector<gstTextureGuard> drawlist;
        texture_manager_->GetOverlayList(tile, drawlist,
                                         state_.IsMercatorPreview());

        for (std::vector<gstTextureGuard>::reverse_iterator it = drawlist.rbegin();
             it != drawlist.rend(); ++it) {
          if (alpha_mode_ == Transparent || alpha_mode_ == Solid ||
              alpha_mode_ == NoTexture) {
            TexTile atile(xx, yy, grid, level, 1);
            int alev = texture_manager_->prepareTexture(atile, *it);
            if (alev != -1) {
              if (alpha_mode_ == NoTexture)
                drawSolidTile(xx, yy, grid);
              drawAlphaTile(atile);
              texture_manager_->recycleTexture(atile.addr());
            }
          }

          if (alpha_mode_ == Transparent || alpha_mode_ == None) {
            TexTile otile(xx, yy, grid, level, 0);
            int olev = texture_manager_->prepareTexture(otile, *it);
            if (olev != -1) {
              drawTextureTile(otile, true);
              texture_manager_->recycleTexture(otile.addr());
              lev = olev;
            }
          }
        }
      }


      // debug decorations only get applied to valid tiles
      if ((state_.mode & DrawGrids) && validBounds &&
          (!(state_.mode & DrawDebug)))
        drawGridLines(xx, yy, grid, lev);

      if (state_.mode & DrawDebug)
        drawTileAddress(xx, yy, grid, level);
      else if (state_.mode & DrawTileMsg)
        drawTileMessages(tile);
    }
  }

  // if there are outstanding texture loads then
  //   start a timer which forces a redraw
  if (texture_manager_->renderUnfinished()) {
    if (tex_load_timer_ == 0) {
      tex_load_timer_ = startTimer(40);
      if (tex_load_timer_ == 0)
        notify(NFY_WARN, "Unable to start texture loading timer!");
    }
    emit renderbusy(texture_manager_->renderUnfinished());
  } else if (tex_load_timer_ != 0) {
    // stop the timer when there are no longer
    //   any outstanding texture loads
    killTimer(tex_load_timer_);
    tex_load_timer_ = 0;
    emit renderbusy(0);
  }
}


// alpha tile
void GfxView::drawAlphaTile(const TexTile& tile) {
  glEnable(GL_TEXTURE_2D);

  if (alpha_mode_ == Transparent) {
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  } else {
    glColor4f(1.0, 0.0, 0.0, 1.0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
  }

  glBegin(GL_QUADS);
  glTexCoord2f(tile.s0, tile.t0);
  glVertex2f(tile.xx - state_.frust.w, tile.yy - state_.frust.s);

  glTexCoord2f(tile.s0, tile.t1);
  glVertex2f(tile.xx - state_.frust.w, tile.yy + tile.grid - state_.frust.s);

  glTexCoord2f(tile.s1, tile.t1);
  glVertex2f(tile.xx - state_.frust.w + tile.grid, tile.yy + tile.grid -
             state_.frust.s);

  glTexCoord2f(tile.s1, tile.t0);
  glVertex2f(tile.xx - state_.frust.w + tile.grid, tile.yy - state_.frust.s);
  glEnd();

  glDisable(GL_TEXTURE_2D);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDisable(GL_BLEND);
}


// textured tile
void GfxView::drawTextureTile(const TexTile& tile, bool blend) {
  if (blend) {
    glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA);
    glEnable(GL_BLEND);
  }

  glColor4f(1.0, 1.0, 1.0, 1.0);

  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glTexCoord2f(tile.s0, tile.t0);
  glVertex2f(tile.xx - state_.frust.w, tile.yy - state_.frust.s);

  glTexCoord2f(tile.s0, tile.t1);
  glVertex2f(tile.xx - state_.frust.w, tile.yy + tile.grid - state_.frust.s);

  glTexCoord2f(tile.s1, tile.t1);
  glVertex2f(tile.xx - state_.frust.w + tile.grid, tile.yy + tile.grid -
             state_.frust.s);

  glTexCoord2f(tile.s1, tile.t0);
  glVertex2f(tile.xx - state_.frust.w + tile.grid, tile.yy - state_.frust.s);
  glEnd();
  glDisable(GL_TEXTURE_2D);

  if (blend)
    glDisable(GL_BLEND);
}

// solid color tile
void GfxView::drawSolidTile(double xx, double yy, double grid) {
  glColor3f(.10, .10, .50);
  glBegin(GL_QUADS);
  glVertex2f(xx - state_.frust.w, yy - state_.frust.s);
  glVertex2f(xx - state_.frust.w, yy + grid - state_.frust.s);
  glVertex2f(xx - state_.frust.w + grid, yy + grid - state_.frust.s);
  glVertex2f(xx - state_.frust.w + grid, yy - state_.frust.s);
  glEnd();
}

// invalid (out of bounds) tile
void GfxView::drawInvalidTile(double xx, double yy, double grid) {
  double left = xx - state_.frust.w;
  double right = xx + grid - state_.frust.w;
  double bottom = yy - state_.frust.s;
  double top = yy + grid - state_.frust.s;
  double s = (right - left) * .2;

  double x0;
  double x1;
  double y0;
  double y1;

  // draw light-grey background
  glColor3f(0.4, 0.4, 0.4);
  glBegin(GL_POLYGON);
    glVertex2f(left, bottom);
    glVertex2f(right, bottom);
    glVertex2f(right, top);
    glVertex2f(left, top);
  glEnd();

  // draw hash pattern
  glLineWidth(2);
  glColor3f(0.05, 0.05, 1.0);
  glBegin(GL_LINES);
  for (float x = left - grid; x < right; x += s) {
    if (x < left) {
      x0 = left;
      y0 = bottom + (left - x);
      x1 = x + grid;
      y1 = top;
      glVertex2f(x0, y0);
      glVertex2f(x1, y1);
      y0 = top - (left - x);
      y1 = bottom;
      glVertex2f(x0, y0);
      glVertex2f(x1, y1);
    } else {
      x0 = x;
      y0 = bottom;
      x1 = right;
      y1 = top - (x - left);
      glVertex2f(x0, y0);
      glVertex2f(x1, y1);
      y0 = top;
      y1 = bottom + (x - left);
      glVertex2f(x0, y0);
      glVertex2f(x1, y1);
    }
  }
  glEnd();
}


void GfxView::drawGridLines(double xx, double yy, double grid, int lev) {
  glPushAttrib(GL_LINE_BIT);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glColor4fv(tileColors[lev % 6]);

  double line_width = grid * .05;

  double outer_left = xx - state_.frust.w;
  double outer_right = xx + grid - state_.frust.w;
  double outer_bottom = yy - state_.frust.s;
  double outer_top = yy + grid - state_.frust.s;

  double inner_left = xx + line_width - state_.frust.w;
  double inner_right = xx + grid - line_width - state_.frust.w;
  double inner_bottom = yy + line_width - state_.frust.s;
  double inner_top = yy + grid -line_width - state_.frust.s;

  glBegin(GL_TRIANGLE_STRIP);
  glVertex2f(outer_left, outer_top);
  glVertex2f(inner_left, inner_top);
  glVertex2f(outer_right, outer_top);
  glVertex2f(inner_right, inner_top);
  glVertex2f(outer_right, outer_bottom);
  glVertex2f(inner_right, inner_bottom);
  glVertex2f(outer_left, outer_bottom);
  glVertex2f(inner_left, inner_bottom);
  glVertex2f(outer_left, outer_top);
  glVertex2f(inner_left, inner_top);
  glEnd();

  glDisable(GL_BLEND);

  glPopAttrib();
}

void GfxView::drawTileAddress(double xx, double yy, double grid, int lev) {
  unsigned int row = (unsigned int)((yy) / grid);
  unsigned int col = (unsigned int)((xx) / grid);

  glColor3f(1.0, 1.0, .2);
  char txt[512];
  double west = xx - state_.frust.w + (grid * .1);
  double line = (grid * .1);
  double south = yy -  state_.frust.s + line;

  unsigned char blist[32];
  char child[] = "0123";
  xyToBlist(col, row, lev, blist);
  for (int l = 0; l < lev; l++)
    txt[l] = child[blist[l]];
  txt[lev] = '\0';
  glRasterPos2d(west, south);
  PrintGLString(txt);
  south += line;

  glRasterPos2d(west, south);
  PrintGLString("c:%d", col);
  south += line;

  glRasterPos2d(west, south);
  PrintGLString("r:%d", row);
  south += line;

  glRasterPos2d(west, south);
  PrintGLString("l:%d", lev);
  south += line;

  // Draw raster product tile address
  if (((row & 0x3) == 3) && ((col & 0x3) == 0)) {
    south = (yy + grid) -  state_.frust.s - line;

    // Draw in red
    glColor3f(1.0, 0.0, 0.0);

    glRasterPos2d(west, south);
    PrintGLString("rp-l:%d", ImageryToProductLevel(lev));
    south -= line;

    glRasterPos2d(west, south);
    PrintGLString("rp-r:%d", row >> 2);
    south -= line;

    glRasterPos2d(west, south);
    PrintGLString("rp-c:%d", col >> 2);
    south -= line;
  }
}

void GfxView::drawTileMessages(TexTile & /* tile */) {
#if 0
  char msg[1024];
  int status = texture_manager_->getTileMessage(tile, msg);

  if (status != GST_OKAY)
    return;

  unsigned int row = (unsigned int)((tile.yy) / tile.grid);
  unsigned int col = (unsigned int)((tile.xx) / tile.grid);

  glColor3f(1.0, 1.0, .2);
  char txt[512];
  double west = tile.xx - state_.frust.w + (grid * .1);
  double line = (grid * .1);
  double south = tile.yy -  state_.frust.s + line;


  glRasterPos2d(west, south);
  printString(msg);
  south += line;

#endif
}

void GfxView::dragEnterEvent(QDragEnterEvent* event) {
  if (event->provides("text/plain"))
    event->accept(true);
}

void GfxView::dropEvent(QDropEvent* event) {
  QString text;

  if (QTextDrag::decode(event, text))
    emit dropFile(text);
}
