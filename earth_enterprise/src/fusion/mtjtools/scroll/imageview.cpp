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

//
//      Qt-based Image View widget
//

#include <qimage.h>
#include <qbitmap.h>
#include <qpainter.h>
#include <qstatusbar.h>
#include <qcursor.h>
#include <QtGui/QKeyEvent>
#include "imageview.h"
#include "khistogram.h"
#include "zoom.h"
#include <khFileUtils.h>

/* XPM */
static const char *box0[] = {
  "32 32 2 1",
  ". c none",
  "# c white",
  "................................",
  "................................",
  "................................",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  ".........#.....#......#.........",
  "..........#....#.....#..........",
  "...........#...#....#...........",
  "............#..#...#............",
  ".............#.#..#.............",
  "................................",
  "...###########...###########....",
  "................................",
  ".............#.#.#..............",
  "............#..#..#.............",
  "...........#...#...#............",
  "..........#....#....#...........",
  ".........#.....#.....#..........",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "................................",
  "................................",
  "................................",
  "................................",
};

/* XPM */
static const char *box1[] = {
  "32 32 2 1",
  ". c none",
  "# c white",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  ".............#####..............",
  ".............#...#..............",
  ".........#####...#####..........",
  ".............#...#..............",
  ".............#####..............",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
};

/* XPM */
static const char *box2[] = {
  "32 32 2 1",
  ". c none",
  "# c white",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "............#######.............",
  "............#.....#.............",
  "............#.....#.............",
  "........#####.....#####.........",
  "............#.....#.............",
  "............#.....#.............",
  "............#######.............",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
};

/* XPM */
static const char *box3[] = {
  "32 32 2 1",
  ". c none",
  "# c white",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "...........#########............",
  "...........#.......#............",
  "...........#.......#............",
  "...........#.......#............",
  ".......#####.......#####........",
  "...........#.......#............",
  "...........#.......#............",
  "...........#.......#............",
  "...........#########............",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
};

/* XPM */
static const char *box4[] = {
  "32 32 2 1",
  ". c none",
  "# c white",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "..........###########...........",
  "..........#.........#...........",
  "..........#.........#...........",
  "..........#.........#...........",
  "..........#.........#...........",
  "......#####.........#####.......",
  "..........#.........#...........",
  "..........#.........#...........",
  "..........#.........#...........",
  "..........#.........#...........",
  "..........###########...........",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
};

/* XPM */
static const char *box5[] = {
  "32 32 2 1",
  ". c none",
  "# c white",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  ".........#############..........",
  ".........#...........#..........",
  ".........#...........#..........",
  ".........#...........#..........",
  ".........#...........#..........",
  ".........#...........#..........",
  ".....#####...........#####......",
  ".........#...........#..........",
  ".........#...........#..........",
  ".........#...........#..........",
  ".........#...........#..........",
  ".........#...........#..........",
  ".........#############..........",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
};

/* XPM */
static const char *box6[] = {
  "32 32 2 1",
  ". c none",
  "# c white",
  "................................",
  "................................",
  "................................",
  "................................",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "........###############.........",
  "........#.............#.........",
  "........#.............#.........",
  "........#.............#.........",
  "........#.............#.........",
  "........#.............#.........",
  "........#.............#.........",
  "....#####.............#####.....",
  "........#.............#.........",
  "........#.............#.........",
  "........#.............#.........",
  "........#.............#.........",
  "........#.............#.........",
  "........#.............#.........",
  "........###############.........",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "................................",
  "................................",
  "................................",
  "................................",
  "................................",
};

/* XPM */
static const char *box7[] = {
  "32 32 2 1",
  ". c none",
  "# c white",
  "................................",
  "................................",
  "................................",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  ".......#################........",
  ".......#...............#........",
  ".......#...............#........",
  ".......#...............#........",
  ".......#...............#........",
  ".......#...............#........",
  ".......#...............#........",
  ".......#...............#........",
  "...#####...............#####....",
  ".......#...............#........",
  ".......#...............#........",
  ".......#...............#........",
  ".......#...............#........",
  ".......#...............#........",
  ".......#...............#........",
  ".......#...............#........",
  ".......#################........",
  "...............#................",
  "...............#................",
  "...............#................",
  "...............#................",
  "................................",
  "................................",
  "................................",
  "................................",
};
  
static const size_t maxbuflen  = 512;

ImageView::ImageView(QWidget *parent, const char *name, WFlags f)
    :QScrollView (parent, name, f
                  |Qt::WNoAutoErase
                  |Qt::WStaticContents
                  |Qt::WPaintClever
                  //|Qt::WPaintUnclipped
                  ) , _hr(65536), _hg(65536), _hb(65536) {
  _filename[0] = '\0';

  _resetOnFileOpen = 1;
  _swapRedAndBlue = 0;

  _width = 0;
  _height = 0;
  _bytes = 0;

  _dataset = NULL;
  _datasetType = 0;

  _statusBar = NULL;

  _cursors[0] = box0;
  _cursors[1] = box1;
  _cursors[2] = box2;
  _cursors[3] = box3;
  _cursors[4] = box4;
  _cursors[5] = box5;
  _cursors[6] = box6;
  _cursors[7] = box7;

  _accumulate = true;
  _leftN  = 10;
  _leftD  = 2000;
  _left = double(_leftN)/double(_leftD);

  _rightN = 10;
  _rightD = 2000;
  _right = double(_rightN)/double(_rightD);

  // pixel averaging work area
  _radius = 0;
  _radiusLimit = 7;
  int size = 2*_radiusLimit + 1; // 1x1, 3x3, 5x5, ..., 15x15
  _ra = (unsigned char *)calloc(size*size*2, 1);
  _ga = (unsigned char *)calloc(size*size*2, 1);
  _ba = (unsigned char *)calloc(size*size*2, 1);

  prevZoom  = _zoom = 0;
  prevScale = _scale = 1.0;
  prevX     = 0;
  prevY     = 0;

  // image read and conversion work area

  //_rowLimit = 99999; // "infinite" limit on rows-per-update, can change on the fly
  _rowLimit = 32; // moderate value to provide constant feedback

  //_pixelCache = 20*2048; //tiny test value to force row-at-a-time updates
  _pixelCache = 262144; //seems smooth on Athlon-XP/1800
  //_pixelCache = (1280+128)*(1024+128); //big value allows screen-at-a-time updates
  //_pixelCache = 4096*4096; //huge test value

  _r16    = (unsigned short *)calloc(1*_pixelCache*2, 1);
  _g16    = (unsigned short *)calloc(1*_pixelCache*2, 1);
  _b16    = (unsigned short *)calloc(1*_pixelCache*2, 1);

  _r8     = (unsigned char  *)calloc(1*_pixelCache*1, 1);
  _g8     = (unsigned char  *)calloc(1*_pixelCache*1, 1);
  _b8     = (unsigned char  *)calloc(1*_pixelCache*1, 1);

  _buffer = (unsigned char  *)calloc(4*_pixelCache*1, 1);

  _sample = NULL;
  _samples = 0;
  _sampleMax = 0;
  _saveNeeded = 0;

  _rLut = (unsigned char *)calloc(65536*sizeof(unsigned char), 1);
  _gLut = (unsigned char *)calloc(65536*sizeof(unsigned char), 1);
  _bLut = (unsigned char *)calloc(65536*sizeof(unsigned char), 1);

  _correct = true;
  _protect = true;

  _preGammaN = 40;
  _preGammaD = 40;
  _preGamma = double(_preGammaN)/double(_preGammaD);

  _postGammaN = 40;
  _postGammaD = 40;
  _postGamma = double(_postGammaN)/double(_postGammaD);

  _contrast = 0;

  _globalMask = 0;

  updateLUTs();

  setHScrollBarMode(QScrollView::AlwaysOn);
  setVScrollBarMode(QScrollView::AlwaysOn);

  viewport()->setMouseTracking(true);
  viewport()->setFocusPolicy(Qt::StrongFocus);

  setRadius(0);
}

