/*
 * Copyright 2017 Google Inc.
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

#ifndef FUSION_KHRASTER_EXTRACT_H__
#define FUSION_KHRASTER_EXTRACT_H__

#include <khExtents.h>

// ****************************************************************************
// ***  Routine to extract a sub buffer from an interleaved buffer
// ***
// ***  Simple - no conversion, order changing, error handling, cropping, or
// ***  anything else. Just extract the requested pixels.
// ****************************************************************************
void ExtractInterleavedSubBuffer(char *destBuf,
                                 const khExtents<uint32> &targetPixelExtents,
                                 const char *srcBuf,
                                 const khSize<uint32> &srcRasterSize,
                                 const uint pixelByteSize);

// ****************************************************************************
// ***  Routine to extract a sub buffer from an interleaved buffer
// ***
// ***  Flip the Y axis
// ****************************************************************************
void ExtractInterleavedSubBufferFlipY
(char *destBuf,
 const khExtents<uint32> &targetPixelExtents,
 const char *srcBuf,
 const khSize<uint32> &srcRasterSize,
 const uint pixelByteSize);


#endif // FUSION_KHRASTER_EXTRACT_H__
