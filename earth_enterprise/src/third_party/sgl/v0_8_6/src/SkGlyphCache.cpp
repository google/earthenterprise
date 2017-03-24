/* libs/graphics/sgl/SkGlyphCache.cpp
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

#include "SkGlyphCache.h"
#include "SkPaint.h"
#include "SkTemplates.h"

// this guy should be in SkFontHost, so the porting layer can specify it
#define FONT_CACHE_MEMORY_BUDGET    (256 * 1024)

//////////////////////////////////////////////////////////////////////

#define kMinGlphAlloc       (sizeof(SkGlyph) * 64)
#define kMinImageAlloc      (24 * 64)   // this guy should be pointsize-dependent IMHO

SkGlyphCache::SkGlyphCache(const SkDescriptor* desc)
    : fGlyphAlloc(kMinGlphAlloc), fImageAlloc(kMinImageAlloc)
{
    fNext = NULL;

    fDesc = desc->copy();
    fScalerContext = SkScalerContext::Create(desc);
    fScalerContext->getLineHeight(&fAbove, &fBelow);

    // init to 0 so that all of the pointers will be null
    memset(fGlyphHash, 0, sizeof(fGlyphHash));
    // init with 0xFF so that the charCode field will be -1, which is invalid
    memset(fCharToGlyphHash, 0xFF, sizeof(fCharToGlyphHash));
    
    fMemoryUsed = sizeof(*this) + kMinGlphAlloc + kMinImageAlloc;
}

SkGlyphCache::~SkGlyphCache()
{
    SkGlyph**   gptr = fGlyphArray.begin();
    SkGlyph**   stop = fGlyphArray.end();
    while (gptr < stop)
    {
        SkPath* path = (*gptr)->fPath;
        if (path)
            SkDELETE(path);
        gptr += 1;
    }
    SkDescriptor::Free(fDesc);
    SkDELETE(fScalerContext);
}

const SkGlyph& SkGlyphCache::getUnicharMetrics(SkUnichar charCode)
{
    CharGlyphRec* rec = &fCharToGlyphHash[charCode & kHashMask];

    if (rec->fCharCode != charCode)
    {
        rec->fCharCode  = charCode;
        rec->fGlyph     = this->lookupMetrics(fScalerContext->charToGlyphID(charCode));
    }
    return *rec->fGlyph;
}

const SkGlyph& SkGlyphCache::getGlyphIDMetrics(uint16_t glyphID)
{
    SkGlyph* glyph = fGlyphHash[glyphID & kHashMask];

    if (NULL == glyph || glyph->f_GlyphID != glyphID)
    {
        glyph = this->lookupMetrics(glyphID);
        fGlyphHash[glyphID & kHashMask] = glyph;
    }
    return *glyph;
}

SkGlyph* SkGlyphCache::lookupMetrics(uint16_t glyphID)
{
    SkGlyph* glyph;

    int     hi = 0;
    int     count = fGlyphArray.count();

    if (count)
    {
        SkGlyph**   gptr = fGlyphArray.begin();
        int     lo = 0;

        hi = count - 1;
        while (lo < hi)
        {
            int mid = (hi + lo) >> 1;
            if (gptr[mid]->f_GlyphID < glyphID)
                lo = mid + 1;
            else
                hi = mid;
        }
        glyph = gptr[hi];
        if (glyph->f_GlyphID == glyphID)
            return glyph;

        // check if we need to bump hi before falling though to the allocator
        if (glyph->f_GlyphID < glyphID)
            hi += 1;
    }

    // not found, but hi tells us where to inser the new glyph
    fMemoryUsed += sizeof(SkGlyph);

    glyph = (SkGlyph*)fGlyphAlloc.alloc(sizeof(SkGlyph), SkChunkAlloc::kThrow_AllocFailType);
    glyph->f_GlyphID = glyphID;
    glyph->fImage = NULL;
    glyph->fPath = NULL;
    fScalerContext->getMetrics(glyph);
    *fGlyphArray.insert(hi) = glyph;
    return glyph;
}

const void* SkGlyphCache::findImage(const SkGlyph& glyph)
{
    if (glyph.fWidth)
    {
        if (glyph.fImage == NULL)
        {
            size_t  size = glyph.computeImageSize();
            const_cast<SkGlyph&>(glyph).fImage = fImageAlloc.alloc(size, SkChunkAlloc::kReturnNil_AllocFailType);
            fScalerContext->getImage(glyph);
            fMemoryUsed += size;
        }
    }
    return glyph.fImage;
}

const SkPath* SkGlyphCache::findPath(const SkGlyph& glyph)
{
    if (glyph.fWidth)
    {
        if (glyph.fPath == NULL)
        {
            const_cast<SkGlyph&>(glyph).fPath = SkNEW(SkPath);
            fScalerContext->getPath(glyph, glyph.fPath);
            fMemoryUsed += sizeof(SkPath) + glyph.fPath->getPoints(NULL, 0x7FFFFFFF) * sizeof(SkPoint);
        }
    }
    return glyph.fPath;
}

void SkGlyphCache::getLineHeight(SkPoint* above, SkPoint* below)
{
    if (above)
        *above = fAbove;
    if (below)
        *below = fBelow;
}

/////////////////////////////////////////////////////////////////

SkGlyphCache* SkGlyphCache::DetachCache(const SkPaint& paint, const SkMatrix* matrix)
{
    return paint.detachCache(matrix);
}

#include "SkGlobals.h"
#include "SkThread.h"

#define SkGlyphCache_GlobalsTag     SkSetFourByteTag('g', 'l', 'f', 'c')

class SkGlyphCache_Globals : public SkGlobals::Rec {
public:
    SkMutex         fMutex;
    SkGlyphCache*   fHead;
    size_t          fTotalMemoryUsed;
};

#ifdef SK_USE_RUNTIME_GLOBALS
    static SkGlobals::Rec* create_globals()
    {
        SkGlyphCache_Globals* rec = SkNEW(SkGlyphCache_Globals);
        rec->fHead = NULL;
        rec->fTotalMemoryUsed = 0;
        return rec;
    }

    #define FIND_GC_GLOBALS()   *(SkGlyphCache_Globals*)SkGlobals::Find(SkGlyphCache_GlobalsTag, create_globals)
    #define GET_GC_GLOBALS()    *(SkGlyphCache_Globals*)SkGlobals::Get(SkGlyphCache_GlobalsTag)
#else
    static SkGlyphCache_Globals gGCGlobals;
    #define FIND_GC_GLOBALS()   gGCGlobals
    #define GET_GC_GLOBALS()    gGCGlobals
#endif

SkGlyphCache* SkGlyphCache::DetachCache(const SkDescriptor* desc)
{
    SkASSERT(desc);

    SkGlyphCache_Globals& globals = FIND_GC_GLOBALS();

    globals.fMutex.acquire();
    SkGlyphCache*   cache = globals.fHead;
    SkGlyphCache*   prev = NULL;

    while (cache)
    {
        SkGlyphCache* next = cache->fNext;

        if (*cache->fDesc == *desc)
        {
            if (prev)
                prev->fNext = next;
            else
                globals.fHead = next;
            cache->fNext = NULL;

            SkASSERT(globals.fTotalMemoryUsed >= cache->fMemoryUsed);
            globals.fTotalMemoryUsed -= cache->fMemoryUsed;
            break;
        }
        prev = cache;
        cache = next;
    }
    globals.fMutex.release();

    if (cache == NULL)
        cache = SkNEW_ARGS(SkGlyphCache, (desc));
    return cache;
}

void SkGlyphCache::AttachCache(SkGlyphCache* cache)
{
    SkASSERT(cache);
    SkASSERT(cache->fNext == NULL);

    SkGlyphCache_Globals& globals = GET_GC_GLOBALS();

    globals.fMutex.acquire();

    // if we have a fixed budget for our cache, do a purge here
    {
        size_t size = globals.fTotalMemoryUsed + cache->fMemoryUsed;
        if (size > FONT_CACHE_MEMORY_BUDGET)
            (void)InternalFreeCache(&globals, size - FONT_CACHE_MEMORY_BUDGET);
    }

    cache->fNext = globals.fHead;
    globals.fHead = cache;

    globals.fTotalMemoryUsed += cache->fMemoryUsed;

    globals.fMutex.release();
}

bool SkGlyphCache::FreeCache(size_t bytesNeeded)
{
    SkGlyphCache_Globals& globals = FIND_GC_GLOBALS();
    SkAutoMutexAcquire  ac(globals.fMutex);

   return InternalFreeCache(&globals, bytesNeeded) > 0;
}

////////////////////////////////////////////////////////////////////////////////////////////

SkGlyphCache* SkGlyphCache::ReverseLinkList(SkGlyphCache* head)
{
    if (head == NULL) return head;

    SkGlyphCache* prev = NULL;
    do {
        SkGlyphCache* next = head->fNext;
        head->fNext = prev;
        prev = head;
        head = next;
    } while (head != NULL);

    return prev;
}

#ifdef SK_DEBUG
size_t SkGlyphCache::ComputeMemoryUsed(const SkGlyphCache* head)
{
    size_t size = 0;
    
    while (head != NULL)
    {
        size += head->fMemoryUsed;
        head = head->fNext;
    }
    return size;
}
#endif

size_t SkGlyphCache::InternalFreeCache(SkGlyphCache_Globals* globals, size_t bytesNeeded)
{
    SkGlyphCache*   cache = globals->fHead;
    size_t          bytesFreed = 0;
    int             count = 0;

    // don't do any "small" purges
    size_t minToPurge = globals->fTotalMemoryUsed >> 2;
    if (bytesNeeded < minToPurge)
        bytesNeeded = minToPurge;

    cache = ReverseLinkList(cache);
    while (cache != NULL && bytesFreed < bytesNeeded)
    {
        SkGlyphCache* next = cache->fNext;
        bytesFreed += cache->fMemoryUsed;
        SkDELETE(cache);
        cache = next;
        count += 1;
    }
    cache = ReverseLinkList(cache);

    SkASSERT(bytesFreed <= globals->fTotalMemoryUsed);
    globals->fTotalMemoryUsed -= bytesFreed;
    SkASSERT(globals->fTotalMemoryUsed == ComputeMemoryUsed(cache));

if (count) printf("purging %dK from font cache [%d entries]\n", (int)(bytesFreed >> 10), count);

    globals->fHead = cache;
    return bytesFreed;
}

