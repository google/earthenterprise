/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/******************************************************************************
File:        BlendTile.h
******************************************************************************/
#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_KHRASTER_BLENDTILE_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_KHRASTER_BLENDTILE_H_

#include <string.h>

#include "common/khCalcHelper.h"
#include "fusion/khraster/khRasterTile.h"
#include "fusion/khraster/ProcessNeighbors.h"


// ****************************************************************************
// ***  AlphaModulator
// ***
// ***  foreach band
// ***     dest[band][pos] += src[band][pos] * normAlpha
// ****************************************************************************
template <class T, unsigned int numcomp> class AlphaModulator { };
template <class T>
class AlphaModulator<T, 1> {
 public:
  inline static void Modulate(T *const dest[], const T *const src[],
                              std::uint32_t pos, double normAlpha) {
    dest[0][pos] += khCalcHelper<T>::Scale(src[0][pos], normAlpha);
  }
};
template <class T>
class AlphaModulator<T, 3> {
 public:
  inline static void Modulate(T *const dest[], const T *const src[],
                              std::uint32_t pos, double normAlpha) {
    dest[0][pos] += khCalcHelper<T>::Scale(src[0][pos], normAlpha);
    dest[1][pos] += khCalcHelper<T>::Scale(src[1][pos], normAlpha);
    dest[2][pos] += khCalcHelper<T>::Scale(src[2][pos], normAlpha);
  }
};

// ****************************************************************************
// ***  AltAlphaModulator
// ***  Like AlphaModulator, but src is just any array of pixel values, not an
// ***  entire tile
// ***
// ***  foreach band
// ***     dest[band][pos] += src[band] * normAlpha
// ****************************************************************************
template <class T, unsigned int numcomp> class AltAlphaModulator { };
template <class T>
class AltAlphaModulator<T, 1> {
 public:
  inline static void Modulate(T *const dest[], const T src[],
                              std::uint32_t pos, double normAlpha) {
    dest[0][pos] += khCalcHelper<T>::Scale(src[0], normAlpha);
  }
};
template <class T>
class AltAlphaModulator<T, 3> {
 public:
  inline static void Modulate(T *const dest[], const T src[],
                              std::uint32_t pos, double normAlpha) {
    dest[0][pos] += khCalcHelper<T>::Scale(src[0], normAlpha);
    dest[1][pos] += khCalcHelper<T>::Scale(src[1], normAlpha);
    dest[2][pos] += khCalcHelper<T>::Scale(src[2], normAlpha);
  }
};

// ****************************************************************************
// ***  AlphaApplier
// ***
// ***  foreach band
// ***     dest[band][pos] = src[band][pos] * normAlpha
// ****************************************************************************
template <class T, unsigned int numcomp> class AlphaApplier { };
template <class T>
class AlphaApplier<T, 1> {
 public:
  inline static void Apply(T *const dest[], const T *const src[],
                           std::uint32_t pos, double normAlpha) {
    dest[0][pos] = khCalcHelper<T>::Scale(src[0][pos], normAlpha);
  }
};
template <class T>
class AlphaApplier<T, 3> {
 public:
  inline static void Apply(T *const dest[], const T *const src[],
                           std::uint32_t pos, double normAlpha) {
    dest[0][pos] = khCalcHelper<T>::Scale(src[0][pos], normAlpha);
    dest[1][pos] = khCalcHelper<T>::Scale(src[1][pos], normAlpha);
    dest[2][pos] = khCalcHelper<T>::Scale(src[2][pos], normAlpha);
  }
};


// ****************************************************************************
// ***  AltAlphaApplier
// ***  Like AlphaApplier, but src is just any array of pixel values, not an
// ***  entire tile
// ***
// ***  foreach band
// ***     dest[band][pos] = src[band] * normAlpha
// ****************************************************************************
template <class T, unsigned int numcomp> class AltAlphaApplier { };
template <class T>
class AltAlphaApplier<T, 1> {
 public:
  inline static void Apply(T *const dest[], const T src[],
                           std::uint32_t pos, double normAlpha) {
    dest[0][pos] = khCalcHelper<T>::Scale(src[0], normAlpha);
  }
};
template <class T>
class AltAlphaApplier<T, 3> {
 public:
  inline static void Apply(T *const dest[], const T src[],
                           std::uint32_t pos, double normAlpha) {
    dest[0][pos] = khCalcHelper<T>::Scale(src[0], normAlpha);
    dest[1][pos] = khCalcHelper<T>::Scale(src[1], normAlpha);
    dest[2][pos] = khCalcHelper<T>::Scale(src[2], normAlpha);
  }
};


// ****************************************************************************
// ***  BleedingAlphaNeighborAverager - Helper class for ApplyAlpha
// ****************************************************************************
template <class TileType> class BleedingAlphaNeighborAverager {};