void ImageView::setGlobalHisIn(const std::string &in) {
  _globalMask |= 1;
  gHisIn = in;
  if (khHasExtension(in, ".his")) {
    gLutOut = khReplaceExtension(in, ".lut");
    gLutWorkOut = khReplaceExtension(in, ".lutwork");
  } else {
    gLutOut = in;
    gLutOut += ".lut";
    gLutWorkOut += ".lutwork";
  }
}

void ImageView::setGlobalLutOut(const std::string &out) {
  _globalMask  |= 2;
  gLutOut = out;
}

void ImageView::loadInitImage(const std::string &image_file_path) {
  char *temp_path;
  int pathlen = image_file_path.length();
  temp_path = new char[pathlen + 1];
  strncpy(temp_path, image_file_path.c_str(), pathlen + 1);
  this->setFilename(temp_path);
  delete [] temp_path;
}

void ImageView::setLutWork(const std::string &lutwork_file) {
  // Read points from point file as if the user had clicked them
  if (khExists(lutwork_file)) {
    FILE *fh;
    int r, g, b;
//    float left, right, postGamma, rsa, rsb, bsa, bsb ;

    if (_sampleMax < 256) _sampleMax = 256; // minimum array size for _sample
    if (_sample == NULL) {
      _sample = (RGBSample *)calloc(sizeof(RGBSample), _sampleMax);
    }
    int filelength = lutwork_file.length();
    char *ascii_lutwork = new char[filelength + 1];
    strncpy(ascii_lutwork, lutwork_file.c_str(), filelength + 1);
    fh = fopen(ascii_lutwork, "r");
    delete [] ascii_lutwork;
    fscanf(fh,
      "%lf %d %d %lf %d %d %lf %lf %d",
      &_left,
      &_leftN,
      &_leftD,
      &_right,
      &_rightN,
      &_rightD,
      &_preGamma,
      &_postGamma,
      &_contrast
    );
    while (fscanf(fh, "%d %d %d", &r, &g, &b) != EOF) {
      if (_samples >= _sampleMax) {
        _sampleMax *= 2;
        _sample = (RGBSample *)realloc(_sample, sizeof(RGBSample)*_sampleMax);
      }
      // remember
      _sample[_samples].x = 0; // x, y unknown, hope this does not hurt us.
      _sample[_samples].y = 0;
      _sample[_samples].r = (unsigned short)r;
      _sample[_samples].g = (unsigned short)g;
      _sample[_samples].b = (unsigned short)b;
      _rSolve.add(_sample[_samples].r, _sample[_samples].g); // (x,y) := (r,g)
      _bSolve.add(_sample[_samples].b, _sample[_samples].g); // (x,y) := (b,g)
      _samples++;
    }
    fclose(fh);
    // update the RGB look-up-tables (LUTs) using new color-correction coefficients
    updateClip(_left, _right);
    updateLUTs();
    if (_correct) {
      updateContents(); // redraw contents to show effect of updated color correction
    } else {
      update(); // just update GUI elements
    }
  }
}

void ImageView::setRadius(int r) {
  if (r < 0)
    r = 0;
  if (r > _radiusLimit)
    r = _radiusLimit;

  _radius = r;

  // change cursor to indicate "window" in which pixels will be averaged
  QPixmap box(_cursors[_radius]);
  box.selfMask();
  QCursor cursor(box, 15, 15); // hot-spot at (15,15)
  viewport()->setCursor(cursor);

  int rValue = 0;
  int gValue = 0;
  int bValue = 0;
  QPoint where = QCursor::pos();
  sampleImage(where.x(), where.y(), rValue, gValue, bValue);
}

ImageView::~ImageView() {
  free (_rLut);
  free (_gLut);
  free (_bLut);

  free(_r16);
  free(_g16);
  free(_b16);

  free(_r8);
  free(_g8);
  free(_b8);

  free(_buffer);

  free(_ra);
  free(_ga);
  free(_ba);
}

void ImageView::setFilename(char *name) {
  // Display specified image file
  char text[maxbuflen];

  // close old file
  if (_dataset != NULL) {
    GDALClose(_dataset);
    _dataset = NULL;
    _filename[0] = '\0';
  }

  if (name == NULL || *name == '\0') {
    _statusBar->message("No filename specified", 0);
    return;
  }

  // remember new file name
  strncpy(_filename, name, maxbuflen);

  // open file using GDAL
  _dataset = (GDALDataset *)GDALOpen(_filename, GA_ReadOnly);
  if (_dataset == NULL) {
    snprintf(text, maxbuflen, "GDAL error opening image '%s'", _filename);
    _statusBar->message(text, 0);
    _filename[0] = '\0';
    return;
  }

  // remember dataset particulars
  GDALRasterBand *band = _dataset->GetRasterBand(1); // band index is one-based [1,2,3,...]
  setWidth(band->GetXSize());
  setHeight(band->GetYSize());
  setBytes((band->GetRasterDataType() == GDT_Byte) ? 1 : 2);

  // global histogram?
  if (_globalMask & GLOBAL_HIS_IN_BIT) {
    if (bytes() == 1) {
      _hr.setSize(256);
      _hg.setSize(256);
      _hb.setSize(256);
    } else {
      _hr.setSize(65536);
      _hg.setSize(65536);
      _hb.setSize(65536);
    }

    KHistogram kh;
    if (kh.read(gHisIn.c_str()) == 0) {
      _hr.load((unsigned int *)(kh.band(0).getBinPointer()));
      _hg.load((unsigned int *)(kh.band(1).getBinPointer()));
      _hb.load((unsigned int *)(kh.band(2).getBinPointer()));
    }

    _accumulate = false;
    _resetOnFileOpen = false;
  } else {
    // reset histogram (set size and clear contents)
    _accumulate = true;

    if (_resetOnFileOpen) {
      if (bytes() == 1) {
        _hr.setSize(256);
        _hg.setSize(256);
        _hb.setSize(256);
      } else {
        _hr.setSize(65536);
        _hg.setSize(65536);
        _hb.setSize(65536);
      }
    }

    // has a histogram for this image already been computed?
    char hname[maxbuflen];
    snprintf(hname, maxbuflen, "%s.his", _filename);
    KHistogram kh;
    if (kh.read(hname) == 0) {
      // && kh.size() == _hr.size()) //KH is not auto-sizing
      if (_resetOnFileOpen) {
        _hr.load((unsigned int *)(kh.band(0).getBinPointer()));
        _hg.load((unsigned int *)(kh.band(1).getBinPointer()));
        _hb.load((unsigned int *)(kh.band(2).getBinPointer()));
      } else {
        _hr.accumulate((unsigned int *)(kh.band(0).getBinPointer()));
        _hg.accumulate((unsigned int *)(kh.band(1).getBinPointer()));
        _hb.accumulate((unsigned int *)(kh.band(2).getBinPointer()));
      }

      _accumulate = false;
    }
  }

  updateClip(_left, _right);
  updateLUTs();

  if (_resetOnFileOpen) {
    _samples = 0;

    _rSolve.reset();
    _bSolve.reset();
  }

  prevZoom  = _zoom = 0;
  prevScale = _scale = pow(2.0, _zoom);
  prevX     = double(width()) / 3;
  prevY     = double(height()) / 3;

  resizeContents(int(width()*_scale), int(height()*_scale));

  setContentsPos(int(prevX), int(prevY));

  repaintContents();

  snprintf(text, maxbuflen, "Loaded '%s' (%dx%d, %d bits per R/G/B)", _filename, width(), height(), 8*bytes());
  _statusBar->message(text, 0);
}

