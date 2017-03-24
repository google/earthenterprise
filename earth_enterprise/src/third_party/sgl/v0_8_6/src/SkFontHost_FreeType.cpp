/* libs/graphics/ports/SkFontHost_FreeType.cpp
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

#include "SkScalerContext.h"
#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkDescriptor.h"
#include "SkFDot6.h"
#include "SkFontHost.h"
#include "SkMask.h"
#include "SkStream.h"
#include "SkString.h"
#include "SkThread.h"
#include "SkTemplates.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_SIZES_H

//#define ENABLE_GLYPH_SPEW     // for tracing calls to generateMetrics/generateImage

#ifdef SK_DEBUG
    #define SkASSERT_CONTINUE(pred)                                                         \
        do {                                                                                \
            if (!(pred))                                                                    \
                SkDebugf("file %s:%d: assert failed '" #pred "'\n", __FILE__, __LINE__);    \
        } while (false)
#else
    #define SkASSERT_CONTINUE(pred)
#endif

//////////////////////////////////////////////////////////////////////////

struct SkFaceRec;

static SkMutex      gFTMutex;
static int          gFTCount;
static FT_Library   gFTLibrary;
static SkFaceRec*   gFaceRecHead;

/////////////////////////////////////////////////////////////////////////

class SkScalerContext_FreeType : public SkScalerContext {
public:
    SkScalerContext_FreeType(const SkDescriptor* desc);
    virtual ~SkScalerContext_FreeType();

protected:
    virtual unsigned generateGlyphCount() const;
    virtual uint16_t generateCharToGlyph(SkUnichar uni);
    virtual void generateMetrics(SkGlyph* glyph);
    virtual void generateImage(const SkGlyph& glyph);
    virtual void generatePath(const SkGlyph& glyph, SkPath* path);
    virtual void generateLineHeight(SkPoint* ascent, SkPoint* descent);

private:
    SkFaceRec*  fFaceRec;
    FT_Face     fFace;              // reference to shared face in gFaceRecHead
    FT_Size     fFTSize;            // our own copy
    SkFixed     fScaleX, fScaleY;
    FT_Matrix   fMatrix22;
    uint32_t    fLoadGlyphFlags;

    FT_Error setupSize();
};

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#include "SkStream.h"

struct SkFaceRec {
    SkFaceRec*      fNext;
    FT_Face         fFace;
    FT_StreamRec    fFTStream;
    SkStream*       fSkStream;
    uint32_t        fRefCnt;
    SkString        fKey;

    SkFaceRec(SkStream* strm, const SkString& key);    
    ~SkFaceRec()
    {
        delete fSkStream;
//        SkDEBUGF(("~SkFaceRec: closing %s\n", fKey.c_str()));
    }
};

extern "C" {
    static unsigned long sk_stream_read(FT_Stream       stream,
                                        unsigned long   offset,
                                        unsigned char*  buffer,
                                        unsigned long   count )
    {
        SkFaceRec* rec = (SkFaceRec*)stream->descriptor.pointer;
        SkStream* str = rec->fSkStream;

        if (count)
        {
            if (!str->rewind())
            {
                SkDEBUGF(("sk_stream_read(file:%s offset:%ld count:%ld): rewind failed\n",
                rec->fKey.c_str(), offset, count));
                return 0;
            }
            else
            {
                unsigned long ret;
                if (offset)
                {
                    ret = str->read(NULL, offset);
                    if (ret != offset) {
                        SkDEBUGF(("sk_stream_read(file:%s offset:%d) read returned %d\n",
                                  rec->fKey.c_str(), offset, ret));
                        return 0;
                    }
                }
                ret = str->read(buffer, count);
                if (ret != count) {
                    SkDEBUGF(("sk_stream_read(file:%s offset:%d count:%d) read returned %d\n",
                             rec->fKey.c_str(), offset, count, ret));
                    return 0;
                }
                count = ret;
            }
        }
        return count;
    }

    static void sk_stream_close( FT_Stream stream)
    {
    }
}

SkFaceRec::SkFaceRec(SkStream* strm, const SkString& key) : fSkStream(strm), fKey(key)
{
//    SkDEBUGF(("SkFaceRec: opening %s (%p)\n", key.c_str(), strm));

    memset(&fFTStream, 0, sizeof(fFTStream));
    fFTStream.size = fSkStream->read(NULL, 0);   // find out how big the stream is
    fFTStream.descriptor.pointer = this;
    fFTStream.read  = sk_stream_read;
    fFTStream.close = sk_stream_close;
}

static SkFaceRec* ref_ft_face(const SkDescriptor* desc)
{
    SkString     keyString;
    SkFontHost::GetDescriptorKeyString(desc, &keyString);

    SkFaceRec* rec = gFaceRecHead;
    while (rec)
    {
        if (rec->fKey == keyString)
        {
            SkASSERT(rec->fFace);
            rec->fRefCnt += 1;
            return rec;
        }
        rec = rec->fNext;
    }

    SkStream* strm = SkFontHost::OpenDescriptorStream(desc, keyString.c_str());
    if (NULL == strm)
    {
        SkDEBUGF(("SkFontHost::OpenDescriptorStream failed opening %s\n", keyString.c_str()));
        sk_throw();
        return 0;
    }

    // this passes ownership of strm to the rec
    rec = SkNEW_ARGS(SkFaceRec, (strm, keyString));

    FT_Open_Args    args;
    memset(&args, 0, sizeof(args));
    const void* memoryBase = strm->getMemoryBase();

    if (NULL != memoryBase)
    {
//printf("mmap(%s)\n", keyString.c_str());
        args.flags = FT_OPEN_MEMORY;
        args.memory_base = (const FT_Byte*)memoryBase;
        args.memory_size = strm->getLength();
    }
    else
    {
//printf("fopen(%s)\n", keyString.c_str());
        args.flags = FT_OPEN_STREAM;
        args.stream = &rec->fFTStream;
    }

    FT_Error err = FT_Open_Face(gFTLibrary, &args, 0, &rec->fFace);

    if (err)    // bad filename, try the default font
    {
        fprintf(stderr, "ERROR: unable to open font '%s'\n", keyString.c_str());
        SkDELETE(rec);
        sk_throw();
        return 0;
    }
    else
    {
        SkASSERT(rec->fFace);
        //fprintf(stderr, "Opened font '%s'\n", filename.c_str());
        rec->fNext = gFaceRecHead;
        gFaceRecHead = rec;
        rec->fRefCnt = 1;
        return rec;
    }
}

static void unref_ft_face(FT_Face face)
{
    SkFaceRec*  rec = gFaceRecHead;
    SkFaceRec*  prev = NULL;
    while (rec)
    {
        SkFaceRec* next = rec->fNext;
        if (rec->fFace == face)
        {
            if (--rec->fRefCnt == 0)
            {
                if (prev)
                    prev->fNext = next;
                else
                    gFaceRecHead = next;

                FT_Done_Face(face);
                SkDELETE(rec);
            }
            return;
        }
        prev = rec;
        rec = next;
    }
    SkASSERT("shouldn't get here, face not in list");
}

///////////////////////////////////////////////////////////////////////////

SkScalerContext_FreeType::SkScalerContext_FreeType(const SkDescriptor* desc)
    : SkScalerContext(desc), fFTSize(NULL)
{
    SkAutoMutexAcquire  ac(gFTMutex);

    FT_Error    err;

    if (gFTCount == 0)
    {
        err = FT_Init_FreeType(&gFTLibrary);
//        SkDEBUGF(("FT_Init_FreeType returned %d\n", err));
        SkASSERT(err == 0);
    }
    ++gFTCount;

    // load the font file
    {
        fFaceRec = ref_ft_face(desc);
        fFace = fFaceRec ? fFaceRec->fFace : NULL;
    }

    // compute our factors from the record

    SkMatrix    m;

    fRec.getSingleMatrix(&m);

    //  now compute our scale factors
    SkScalar    sx = m.getScaleX();
    SkScalar    sy = m.getScaleY();

    if (m.getSkewX() || m.getSkewY() || sx < 0 || sy < 0)   // sort of give up on hinting
    {
        sx = SkMaxScalar(SkScalarAbs(sx), SkScalarAbs(m.getSkewX()));
        sy = SkMaxScalar(SkScalarAbs(m.getSkewY()), SkScalarAbs(sy));
        sx = sy = SkScalarAve(sx, sy);

        SkScalar inv = SkScalarInvert(sx);

        // flip the skew elements to go from our Y-down system to FreeType's
        fMatrix22.xx = SkScalarToFixed(SkScalarMul(m.getScaleX(), inv));
        fMatrix22.xy = -SkScalarToFixed(SkScalarMul(m.getSkewX(), inv));
        fMatrix22.yx = -SkScalarToFixed(SkScalarMul(m.getSkewY(), inv));
        fMatrix22.yy = SkScalarToFixed(SkScalarMul(m.getScaleY(), inv));
    }
    else
    {
        fMatrix22.xx = fMatrix22.yy = SK_Fixed1;
        fMatrix22.xy = fMatrix22.yx = 0;
    }

    fScaleX = SkScalarToFixed(sx);
    fScaleY = SkScalarToFixed(sy);

    // compute the flags we send to Load_Glyph
    {
        uint32_t flags = FT_LOAD_DEFAULT;
        uint32_t render_flags = FT_LOAD_TARGET_NORMAL;

        if (kNo_Hints == fRec.fHints)
            flags |= FT_LOAD_NO_HINTING;
        else if (kAuto_Hints == fRec.fHints)
            flags |= FT_LOAD_FORCE_AUTOHINT;

        // to enable light auto-hinting mode (MacOS X like), uncomment the line below
        // render_flags = FT_LOAD_TARGET_LIGHT

        if (SkMask::kBW_Format == fRec.fMaskFormat)
            render_flags = FT_LOAD_TARGET_MONO;
        else if (SkMask::kLCD_Format == fRec.fMaskFormat)
            render_flags = FT_LOAD_TARGET_LCD;

        fLoadGlyphFlags = flags | render_flags;
    }

    // now create the FT_Size

    {
        FT_Error    err;

        err = FT_New_Size(fFace, &fFTSize);
        if (err != 0) {
            SkDEBUGF(("SkScalerContext_FreeType::FT_New_Size(%s): FT_Set_Char_Size(0x%x, 0x%x) returned 0x%x\n",
                        fFaceRec->fKey.c_str(), fScaleX, fScaleY, err));
            fFace = NULL;
            return;
        }

        err = FT_Activate_Size(fFTSize);
        if (err != 0) {
            SkDEBUGF(("SkScalerContext_FreeType::FT_Activate_Size(%s, 0x%x, 0x%x) returned 0x%x\n",
                        fFaceRec->fKey.c_str(), fScaleX, fScaleY, err));
            fFTSize = NULL;
        }

        err = FT_Set_Char_Size( fFace,
                                SkFixedToFDot6(fScaleX), SkFixedToFDot6(fScaleY),
                                72, 72);
        if (err != 0) {
            SkDEBUGF(("SkScalerContext_FreeType::FT_Set_Char_Size(%s, 0x%x, 0x%x) returned 0x%x\n",
                        fFaceRec->fKey.c_str(), fScaleX, fScaleY, err));
            fFace = NULL;
            return;
        }

        FT_Set_Transform( fFace, &fMatrix22, NULL);
    }
}

SkScalerContext_FreeType::~SkScalerContext_FreeType()
{
    if (fFTSize != NULL)
        FT_Done_Size(fFTSize);

    SkAutoMutexAcquire  ac(gFTMutex);

    if (fFace != NULL)
        unref_ft_face(fFace);

    if (--gFTCount == 0)
    {
//        SkDEBUGF(("FT_Done_FreeType\n"));
        FT_Done_FreeType(gFTLibrary);
        SkDEBUGCODE(gFTLibrary = NULL;)
    }
}

/*  We call this before each use of the fFace, since we may be sharing
    this face with other context (at different sizes).
*/
FT_Error SkScalerContext_FreeType::setupSize()
{
    FT_Error    err = FT_Activate_Size(fFTSize);

    if (err != 0) {
        SkDEBUGF(("SkScalerContext_FreeType::FT_Activate_Size(%s, 0x%x, 0x%x) returned 0x%x\n",
                    fFaceRec->fKey.c_str(), fScaleX, fScaleY, err));
        fFTSize = NULL;
    }
    else
    {
        // seems we need to reset this every time (not sure why, but without it
        // I get random italics from some other fFTSize)
        FT_Set_Transform( fFace, &fMatrix22, NULL);
    }
    return err;
}

