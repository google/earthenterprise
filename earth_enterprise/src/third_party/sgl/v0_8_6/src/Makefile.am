if EFFECTS
EFFECTS_SRCS := Sk1DPathEffect.cpp \
	Sk2DPathEffect.cpp \
	SkAvoidXfermode.cpp \
	SkBlurMask.cpp \
	SkBlurMaskFilter.cpp \
	SkCamera.cpp \
	SkColorFilters.cpp \
	SkCornerPathEffect.cpp \
	SkCullPoints.cpp \
	SkDashPathEffect.cpp \
	SkDiscretePathEffect.cpp \
	SkEmbossMask.cpp \
	SkEmbossMaskFilter.cpp \
	SkGradientShader.cpp \
	SkLayerRasterizer.cpp \
	SkShaderExtras.cpp \
	SkTransparentShader.cpp
else
EFFECTS_SRCS :=
endif

if IMAGES
IMAGES_SRCS := SkImageDecoder.cpp
else
IMAGES_SRCS :=
endif

if GIF
GIF_SRCS := SkImageDecoder_libgif.cpp
else
GIF_SRCS :=
endif

if JPEG
JPEG_SRCS := SkImageDecoder_libjpeg.cpp
else
JPEG_SRCS :=
endif

if PNG
PNG_SRCS := SkImageDecoder_libpng.cpp
else
PNG_SRCS :=
endif

if FREETYPE
FONT_SRCS := SkFontHost_FreeType.cpp \
	SkFontHost_FONTPATH.cpp
else
FONT_SRCS := SkFontHost_none.cpp
endif

lib_LIBRARIES = libsgl.a

libsgl_a_SOURCES = \
	Sk64.cpp \
	SkAlphaRuns.cpp \
	SkBitmap.cpp \
	SkBitmapRef.cpp \
	SkBitmapSampler.cpp \
	SkBitmapShader.cpp \
	SkBlitter.cpp \
	SkBlitter_A1.cpp \
	SkBlitter_A8.cpp \
	SkBlitter_ARGB32.cpp \
	SkBlitter_RGB16.cpp \
	SkBlitter_Sprite.cpp \
	SkBuffer.cpp \
	SkCanvas.cpp \
	SkChunkAlloc.cpp \
	SkColor.cpp \
	SkColorFilter.cpp \
	SkColorTable.cpp \
	SkCordic.cpp \
	SkDebug.cpp \
	SkDebug_stdio.cpp \
	SkDeque.cpp \
	SkDevice.cpp \
	SkDraw.cpp \
	SkEdge.cpp \
	SkFilterProc.cpp \
	SkFloat.cpp \
	SkGeometry.cpp \
	SkGlobals.cpp \
	SkGlobals_global.cpp \
	SkGlyphCache.cpp \
	SkGraphics.cpp \
	SkInterpolator.cpp \
	SkMask.cpp \
	SkMaskFilter.cpp \
	SkMath.cpp \
	SkMatrix.cpp \
	SkMemory_stdlib.cpp \
	SkOSFile_stdio.cpp \
	SkPaint.cpp \
	SkPath.cpp \
	SkPathEffect.cpp \
	SkPathMeasure.cpp \
	SkPoint.cpp \
	SkProcSpriteBlitter.cpp \
	SkRasterizer.cpp \
	SkRect.cpp \
	SkRefCnt.cpp \
	SkRegion.cpp \
	SkRegion_path.cpp \
	SkScalerContext.cpp \
	SkScan.cpp \
	SkScan_AntiPath.cpp \
	SkScan_Antihair.cpp \
	SkScan_Hairline.cpp \
	SkScan_Path.cpp \
	SkShader.cpp \
	SkSpriteBlitter_ARGB32.cpp \
	SkSpriteBlitter_RGB16.cpp \
	SkStream.cpp \
	SkString.cpp \
	SkStroke.cpp \
	SkStrokerPriv.cpp \
	SkThread_none.cpp \
	SkTSearch.cpp \
	SkUtils.cpp \
	SkXfermode.cpp \
	$(EFFECTS_SRCS) \
	$(IMAGES_SRCS) \
	$(GIF_SRCS) \
	$(PNG_SRCS) \
	$(JPEG_SRCS) \
	$(FONT_SRCS)

AM_CPPFLAGS = -I../include 