void ImageView::keyPressEvent(QKeyEvent *e) {
  if (e->text().toAscii() >= QChar('0') &&
      e->text().toAscii() <= QChar('7')) {
    // only looking at a single character
    setRadius(e->text().toAscii().at(0) - '0');
  } else if (e->key() == Qt::Key_Up && e->state() == 0) {
    QPoint where = QCursor::pos();
    where.setY(where.y() - 1);
    QCursor::setPos(where);
  } else if (e->key()== Qt::Key_Down && e->state() == 0) {
    QPoint where = QCursor::pos();
    where.setY(where.y() + 1);
    QCursor::setPos(where);
  } else if (e->key() == Qt::Key_Left && e->state() == 0) {
    QPoint where = QCursor::pos();
    where.setX(where.x() - 1);
    QCursor::setPos(where);
  } else if (e->key() == Qt::Key_Right && e->state() == 0) {
    QPoint where = QCursor::pos();
    where.setX(where.x() + 1);
    QCursor::setPos(where);
  } else if (e->key() == Qt::Key_PageUp) {
    scrollBy(0, -visibleHeight());
  } else if (e->key() == Qt::Key_PageDown) {
    scrollBy(0, visibleHeight());
  } else if (e->key() == Qt::Key_Up && e->state() & Qt::ShiftButton) {
    int step = (e->state() & Qt::ControlButton) ? visibleHeight() : 1;
    scrollBy(0, -step);
  } else if (e->key() == Qt::Key_Down && e->state() & Qt::ShiftButton) {
    int step = (e->state() & Qt::ControlButton) ? visibleHeight() : 1;
    scrollBy(0, step);
  } else if (e->key() == Qt::Key_Left && e->state() & Qt::ShiftButton) {
    int step = (e->state() & Qt::ControlButton) ? visibleWidth() : 1;
    scrollBy(-step, 0);
  } else if (e->key() == Qt::Key_Right && e->state() & Qt::ShiftButton) {
    int step = (e->state() & Qt::ControlButton) ? visibleWidth() : 1;
    scrollBy(step, 0);
  } else if (e->key() == Qt::Key_Home) {
    _zoom = prevZoom;
    _scale = prevScale;
    resizeContents(int(width()*_scale), int(height()*_scale));
    center(int(width()*_scale*prevX), int(height()*_scale*prevY));
    updateContents();
  } else if (e->text().toAscii() == QChar('=')) {
    _resetOnFileOpen = (_resetOnFileOpen ? 0 : 1);
  } else if (e->text().toAscii() == QChar('-')) {
    _swapRedAndBlue = !_swapRedAndBlue;
    updateContents();
  } else if (e->text().toAscii() == QChar('u')) {
    int limit = rowLimit()/2;
    if (limit < 1)
      limit = 1;
    setRowLimit(limit);
  } else if (e->text().toAscii() == QChar('U')) {
    int limit = rowLimit();
    if (limit < 1)
      limit = 1;
    setRowLimit(2*limit);
  } else if (e->text().toAscii() == QChar('l')) {
    // left tail-clip less during histogram processing
    _leftN -= (e->state() & Qt::ControlButton) ? 10 : 1;
    if (_leftN < 0)
      _leftN = 0;
    _left = double(_leftN)/double(_leftD);

    updateClip(_left, _right);
#ifdef really_raw
    if (_correct) {
      updateLUTs();
      updateContents();
    } else {
      update(); // just the frame
    }
#else
    updateLUTs();
    updateContents();
#endif
  } else if (e->text().toAscii() == QChar('L')) {
    // left tail-clip more during histogram processing
    _leftN += (e->state() & Qt::ControlButton) ? 10 : 1;
    _left = double(_leftN)/double(_leftD);

    updateClip(_left, _right);
#ifdef really_raw
    if (_correct) {
      updateLUTs();
      updateContents();
    } else {
      update(); // just the frame
    }
#else
    updateLUTs();
    updateContents();
#endif
  } else if (e->text().toAscii() == QChar('r')) {
    // right tail-clip less during histogram processing
    _rightN -= (e->state() & Qt::ControlButton) ? 10 : 1;
    if (_rightN < 0)
      _rightN = 0;
    _right = double(_rightN)/double(_rightD);

    updateClip(_left, _right);
#ifdef really_raw
    if (_correct) {
      updateLUTs();
      updateContents();
    } else {
      update(); // just the frame
    }
#else
    updateLUTs();
    updateContents();
#endif
  } else if (e->text().toAscii() == QChar('R')) {
    // right tail-clip more during histogram processing
    _rightN += (e->state() & Qt::ControlButton) ? 10 : 1;
    _right = double(_rightN)/double(_rightD);

    updateClip(_left, _right);
#ifdef really_raw
    if (_correct) {
      updateLUTs();
      updateContents();
    } else {
      update(); // just the frame
    }
#else
    updateLUTs();
    updateContents();
#endif
  } else if (e->text().toAscii() == QChar('p')) {
    // less pre-gamma
    --_preGammaN;
    if (_preGammaN < 0)
      _preGammaN = 0;
    _preGamma = double(_preGammaN)/double(_preGammaD);

    // need to redo color correction with differently gamma-corrected gray points
    refreshSamples();

    if (_correct) {
      updateLUTs();
      updateContents();
    } else {
      update(); // just the frame
    }
  } else if (e->text().toAscii() == QChar('P')) {
    // more pre-gamma
    ++_preGammaN;
    _preGamma = double(_preGammaN)/double(_preGammaD);

    // need to redo color correction with differently gamma-corrected gray points
    refreshSamples();

    if (_correct) {
      updateLUTs();
      updateContents();
    } else {
      update(); // just the frame
    }
  } else if (e->text().toAscii() == QChar('g')) {
    // less post-gamma
    --_postGammaN;
    if (_postGammaN < 0)
      _postGammaN = 0;
    _postGamma = double(_postGammaN)/double(_postGammaD);

    if (_correct) {
      updateLUTs();
      updateContents();
    } else {
      update(); // just the frame
    }
  } else if (e->text().toAscii() == QChar('G')) {
    // more post-gamma
    ++_postGammaN;
    _postGamma = double(_postGammaN)/double(_postGammaD);

    if (_correct) {
      updateLUTs();
      updateContents();
    } else {
      update(); // just the frame
    }
  } else if (e->text().toAscii() == QChar('z')) {
    // less zoom
    if (e->state() & Qt::AltButton) {
      _zoom -= 4;
    } else {
      _zoom -= 1;
    }
    double xCenter = (contentsX() + 0.5*visibleWidth()) /double(width()) /_scale;
    double yCenter = (contentsY() + 0.5*visibleHeight())/double(height())/_scale;

    _scale = pow(1.414213562, _zoom);

    resizeContents(int(width()*_scale), int(height()*_scale));
    center(int(width()*_scale*xCenter), int(height()*_scale*yCenter));

    updateContents();
  } else if (e->text().toAscii() == QChar('Z')) {
    // more zoom
    if (e->state() & Qt::AltButton) {
      _zoom += 4;
    } else {
      _zoom += 1;
    }
    double xCenter = (contentsX() + 0.5*visibleWidth()) /double(width()) /_scale;
    double yCenter = (contentsY() + 0.5*visibleHeight())/double(height())/_scale;

    _scale = pow(1.414213562, _zoom);

    resizeContents(int(width()*_scale), int(        height()*_scale));
    center(int(xCenter*width()*_scale), int(yCenter*height()*_scale));

    updateContents();
  } else if (e->text().toAscii() == QChar('h')) {
    if (_accumulate) {
      // compute whole-image histogram (slow for huge files)
      _statusBar->message("Computing red histogram", 0);
      _hr.compute(_dataset->GetRasterBand(1));

      _statusBar->message("Computing green histogram", 0);
      _hg.compute(_dataset->GetRasterBand(2));

      _statusBar->message("Computing blue histogram", 0);
      _hb.compute(_dataset->GetRasterBand(3));

      // construct and fill KHistogram with tabulated histogram data
      int size = _hr.size(); //(bytes() == 1) ? 256 : 65536;
      KHistogram kh(3, size); // 3 bands of 'size' entries
      kh.band(0).load(_hr.getBinPointer(), size);
      kh.band(1).load(_hg.getBinPointer(), size);
      kh.band(2).load(_hb.getBinPointer(), size);

      // write histogram to file
      char hname[maxbuflen];
      snprintf(hname, maxbuflen, "%s.his", _filename);
      /*int hstatus = */ kh.write(hname);

      // update clip values
      updateClip(_left, _right);

      // disable accumulation
      _accumulate = false;

      updateLUTs();
      updateContents();
    }
  } else if (e->text().toAscii() == QChar('c')) {
    _correct = !_correct;

#ifdef really_raw
    if (_correct)
      updateClip(_left, _right);
    else
      updateClip(0.00005, 0.00005);
#else
#endif
    updateLUTs();
    updateContents();
  } else if (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) {
    removeSample();
    updateLUTs();

    if (_correct) {
      updateContents(); // redraw contents to show effect of updated color correction
    } else {
      update(); // just update GUI elements
    }
  } else if (e->text().toAscii() == QChar('w') || e->text().toAscii() == QChar('W')) {
    SaveLutWork();
  }
  else if (e->text().toAscii() == QChar('A')) {
    // apply the LUT and make a new image!
    char nname[maxbuflen];
    strncpy(nname, _filename, maxbuflen);

    char *slash = nname + strlen(nname) - 1; // last character
    while (slash != nname && *slash != '\\' && *slash != '/')
      slash--;
    if (*slash == '\\' || *slash == '/')
      slash++;

    char rest[maxbuflen];
    strncpy(rest, slash, maxbuflen);

    int slashlen = (strlen(nname) * 2) - 1;
    snprintf(slash, slashlen, "new-%s", rest);

    char text[maxbuflen];
    snprintf(text, maxbuflen, "Applying correction to create new image '%s'", nname);
    _statusBar->message(text, 0);

    // construct and fill KHistogram with LUT data
    int size = _hr.size(); //(bytes() == 1) ? 256 : 65536;
    KHistogram kh(3, size); // 3 bands of 'size' entries
    kh.band(0).load(_rLut, size);
    kh.band(1).load(_gLut, size);
    kh.band(2).load(_bLut, size);

    // use LUT to create new, transformed image
    kh.transform(_dataset, nname);
  } else if (e->text().toAscii() == QChar('s')) {
    _contrast--;
    updateLUTs();
    updateContents();
  } else if (e->text().toAscii() == QChar('S')) {
    _contrast++;
    updateLUTs();
    updateContents();
  } else {
    e->ignore();
    return;
  }

  int r, g, b;
  QPoint where = QCursor::pos();
  sampleImage(where.x(), where.y(), r, g, b);
}