unsigned SkScalerContext_FreeType::generateGlyphCount() const
{
    return fFace->num_glyphs;
}

uint16_t SkScalerContext_FreeType::generateCharToGlyph(SkUnichar uni)
{
    return SkToU16(FT_Get_Char_Index( fFace, uni ));
}

static FT_Pixel_Mode compute_pixel_mode(SkMask::Format format)
{
    switch (format) {
    case SkMask::kBW_Format:
        return FT_PIXEL_MODE_MONO;
    case SkMask::kLCD_Format:
        return FT_PIXEL_MODE_LCD;
    case SkMask::kA8_Format:
    default:
        return FT_PIXEL_MODE_GRAY;
    }
}

void SkScalerContext_FreeType::generateMetrics(SkGlyph* glyph)
{
    SkAutoMutexAcquire  ac(gFTMutex);

    FT_Error    err;

    if (this->setupSize())
        goto ERROR;

    err = FT_Load_Glyph( fFace, glyph->getGlyphID(fBaseGlyphCount), fLoadGlyphFlags );
    if (err != 0)
    {
        SkDEBUGF(("SkScalerContext_FreeType::generateMetrics(%s): FT_Load_Glyph(glyph:%d flags:%d) returned 0x%x\n",
                    fFaceRec->fKey.c_str(), glyph->getGlyphID(fBaseGlyphCount), fLoadGlyphFlags, err));
    ERROR:
        glyph->fWidth   = 0;
        glyph->fHeight  = 0;
        glyph->fTop     = 0;
        glyph->fLeft    = 0;
        glyph->fAdvanceX = 0;
        glyph->fAdvanceY = 0;
        return;
    }
    
    switch ( fFace->glyph->format )
    {
      case FT_GLYPH_FORMAT_OUTLINE:
        FT_BBox bbox;
        
        FT_Outline_Get_CBox(&fFace->glyph->outline, &bbox);
        bbox.xMin &= ~63;
        bbox.yMin &= ~63;
        bbox.xMax  = (bbox.xMax + 63) & ~63;
        bbox.yMax  = (bbox.yMax + 63) & ~63;

        glyph->fWidth   = SkToU16((bbox.xMax - bbox.xMin) >> 6);
        glyph->fHeight  = SkToU16((bbox.yMax - bbox.yMin) >> 6);
        glyph->fTop     = -SkToS16(bbox.yMax >> 6);
        glyph->fLeft    = SkToS16(bbox.xMin >> 6);
        break;
        
      case FT_GLYPH_FORMAT_BITMAP:
        glyph->fWidth   = SkToU16(fFace->glyph->bitmap.width);
        glyph->fHeight  = SkToU16(fFace->glyph->bitmap.rows);
        glyph->fTop     = -SkToS16(fFace->glyph->bitmap_top);
        glyph->fLeft    = SkToS16(fFace->glyph->bitmap_left);
        break;

      default:
        SkASSERT(!"unknown glyph format");
        goto ERROR;      
    }
    
    if (kNo_Hints == fRec.fHints)
    {
        glyph->fAdvanceX = SkFixedMul(fMatrix22.xx, fFace->glyph->linearHoriAdvance);
        glyph->fAdvanceY = -SkFixedMul(fMatrix22.yx, fFace->glyph->linearHoriAdvance);
    }
    else
    {
        glyph->fAdvanceX = SkFDot6ToFixed(fFace->glyph->advance.x);
        glyph->fAdvanceY = -SkFDot6ToFixed(fFace->glyph->advance.y);
    }

#ifdef ENABLE_GLYPH_SPEW
    SkDEBUGF(("FT_Set_Char_Size(this:%p sx:%x sy:%x ", this, fScaleX, fScaleY));
    SkDEBUGF(("Metrics(glyph:%d flags:0x%x) w:%d\n", glyph->getGlyphID(fBaseGlyphCount), fLoadGlyphFlags, glyph->fWidth));
#endif
}