template <class TileType>
class BleedingAlphaBase_ {
 public:
  typedef khCalcHelper<typename TileType::PixelType> DataCalcHelper;
  typedef khCalcHelper<unsigned char> AlphaCalcHelper;

  const TileType &dataTile;
  const unsigned char *alphaBuf;
  unsigned int count;
  typename AlphaCalcHelper::AccumType alphaAccum;
  typename DataCalcHelper::AccumType  bandAccum[TileType::NumComp];

  unsigned char alphaAverage;
  typename TileType::PixelType bandAverage[TileType::NumComp];

  inline BleedingAlphaBase_(const TileType &dt, const unsigned char *ab) :
      dataTile(dt), alphaBuf(ab) { }
};
template <class T, unsigned int tilesize>
class BleedingAlphaNeighborAverager<khRasterTile<T, tilesize, tilesize, 1> >
    : public BleedingAlphaBase_<khRasterTile<T, tilesize, tilesize, 1> > {
 public:
  typedef khRasterTile<T, tilesize, tilesize, 1> TileType;
  typedef BleedingAlphaBase_<TileType> Base;


  inline BleedingAlphaNeighborAverager(const TileType &dt, const unsigned char *ab) :
      Base(dt, ab) {
  }
  inline void Reset(void) {
    this->bandAccum[0] = 0;
    this->alphaAccum = 0;
    this->count = 0;
  }
  inline void CalcAverage(void) {
    this->alphaAverage   =
      Base::AlphaCalcHelper::Average(this->alphaAccum,
                                     this->count);
    this->bandAverage[0] =
      Base::DataCalcHelper::Average(this->bandAccum[0],
                                    this->count);
  }
  inline void handle(std::uint32_t pos) {
    typedef typename Base::DataCalcHelper::AccumType  DataAccumType;
    typedef typename Base::AlphaCalcHelper::AccumType AlphaAccumType;
    if (this->alphaBuf[pos] != 0) {
      this->alphaAccum   += (AlphaAccumType)this->alphaBuf[pos];
      this->bandAccum[0] += (DataAccumType)this->dataTile.bufs[0][pos];
      ++this->count;
    }
  }
};
template <class T, unsigned int tilesize>
class BleedingAlphaNeighborAverager<khRasterTile<T, tilesize, tilesize, 3> >
    : public BleedingAlphaBase_<khRasterTile<T, tilesize, tilesize, 3> > {
 public:
  typedef khRasterTile<T, tilesize, tilesize, 3> TileType;
  typedef BleedingAlphaBase_<TileType> Base;

  inline BleedingAlphaNeighborAverager(const TileType &dt, const unsigned char *ab) :
      Base(dt, ab) {
  }
  inline void Reset(void) {
    this->bandAccum[0] = 0;
    this->bandAccum[1] = 0;
    this->bandAccum[2] = 0;
    this->alphaAccum = 0;
    this->count = 0;
  }
  inline void CalcAverage(void) {
    this->alphaAverage   = Base::AlphaCalcHelper::Average(this->alphaAccum,
                                                          this->count);
    this->bandAverage[0] =
      Base::DataCalcHelper::Average(this->bandAccum[0],
                                    this->count);
    this->bandAverage[1] =
      Base::DataCalcHelper::Average(this->bandAccum[1],
                                    this->count);
    this->bandAverage[2] =
      Base::DataCalcHelper::Average(this->bandAccum[2],
                                    this->count);
  }
  inline void handle(std::uint32_t pos) {
    typedef typename Base::DataCalcHelper::AccumType  DataAccumType;
    typedef typename Base::AlphaCalcHelper::AccumType AlphaAccumType;

    if (this->alphaBuf[pos] != 0) {
      this->alphaAccum   += (AlphaAccumType)this->alphaBuf[pos];
      this->bandAccum[0] += (DataAccumType)this->dataTile.bufs[0][pos];
      this->bandAccum[1] += (DataAccumType)this->dataTile.bufs[1][pos];
      this->bandAccum[2] += (DataAccumType)this->dataTile.bufs[2][pos];
      ++this->count;
    }
  }
};




// ****************************************************************************
// ***  Blend Tile routines
// ****************************************************************************

template <class TileType>
inline void
BlendTerminalTile(TileType *accumData,
                  unsigned char accumAlpha[],
                  const TileType &otherData) {
  // this is called only when we know the tile is opaque, so we always apply
  // the remainder of the alpha
  for (std::uint32_t i = 0; i < TileType::BandPixelCount; ++i) {
    double normBalanceAlpha = (255 - accumAlpha[i]) / 255.0;
    AlphaModulator<typename TileType::PixelType,
      TileType::NumComp>::Modulate(accumData->bufs,
                                   otherData.bufs,
                                   i, normBalanceAlpha);
  }

  // Saturate alpha.
  memset(accumAlpha, 255, TileType::BandPixelCount);
}