void ImageView::contentsMouseMoveEvent(QMouseEvent *e) {
  int r, g, b;
  sampleImage(e->x(), e->y(), r, g, b);
}

void ImageView::contentsMousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton) {
    // sample the image
    int r, g, b;
    sampleImage(e->x(), e->y(), r, g, b);   // get RGB
    addSample(e->x(), e->y(), r, g, b);    // remember RGB and update color-correction coefficients

    // update the RGB look-up-tables (LUTs) using new color-correction coefficients
    updateLUTs();

    if (_correct)
      updateContents(); // redraw contents to show effect of updated color correction
    else
      update(); // just update GUI elements
  } else if (e->button() == Qt::RightButton) {
    // recenter and zoom
    double xCenter = e->x() / double(width())  / _scale;
    double yCenter = e->y() / double(height()) /_scale;

    prevZoom  = _zoom;
    prevScale = _scale;
    prevX     = xCenter;
    prevY     = yCenter;

    _zoom = 1;
    _scale = pow(1.414213562, _zoom);

    resizeContents(int(width()*_scale), int(height()*_scale));
    center(int(width()*_scale*xCenter), int(height()*_scale*yCenter));

    updateContents();
  }
}

void ImageView::sampleImage(int x, int y, int& rValue, int& gValue, int& bValue) {
  if (_dataset == NULL)
    return;

  x = int(x/_scale);
  y = int(y/_scale);

  pixel(x, y, rValue, gValue, bValue);

  if (_statusBar != NULL) {
    char text[maxbuflen];
    snprintf(text, maxbuflen, "%s%s(%d,%d) r=%d, g=%d, b=%d (%dx%d) [rows=%d][%.2f%%/%.2f%%: %u/%u %u/%u %u/%u][%d%s](%6.3f+%6.3f*r, %2.0f%%)(%7.4f+%7.4f*b, %2.0f%%)[%d][%.3f/%.3f][%d]%.2f",
            _resetOnFileOpen ? "1" : "n",
            _correct ? "C" : "R",
            x, y,
            rValue, gValue, bValue,
            2*radius() + 1, 2*radius() + 1,
            _rowLimit,
            100.0*_leftN/double(_leftD),
            100.0*_rightN/double(_rightD),
            _hr.left(), _hr.right(),
            _hg.left(), _hg.right(),
            _hb.left(), _hb.right(),
            samples(),
            _saveNeeded?"*":"",
            _rSolve.a(), _rSolve.b(), 100.0*_rSolve.coefCorrel(),
            _bSolve.a(), _bSolve.b(), 100.0*_bSolve.coefCorrel(),
            (int)_hr.count(),
            _preGamma,
            _postGamma,
            _zoom,
            _contrast*0.02
            );

    _statusBar->message(text, 0);
  }
}

