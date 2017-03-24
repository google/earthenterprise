/* libs/graphics/sgl/SkScan_AntiPath.cpp
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

#include "SkScanPriv.h"
#include "SkPath.h"
#include "SkMatrix.h"
#include "SkBlitter.h"
#include "SkRegion.h"
#include "SkAntiRun.h"

#define SHIFT   2
#define SCALE   (1 << SHIFT)
#define MASK    (SCALE - 1)

///////////////////////////////////////////////////////////////////////////////////////////

class BaseSuperBlitter : public SkBlitter {
public:
    BaseSuperBlitter(SkBlitter* realBlitter, const SkRegion* clip, const SkRect16& ir);

    virtual void blitAntiH(int x, int y, const SkAlpha antialias[], const int16_t runs[])
    {
        SkASSERT(!"How did I get here?");
    }
    virtual void blitV(int x, int y, int height, SkAlpha alpha)
    {
        SkASSERT(!"How did I get here?");
    }
    virtual void blitRect(int x, int y, int width, int height)
    {
        SkASSERT(!"How did I get here?");
    }

protected:
    SkBlitter*  fRealBlitter;
    int         fCurrIY;
    int         fWidth, fLeft, fSuperLeft;

    SkDEBUGCODE(int fCurrX;)
    SkDEBUGCODE(int fCurrY;)
};

BaseSuperBlitter::BaseSuperBlitter(SkBlitter* realBlitter, const SkRegion* clip, const SkRect16& ir)
{
    fRealBlitter = realBlitter;
    fLeft = ir.fLeft;
    fSuperLeft = ir.fLeft << SHIFT;
    fWidth = ir.width();
    fCurrIY = -1;
    SkDEBUGCODE(fCurrX = -1; fCurrY = -1;)
}

class SuperBlitter : public BaseSuperBlitter {
public:
    SuperBlitter(SkBlitter* realBlitter, const SkRegion* clip, const SkRect16& ir);
    virtual ~SuperBlitter()
    {
        this->flush();
        sk_free(fRuns.fRuns);
    }
    void flush();

    virtual void blitH(int x, int y, int width);

private:
    SkAlphaRuns fRuns;
};

SuperBlitter::SuperBlitter(SkBlitter* realBlitter, const SkRegion* clip, const SkRect16& ir)
    : BaseSuperBlitter(realBlitter, clip, ir)
{
    int width = ir.width();

    // extra one to store the zero at the end
    fRuns.fRuns = (int16_t*)sk_malloc_throw((width + 1 + (width + 2)/2) * sizeof(int16_t));
    fRuns.fAlpha = (uint8_t*)(fRuns.fRuns + width + 1);
    fRuns.reset(width);
}

void SuperBlitter::flush()
{
    if (fCurrIY >= 0)
    {
        if (!fRuns.empty())
        {
        //  SkDEBUGCODE(fRuns.dump();)
            fRealBlitter->blitAntiH(fLeft, fCurrIY, fRuns.fAlpha, fRuns.fRuns);
            fRuns.reset(fWidth);
        }
        fCurrIY = -1;
        SkDEBUGCODE(fCurrX = -1;)
    }
}

static inline int coverage_to_alpha(int aa)
{
    aa <<= 8 - 2*SHIFT;
    aa -= aa >> (8 - SHIFT - 1);
    return aa;
}

#define SUPER_Mask      ((1 << SHIFT) - 1)

void SuperBlitter::blitH(int x, int y, int width)
{
    int iy = y >> SHIFT;
    SkASSERT(iy >= fCurrIY);

    x -= fSuperLeft;
#if 0   // I should just need to assert
    SkASSERT(x >= 0);
#else
    // hack, until I figure out why my cubics (I think) go beyond the bounds
    if (x < 0)
    {
        width += x;
        x = 0;
    }
#endif

#ifdef SK_DEBUG
    SkASSERT(y >= fCurrY);
    SkASSERT(y != fCurrY || x >= fCurrX);
    fCurrY = y;
#endif

    if (iy != fCurrIY)  // new scanline
    {
        this->flush();
        fCurrIY = iy;
    }

    // we sub 1 from maxValue 1 time for each block, so that we don't
    // hit 256 as a summed max, but 255.
//  int maxValue = (1 << (8 - SHIFT)) - (((y & MASK) + 1) >> SHIFT);

#if 0
    SkAntiRun<SHIFT>    arun;   
    arun.set(x, x + width);
    fRuns.add(x >> SHIFT, arun.getStartAlpha(), arun.getMiddleCount(), arun.getStopAlpha(), maxValue);
#else
    {
        int start = x;
        int stop = x + width;

        SkASSERT(start >= 0 && stop > start);
        int fb = start & SUPER_Mask;
        int fe = stop & SUPER_Mask;
        int n = (stop >> SHIFT) - (start >> SHIFT) - 1;

        if (n < 0)
        {
            fb = fe - fb;
            n = 0;
            fe = 0;
        }
        else
        {
            if (fb == 0)
                n += 1;
            else
                fb = (1 << SHIFT) - fb;
        }
        fRuns.add(x >> SHIFT, coverage_to_alpha(fb), n, coverage_to_alpha(fe), (1 << (8 - SHIFT)) - (((y & MASK) + 1) >> SHIFT));
    }
#endif

#ifdef SK_DEBUG
    fRuns.assertValid(y & MASK, (1 << (8 - SHIFT)));
    fCurrX = x + width;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

class MaskSuperBlitter : public BaseSuperBlitter {
public:

    static bool CanHandleRect(const SkRect16& bounds)
    {
        int width = bounds.width();
        int rb = SkAlign4(width);

        return (width <= MaskSuperBlitter::kMAX_WIDTH) &&
               (rb * bounds.height() <= MaskSuperBlitter::kMAX_STORAGE);
    }

    MaskSuperBlitter(SkBlitter* realBlitter, const SkRegion* clip, const SkRect16& ir);
    virtual ~MaskSuperBlitter() { fRealBlitter->blitMask(fMask, fClipRect); }

    virtual void blitH(int x, int y, int width);

private:
    enum {
        kMAX_WIDTH = 32,    // so we don't try to do very wide things, where the RLE blitter would be faster
        kMAX_STORAGE = 1024
    };

    SkMask      fMask;
    SkRect16    fClipRect;
    // we add 1 because add_aa_span can write (unchanged) 1 extra byte at the end, rather than
    // perform a test to see if stopAlpha != 0
    uint32_t    fStorage[(kMAX_STORAGE >> 2) + 1];
};

MaskSuperBlitter::MaskSuperBlitter(SkBlitter* realBlitter, const SkRegion* clip, const SkRect16& ir)
    : BaseSuperBlitter(realBlitter, clip, ir)
{
    SkASSERT(CanHandleRect(ir));

    fMask.fImage    = (uint8_t*)fStorage;
    fMask.fBounds   = ir;
    fMask.fRowBytes = ir.width();
    fMask.fFormat   = SkMask::kA8_Format;
    
    fClipRect = ir;
    if (clip) {
        fClipRect.intersect(clip->getBounds());
    }
    
    memset(fStorage, 0, fMask.fBounds.height() * fMask.fRowBytes);
}

static void add_aa_span(uint8_t* alpha, U8CPU startAlpha)
{
    /*  I should be able to just add alpha[x] + startAlpha.
        However, if the trailing edge of the previous span and the leading
        edge of the current span round to the same super-sampled x value,
        I might overflow to 256 with this add, hence the funny subtract.
    */
    unsigned tmp = *alpha + startAlpha;
    SkASSERT(tmp <= 256);
    *alpha = SkToU8(tmp - (tmp >> 8));
}

