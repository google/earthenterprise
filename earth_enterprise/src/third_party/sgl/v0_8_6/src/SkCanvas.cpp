/* libs/graphics/sgl/SkCanvas.cpp
**
** Copyright 2006, Google Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#include "SkCanvas.h"
#include "SkBounder.h"
#include "SkDevice.h"
#include "SkDraw.h"
#include "SkTemplates.h"
#include "SkUtils.h"

//#define SK_TRACE_SAVERESTORE

#ifdef SK_TRACE_SAVERESTORE
    static int gLayerCounter;
    static void inc_layer() { ++gLayerCounter; printf("----- inc layer %d\n", gLayerCounter); }
    static void dec_layer() { --gLayerCounter; printf("----- dec layer %d\n", gLayerCounter); }

    static int gRecCounter;
    static void inc_rec() { ++gRecCounter; printf("----- inc rec %d\n", gRecCounter); }
    static void dec_rec() { --gRecCounter; printf("----- dec rec %d\n", gRecCounter); }

    static int gCanvasCounter;
    static void inc_canvas() { ++gCanvasCounter; printf("----- inc canvas %d\n", gCanvasCounter); }
    static void dec_canvas() { --gCanvasCounter; printf("----- dec canvas %d\n", gCanvasCounter); }
#else
    #define inc_layer()
    #define dec_layer()
    #define inc_rec()
    #define dec_rec()
    #define inc_canvas()
    #define dec_canvas()
#endif

struct MCRecLayer {
    SkDevice*   fDevice;
    int         fX, fY;
    SkPaint     fPaint;

    MCRecLayer(const SkPaint& paint) : fPaint(paint) { inc_layer(); }
    ~MCRecLayer() { fDevice->safeUnref(); dec_layer(); }
};

struct SkCanvas::MCRec {
    MCRec*              fNext;

    SkMatrix            fMatrix;
    SkMatrix::MapPtProc fMapPtProc;
    SkRegion            fRegion;

    MCRecLayer* fLayer;         // may be NULL
    SkDevice*   fCurrDevice;    // we do not ref/unref this guy

    uint8_t fSetPaintBits;
    uint8_t fClearPaintBits;
    
    MCRec() : fLayer(NULL) { inc_rec(); }
    MCRec(const MCRec& other)
        : fMatrix(other.fMatrix), fRegion(other.fRegion), fLayer(NULL)
    {
        // don't bother initializing fNext
        fMapPtProc      = other.fMapPtProc;
        fCurrDevice     = other.fCurrDevice;
        fSetPaintBits   = other.fSetPaintBits;
        fClearPaintBits = other.fClearPaintBits;
        inc_rec();
    }
    ~MCRec()
    {
        SkDELETE(fLayer);
        dec_rec();
    }
};

class AutoPaintSetClear {
public:
    AutoPaintSetClear(const SkPaint& paint, uint32_t setBits, uint32_t clearBits) : fPaint(paint)
    {
        fFlags = paint.getFlags();
        ((SkPaint*)&paint)->setFlags((fFlags | setBits) & ~clearBits);
    }
    ~AutoPaintSetClear()
    {
        ((SkPaint*)&fPaint)->setFlags(fFlags);
    }
private:
    const SkPaint&  fPaint;
    uint32_t        fFlags;

    // illegal
    AutoPaintSetClear(const AutoPaintSetClear&);
    AutoPaintSetClear& operator=(const AutoPaintSetClear&);
};

class SkAutoBounderCommit {
public:
    SkAutoBounderCommit(SkBounder* bounder) : fBounder(bounder) {}
    ~SkAutoBounderCommit() { if (fBounder) fBounder->commit(); }
private:
    SkBounder*  fBounder;
};

////////////////////////////////////////////////////////////////////////////

void SkCanvas::init(SkDevice* device)
{
    fBounder = NULL;

    fMCRec = (MCRec*)fMCStack.push_back();
    new (fMCRec) MCRec;

    fMCRec->fNext = NULL;
    fMCRec->fMatrix.reset();
    fMCRec->fSetPaintBits = 0;
    fMCRec->fClearPaintBits = 0;
    fMCRec->fMapPtProc = NULL;  // mark as dirty/unknown
    fMCRec->fLayer = NULL;

    fDevice = device;
    if (device)
    {
        device->ref();
        fMCRec->fCurrDevice = device;
        fMCRec->fRegion.setRect(0, 0, device->width(), device->height());
    }
    else
        fMCRec->fCurrDevice = &fEmptyDevice;
}

SkCanvas::SkCanvas(SkDevice* device) : fMCStack(sizeof(MCRec), fMCRecStorage, sizeof(fMCRecStorage)), fEmptyDevice(SkBitmap(), false)
{
    inc_canvas();
    this->init(device);
}

SkCanvas::SkCanvas(const SkBitmap& bitmap) : fMCStack(sizeof(MCRec), fMCRecStorage, sizeof(fMCRecStorage)), fEmptyDevice(SkBitmap(), false)
{
    inc_canvas();
    this->init(SkNEW_ARGS(SkDevice, (bitmap, false)));
    fDevice->unref();
}

SkCanvas::~SkCanvas()
{
    // free up the contents of our deque
    this->restoreToCount(0);

    fDevice->safeUnref();
    fBounder->safeUnref();
    
    dec_canvas();
}

SkBounder* SkCanvas::setBounder(SkBounder* bounder)
{
    SkRefCnt_SafeAssign(fBounder, bounder);
    return bounder;
}

void SkCanvas::setBitmapDevice(const SkBitmap& bitmap, bool transferPixelOwnershp)
{
    this->setDevice(SkNEW_ARGS(SkDevice, (bitmap, transferPixelOwnershp)))->unref();
}

SkDevice* SkCanvas::setDevice(SkDevice* device)
{
    if (fDevice == device)
        return device;

    int prevWidth = 0;
    int prevHeight = 0;
    int width = 0;
    int height = 0;

    if (fDevice)
    {
        prevWidth = fDevice->width();
        prevHeight = fDevice->height();
        fDevice->unref();
    }
    if (device)
    {
        width = device->width();
        height = device->height();
        device->ref();
    }
    fDevice = device;

    /*  Now we update our initial region to have the bounds of the new device,
        and then intersect all of the clips in our stack with these bounds,
        to ensure that we can't draw outside of the device's bounds (and trash
        memory).

        NOTE: this is only a partial-fix, since if the new device is larger than
        the previous one, we don't know how to "enlarge" the clips in our stack,
        so drawing may be artificially restricted. Without keeping a history of 
        all calls to canvas->clipRect() and canvas->clipPath(), we can't exactly
        reconstruct the correct clips, so this approximation will have to do.
        The caller really needs to restore() back to the base if they want to
        accurately take advantage of the new device bounds.
    */

    if (prevWidth != width || prevHeight != height)
    {
        SkRect16    r;
        r.set(0, 0, width, height);

        SkDeque::Iter   iter(fMCStack);
        MCRec*          rec = (MCRec*)iter.next();
        
        SkASSERT(rec);
        rec->fRegion.setRect(r);
        rec->fCurrDevice = device ? device : &fEmptyDevice;

        while ((rec = (MCRec*)iter.next()) != NULL)
            (void)rec->fRegion.op(r, SkRegion::kIntersect_Op);
    }
    
    return device;
}