// literal coordinates for X and Y
void ImageView::pixel(int x, int y, int& rValue, int& gValue, int& bValue, int w, int h) {
  if (_dataset == NULL) {
    rValue = gValue = bValue = 65537;
    return;
  }
  if (x < 0 || x >= width() || y < 0 || y >= height()) {
    rValue = gValue = bValue = 65538;
    return;
  }

  // window for computing mean pixel value is (2*radius+1)^2
  if (w < 1) w = 2*radius() + 1;
  if (h < 1) h = 2*radius() + 1;

  // window is centered at (x,y), so move origin
  x -= w/2;
  y -= h/2;

  // location of lower-right corner
  int x2 = x + w - 1;
  int y2 = y + h - 1;

  // clamp upper-left corner
  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;

  // clamp lower-right corner
  if (x2 > width() - 1)
    x2 = width() - 1;
  if (y2 > height() - 1)
    y2 = height() - 1;

  // recompute cropped sample window width and height
  w = x2 - x + 1;
  h = y2 - y + 1;

  // get data from GDAL
  if (bytes() == 1) {
    unsigned char *r = (unsigned char *)_ra; //calloc(1*w*h*1, 1);
    unsigned char *g = (unsigned char *)_ga; //calloc(1*w*h*1, 1);
    unsigned char *b = (unsigned char *)_ba; //calloc(1*w*h*1, 1);

    // compute mean values in each band
    rValue = 0;
    gValue = 0;
    bValue = 0;

    int pixels = w*h;
    for (int i=0; i < pixels; i++) {
      unsigned int rRaw = r[i];
      unsigned int gRaw = g[i];
      unsigned int bRaw = b[i];

      // remove existing gamma encoding
      if (_preGamma != 1.0f) {
        rRaw = (unsigned int)(255.0f*pow(rRaw/255.0f, _preGamma));
        gRaw = (unsigned int)(255.0f*pow(gRaw/255.0f, _preGamma));
        bRaw = (unsigned int)(255.0f*pow(bRaw/255.0f, _preGamma));
      }

      rValue += rRaw;
      gValue += gRaw;
      bValue += bRaw;
    }
    rValue /= pixels;
    gValue /= pixels;
    bValue /= pixels;
  } else {
    unsigned short *r = (unsigned short *)_ra;
    unsigned short *g = (unsigned short *)_ga;
    unsigned short *b = (unsigned short *)_ba;

    // compute mean values in each band
    rValue = 0;
    gValue = 0;
    bValue = 0;

    int pixels = w*h;
    for (int i=0; i < pixels; i++) {
      rValue += r[i];
      gValue += g[i];
      bValue += b[i];
    }
    rValue /= pixels;
    gValue /= pixels;
    bValue /= pixels;
  }

  //char text[maxbuflen];
  //sprintf(text, "[(%d,%d) (%dx%d) r=%d]", x, y, w, h, radius());
  //_statusBar->message(text, 0);
}

void ImageView::resizeEvent (QResizeEvent *e) {
  QScrollView::resizeEvent(e);

  //char text[maxbuflen];
  //sprintf(text, "resize w=%d, h=%d", visibleWidth(), visibleHeight());
  //_statusBar->message(text, 0);
}

// scaled coordinates for X and Y
void ImageView::drawContents (QPainter *p, int cx, int cy, int cw, int ch) {
  if (_dataset == NULL || cw < 1 || ch < 1 ) {
    // let Qt handle the repaint
    QScrollView::drawContents(p, cx, cy, cw, ch);
    return;
  }

  // check arguments
  if (cw < 0 || ch < 0 || cx < 0 || cy < 0) return;

  int scaledWidth  = (int)floor(_scale*width());  // ignore right-most partial column
  int scaledHeight = (int)floor(_scale*height()); // ignore bottom-most partial row

  // is redraw request completely beyond the extent of the image?
  //   ..must be zooming down with right or bottom visible. (erase)
  if (cx >= scaledWidth || cy >= scaledHeight) {
    p->eraseRect(cx, cy, cw, ch);
    return;
  }

  bool tooWide = (cx + cw - 1) >= scaledWidth;
  bool tooHigh = (cy + ch - 1) >= scaledHeight;

  if (tooWide && tooHigh) {
    // need to clip request into three parts:
    //  a: outside the image on the right, but clipped by the bottom (erase)
    //  b: outside the image on the bottom, and extending all the way to the right (erase)
    //  c: inside the image (we will handle that part)
    int insideWidth  = scaledWidth  - cx;
    int insideHeight = scaledHeight - cy;

    p->eraseRect(cx+insideWidth, cy,              cw-insideWidth,    insideHeight); // part A
    p->eraseRect(cx,             cy+insideHeight, cw,             ch-insideHeight); // part B

    ch = insideHeight; // part of height inside image
    cw = insideWidth;  // part of width inside image
  } else if (tooWide) {
    // need to clip into two parts
    //   a: outside the image on the right (Qt)
    //   b: inside the image (we will handle that part)
    int insideWidth = scaledWidth - cx;

    p->eraseRect(cx+insideWidth, cy, cw-insideWidth, ch); // part A

    cw = insideWidth; // part of width inside image
  } else if (tooHigh) {
    // need to clip into two parts
    //   a: outside the image on the bottom (Qt)
    //   b: inside the image (we will handle that part)
    int insideHeight = scaledHeight - cy;

    p->eraseRect(cx, cy+insideHeight, cw, ch-insideHeight); // part A

    ch = insideHeight; // part of height inside image
  }

  // the tile is now at least 1x1 and clipped to be completely within the image extent

  // when update is too large, draw it in a series of requests
  if ((_scale >= 1.0f) && ((cw+10)*(ch+10) > _pixelCache || ch > _rowLimit)) {
    // presume allocation large enough for at least one row
    int rows = _pixelCache/(cw+10);
    if (rows > ch)
      rows = ch;

    // limit drawing to desired value even if we have room for more
    if (rows > _rowLimit)
      rows = _rowLimit;

    // draw all but last tile
    while (ch > rows) {
      drawScaledContents(p, cx, cy, cw, rows);
      cy += rows;
      ch -= rows;
    }

    // draw last row (variable height: 1 to 'rows' rows)
    drawScaledContents(p, cx, cy, cw, ch);
  } else if ((_scale < 1.0f) && (ceil((cw+10)/_scale)*ceil((ch+10)/_scale) > _pixelCache)) {
    // divide along larger axis
    if (cw >= ch) {
      int half = (cw + 1)/2;
      drawContents(p, cx,        cy,      half, ch);
      drawContents(p, cx + half, cy, cw - half, ch);
    } else {
      int half = (ch + 1)/2;
      drawContents(p, cx, cy,        cw,      half);
      drawContents(p, cx, cy + half, cw, ch - half);
    }
  } else {
    // not too large -- draw it in a single request
    drawScaledContents(p, cx, cy, cw, ch);
  }
}

