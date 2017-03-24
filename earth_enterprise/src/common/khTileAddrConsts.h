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

//
// Constants specifying what gets generated at what levels of the quadtree.

#ifndef COMMON_KHTILEADDRCONSTS_H__
#define COMMON_KHTILEADDRCONSTS_H__

// ****************************************************************************
// ***  Min/Max level definitions
// ****************************************************************************
// Maximum level of any sort (limited in all kinds of places, for all sorts
// of reasons). TILEADDR above limits it to 31 by using 5bits to level & 24
// bits to store row & column for 256x256 quadnode tiles.
static const unsigned int MaxFusionLevel  = 31;
static const unsigned int NumFusionLevels = MaxFusionLevel+1;

// Maximum quad/node level supported by client (for it's own reasons)
static const unsigned int MaxClientLevel = 24;

// Maximum level allowed for 2D client (for no particular reason)
static const unsigned int Max2DClientLevel = 24;

// We don't show terrain until client/quad tmesh level 4.  Terrain on earth
// begins to get interesting at level 8, on the moon at about level 6.  Due to
// some settings in the client, level 4 is the lowest resolution terrain that
// ever gets used.  Generating to the root would require special-case code for
// the root quadsets and there's no point if it never gets used. This is
// completely independent of any tile/mesh sizes.
static const unsigned int StartTmeshLevel = 4;
static const unsigned int MaxTmeshLevel   = MaxClientLevel;

//   We show imagery through the whole range of client levels.
static const unsigned int StartImageryLevel = 0;
static const unsigned int MaxImageryLevel   = MaxClientLevel;

//   We show vector through the whole range of client levels.
static const unsigned int StartVectorLevel = 4;
static const unsigned int MaxVectorLevel   = MaxClientLevel;

// Recommended that roads are limited to level 18. Note that this
// is not a technical limitation so much as a stylistic recommendations.
static const unsigned int kMaxRecommendedRoadLevel = 18;

// Recommended that vectors are only built to level 16 due to the amount of
// disk and work involved for tiles beyond that level.
static const unsigned int kMaxRecommendedBuildLevel = 16;

// Maximum display margin in pixel
static const int MaxDisplayMarginPxl = 1000;
static const int MinDisplayMarginPxl = -1000;


#endif // COMMON_KHTILEADDRCONSTS_H__