static void add_aa_span(uint8_t* alpha, U8CPU startAlpha, int middleCount, U8CPU stopAlpha, U8CPU maxValue)
{
    SkASSERT(middleCount >= 0);

    /*  I should be able to just add alpha[x] + startAlpha.
        However, if the trailing edge of the previous span and the leading
        edge of the current span round to the same super-sampled x value,
        I might overflow to 256 with this add, hence the funny subtract.
    */
    unsigned tmp = *alpha + startAlpha;
    SkASSERT(tmp <= 256);
    *alpha++ = SkToU8(tmp - (tmp >> 8));

    while (--middleCount >= 0)
    {
        alpha[0] = SkToU8(alpha[0] + maxValue);
        alpha += 1;
    }

    // potentially this can be off the end of our "legal" alpha values, but that
    // only happens if stopAlpha is also 0. Rather than test for stopAlpha != 0
    // every time (slow), we just do it, and ensure that we've allocated extra space
    // (see the + 1 comment in fStorage[]
    *alpha = SkToU8(*alpha + stopAlpha);
}

void MaskSuperBlitter::blitH(int x, int y, int width)
{
    int iy = (y >> SHIFT);
    
    SkASSERT(iy >= fMask.fBounds.fTop && iy < fMask.fBounds.fBottom);
    iy -= fMask.fBounds.fTop;   // make it relative to 0

    x -= fSuperLeft;
    // hack, until I figure out why my cubics (I think) go beyond the bounds
    if (x < 0)
    {
        width += x;
        x = 0;
    }

    // we sub 1 from maxValue 1 time for each block, so that we don't
    // hit 256 as a summed max, but 255.
//  int maxValue = (1 << (8 - SHIFT)) - (((y & MASK) + 1) >> SHIFT);

    uint8_t* row = fMask.fImage + iy * fMask.fRowBytes + (x >> SHIFT);

    int start = x;
    int stop = x + width;

    SkASSERT(start >= 0 && stop > start);
    int fb = start & SUPER_Mask;
    int fe = stop & SUPER_Mask;
    int n = (stop >> SHIFT) - (start >> SHIFT) - 1;


    if (n < 0)
    {
        add_aa_span(row, coverage_to_alpha(fe - fb));
    }
    else
    {
        fb = (1 << SHIFT) - fb;
        add_aa_span(row,  coverage_to_alpha(fb), n, coverage_to_alpha(fe),
                    (1 << (8 - SHIFT)) - (((y & MASK) + 1) >> SHIFT));
    }

#ifdef SK_DEBUG
    fCurrX = x + width;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////

static int overflows_short(int value)
{
    return value - (short)value;
}

void SkScan::AntiFillPath(const SkPath& path, const SkRegion* clip, SkBlitter* blitter)
{
    if (clip && clip->isEmpty())
        return;

    SkRect      r;
    SkRect16    ir;

    path.computeBounds(&r, SkPath::kFast_BoundsType);
    r.roundOut(&ir);
    if (ir.isEmpty())
        return;

    if (overflows_short(ir.fLeft << SHIFT) ||
        overflows_short(ir.fRight << SHIFT) ||
        overflows_short(ir.width() << SHIFT) ||
        overflows_short(ir.fTop << SHIFT) ||
        overflows_short(ir.fBottom << SHIFT) ||
        overflows_short(ir.height() << SHIFT))
        return;

    SkScanClipper   clipper(blitter, clip, ir);
    const SkRect16* clipRect = clipper.getClipRect();

    blitter = clipper.getBlitter();
    if (blitter == NULL) // clipped out
        return;

    SkRect16    superIR, superRect, *superClipRect = NULL;

    superIR.set(ir.fLeft << SHIFT, ir.fTop << SHIFT,
                ir.fRight << SHIFT, ir.fBottom << SHIFT);

    if (clipRect)
    {
        superRect.set(  clipRect->fLeft << SHIFT, clipRect->fTop << SHIFT,
                        clipRect->fRight << SHIFT, clipRect->fBottom << SHIFT);
        superClipRect = &superRect;
    }

    if (MaskSuperBlitter::CanHandleRect(ir))
    {
        MaskSuperBlitter    superBlit(blitter, clip, ir);
        sk_fill_path(path, superClipRect, &superBlit, superIR, SHIFT);
    }
    else
    {
        SuperBlitter    superBlit(blitter, clip, ir);
        sk_fill_path(path, superClipRect, &superBlit, superIR, SHIFT);
    }
}