// scaled coordinates for X and Y
void ImageView::drawScaledContents(QPainter *p, int cx, int cy, int cw, int ch) {
  // check arguments
  if (cw < 0 || ch < 0 || cx < 0 || cy < 0)
    return;

  // read image, process tile, and store in pixel cache (_r8, _g8, _b8)
  readScaledTile(cx, cy, cw, ch);

  // pack 8-bit data bands into interleaved RGBA buffer
  unsigned char *bp = _buffer;
  unsigned char *r = _r8;
  unsigned char *g = _g8;
  unsigned char *b = _b8;
  for (int i = 0; i < cw*ch; i++) {
    *bp++ = *b++;
    *bp++ = *g++;
    *bp++ = *r++;
    *bp++ = 0;
  }

  // construct a QImage object by reference to interleaved data buffer
  QImage qi(_buffer, cw, ch, 32, 0, 0, QImage::LittleEndian);

  // copy the QImage to the scroll-view's contents area
  //  "...This function may convert image to a pixmap and then draw it, if device() is a
  //  QPixmap or a QWidget, or else draw it directly, if device() is a QPrinter or QPicture."
  //QImage si = qi.smoothScale (cw/2, ch/2);
  p->drawImage(cx, cy, qi);

  //char text[maxbuflen];
  //sprintf(text, "draw x=%d, y=%d, w=%d, h=%d", cx, cy, cw, ch);
  //_statusBar->message(text, 0);
}

// scaled coordinates for X and Y
int ImageView::readScaledTile (int cx, int cy, int cw, int ch) {
  if (_dataset == NULL || cw < 1 || ch < 1) return 1;

  if (_zoom == 0) {
    if (readTile(cx, cy, cw, ch) != 0) return 2;
  } else {
    // select resampling filter
    Filter::Filter_t filter;
    double support;

    if (_scale < 1.0) {
      // minification
      filter  = Filter::boxFilter;
      support = Filter::boxSupport;
    } else {
      // magnification
      filter  = Filter::MitchellFilter;
      support = Filter::MitchellSupport;
    }

    // transform source coordinates to destination coordinates

    double ulx = (cx)/_scale;
    double uly = (cy)/_scale;
    int ulxRaw = (int)ulx;
    int ulyRaw = (int)uly;

    double xOffset = ulx - ulxRaw;
    double yOffset = uly - ulyRaw;

    double lrx = (cx + cw)/_scale;
    double lry = (cy + ch)/_scale;
    int lrxRaw = (int)ceil(lrx);
    int lryRaw = (int)ceil(lry);

    // enlarge destination rectangle with support-sized border
    int border = (int)ceil(support);
    ulxRaw -= border;
    ulyRaw -= border;

    lrxRaw += border;
    lryRaw += border;

    xOffset += border; // compensate for moving ulx
    yOffset += border; // compensate for moving uly

    // clip destination rectangle to image extent (will cause filtering errors)
    if (ulxRaw < 0) {
      xOffset += ulxRaw;
      ulxRaw = 0;
    } if (ulyRaw < 0) {
      yOffset += ulyRaw;
      ulyRaw = 0;
    }

    if (lrxRaw > width() - 1)
      lrxRaw = width() - 1;
    if (lryRaw > height() - 1)
      lryRaw = height() - 1;

    int wRaw = lrxRaw - ulxRaw + 1;
    int hRaw = lryRaw - ulyRaw + 1;

    if (readTile(ulxRaw, ulyRaw, wRaw, hRaw) != 0)
      return 3;

    // package raw data just read from image in an Image structure
    Image rOld(wRaw, hRaw, _r8);
    Image gOld(wRaw, hRaw, _g8);
    Image bOld(wRaw, hRaw, _b8);

    // build rescaled image by resampling old (natural-resolution) image
#if 0
    Image rNew(cw, ch, rOld, filter, support, xOffset, yOffset, _scale, _scale);
    Image gNew(cw, ch, gOld, filter, support, xOffset, yOffset, _scale, _scale);
    Image bNew(cw, ch, bOld, filter, support, xOffset, yOffset, _scale, _scale);
#else
    Image rNew(cw, ch, (unsigned char *)_r16); // use _r16 to avoid allocating tmp image. (optimization)
    Image gNew(cw, ch, (unsigned char *)_g16); // use _g16 to avoid allocating tmp image. (optimization)
    Image bNew(cw, ch, (unsigned char *)_b16); // use _b16 to avoid allocating tmp image. (optimization)

    // construct table of image resizing weights (that resize rOld to rNew, for example)
    //Resize weights(wRaw, hRaw, cw, ch, filter, support, xOffset, yOffset, _scale, _scale); // same as...
    Resize weights(rOld, rNew, filter, support, xOffset, yOffset, _scale, _scale);

    // ...then apply it to each of the independent R, G, and B image planes
    weights.apply(rOld, rNew); // slight optimization (5% to 10%)
    weights.apply(gOld, gNew);
    weights.apply(bOld, bNew);
#endif

    // copy rescaled image to buffer area
    int pixels = cw*ch;
    memcpy(_r8, rNew.data, pixels*sizeof(unsigned char));
    memcpy(_g8, gNew.data, pixels*sizeof(unsigned char));
    memcpy(_b8, bNew.data, pixels*sizeof(unsigned char));

    //char text[maxbuflen];
    //sprintf(text, "ReadScaledTile [x=%d, y=%d, w=%d, h=%d]%f[%d, %d, %f, %f]", cx, cy, cw, ch, _scale, wRaw, hRaw, xOffset, yOffset);
    //_statusBar->message(text, 0);
  }

  return 0;
}

// read tile from file, update histogram, apply color correction
// result stored in pixel cache (_r8, _b8, and _g8)

