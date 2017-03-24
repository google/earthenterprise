#ifndef SkDevice_DEFINED
#define SkDevice_DEFINED

#include "SkRefCnt.h"
#include "SkBitmap.h"

class SkDevice : public SkRefCnt {
public:
    /** Construct a new device, extracting the width/height/config/isOpaque values from
        the bitmap. If transferPixelOwnership is true, and the bitmap claims to own its
        own pixels (getOwnsPixels() == true), then transfer this responsibility to the
        device, and call setOwnsPixels(false) on the bitmap.
        
        Subclasses may override the destructor, which is virtual, even though this class
        doesn't have one. SkRefCnt does.
    */
    SkDevice(const SkBitmap& bitmap, bool transferPixelOwnership);

    /** Return the width of the device (in pixels).
    */
    int width() const { return fBitmap.width(); }
    /** Return the height of the device (in pixels).
    */
    int height() const { return fBitmap.height(); }
    /** Return the bitmap config of the device's pixels
    */
    SkBitmap::Config config() const { return fBitmap.getConfig(); }
    /** Returns true if the device's bitmap's config treats every pixels as
        implicitly opaque.
    */
    bool isOpaque() const { return fBitmap.isOpaque(); }

    /** Return the bitmap associated with this device. Call this each time you need
        to access the bitmap, as it notifies the subclass to perform any flushing
        etc. before you examine the pixels.
        @return the device's bitmap
    */
    const SkBitmap& accessBitmap();

protected:
    /** Update as needed the pixel value in the bitmap, so that the caller can access
        the pixels directly. Note: only the pixels field should be altered. The config/width/height/rowbytes
        must remain unchanged.
    */
    virtual void onAccessBitmap(SkBitmap*);
    
private:
    SkBitmap fBitmap;
};

#endif
