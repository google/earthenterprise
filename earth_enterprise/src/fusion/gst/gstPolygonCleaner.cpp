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

#include "fusion/gst/gstPolygonCleaner.h"

namespace fusion_gst {

// PolygonCleaner
PolygonCleaner::PolygonCleaner()
    : polygon_builder_(PolygonBuilderOptions::NO_CUT_HOLES_CLEAN()) {
}

PolygonCleaner::~PolygonCleaner() {
}

void PolygonCleaner::Run(gstGeodeHandle* const geodeh) {
  assert(geodeh);
  geode_creator_.SetPrimType((*geodeh)->PrimType());

  switch ((*geodeh)->PrimType()) {
    case gstUnknown:
    case gstPoint:
    case gstPoint25D:
      return;

    case gstPolyLine:
    case gstStreet:
    case gstPolyLine25D:
    case gstStreet25D:
      return;

    case gstPolygon:
    case gstPolygon25D:
      ProcessPolygon(geodeh);
      break;

    case gstPolygon3D:
      notify(NFY_WARN,
             "PolygonCleaner: not supported geometry type: %d",
             (*geodeh)->PrimType());
      return;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
      {
        gstGeodeCollection *multi_geode =
            static_cast<gstGeodeCollection*>(&(**geodeh));
        for (unsigned int p = 0; p < multi_geode->NumParts(); ++p) {
          gstGeodeHandle &cur_geodeh = multi_geode->GetGeode(p);
          ProcessPolygon(&cur_geodeh);
          if (cur_geodeh->IsEmpty()) {
            notify(NFY_DEBUG, "Multi-geode part was empty or invalid"
                   " (and forced empty): skipped.");
          }
        }
      }
      break;

    case gstMultiPolygon3D:
      notify(NFY_WARN,
             "PolygonCleaner: not supported geometry type: %d",
             (*geodeh)->PrimType());
      return;
  }

  // Reports result.
  (*geodeh)->Clear();
  geode_creator_.Report(geodeh);
  geode_creator_.Clean();
}

void PolygonCleaner::ProcessPolygon(gstGeodeHandle* const geodeh) {
  assert((*geodeh)->PrimType() == gstPolygon ||
         (*geodeh)->PrimType() == gstPolygon25D);

  if ((*geodeh)->IsDegenerate()) {
    (*geodeh)->Clear();
    return;
  }

  GeodeList pieces;
  const gstGeode *geode = static_cast<const gstGeode*>(&(**geodeh));
  polygon_builder_.AcceptPolygon(geode);
  polygon_builder_.SetPrimType(geode->PrimType());
  polygon_builder_.Run(&pieces);

  for (GeodeList::iterator it = pieces.begin(); it != pieces.end(); ++it) {
    geode_creator_.Accept(*it);
  }
}

}  // namespace fusion_gst
