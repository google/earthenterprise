// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <string.h>
#include "Extract.h"


// ****************************************************************************
// ***  Routine to extract a sub buffer from an interleaved buffer
// ***
// ***  Simple - no conversion, order changing, error handling, cropping, or
// ***  anything else. Just extract the requested pixels.
// ****************************************************************************
void
ExtractInterleavedSubBuffer(char *destBuf,
                            const khExtents<std::uint32_t> &targetPixelExtents,
                            const char *srcBuf,
                            const khSize<std::uint32_t> &srcRasterSize,
                            const unsigned int pixelByteSize)
{
  std::uint32_t srcRowByteSize  = srcRasterSize.width * pixelByteSize;
  std::uint32_t destRowByteSize = targetPixelExtents.width() * pixelByteSize;

  std::uint32_t srcOffset = ((targetPixelExtents.beginRow() * srcRowByteSize) +
                      (targetPixelExtents.beginCol() * pixelByteSize));
  std::uint32_t destOffset = 0;

  for (std::uint32_t i = 0; i < targetPixelExtents.numRows(); ++i) {
    memcpy(&destBuf[destOffset], &srcBuf[srcOffset], destRowByteSize);
    destOffset += destRowByteSize;
    srcOffset  += srcRowByteSize;
  }
}


// ****************************************************************************
// ***  Routine to extract a sub buffer from an interleaved buffer
// ***
// ***  Flip the Y axis
// ****************************************************************************
void
ExtractInterleavedSubBufferFlipY(char *destBuf,
                                 const khExtents<std::uint32_t> &targetPixelExtents,
                                 const char *srcBuf,
                                 const khSize<std::uint32_t> &srcRasterSize,
                                 const unsigned int pixelByteSize)
{
  std::uint32_t srcRowByteSize  = srcRasterSize.width * pixelByteSize;
  std::uint32_t destRowByteSize = targetPixelExtents.width() * pixelByteSize;

  std::uint32_t srcOffset = ((targetPixelExtents.beginRow() * srcRowByteSize) +
                      (targetPixelExtents.beginCol() * pixelByteSize));
  std::uint32_t destOffset = (targetPixelExtents.numRows()-1) * destRowByteSize;

  for (std::uint32_t i = 0; i < targetPixelExtents.numRows(); ++i) {
    memcpy(&destBuf[destOffset], &srcBuf[srcOffset], destRowByteSize);
    destOffset -= destRowByteSize;
    srcOffset  += srcRowByteSize;
  }
}