// literal coordinates for X and Y
int ImageView::readTile (int cx, int cy, int cw, int ch) {
  int pixels = cw*ch;

  // "null read" is valid
  if (cw == 0 || ch == 0)
    return 0;

  // arguments must be strictly valid
  //#define PATTERN
#ifndef PATTERN
  if (_dataset == NULL) // no file to read
    return 1;
#endif
  if (cw < 0 || ch < 0) // invalid size
    return 2;
  if (cx < 0 || cy < 0 || cx >= width() || cy >= height()) // invalid origin
    return 3;
  if (cw > width() - cx) // too wide (extends beyond right column of image)
    return 4;
  if (ch > height() - cy) // too tall (extends beyond bottom row of image)
    return 5;
  if (pixels > _pixelCache) // too much data for one tile (use smaller tiles or larger cache)
    return 6;

#ifdef PATTERN
  // generate patterns for testing image resampling filters
  for (int y = 0; y < ch; y++) {
    for (int x = 0; x < cw; x++) {
      int xImage = cx + x;
      int yImage = cy + y;

#if 0
      // horizontal stripes with 32-pixel bands
      unsigned char pixel = ((yImage/32) & 1) ? 255 : 0;
#endif
#if 0
      // vertical stripes with 32-pixel bands
      unsigned char pixel = ((xImage/32) & 1) ? 255 : 0;
#endif
#if 0
      // horizontal/vertical chessboard with 32-pixel squares
      unsigned char pixel = (((xImage/32) & 1) ^ ((yImage/32) & 1)) ? xImage : yImage;
#endif
#if 0
      // SW-NE diagonal with 31.82 pixel bands
      unsigned char pixel = (((xImage+yImage)/45)&1) ? 255 : 0;
#endif
#if 0
      // NW-SW diagonal with 31.82 pixel squares
      int flip = (xImage < yImage) ? 1 : 0;
      unsigned char pixel = ((((xImage-yImage)/45)&1) ^ flip) ? 255 : 0;
#endif
#if 0
      // diagonal chessboard with 31.82 pixel squares
      int swne = ((xImage+yImage)/45) & 1;
      int flip = (xImage < yImage) ? 1 : 0;
      int nwse = (((xImage-yImage)/45) & 1) ^ flip;
      unsigned char pixel = (swne ^ nwse) ? xImage : yImage;
#endif
#if 1
      // L(x,y) function
      float X = xImage/3.0f;
      float Y = yImage/3.0f;
      unsigned char pixel = (int)(255.0f*(0.5f*(1.0f + sinf(0.01f*X*X + 0.01f*Y*Y))));
#endif

      int offset = y*cw + x;
#if 0
      _r8[offset] = ((xImage/32) & 0x01) ? pixel : pixel/2;
      _g8[offset] = ((xImage/32) & 0x02) ? pixel : pixel/2;
      _b8[offset] = ((yImage/32) & 0x01) ? pixel : pixel/2;
#else
      _r8[offset] = pixel;
      _g8[offset] = pixel;
      _b8[offset] = pixel;
#endif
    }
  }
#else
  // get data from GDAL
  if (bytes() == 1) {
    // 2: update histogram
    if (_accumulate) {
      // incrementally accumulate approximate whole-image histogram
      _hr.add((unsigned char *)_r16, pixels);
      _hg.add((unsigned char *)_g16, pixels);
      _hb.add((unsigned char *)_b16, pixels);

      // update clip values
      updateClip(_left, _right);

      // update the RGB look-up-tables (LUTs) to reflect new clip values
      updateLUTs();
    }

    // 3: apply histogram-based clipping, color correction, gamma encoding
    transform((unsigned char *)_r16, _r8, pixels, _rLut);
    transform((unsigned char *)_g16, _g8, pixels, _gLut);
    transform((unsigned char *)_b16, _b8, pixels, _bLut);
  } else {
    // 1: read 16-bit image data using GDAL

#if 0
    if (1) // if (_ApplyQuickbirdAntiBlue) {
      const float f = 0.05f;
      const float GtoB = 0.3f; // semi-random
      const float OneOverOneMinusF = 1.0f/(1.0f-f);
      for (int k=0; k<pixels; k++) {
        float G = _g16[k]*OneOverOneMinusF;
        float B = _b16[k] - f*G*GtoB;

        _g16[k] = int(G);
        _b16[k] = int(B);
      }
    }
#endif

    // 2: update histogram
    if (_accumulate) {
      // incrementally accumulate approximate whole-image histogram
      _hr.add(_r16, pixels);
      _hg.add(_g16, pixels);
      _hb.add(_b16, pixels);

      // update clip values
      updateClip(_left, _right);

      // update the RGB look-up-tables (LUTs) to reflect new clip values
      updateLUTs();
    }

    // 3: apply histogram-based clipping, color correction, gamma encoding (8-bit result)
    if (_swapRedAndBlue == 0) {
      transform(_r16, _r8, pixels, _rLut);
      transform(_g16, _g8, pixels, _gLut);
      transform(_b16, _b8, pixels, _bLut);
    } else {
      transform(_r16, _b8, pixels, _rLut);
      transform(_g16, _g8, pixels, _gLut);
      transform(_b16, _r8, pixels, _bLut);
    }
  }
#endif

  return 0;
}

// transform image data: histogram-based clipping, color correction, gamma encoding, etc. (8->8 transfer)
void ImageView::transform(unsigned char *src, unsigned char *dst, int size, unsigned char *lut) {
  int i;
  for (i=0; i < size; i++)
    dst[i] = lut[src[i]];
}

// transform image data: histogram-based clipping, color correction, gamma encoding, etc. (16->8 conversion)
void ImageView::transform(unsigned short *src, unsigned char *dst, int size, unsigned char *lut) {
  int i;
  for (i=0; i < size; i++)
    dst[i] = lut[src[i]];
}

void ImageView::updateLUTs() {
  updateLUTBand(_hr, _rSolve.a(), _rSolve.b(), _rLut);
  updateLUTBand(_hg,         0.0,         1.0, _gLut);
  updateLUTBand(_hb, _bSolve.a(), _bSolve.b(), _bLut);
}


double ImageView::tone_encode (double v, double g, double start, double slope) {
  if (g < 1.0) {
    return tone_decode(v, 1.0/g, start, slope);
  } else if (v <= start) {
    return v*slope;
  } else {
    double t3 = pow(v, 0.9000000000000000e0 / g);
    double t5 = 0.1099000000000000e1 * t3 - 0.9900000000000000e-1;
    return t5;
  }
}

double ImageView::tone_decode (double v, double g, double start, double slope) {
#if 0
  return pow(v, 1/g);
#else
  if (g < 1.0) {
    return tone_encode(v, 1.0/g, start, slope);
  } else if (v <= start*slope) {
    return v/slope;
  } else {
    double cg3 = exp(0.1111111111e1 * log(0.9099181074e0 * v + 0.9008189263e-1) * g);
    return cg3;
  }
#endif
}

double ImageView::tone_start (double g) {
  double t6 = log(0.9008189262966333e0 * g / (-0.9e1 + 0.10e2 * g));
  double t9 = exp(0.1111111111111111e1 * t6 * g);
  return t9;
}

double ImageView::tone_slope (double g) {
  double t6 = log(0.9008189262966333e0 * g / (-0.9e1 + 0.10e2 * g));
  double t9 = exp(0.1111111111111111e1 * t6 * g);
  double t12 = pow(t9, 0.9000000000000000e0 / g);
  double t16 = (0.1099000000000000e1 * t12 - 0.9900000000000000e-1) / t9;
  return t16;
}