///////////////////////////////////////////////////////////////////////////////////////////

uint32_t SkCanvas::getPaintSetBits() const
{
    return fMCRec->fSetPaintBits;
}

uint32_t SkCanvas::getPaintClearBits() const
{
    return fMCRec->fClearPaintBits;
}

void SkCanvas::setPaintSetClearBits(uint32_t setBits, uint32_t clearBits)
{
    fMCRec->fSetPaintBits = SkToU8(setBits & SkPaint::kAllFlags);
    fMCRec->fClearPaintBits = SkToU8(clearBits & SkPaint::kAllFlags);
}

void SkCanvas::orPaintSetClearBits(uint32_t setBits, uint32_t clearBits)
{
    // call through to the virtual method
    this->setPaintSetClearBits(fMCRec->fSetPaintBits | setBits,
                               fMCRec->fClearPaintBits | clearBits);
}

/////////////////////////////////////////////////////////////////////////////

int SkCanvas::save()
{
    int saveCount = this->getSaveCount(); // record this before the actual save

    MCRec* newTop = (MCRec*)fMCStack.push_back();
    new (newTop) MCRec(*fMCRec);    // balanced in restore()

    newTop->fNext = fMCRec;
    fMCRec = newTop;

    return saveCount;
}

static SkBitmap::Config resolve_config(SkBitmap::Config device, SkCanvas::LayerFlags flags, bool* isOpaque)
{
    *isOpaque = (flags & SkCanvas::kHasAlpha_LayerFlag) == 0;

    switch (device) {
    case SkBitmap::kARGB_8888_Config:
        return SkBitmap::kARGB_8888_Config;

    case SkBitmap::kRGB_565_Config:
        return (flags & (SkCanvas::kFullColor_LayerFlag | SkCanvas::kHasAlpha_LayerFlag)) ?
                    SkBitmap::kARGB_8888_Config : SkBitmap::kRGB_565_Config;
            
    case SkBitmap::kA8_Config:
        return SkBitmap::kA8_Config;

    default:
        SkASSERT(!"unsupported device config");
    }
    return SkBitmap::kNo_Config;
}

