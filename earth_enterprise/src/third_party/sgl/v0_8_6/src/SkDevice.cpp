#include "SkDevice.h"

SkDevice::SkDevice(const SkBitmap& bitmap, bool transferPixelOwnership)
    : fBitmap(bitmap)
{
    if (transferPixelOwnership && bitmap.getOwnsPixels())
    {
        fBitmap.setOwnsPixels(true);
        ((SkBitmap*)&bitmap)->setOwnsPixels(false);
    }
}

const SkBitmap& SkDevice::accessBitmap()
{
    this->onAccessBitmap(&fBitmap);
    return fBitmap;
}

void SkDevice::onAccessBitmap(SkBitmap* bitmap)
{
    // virtual, may be overridden by subclass
}

