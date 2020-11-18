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

/* TODO: High-level file comment. */
////////////////////////////////////////////////////////////////////////////////
#ifndef __IMAGE_VIEW_H__
#define __IMAGE_VIEW_H__

#include <Qt3Support/Q3ScrollView>
#include "gdal_priv.h"

#include "histogram.h"
#include "linreg.h"
#include <string>

//
//      Qt-based Image View widget
//

#define GLOBAL_HIS_IN_BIT (1)
#define GLOBAL_LUT_OUT_BIT (2)
#define GLOBAL_INFO_BIT (4)
#define GLOBAL_IMAGE_BIT (8)

// for support
using QScrollView = Q3ScrollView;
using WFlags = Qt::WindowFlags;

struct RGBSample
{
  unsigned short r, g, b;
  int x, y;
};

class ImageView : public QScrollView {
 public:
  ImageView(QWidget *parent=0, const char *name=0, WFlags f=0);
  ~ImageView();

  void setFilename(char *name);
  char *filename() {return _filename;}

  // parameters of image
  void setWidth(int w) {_width  = w;}
  void setHeight(int h) {_height = h;}
  void setBytes(int b) {_bytes  = b;}
  int width() {return _width;}
  int height() {return _height;}
  int bytes() {return _bytes;}

  void setRowLimit(int limit) {_rowLimit = limit;}
  int rowLimit() {return _rowLimit;}

  void pixel(int x, int y, int& rValue, int& gValue, int& bValue, int h=0, int w=0);
  void setRadius(int r);
  int radius() {return _radius;}

  void addSample(int x, int y, int r, int g, int b);
  void refreshSamples();
  void removeSample();
  int samples() {return _samples;}
  void writeSamples(char *filename);

  void setStatusBar(QStatusBar *status) { _statusBar = status; }

  void setGlobalHisIn(const std::string &in);
  void setGlobalLutOut(const std::string &out);
  void loadInitImage(const std::string &image_file_path);

  void setLutWork(const std::string &lutwork_file);

  void SaveLutWork(void);
 private:
  void keyPressEvent(QKeyEvent *e);
  void contentsMouseMoveEvent(QMouseEvent *e);
  void contentsMousePressEvent(QMouseEvent *e);
  void resizeEvent(QResizeEvent *e);
  void drawContents (QPainter *p, int cx, int cy, int cw, int ch);
  void drawScaledContents(QPainter *p, int cx, int cy, int cw, int ch);
  int readScaledTile (int x, int y, int w, int h);
  int readTile (int x, int y, int w, int h);
  void sampleImage(int x, int y, int& rValue, int& gValue, int& bValue);
  void updateLUTs();
  void updateLUTBand(Histogram& histogram, double a, double b, unsigned char *lut);


  double tone_encode (double v, double g, double start, double slope);
  double tone_decode (double v, double g, double start, double slope);
  double tone_start (double g);
  double tone_slope (double g);

  char _filename[512];

  // image size can be as large as you please
  int _width;
  int _height;
  int _bytes;

  GDALDataset *_dataset;
  int _datasetType;

  QStatusBar *_statusBar;

  const char **_cursors[8];

  void transform(unsigned  char *src, unsigned char *dst, int size, unsigned char *lut);
  void transform(unsigned short *src, unsigned char *dst, int size, unsigned char *lut);
  Histogram _hr;
  Histogram _hg;
  Histogram _hb;
  void setSkipCount (int count)
  {
    _hr.setSkipCount(count);
    _hg.setSkipCount(count);
    _hb.setSkipCount(count);
  }
  void updateClip(double leftRatio, double rightRatio)
  {
    setSkipCount(3);
    _hr.clip(leftRatio, rightRatio);
    _hg.clip(leftRatio, rightRatio);
    _hb.clip(leftRatio, rightRatio);
  }

  bool _accumulate;
  int _leftN, _leftD;
  double _left;
  int _rightN, _rightD;
  double _right;

  int _radius;
  int _radiusLimit;
  unsigned char *_ra; // working storage for pixel average computation
  unsigned char *_ga;
  unsigned char *_ba;

  int _rowLimit; // number of rows to draw in each individual image update
  int _pixelCache; // number of pixels usable in each of the following
  unsigned short *_r16;
  unsigned short *_g16;
  unsigned short *_b16;
  unsigned char  *_r8;
  unsigned char  *_g8;
  unsigned char  *_b8;
  unsigned char  *_buffer;

  RGBSample *_sample;
  int _samples;
  int _sampleMax;
  int _saveNeeded;

  LinearRegression _rSolve;
  LinearRegression _bSolve;

  unsigned char *_rLut;
  unsigned char *_gLut;
  unsigned char *_bLut;

  bool _correct;
  bool _protect; // protect data value 0 (it means "NO DATA")

  double _preGamma;
  int _preGammaN;
  int _preGammaD;

  double _postGamma;
  int _postGammaN;
  int _postGammaD;

  int _zoom;
  double _scale;

  int prevZoom;
  double prevScale;
  double prevX;
  double prevY;

  int _resetOnFileOpen;
  int _swapRedAndBlue;

  int _contrast;

  int _globalMask;
  std::string gHisIn;
  std::string gLutOut;
  std::string gLutWorkOut;
};

#endif