int SkCanvas::saveLayer(const SkRect* bounds, const SkPaint& paint, LayerFlags flags)
{
    // do this before we create the layer
    int count = this->save();

    SkRect      r;
    SkRect16    ir;

    if (NULL == bounds)
    {
        (void)this->getClipBounds(&r);
        bounds = &r;
    }

    this->getTotalMatrix().mapRect(&r, *bounds);
    r.roundOut(&ir);
    
    if (ir.intersect(this->getTotalClip().getBounds()))
    {
        bool isOpaque;
        SkBitmap::Config config = resolve_config(fMCRec->fCurrDevice->config(), flags, &isOpaque);

        MCRecLayer* layer = SkNEW_ARGS(MCRecLayer, (paint));

        layer->fDevice = this->createDevice(config, ir.width(), ir.height(), isOpaque);
        layer->fX = ir.fLeft;
        layer->fY = ir.fTop;

        fMCRec->fLayer = layer;
        fMCRec->fCurrDevice = layer->fDevice;

        fMCRec->fMatrix.postTranslate(-SkIntToScalar(ir.fLeft), -SkIntToScalar(ir.fTop));
        fMCRec->fMapPtProc = NULL;

        fMCRec->fRegion.op(ir, SkRegion::kIntersect_Op);
        fMCRec->fRegion.translate(-ir.fLeft, -ir.fTop);
    }
    return count;
}

int SkCanvas::saveLayerAlpha(const SkRect* bounds, U8CPU alpha, LayerFlags flags)
{
    SkPaint paint;
    
    paint.setAlpha(alpha);
    return this->saveLayer(bounds, paint, flags);
}

void SkCanvas::restore()
{
    SkASSERT(!fMCStack.empty());

    MCRecLayer*                 layer = fMCRec->fLayer;
    SkAutoTDelete<MCRecLayer>   ad(layer);
    // now detach it from fMCRec
    fMCRec->fLayer = NULL;

    // now do the normal restore()
    fMCRec->~MCRec();       // balanced in save()
    fMCStack.pop_back();
    fMCRec = (MCRec*)fMCStack.back();

    // now handle the layer if needed
    if (layer)
    {
        this->drawSprite(layer->fDevice->accessBitmap(), layer->fX, layer->fY, layer->fPaint);
    }
}

int SkCanvas::getSaveCount() const
{
    return fMCStack.count();
}

void SkCanvas::restoreToCount(int count)
{
    SkASSERT(fMCStack.count() >= count);

    while (fMCStack.count() > count)
        this->restore();
}

/////////////////////////////////////////////////////////////////////////////

bool SkCanvas::translate(SkScalar dx, SkScalar dy)
{
    fMCRec->fMapPtProc = NULL;  // mark as dirty/unknown
    return fMCRec->fMatrix.preTranslate(dx, dy);
}

bool SkCanvas::scale(SkScalar sx, SkScalar sy, SkScalar px, SkScalar py)
{
    fMCRec->fMapPtProc = NULL;  // mark as dirty/unknown
    return fMCRec->fMatrix.preScale(sx, sy, px, py);
}

bool SkCanvas::scale(SkScalar sx, SkScalar sy)
{
    fMCRec->fMapPtProc = NULL;  // mark as dirty/unknown
    return fMCRec->fMatrix.preScale(sx, sy);
}

bool SkCanvas::rotate(SkScalar degrees, SkScalar px, SkScalar py)
{
    fMCRec->fMapPtProc = NULL;  // mark as dirty/unknown
    return fMCRec->fMatrix.preRotate(degrees, px, py);
}

bool SkCanvas::rotate(SkScalar degrees)
{
    fMCRec->fMapPtProc = NULL;  // mark as dirty/unknown
    return fMCRec->fMatrix.preRotate(degrees);
}

bool SkCanvas::skew(SkScalar sx, SkScalar sy, SkScalar px, SkScalar py)
{
    fMCRec->fMapPtProc = NULL;  // mark as dirty/unknown
    return fMCRec->fMatrix.preSkew(sx, sy, px, py);
}

