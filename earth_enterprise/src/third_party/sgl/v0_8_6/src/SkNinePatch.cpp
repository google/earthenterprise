/* libs/graphics/effects/SkNinePatch.cpp
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

#include "SkNinePatch.h"
#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkPaint.h"

#define USE_TRACEx

#ifdef USE_TRACE
    static bool gTrace;
#endif

#include "SkColorPriv.h"

static SkColor undo_premultiply(SkPMColor c)
{
    unsigned a = SkGetPackedA32(c);
    if (a != 0 && a != 0xFF)
    {
        unsigned scale = (SK_Fixed1 * 255) / a;
        unsigned r = SkFixedRound(SkGetPackedR32(c) * scale);
        unsigned g = SkFixedRound(SkGetPackedG32(c) * scale);
        unsigned b = SkFixedRound(SkGetPackedB32(c) * scale);
        c = SkColorSetARGB(a, r, g, b);
    }
    return c;
}

static void drawColumn(SkCanvas* canvas, SkRect16* src, SkRect* dst,
                       const SkRect& bounds, const android::Res_png_9patch& chunk,
                       const SkBitmap& bitmap, const SkPaint& paint, int first, SkColor initColor)
{
    const int32_t d0 = chunk.yDivs[0], d1 = chunk.yDivs[1];
    const bool hasXfer = paint.getXfermode() != NULL;
    uint32_t c;

// upper row
    src->fTop = 0;
    src->fBottom = d0;
    dst->fTop = bounds.fTop;
    dst->fBottom = bounds.fTop + SkIntToScalar(d0);
    c = chunk.colors[first];
    if (d0 > 0 && (c != chunk.TRANSPARENT_COLOR || hasXfer))
    {
        if (c == chunk.NO_COLOR)
        {
            canvas->drawBitmap(bitmap, src, *dst, paint);
        }
        else
        {
            ((SkPaint*)&paint)->setColor(c);
            canvas->drawRect(*dst, paint);
            ((SkPaint*)&paint)->setColor(initColor);
        }
    }

    first += 3;

// mid row
    src->fTop = d0;
    src->fBottom = d1;
    dst->fTop = dst->fBottom;
    dst->fBottom = bounds.fBottom - SkIntToScalar(bitmap.height() - d1);

    if (dst->fBottom > dst->fTop)
    {
        c = chunk.colors[first];
        if (c != chunk.NO_COLOR)
        {
            if (c != chunk.TRANSPARENT_COLOR || hasXfer) {
                ((SkPaint*)&paint)->setColor(c);
                canvas->drawRect(*dst, paint);
                ((SkPaint*)&paint)->setColor(initColor);
            }
        }
        else if (src->width() == 1 && src->height() == 1)
        {
            SkColor c = 0;
            int     x = src->fLeft;
            int     y = src->fTop;
    
            switch (bitmap.getConfig()) {
            case SkBitmap::kARGB_8888_Config:
                c = undo_premultiply(*bitmap.getAddr32(x, y));
                break;
            case SkBitmap::kRGB_565_Config:
                c = SkPixel16ToPixel32(*bitmap.getAddr16(x, y));
                break;
            case SkBitmap::kIndex8_Config:
                {
                    SkColorTable* ctable = bitmap.getColorTable();
                    c = undo_premultiply((*ctable)[*bitmap.getAddr8(x, y)]);
                }
                break;
            default:
                goto SLOW_CASE;
            }

#ifdef SK_DEBUG
//        printf("---------------- center color for the ninepatch: 0x%X\n", c);
#endif

            if (0 != c || paint.getXfermode() != NULL)
            {
                ((SkPaint*)&paint)->setColor(c);
                canvas->drawRect(*dst, paint);
            }
        }
        else
        {
            SLOW_CASE:
            canvas->drawBitmap(bitmap, src, *dst, paint);
        }
    }
    else
    {
        dst->fBottom = dst->fTop;
    }

    first += 3;

// lower row
    src->fTop = src->fBottom;
    src->fBottom = bitmap.height();
    dst->fTop = dst->fBottom;
    dst->fBottom = bounds.fBottom;
    c = chunk.colors[first];
    if (d0 > 0 && (c != chunk.TRANSPARENT_COLOR || hasXfer))
    {
        if (c == chunk.NO_COLOR)
        {
            canvas->drawBitmap(bitmap, src, *dst, paint);
        }
        else
        {
            ((SkPaint*)&paint)->setColor(c);
            canvas->drawRect(*dst, paint);
            ((SkPaint*)&paint)->setColor(initColor);
        }
    }
}

void SkNinePatch::Draw(SkCanvas* canvas, const SkRect& bounds,
                     const SkBitmap& bitmap, const android::Res_png_9patch& chunk,
                     const SkPaint* paint)
{
#ifdef USE_TRACE
    if (10 == margin.fLeft) gTrace = true;
#endif

    SkASSERT(canvas);

#ifdef USE_TRACE
    if (gTrace) SkDebugf("======== ninepatch bounds [%g %g]\n", SkScalarToFloat(bounds.width()), SkScalarToFloat(bounds.height()));
    if (gTrace) SkDebugf("======== ninepatch paint bm [%d,%d]\n", bitmap.width(), bitmap.height());
    //if (gTrace) SkDebugf("======== ninepatch margin [%d,%d,%d,%d]\n", margin.fLeft, margin.fTop, margin.fRight, margin.fBottom);
#endif


    if (bounds.isEmpty() ||
        bitmap.width() == 0 || bitmap.height() == 0 || bitmap.getPixels() == NULL ||
        paint && paint->getXfermode() == NULL && paint->getAlpha() == 0)
    {
#ifdef USE_TRACE
        if (gTrace) SkDebugf("======== abort ninepatch draw\n");
#endif
        return;
    }

    SkPaint defaultPaint;
    if (NULL == paint)
        paint = &defaultPaint;

    SkRect      dst;
    SkRect16    src;

    const int32_t d0 = chunk.xDivs[0], d1 = chunk.xDivs[1];
    const SkColor initColor = ((SkPaint*)paint)->getColor();

// left column
    src.fLeft = 0;
    src.fRight = d0;
    dst.fLeft = bounds.fLeft;
    dst.fRight = bounds.fLeft + SkIntToScalar(d0);
    if (d0 > 0)
    {
        drawColumn(canvas, &src, &dst, bounds, chunk, bitmap, *paint, 0, initColor);
    }
// mid column
    src.fLeft = src.fRight;
    src.fRight = d1;
    dst.fLeft = dst.fRight;
    dst.fRight = bounds.fRight - SkIntToScalar(bitmap.width() - d1);
    if (dst.fRight > dst.fLeft)
    {
        drawColumn(canvas, &src, &dst, bounds, chunk, bitmap, *paint, 1, initColor);
    }
    else
    {
        dst.fRight = dst.fLeft;
    }
// right column
    src.fLeft = src.fRight;
    src.fRight = bitmap.width();
    dst.fLeft = dst.fRight;
    dst.fRight = bounds.fRight;
    if (src.fRight > src.fLeft)
    {
        drawColumn(canvas, &src, &dst, bounds, chunk, bitmap, *paint, 2, initColor);
    }

#ifdef USE_TRACE
    gTrace = false;
#endif
}

void SkNinePatch::Draw(SkCanvas* canvas, const SkRect& bounds,
                     const SkBitmap& bitmap, const SkRect16& margins,
                     const SkPaint* paint)
{
    android::Res_png_9patch chunk;
    chunk.xDivs[0] = margins.fLeft;
    chunk.xDivs[1] = bitmap.width() - margins.fRight;
    chunk.yDivs[0] = margins.fTop;
    chunk.yDivs[1] = bitmap.height() - margins.fBottom;
    for (int i=0; i<9; i++) {
        chunk.colors[i] = chunk.NO_COLOR;
    }
    Draw(canvas, bounds, bitmap, chunk, paint);
}

#if 0
void SkNinePatch::Draw(SkCanvas* canvas, const SkRect& bounds,
                     const SkBitmap& bitmap, int cx, int cy,
                     const SkPaint* paint)
{
    SkRect16    marginRect;
    
    marginRect.set(cx, cy, bitmap.width() - cx - 1, bitmap.height() - cy - 1);

    SkNinePatch::Draw(canvas, bounds, bitmap, marginRect, paint);
}
#endif

