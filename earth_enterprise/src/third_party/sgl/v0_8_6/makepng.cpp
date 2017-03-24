#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkGraphics.h"
#include "SkPaint.h"

void SkBitmap_writebmp(const SkBitmap& bm, const char name[]);

///////////////////////////////////////////////////////////////////////

#include "Sk1DPathEffect.h"

// use the 1DPathEffect to create the traffic effect
static void apply_pe(SkPaint* paint)
{
    static const int gXY[] = {
        4, 0, 0, -4, 8, -4, 12, 0, 8, 4, 0, 4
    };
    SkPath  path;
    path.moveTo(SkIntToScalar(gXY[0]), SkIntToScalar(gXY[1]));
    for (unsigned i = 2; i < SK_ARRAY_COUNT(gXY); i += 2)
        path.lineTo(SkIntToScalar(gXY[i]), SkIntToScalar(gXY[i+1]));
    path.close();
    path.offset(SkIntToScalar(-6), 0);

    SkPathEffect* outer = new SkPath1DPathEffect(path, SkIntToScalar(12), SkIntToScalar(6),
                                                SkPath1DPathEffect::kRotate_Style);
    paint->setPathEffect(outer)->unref();
}

///////////////////////////////////////////////////////////////////////

#include "SkGradientShader.h"

static void apply_shader(SkPaint* paint)
{
    SkPoint pts[] = { 0, 0, SkIntToScalar(40), SkIntToScalar(20) };
    SkColor colors[] = { SkColorSetARGB(0xFF, 0xFF, 0x88, 0x44), SK_ColorWHITE, SkColorSetARGB(0x80, 0, 0xFF, 0) };
    SkShader* shader = SkGradientShader::CreateLinear(pts, colors, NULL, SK_ARRAY_COUNT(colors), SkShader::kMirror_TileMode);

    paint->setShader(shader)->unref();
}

///////////////////////////////////////////////////////////////////////

#define WIDTH   320
#define HEIGHT  240

int main(int argc, char* argv[])
{
    //  the bool parameter runs unittests if it is true
    SkGraphics::Init(false);
    
    SkBitmap    bitmap;
    
    /*
        Setup a bitmap to describe the pixel memory we're going to draw into.
        Config describes the bits-per-pixel:
            kA8_Config          - 8 bits, just records the alpha layer
            kRGB_565_Config     - 16 bits, use SkColorPriv.h to access components
            kARGB_8888_Config   - 32 bits, use SkColorPriv.h to access components
        width, height are obvious
        rowbytes is optional (defaults to 0), the number of bytes between rows. If 0, then
            setConfig() will compute a reasonable value. The total number of bytes of pixels
            must be >= height * rowbytes
    */
    bitmap.setConfig(SkBitmap::kARGB_8888_Config, WIDTH, HEIGHT);
    // this allocates memory based on height and rowbytes. the bitmap will handle freeing this memory
    // when its destructor is called, or if setPixels() is called with another address.
    // If you already have memory to draw into, don't call this, but call setPixels() directly.
    bitmap.allocPixels();

    // This is optional. It is the fastest way to set all the pixels in the bitmap. It also makes it
    // easy to set the alpha to 0 for configs that support alpha (A8, ARGB_8888).
    bitmap.eraseColor(0);

    // Now create a canvas to host the drawing API. All drawings will be sent to the pixels pointed
    // to by the bitmap. The canvas makes a copy of the bitmap settings, but does not copy the actual
    // pixels, just the address.
    SkCanvas    canvas(bitmap);

    // Paints hold all the drawing attributes that affect the color or style of the drawing. You can
    // have as many of these as you wish, since each draw call takes a paint as one of the parameters.
    SkPaint     paint;

    SkPath      path;
    SkRect      r;
    
    paint.setAntiAliasOn(true);
    paint.setColor(SK_ColorBLUE);

    r.set(0, 0, SkIntToScalar(WIDTH), SkIntToScalar(HEIGHT));
    canvas.drawOval(r, paint);

    path.moveTo(0, 0);
    path.cubicTo(SkIntToScalar(WIDTH)/2, SkIntToScalar(HEIGHT),
                 SkIntToScalar(WIDTH)/2, 0,
                 SkIntToScalar(WIDTH),   SkIntToScalar(HEIGHT));

    paint.setARGB(0xC0, 0xFF, 0xFF, 0xFF);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(SkIntToScalar(10));
    canvas.drawPath(path, paint);

    paint.setColor(SK_ColorRED);
    apply_pe(&paint);
    canvas.drawPath(path, paint);

    // now setup the paint to draw text
    paint.setStyle(SkPaint::kFill_Style);
    paint.setTextSize(SkIntToScalar(30));
    paint.setTextAlign(SkPaint::kCenter_Align);
    paint.setColor(SK_ColorWHITE);
    apply_shader(&paint);
    canvas.drawText("Graphics", 8, SkIntToScalar(WIDTH/2), SkIntToScalar(50), paint);

    // We're done drawing, so write the pixels to a file so we can view them
    SkBitmap_writebmp(bitmap, "test.bmp");
    return 0;
}