bool SkCanvas::skew(SkScalar sx, SkScalar sy)
{
    fMCRec->fMapPtProc = NULL;  // mark as dirty/unknown
    return fMCRec->fMatrix.preSkew(sx, sy);
}

bool SkCanvas::concat(const SkMatrix& matrix)
{
    fMCRec->fMapPtProc = NULL;  // mark as dirty/unknown
    return fMCRec->fMatrix.preConcat(matrix);
}

//////////////////////////////////////////////////////////////////////////////

bool SkCanvas::clipRect(const SkRect& rect)
{
    if (fMCRec->fMatrix.rectStaysRect())
    {
        SkRect      r;
        SkRect16    ir;

        fMCRec->fMatrix.mapRect(&r, rect);
        r.round(&ir);
        return fMCRec->fRegion.op(ir, SkRegion::kIntersect_Op);
    }
    else
    {
        SkPath  path;

        path.addRect(rect);
        return this->clipPath(path);
    }
}

bool SkCanvas::clipRect(SkScalar left, SkScalar top, SkScalar right, SkScalar bottom)
{
    SkRect  r;
    
    r.set(left, top, right, bottom);
    return this->clipRect(r);
}

bool SkCanvas::clipPath(const SkPath& path)
{
    SkPath  devPath;

    path.transform(fMCRec->fMatrix, &devPath);
    return fMCRec->fRegion.setPath(devPath, &fMCRec->fRegion);
}

bool SkCanvas::clipDeviceRgn(const SkRegion& rgn)
{
    return fMCRec->fRegion.op(rgn, SkRegion::kIntersect_Op);
}

bool SkCanvas::quickReject(const SkRect& rect, bool antialiased) const
{
    if (fMCRec->fRegion.isEmpty() || rect.isEmpty())
        return true;

    SkRect      r;
    SkRect16    ir;

    fMCRec->fMatrix.mapRect(&r, rect);
    if (antialiased)
        r.roundOut(&ir);
    else
        r.round(&ir);

    return fMCRec->fRegion.quickReject(ir);
}

bool SkCanvas::quickReject(const SkPath& path, bool antialiased) const
{
    if (fMCRec->fRegion.isEmpty() || path.isEmpty())
        return true;        

    if (fMCRec->fMatrix.rectStaysRect())
    {
        SkRect  r;
        path.computeBounds(&r, SkPath::kExact_BoundsType);
        return this->quickReject(r, antialiased);
    }

    SkPath      dstPath;
    SkRect      r;
    SkRect16    ir;

    path.transform(fMCRec->fMatrix, &dstPath);
    dstPath.computeBounds(&r, SkPath::kExact_BoundsType);
    if (antialiased)
        r.roundOut(&ir);
    else
        r.round(&ir);

    return fMCRec->fRegion.quickReject(ir);
}