void SkScalerContext_FreeType::generateImage(const SkGlyph& glyph)
{
    SkAutoMutexAcquire  ac(gFTMutex);

    FT_Error    err;

    if (this->setupSize())
        goto ERROR;

    err = FT_Load_Glyph( fFace, glyph.getGlyphID(fBaseGlyphCount), fLoadGlyphFlags);
    if (err != 0) {
        SkDEBUGF(("SkScalerContext_FreeType::generateImage: FT_Load_Glyph(glyph:%d width:%d height:%d rb:%d flags:%d) returned 0x%x\n",
                    glyph.getGlyphID(fBaseGlyphCount), glyph.fWidth, glyph.fHeight, glyph.fRowBytes, fLoadGlyphFlags, err));
    ERROR:
        memset(glyph.fImage, 0, glyph.fRowBytes * glyph.fHeight);
        return;
    }

    switch ( fFace->glyph->format ) {
    case FT_GLYPH_FORMAT_OUTLINE:
        {
            FT_Outline* outline = &fFace->glyph->outline;
            FT_BBox     bbox;
            FT_Bitmap   target;
            
            FT_Outline_Get_CBox(outline, &bbox);
            FT_Outline_Translate(outline, -(bbox.xMin & ~63), -(bbox.yMin & ~63));

            target.width = glyph.fWidth;
            target.rows = glyph.fHeight;
            target.pitch = glyph.fRowBytes;
            target.buffer = reinterpret_cast<uint8_t*>(glyph.fImage);
            target.pixel_mode = compute_pixel_mode((SkMask::Format)fRec.fMaskFormat);
            target.num_grays = 256;
            
            memset(glyph.fImage, 0, glyph.fRowBytes * glyph.fHeight);
            FT_Outline_Get_Bitmap(gFTLibrary, outline, &target);
        }
        break;
    
    case FT_GLYPH_FORMAT_BITMAP:
        SkASSERT_CONTINUE(glyph.fWidth == fFace->glyph->bitmap.width);
        SkASSERT_CONTINUE(glyph.fHeight == fFace->glyph->bitmap.rows);
        SkASSERT_CONTINUE(glyph.fTop == -fFace->glyph->bitmap_top);
        SkASSERT_CONTINUE(glyph.fLeft == fFace->glyph->bitmap_left);

        {
            const uint8_t*  src = (const uint8_t*)fFace->glyph->bitmap.buffer;
            uint8_t*        dst = (uint8_t*)glyph.fImage;
            unsigned    srcRowBytes = fFace->glyph->bitmap.pitch;
            unsigned    dstRowBytes = glyph.fRowBytes;
            unsigned    minRowBytes = SkMin32(srcRowBytes, dstRowBytes);
            unsigned    extraRowBytes = dstRowBytes - minRowBytes;

            for (int y = fFace->glyph->bitmap.rows - 1; y >= 0; --y)
            {
                memcpy(dst, src, minRowBytes);
                memset(dst + minRowBytes, 0, extraRowBytes);
                src += srcRowBytes;
                dst += dstRowBytes;
            }
        }
        break;
    
    default:
        SkASSERT(!"unknown glyph format");
        goto ERROR;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

#define ft2sk(x)    SkFixedToScalar((x) << 10)

#if FREETYPE_MAJOR >= 2 && FREETYPE_MINOR >= 3
    #define CONST_PARAM const
#else   // older freetype doesn't use const here
    #define CONST_PARAM
#endif

static int move_proc(CONST_PARAM FT_Vector* pt, void* ctx)
{
    SkPath* path = (SkPath*)ctx;
    path->close();  // to close the previous contour (if any)
    path->moveTo(ft2sk(pt->x), -ft2sk(pt->y));
    return 0;
}

static int line_proc(CONST_PARAM FT_Vector* pt, void* ctx)
{
    SkPath* path = (SkPath*)ctx;
    path->lineTo(ft2sk(pt->x), -ft2sk(pt->y));
    return 0;
}

static int quad_proc(CONST_PARAM FT_Vector* pt0, CONST_PARAM FT_Vector* pt1, void* ctx)
{
    SkPath* path = (SkPath*)ctx;
    path->quadTo(ft2sk(pt0->x), -ft2sk(pt0->y), ft2sk(pt1->x), -ft2sk(pt1->y));
    return 0;
}

static int cubic_proc(CONST_PARAM FT_Vector* pt0, CONST_PARAM FT_Vector* pt1, CONST_PARAM FT_Vector* pt2, void* ctx)
{
    SkPath* path = (SkPath*)ctx;
    path->cubicTo(ft2sk(pt0->x), -ft2sk(pt0->y), ft2sk(pt1->x), -ft2sk(pt1->y), ft2sk(pt2->x), -ft2sk(pt2->y));
    return 0;
}

void SkScalerContext_FreeType::generatePath(const SkGlyph& glyph, SkPath* path)
{
    SkAutoMutexAcquire  ac(gFTMutex);

    SkASSERT(&glyph && path);

    if (this->setupSize()) {
        path->reset();
        return;
    }

    uint32_t flags = fLoadGlyphFlags;
    flags |= FT_LOAD_NO_BITMAP; // ignore embedded bitmaps so we're sure to get the outline
    flags &= ~FT_LOAD_RENDER;   // don't scan convert (we just want the outline)

    FT_Error err = FT_Load_Glyph( fFace, glyph.getGlyphID(fBaseGlyphCount), flags);

    if (err != 0) {
        SkDEBUGF(("SkScalerContext_FreeType::generatePath: FT_Load_Glyph(glyph:%d flags:%d) returned 0x%x\n",
                    glyph.getGlyphID(fBaseGlyphCount), flags, err));
        path->reset();
        return;
    }

    FT_Outline_Funcs    funcs;

    funcs.move_to   = move_proc;
    funcs.line_to   = line_proc;
    funcs.conic_to  = quad_proc;
    funcs.cubic_to  = cubic_proc;
    funcs.shift     = 0;
    funcs.delta     = 0;

    err = FT_Outline_Decompose(&fFace->glyph->outline, &funcs, path);

    if (err != 0) {
        SkDEBUGF(("SkScalerContext_FreeType::generatePath: FT_Load_Glyph(glyph:%d flags:%d) returned 0x%x\n",
                    glyph.getGlyphID(fBaseGlyphCount), flags, err));
        path->reset();
        return;
    }

    path->close();
}

static void map_y_to_pt(const FT_Matrix& mat, SkFixed y, SkPoint* pt)
{
    SkFixed x = SkFixedMul(mat.xy, y);
    y = SkFixedMul(mat.yy, y);

    pt->set(SkFixedToScalar(x), SkFixedToScalar(y));
}

void SkScalerContext_FreeType::generateLineHeight(SkPoint* ascent, SkPoint* descent)
{
    SkAutoMutexAcquire  ac(gFTMutex);

    if (this->setupSize()) {
        if (ascent)
            ascent->set(0, 0);
        if (descent)
            descent->set(0, 0);
        return;
    }

    if (ascent)
    {
        SkFixed a = SkMulDiv(fScaleY, -fFace->ascender, fFace->units_per_EM);
        map_y_to_pt(fMatrix22, a, ascent);
    }
    if (descent)
    {
        SkFixed d = SkMulDiv(fScaleY, -fFace->descender, fFace->units_per_EM);
        map_y_to_pt(fMatrix22, d, descent);
    }
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

SkScalerContext* SkFontHost::CreateScalerContext(const SkDescriptor* desc)
{
    return SkNEW_ARGS(SkScalerContext_FreeType, (desc));
}