// #define CHIKAIS_BLEND

template <class TileType>
inline void
BlendTileAndSaturateAlpha(TileType *accumData,
                          unsigned char accumAlpha[],
                          const TileType &otherData,
                          const unsigned char otherAlpha[]) {
  BleedingAlphaNeighborAverager<TileType> averager(otherData, otherAlpha);

  for (std::uint32_t i = 0; i < TileType::BandPixelCount; ++i) {
    if (accumAlpha[i] == 255) {  // This pixel is opaque for anymore blending
      continue;
    }

    if (otherAlpha[i] == 0) {
      // Special Case
      // we need to bleed the edge of the blend out
      // by one pixel to compensate for the mask
      // erosion that we do during minification.
      // If we don't, overlapping insets can show a
      // gap between them at lower resolutions.
      averager.Reset();
      ProcessNeighbors<TileType::TileWidth,
        TileType::TileHeight>(i, averager);
      averager.CalcAverage();

#ifdef CHIKAIS_BLEND
      unsigned char effectiveAlpha = std::min((unsigned char)(255-accumAlpha[i]),
                                      averager.alphaAverage);
      accumAlpha[i] += effectiveAlpha;
      double normApplyAlpha = effectiveAlpha / 255.0;
#else
      // how much alpha is left over for us to use
      double normBalanceAlpha = (255 - accumAlpha[i]) / 255.0;

      // how much alpha we want of ourself
      double normOtherAlpha = averager.alphaAverage / 255.0;

      // how much alpha to actually apply to the pixels
      double normApplyAlpha = normOtherAlpha * normBalanceAlpha;

      // add the fraction that I just applied back into the accumAlpha
      double newAccumAlpha = 0.5 + (averager.alphaAverage * normBalanceAlpha) +
                             accumAlpha[i];
      accumAlpha[i] = (newAccumAlpha > 255) ? 255 : (unsigned char)newAccumAlpha;
#endif
      AltAlphaModulator<typename TileType::PixelType,
        TileType::NumComp>::Modulate(accumData->bufs,
                                     averager.bandAverage,
                                     i, normApplyAlpha);


    } else {
#ifdef CHIKAIS_BLEND
      unsigned char effectiveAlpha = std::min((unsigned char)(255-accumAlpha[i]),
                                      otherAlpha[i]);
      accumAlpha[i] += effectiveAlpha;
      double normApplyAlpha = effectiveAlpha / 255.0;
#else
      // how much alpha is left over for us to use
      double normBalanceAlpha = (255 - accumAlpha[i]) / 255.0;

      // how much alpha we want of ourself
      double normOtherAlpha = otherAlpha[i] / 255.0;

      // how much alpha to actually apply to the pixels
      double normApplyAlpha = normOtherAlpha * normBalanceAlpha;

      // add the fraction that I just applied back into the accumAlpha
      double newAccumAlpha = 0.5 + (otherAlpha[i] * normBalanceAlpha) +
                             accumAlpha[i];
      accumAlpha[i] = (newAccumAlpha > 255) ? 255 : (unsigned char)newAccumAlpha;
#endif
      AlphaModulator<typename TileType::PixelType,
        TileType::NumComp>::Modulate(accumData->bufs,
                                     otherData.bufs,
                                     i, normApplyAlpha);
    }
  }
}


// ****************************************************************************
// ***  ApplyAlpha implementation
// ****************************************************************************
template <class TileType>
inline void
ApplyAlpha(TileType *destTile,
           unsigned char destABuf[],
           const TileType& srcTile,
           const unsigned char srcABuf[]) {
  BleedingAlphaNeighborAverager<TileType> averager(srcTile, srcABuf);

  for (std::uint32_t i = 0; i < TileType::BandPixelCount; ++i) {
    if (srcABuf[i] == 0) {
      // Special Case
      // we need to bleed the edge of the blend out
      // by one pixel to compensate for the mask
      // erosion that we do during minification.
      // If we don't, overlapping insets can show a
      // gap between them at lower resolutions.
      averager.Reset();
      ProcessNeighbors<TileType::TileWidth,
        TileType::TileHeight>(i, averager);
      averager.CalcAverage();

      double normAlpha = averager.alphaAverage / 255.0;
      AltAlphaApplier<typename TileType::PixelType,
        TileType::NumComp>::Apply(destTile->bufs,
                                  averager.bandAverage,
                                  i, normAlpha);
      destABuf[i] = averager.alphaAverage;
    } else {
      double normAlpha = srcABuf[i] / 255.0;
      AlphaApplier<typename TileType::PixelType,
        TileType::NumComp>::Apply(destTile->bufs, srcTile.bufs,
                                  i, normAlpha);
      destABuf[i] = srcABuf[i];
    }
  }
}

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_KHRASTER_BLENDTILE_H_