bool SkCanvas::getClipBounds(SkRect* bounds) const
{
    const SkRegion& clip = this->getTotalClip();
    if (clip.isEmpty())
    {
        if (bounds)
            bounds->setEmpty();
        return false;
    }

    if (bounds != NULL)
    {
        SkMatrix inverse;
        SkRect   r;

        this->getTotalMatrix().invert(&inverse);
        r.set(clip.getBounds());
        inverse.mapRect(bounds, r);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////

void SkCanvas::drawRGB(U8CPU r, U8CPU g, U8CPU b)
{
    SkPaint paint;

    paint.setARGB(0xFF, r, g, b);
    this->drawPaint(paint);
}

void SkCanvas::drawARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b)
{
    SkPaint paint;

    paint.setARGB(a, r, g, b);
    this->drawPaint(paint);
}

void SkCanvas::drawColor(SkColor c, SkPorterDuff::Mode mode)
{
    SkPaint paint;

    paint.setColor(c);
    paint.setPorterDuffXfermode(mode);
    this->drawPaint(paint);
}

void SkCanvas::drawPaint(const SkPaint& paint)
{
    SkAutoBounderCommit ac(fBounder);
    SkDraw              draw(*this);

    draw.drawPaint(paint);
}

void SkCanvas::drawLine(const SkPoint& start, const SkPoint& stop, const SkPaint& paint)
{
    AutoPaintSetClear   force(paint, fMCRec->fSetPaintBits, fMCRec->fClearPaintBits);
    SkAutoBounderCommit ac(fBounder);
    SkDraw              draw(*this);

    draw.drawLine(start, stop, paint);
}

void SkCanvas::drawLine(SkScalar x0, SkScalar y0, SkScalar x1, SkScalar y1, const SkPaint& paint)
{
    SkPoint pts[2];
    pts[0].set(x0, y0);
    pts[1].set(x1, y1);
    this->drawLine(pts[0], pts[1], paint);
}

void SkCanvas::drawRect(const SkRect& r, const SkPaint& paint)
{
    AutoPaintSetClear   force(paint, fMCRec->fSetPaintBits, fMCRec->fClearPaintBits);
    SkAutoBounderCommit ac(fBounder);
    SkDraw              draw(*this);

    draw.drawRect(r, paint);
}

void SkCanvas::drawRect(SkScalar left, SkScalar top, SkScalar right, SkScalar bottom, const SkPaint& paint)
{
    SkRect  r;
    
    r.set(left, top, right, bottom);
    this->drawRect(r, paint);
}

void SkCanvas::drawOval(const SkRect& oval, const SkPaint& paint)
{
    AutoPaintSetClear   force(paint, fMCRec->fSetPaintBits, fMCRec->fClearPaintBits);
    SkAutoBounderCommit ac(fBounder);
    SkDraw              draw(*this);

    SkPath  path;

    path.addOval(oval);
    draw.drawPath(path, paint, NULL, true);
}

void SkCanvas::drawCircle(SkScalar cx, SkScalar cy, SkScalar radius, const SkPaint& paint)
{
    SkRect  oval;
    
    oval.set(cx - radius, cy - radius, cx + radius, cy + radius);
    this->drawOval(oval, paint);
}

void SkCanvas::drawArc(const SkRect& oval, SkScalar startAngle, SkScalar sweepAngle, const SkPaint& paint)
{
    AutoPaintSetClear   force(paint, fMCRec->fSetPaintBits, fMCRec->fClearPaintBits);
    SkAutoBounderCommit ac(fBounder);
    SkDraw              draw(*this);

    SkPath  path;

    path.addArc(oval, startAngle, sweepAngle);
    draw.drawPath(path, paint, NULL, true);
}

void SkCanvas::drawRoundRect(const SkRect& r, SkScalar rx, SkScalar ry, const SkPaint& paint)
{
    AutoPaintSetClear   force(paint, fMCRec->fSetPaintBits, fMCRec->fClearPaintBits);
    SkAutoBounderCommit ac(fBounder);
    SkDraw              draw(*this);

    SkPath  path;

    path.addRoundRect(r, rx, ry, SkPath::kCW_Direction);
    draw.drawPath(path, paint, NULL, true);
}

void SkCanvas::drawPath(const SkPath& path, const SkPaint& paint)
{
    AutoPaintSetClear   force(paint, fMCRec->fSetPaintBits, fMCRec->fClearPaintBits);
    SkAutoBounderCommit ac(fBounder);
    SkDraw              draw(*this);

    draw.drawPath(path, paint, NULL, false);
}

void SkCanvas::drawBitmap(const SkBitmap& bitmap, SkScalar x, SkScalar y, const SkPaint& paint)
{
    AutoPaintSetClear   force(paint, fMCRec->fSetPaintBits, fMCRec->fClearPaintBits);
    SkAutoBounderCommit ac(fBounder);
    SkDraw              draw(*this);

    draw.drawBitmap(bitmap, x, y, paint);
}

void SkCanvas::drawBitmap(const SkBitmap& bitmap, SkScalar x, SkScalar y)
{
    SkPaint paint;
    this->drawBitmap(bitmap, x, y, paint);
}

////////////////////////////////////////////////////

static void* getSubAddr(const SkBitmap& bm, int x, int y)
{
    SkASSERT((unsigned)x < (unsigned)bm.width());
    SkASSERT((unsigned)y < (unsigned)bm.height());
    
    switch (bm.getConfig()) {
    case SkBitmap::kNo_Config:
    case SkBitmap::kA1_Config:
        return NULL;    // we don't draw these bitmap configs
    case SkBitmap::kA8_Config:
    case SkBitmap:: kIndex8_Config:
        break;
    case SkBitmap::kRGB_565_Config:
        x <<= 1;
        break;
    case SkBitmap::kARGB_8888_Config:
        x <<= 2;
        break;
    default:
        break;
    }
    return (char*)bm.getPixels() + x + y * bm.rowBytes();
}

void SkCanvas::drawBitmap(const SkBitmap& bitmap, const SkRect16* src, const SkRect& dst, const SkPaint& paint)
{
    if (bitmap.width() == 0 || bitmap.height() == 0 || bitmap.getPixels() == NULL)
        return;

    SkBitmap        tmp;
    const SkBitmap* bitmapPtr = &bitmap;
    
    if (NULL != src)
    {
        SkRect16 subset;
        
        subset.set(0, 0, bitmap.width(), bitmap.height());
        if (!subset.intersect(*src))
            return; // nothing to draw in the source
        
        void* pixels = getSubAddr(bitmap, subset.fLeft, subset.fTop);
        if (NULL == pixels)
            return; // not supported

        tmp.setConfig(bitmap.getConfig(), subset.width(), subset.height(), bitmap.rowBytes());
        tmp.setPixels(pixels);
        bitmapPtr = &tmp;
    }
    
    SkMatrix    matrix;
    SkRect      tmpSrc;

    tmpSrc.set(0, 0, SkIntToScalar(bitmapPtr->width()), SkIntToScalar(bitmapPtr->height()));
    matrix.setRectToRect(tmpSrc, dst, SkMatrix::kFill_ScaleToFit);

    this->save();
    this->concat(matrix);
    this->drawBitmap(*bitmapPtr, 0, 0, paint);
    this->restore();
}

void SkCanvas::drawSprite(const SkBitmap& bitmap, int x, int y, const SkPaint& paint)
{
    AutoPaintSetClear   force(paint, fMCRec->fSetPaintBits, fMCRec->fClearPaintBits);
    SkAutoBounderCommit ac(fBounder);
    SkDraw              draw(*this);

    draw.drawSprite(bitmap, x, y, paint);
}

void SkCanvas::drawText(const void* text, size_t byteLength, SkScalar x, SkScalar y, const SkPaint& paint)
{
    AutoPaintSetClear   force(paint, fMCRec->fSetPaintBits, fMCRec->fClearPaintBits);
    SkAutoBounderCommit ac(fBounder);
    SkDraw              draw(*this);

    draw.drawText((const char*)text, byteLength, x, y, paint);
}

void SkCanvas::drawPosText(const void* text, size_t byteLength, const SkPoint pos[], const SkPaint& paint)
{
    AutoPaintSetClear   force(paint, fMCRec->fSetPaintBits, fMCRec->fClearPaintBits);
    SkAutoBounderCommit ac(fBounder);
    SkDraw              draw(*this);

    draw.drawPosText((const char*)text, byteLength, pos, paint);
}

void SkCanvas::drawTextOnPath(const void* text, size_t byteLength, const SkPath& path,
                              SkScalar hOffset, SkScalar vOffset, const SkPaint& paint)
{
    SkMatrix    matrix;
    
    matrix.setTranslate(hOffset, vOffset);
    this->drawTextOnPath(text, byteLength, path, &matrix, paint);
}

void SkCanvas::drawTextOnPath(const void* text, size_t byteLength, const SkPath& path,
                              const SkMatrix* matrix, const SkPaint& paint)
{
    AutoPaintSetClear   force(paint, fMCRec->fSetPaintBits, fMCRec->fClearPaintBits);
    SkAutoBounderCommit ac(fBounder);
    SkDraw              draw(*this);
    
    draw.drawTextOnPath((const char*)text, byteLength, path, matrix, paint);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

SkDevice& SkCanvas::getCurrDevice() const
{
    SkASSERT(fMCRec->fCurrDevice);
    return *fMCRec->fCurrDevice;
}

SkMatrix::MapPtProc SkCanvas::getCurrMapPtProc() const
{
    if (fMCRec->fMapPtProc == NULL)
        fMCRec->fMapPtProc = fMCRec->fMatrix.getMapPtProc();

    return fMCRec->fMapPtProc;
}

const SkMatrix& SkCanvas::getTotalMatrix() const
{
    return fMCRec->fMatrix;
}

const SkRegion& SkCanvas::getTotalClip() const
{
    return fMCRec->fRegion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

SkDevice* SkCanvas::createDevice(SkBitmap::Config config, int width, int height, bool isOpaque)
{
    SkBitmap bitmap;
    
    bitmap.setConfig(config, width, height);
    bitmap.allocPixels();
    bitmap.setIsOpaque(isOpaque);
    if (!bitmap.isOpaque())
        bitmap.eraseARGB(0, 0, 0, 0);
    
    return SkNEW_ARGS(SkDevice, (bitmap, true));
}