void ImageView::updateLUTBand(Histogram& histogram, double a, double b, unsigned char *lut) {
  int i;
  int size = (bytes() == 1) ? 256 : 65536;

  // avoid color correction?
  if (!_correct) {
    if (bytes() == 1) {
#ifdef notdef
      for (i=0; i < size; i++)
        lut[i] = i;
#else
      int fL = histogram.left();
      int fR = histogram.right();

      int tL = _protect ? 1 : 0;
      int tR = 255;

      for (i=0; i < fL; i++) {
        lut[i] = tL;
      }

      double scale = double(tR - tL)/double(fR - fL);
      for (i=fL; i < fR; i++) {
        lut[i] = (unsigned char)(tL + scale*(i - fL));
      }

      for (i=fR; i < 65536; i++) {
        lut[i] = tR;
      }

      if (_protect) lut[0] = 0;
#endif
    } else {
      int fL = histogram.left();
      int fR = histogram.right();

      int tL = _protect ? 1 : 0;
      int tR = 255;

      for (i=0; i < fL; i++) {
        lut[i] = tL;
      }

      double scale = double(tR - tL)/double(fR - fL);
      for (i=fL; i < fR; i++) {
        lut[i] = (unsigned char)(tL + scale*(i - fL));
      }

      for (i=fR; i < 65536; i++) {
        lut[i] = tR;
      }

      if (_protect) lut[0] = 0;
    }
  } else {
    double fromLeft;
    double fromRight;

    if (_rSolve.count() < 8) {
      // clip this channel using its own limits
      fromLeft  = histogram.left();
      fromRight = histogram.right();

      // omit color correction when number of points is small
      a = 0.0;
      b = 1.0;
    } else {
      // will be correcting color, so need to clip all channels
      // to unified "outside" values (smallest..largest)

      // transform left limits by color-correction parameters
      double rt = _hr.left()*_rSolve.b() + _rSolve.a();
      double gt = _hg.left();
      double bt = _hb.left()*_bSolve.b() + _bSolve.a();

      // determine minimum of corrected R, G, and B left values
      fromLeft = rt;
      if (gt < fromLeft) fromLeft = gt;
      if (bt < fromLeft) fromLeft = bt;

      // transform right limits by color-correction parameters
      rt = _hr.right()*_rSolve.b() + _rSolve.a();
      gt = _hg.right();
      bt = _hb.right()*_bSolve.b() + _bSolve.a();

      // determine maximum of corrected R, G, and B right values
      fromRight = rt;
      if (gt > fromRight) fromRight = gt;
      if (bt > fromRight) fromRight = bt;

      // did transformation did move limits outside valid range?
      if (fromLeft  <= 0   ) fromLeft  = _protect ? 1 : 0;
      if (fromRight >= size) fromRight = size - 1;
    }

    // determine 'to' parameters
    int toLeft = _protect ? 1 : 0;
    int toRight = 255; // display is [0..255]

    // build loop-invariant constants
    double fL = fromLeft  - 0.5;
    double fR = fromRight + 0.5;
    double tL = toLeft    - 0.5;
    double tR = toRight   + 0.5;

    double _preGammaStart = tone_start(_preGamma);
    double _preGammaSlope = tone_slope(_preGamma);

    double _postGammaStart = tone_start(_postGamma);
    double _postGammaSlope = tone_slope(_postGamma);

    // compute LUT that maps [left..right] into [0..255]
    //   with color correction
    //   with gamma
    //   with protection of channel zero ("NO DATA" areas)
    for (i=0; i < size; i++) {
      double pixel = i;

      // remove previous gamma encoding from this pixel
      if (size == 256 && _preGamma != 1.0)
        //pixel = 255.0*pow(pixel/255.0, _preGamma);
        pixel = 255.0*tone_decode(pixel/255.0, _preGamma, _preGammaStart, _preGammaSlope);

      // apply color correction for this channel
      pixel = a + b*pixel;

      // handle out-of-range data
      if (pixel <= fL) {
        lut[i] = toLeft;
      } else if (pixel >= fR) {
        lut[i] = toRight;
      } else {
        // determine position within old range [fL..fR]
        double f = double(pixel - fL)/double(fR - fL);

#if 1
        // apply sigmoidal contrast curve
        if (_contrast != 0) {
          double curve = (1.0 - f)*pow(f, 2.0) + (f)*pow(f, 1/2.0);
          double strength = 0.02*_contrast;

          f = f + strength*(curve - f);

          if (f < 0.0) {
            f = 0.0;
          } else if (f > 1.0) {
            f = 1.0;
          }
        }
#endif

        // remap pixel based on gamma value
        if (_postGamma != 1.0)  {
          f = tone_encode(f, _postGamma, _postGammaStart, _postGammaSlope);
        }

        // map pixel to new range [tL..tR]
        int iPixel = int(tL + f*(tR - tL));

        lut[i] = (unsigned char)iPixel;
      }
    }

    // preserve "NO DATA" value
    lut[0] = 0;
  }
}

void ImageView::addSample(int x, int y, int r, int g, int b) {
  // enlarge as needed
  if (_sampleMax < 256) _sampleMax = 256;
  if (_sample == NULL) {
    _sample = (RGBSample *)calloc(sizeof(RGBSample), _sampleMax);
  } else if (_samples >= _sampleMax) {
    _sampleMax *= 2;
    _sample = (RGBSample *)realloc(_sample, sizeof(RGBSample)*_sampleMax);
  }

  // remember
  _sample[_samples].x = x;
  _sample[_samples].y = y;
  _sample[_samples].r = (unsigned short)r;
  _sample[_samples].g = (unsigned short)g;
  _sample[_samples].b = (unsigned short)b;

  _rSolve.add(_sample[_samples].r, _sample[_samples].g); // (x,y) := (r,g)
  _bSolve.add(_sample[_samples].b, _sample[_samples].g); // (x,y) := (b,g)

  _samples++;

  _saveNeeded = 1;
}

void ImageView::refreshSamples() {
  if (_samples < 1) return;

  // reset
  _rSolve.reset();
  _bSolve.reset();

  // refresh
  for (int i=0; i < _samples; i++) {
    int r, g, b;
    sampleImage(_sample[i].x, _sample[i].y, r, g, b);       // get RGB

    _sample[i].r = (unsigned short)r;
    _sample[i].g = (unsigned short)g;
    _sample[i].b = (unsigned short)b;

    _rSolve.add(_sample[i].r, _sample[i].g); // (x,y) := (r,g)
    _bSolve.add(_sample[i].b, _sample[i].g); // (x,y) := (b,g)
  }

  _saveNeeded = 1;
}

void ImageView::removeSample() {
  if (_samples > 0) {
    _samples--;

    _rSolve.remove(_sample[_samples].r, _sample[_samples].g); // (x,y) := (r,g)
    _bSolve.remove(_sample[_samples].b, _sample[_samples].g); // (x,y) := (b,g)

    _saveNeeded = 1;
  }
}

void ImageView::writeSamples(char *filename) {
  if (samples() < 1) return;
  if (filename == NULL || *filename == '\0') return;

  FILE *fp = fopen(filename, "w");
  if (fp != NULL) {
    //fprintf(fp, "\n");

    for (int i = 0; i < samples(); i++)
      fprintf(fp, "%5d %5d %5d\n", _sample[i].r, _sample[i].g, _sample[i].b);

    fclose(fp);
  }

  _saveNeeded = 0;
}

void ImageView::SaveLutWork(void) {
  // save settings to lutwork file
  size_t glutworklen = gLutWorkOut.length();
  char *ascii_lutwork = new char[glutworklen + 1];
  strncpy(ascii_lutwork, gLutWorkOut.c_str(), glutworklen + 1);
  FILE* fh = fopen(ascii_lutwork, "w");
  delete [] ascii_lutwork;
  if (fh != NULL) {
    fprintf(
      fh,
      "%lf %d %d %lf %d %d %lf %lf %d\n",
      _left,
      _leftN,
      _leftD,
      _right,
      _rightN,
      _rightD,
      _preGamma,
      _postGamma,
      _contrast
    );
    for (int i = 0; i < _samples; i++) {
      fprintf(
        fh,
        "%5d %5d %5d\n",
        _sample[i].r,
        _sample[i].g,
        _sample[i].b
      );
    }
    fclose(fh);
    fflush(stderr);
  }
  // construct and fill KHistogram with LUT data
  int size = _hr.size(); //(bytes() == 1) ? 256 : 65536;
  KHistogram kh(3, size); // 3 bands of 'size' entries
  kh.band(0).load(_rLut, size);
  kh.band(1).load(_gLut, size);
  kh.band(2).load(_bLut, size);
  kh.write(gLutOut.c_str());
}


