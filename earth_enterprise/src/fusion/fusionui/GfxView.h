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


#ifndef _GfxView_h_
#define _GfxView_h_

#include <Qt/qobjectdefs.h>
#include <Qt/qgl.h>
#include <Qt/q3action.h>
#include <Qt/qevent.h>
#include <gstGeode.h>
#include <Qt/q3combobox.h>
class gstTextureManager;
class TexTile;
class QTimer;
class QPainter;

#define STAT_FRAME 50

class GfxView : public QGLWidget {
  Q_OBJECT

 public:
  static QString MaxTexSize;
  static QString BitsPerComp;

  enum ToolMode {
    Unknown  = 0x00000000,
    Select   = 0x00010000,
    Pan      = 0x00020000,
    ZoomDrag = 0x00030000,
    ZoomBox  = 0x00040000,
    Edit     = 0x00050000,
    ToolMask = 0x00ff0000
  };

  static GfxView* instance;

  GfxView(QWidget* parent, const char *name);
  GfxView(QWidget* parent);
  ~GfxView(void);

  gstDrawState* state() { return &state_; }

  double level() const { return state_.level; }

  double CenterLat() const;
  double CenterLon() const;

  void setPosition(double lat, double lon, double lev);

  gstTextureManager* textureManager() const { return texture_manager_; }

  //int TileSize() const { return tile_size_; }

  void newTexture();

  void ValidateGfxMode();

  void SizeToFit(const gstBBox& box);
  void SetScale(double new_scale);
  void SetCenter(double cx, double cy);
  gstVertex ConvertScreenToLatLon(int x, int y) const;
  gstVertex ConvertScreenToNormalizedWorld(int x, int y) const;
  static void SetIsMercatorPreview(bool is_mercator_preview) {
    if (instance != NULL) {
      instance->state_.SetIsMercatorPreview(is_mercator_preview);
    }
  }
  static bool IsMercatorPreview() {
    return ((instance != NULL) ? instance->state_.IsMercatorPreview() : false);
  }

 public slots:
  void drawGrids(bool);
  void toggleTextures();
  void toggleBaseTexture();
  void toggleAlpha();
  void toggleDebug();
  void toolModeSelect(bool);
  void toolModeZoomBox(bool);
  void toolModeZoomDrag(bool);
  void toolModePan(bool);
  void toolModeEdit(bool);
  void levelOffset(int);
  void resetView();
  void zoomToBox(const gstBBox &);
  void drawLabels();
  void ReloadPrefs();
  void snapToLevel(bool);

  signals:
  void levelChanged(double);
  void latlonChanged(double, double);
  void drawVectors(const gstDrawState&);
  void drawLabels(QPainter*, const gstDrawState&);
  void selectBox(const gstDrawState&, Qt::ButtonState);
  void drawstats(int, double, double);
  void renderbusy(int);
  void dropFile(const QString&);
  void contextMenu(QWidget* parent, const QPoint& pos);

  void MousePress(const gstBBox& b, Qt::ButtonState);
  void MouseMove(const gstVertex& v);
  void MouseRelease();

  void KeyPress(QKeyEvent* e);

 protected:
  // inherited from qglwidget
  virtual void initializeGL();
  virtual void paintGL();
  virtual void resizeGL(int w, int h);

  // inherited from qwidget
  virtual void contextMenuEvent(QContextMenuEvent* e);
  virtual void mousePressEvent(QMouseEvent* e);
  virtual void mouseMoveEvent(QMouseEvent* e);
  virtual void mouseReleaseEvent(QMouseEvent* e);
  virtual void keyPressEvent(QKeyEvent* e);
  virtual void wheelEvent(QWheelEvent* e);

  // inherited from qobject
  virtual void timerEvent(QTimerEvent* e);

  void renderText();
  void drawTextures(const gstBBox& box, double, int level);

  ToolMode GetToolMode() const { return tool_mode_; }

  // Adjust the level of the preview display and observe snap_to_level_ setting.
  // step: is an integer increment of the level.
  void adjustLevel(int step);

  void drawTextureTile(const TexTile& tile, bool blend = FALSE);
  void drawAlphaTile(const TexTile& tile);
  void drawSolidTile(double xx, double yy, double grid);
  void drawInvalidTile(double xx, double yy, double grid);
  void drawGridLines(double xx, double yy, double grid, int lev);
  void drawTileAddress(double xx, double yy, double grid, int lev);
  void drawTileMessages(TexTile &tile);

 protected:
  void dragEnterEvent(QDragEnterEvent *e);
  void dropEvent(QDropEvent *e);

 private:
  int frame_num_;
  int rubberband_on_;

  struct DragBox {
    double SX, SY;
    double EX, EY;
    void start(int x, int y) { SX = double(x); SY = double(y); }
    void end(int x, int y) { EX = double(x); EY = double(y); }
    double dX() { return EX - SX; }
    double dY() { return EY - SY; }
    double cX() { return (EX + SX) * .5; }
    double cY() { return (EY + SY) * .5; }
  };

  DragBox drag_box_;

  void selectFeatures(Qt::ButtonState);

  double start_pan_[2];
  double start_scale_;

  gstTextureManager* texture_manager_;
  int tex_load_timer_;
  gstDrawState state_;

  //gstBBox view_box_;
  gstBBox domain_box_;

  int tile_size_;
  int level_zero_;

  ToolMode tool_mode_;
  ToolMode save_tool_mode_;
  bool snap_to_level_;

  enum {
    Transparent = 0,
    Solid = 1,
    NoTexture = 2,
    None = 3
  };

  std::uint32_t alpha_mode_;

  double tex_stat_[STAT_FRAME];
  double geom_stat_[STAT_FRAME];

  QTimer* label_timer_;

  //QPainter *label_painter_;

  float select_outline_color_[4];
  float select_outline_line_width_;
  float select_fill_color_[4];
};

#endif  // !_GfxView_h_
