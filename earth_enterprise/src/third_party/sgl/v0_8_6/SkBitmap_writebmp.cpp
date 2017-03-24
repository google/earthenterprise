#include "SkBitmap.h"
#include "SkColorPriv.h"
#include "SkEndian.h"

#define PIXELS_PER_METER    0

#define HEADERSIZES    (14 + 40)

static void w8(FILE* f, uint8_t x) { fwrite(&x, 1, 1, f); }
static void w16(FILE* f, uint16_t x) { x = SkEndian_SwapLE16(x); fwrite(&x, 2, 1, f); }
static void w32(FILE* f, uint32_t x) { x = SkEndian_SwapLE32(x); fwrite(&x, 4, 1, f); }

void SkBitmap_writebmp(const SkBitmap& bm, const char name[]);
void SkBitmap_writebmp(const SkBitmap& bm, const char name[])
{
    FILE* f = fopen(name, "w");

    w8(f, 'B');
    w8(f, 'M');
    w32(f, HEADERSIZES + bm.width() * bm.height() * 3);
    w16(f, 0);  // reserved1
    w16(f, 0);  // reserved2
    w32(f, HEADERSIZES);
    
    w32(f, 40);
    w32(f, bm.width());
    w32(f, bm.height());
    w16(f, 0);
    w16(f, 24);
    w32(f, 0);  // compression
    w32(f, bm.width() * bm.height() * 3);
    w32(f, PIXELS_PER_METER);
    w32(f, PIXELS_PER_METER);
    w32(f, 0);  // # colors
    w32(f, 0);
    
    // bmp's scanlines are bottom-to-top, and we are top-to-bottom, so write
    // them out in reverse order.
    for (unsigned y = bm.height(); y > 0; --y) {
        const SkPMColor* src = bm.getAddr32(0, y-1);
        for (unsigned x = 0; x < bm.width(); x++) {
            SkPMColor c = src[x];
            
            w8(f, SkGetPackedB32(c));
            w8(f, SkGetPackedG32(c));
            w8(f, SkGetPackedR32(c));
        }
    }
    
    fclose(f);
}

