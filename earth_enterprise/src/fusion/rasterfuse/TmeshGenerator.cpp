// Copyright 2017 Google Inc.
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


#include "TmeshGenerator.h"
#include <notify.h>
#include "vipm/procmesh.h"
#include "vipm/mesh.h"
#include "vipm/compress.h"
#include <khConstants.h>

#define TORAD(a) ((a) * 3.14159265359/180.0)

// Meters to Earth's radius conversion factor
const double EarthPlanetaryConstant = 1.0/khEarthRadius;
// Meters to Marss radius conversion factor
const double MarsPlanetaryConstant = 2.94377391816E-7;


const double PlanetaryConstant = EarthPlanetaryConstant;

// Support negative elevations starting in the 5.0 client, but allow older
// clients to show negative elevations as essentially zero.  Divide negative
// elevations by a large negative factor so that old clients can handle them
// (essentially zero), but new clients will know to multiply very small
// elevation numbers by the factor to recover the original elevation.
// Note: the factor is negative since the client has a performance issue when
// taking even small negative elevations in.
const double NegativeElevationFactor = -pow(2, kNegativeElevationExponentBias);


#undef DEBUG_GENERATE_TRAVERSAL


// ****************************************************************************
// ***     _Slight_ refactoring of the old TmeshTileBuilder::GenerateMesh from
// ***  the terrainfuse directory.
// ***
// ***  Very little has been changed - yet. Mostly just access to the heightmap
// ***  pixels.
// ****************************************************************************
void
TmeshGenerator::Generate(float *srcSamples,
                         const khSize<uint32> &srcSamplesSize,
                         const khOffset<uint32> &wantOffset,
                         const khTileAddr &tmeshAddr,
                         etArray<uchar> &compressed,
                         const size_t reserve_size,
                         bool decimate,
                         double decimation_threshold
                        )
{
  // reset my temporaries - they live in the class so I don't have to
  // constantly reallocate them.
  points.reset();
  faces.reset();
  // mesh.reset();    // mesh gets reset in each iteration of loop below

  // A tmesh tile is actually composed by building meshes for each quad of
  // the wanted samples. All four meshes are then packed together and to
  // make one time. NOTE: This is separate from the fact that the server
  // sends clusters of tmesh tiles to the client.
  // These size variables represent the size of each quad.
  const uint SizeX = (sampleTilespace.tileSize  / 2 + 1);
  const uint SizeY = (sampleTilespace.tileSize / 2 + 1);
  elev.resize(SizeX * SizeY);

  // Error thresholds for decimation - set these to 0 to skip decimation
  // Question - Did these numbers come from empirical testing?
  // The allowable error gets smaller as the resolution increases.
  // The allowable error on border tiles is 1/10 that of interior
  // tiles. This is because we really want the border tiles to match. If not
  // we get rips between mesh tiles in the client.
  double threshold_modifier = pow(2, 11 - (tmeshAddr.level + 1));
  double error = decimation_threshold * threshold_modifier;
  double borderError = decimate ? error / 10.0 : 0.0;

  // calculate position of this quadnode WRT the larger heightmap tile.
  // xoff and yoff are in units of samples, and represent the offset
  // into the larger heightmap from the origin of this quadnode's region
  int32 xoff = wantOffset.x();
  int32 yoff = wantOffset.y();

#ifdef DEBUG_GENERATE_TRAVERSAL
  notify(NFY_DEBUG, "yoff = %u, xoff = %u", yoff, xoff);
#endif

  double deg_per_pixel = sampleTilespace.DegPixelSize(tmeshAddr.level);
  double stepx = deg_per_pixel;
  double stepy = deg_per_pixel;

  // level wide pixel offset to tile
  uint64 tilePixelOffsetX = uint64(tmeshAddr.col) *
                            uint64(sampleTilespace.tileSize);
  uint64 tilePixelOffsetY = uint64(tmeshAddr.row) *
                            uint64(sampleTilespace.tileSize);

  // Loop to traverse each quad.
  // ** WARNING **: This quad order is different from the quad order used
  // everywhere else in Fusion. But this is the order the client is used to.
  // Elsewhere:  2 3       Here: 3 2
  //             0 1             0 1
  // So here:
  //   (quad==0)   ==>   my = 0, mx = 0
  //   (quad==1)   ==>   my = 0, mx = 1
  //   (quad==2)   ==>   my = 1, mx = 1
  //   (quad==3)   ==>   my = 1, mx = 0
  for (uint quad = 0; quad < 4; ++quad) {
    uint my = quad / 2;
    uint mx = ((quad + my) % 2);

    uint quadOffX = mx * (SizeX-1);
    uint quadOffY = my * (SizeY-1);
    uint quadSampleX = xoff + quadOffX;
    uint quadSampleY = yoff + quadOffY;

#ifdef DEBUG_GENERATE_TRAVERSAL
    notify(NFY_DEBUG, "    quad (rc) %u,%u; sample (rc) %u,%u; pos = %u",
           my, mx, quadSampleY, quadSampleX,
           (quadSampleY * srcSamplesSize.width) + quadSampleX);
#endif

    // Mesh origin in degrees (-180 -> +180)
    double ox = ((tilePixelOffsetX + quadOffX) * deg_per_pixel - 180.0);
    double oy = ((tilePixelOffsetY + quadOffY) * deg_per_pixel - 180.0);

    // Creating the mesh
    mesh.reset();

    // ***** Generate the vertices *****
    int index = 0;
    for (uint verty = 0; verty < SizeY; ++verty) {
      for (uint vertx = 0; vertx < SizeX; ++vertx, ++index) {
        uint sampley = quadSampleY + verty;
        uint samplex = quadSampleX + vertx;
        uint samplePos = (sampley * srcSamplesSize.width) + samplex;
        float height = srcSamples[samplePos];

        // convert height in meters to planet radii
        // 0.0 is defined as sea level
        double tmp_z = height * PlanetaryConstant;

#ifdef DEBUG_GENERATE_TRAVERSAL
        fprintf(stderr, "%f ", height);
#endif


        // vertex coord in degrees (-180 -> +180)
        double x_deg = (tilePixelOffsetX + quadOffX + vertx)
                       * deg_per_pixel - 180.0;
        double y_deg = (tilePixelOffsetY + quadOffY + verty)
                       * deg_per_pixel - 180.0;

        // convert to x,y,z (1000 is defined as distance from center
        // of planet to "sea level")
        double radius = (1.0 + tmp_z) * 1000.0;
        double coslat = cos(TORAD(y_deg));
        double sinlat = sin(TORAD(y_deg));
        double coslon = cos(TORAD(x_deg));
        double sinlon = sin(TORAD(x_deg));
        double k = radius * coslat;
        double x = k *  coslon;
        double y = radius *  sinlat;
        double z = k *  -sinlon;

        // Add point to mesh.  These are used to generate and decimate the
        // tmesh, but they do not go in the packets: getTile() replaces them
        // with the elev[] points.
        mesh.addPoint(x, y, z);

        // Also add it to my list of xyz's for the packet.

        // See comment for NegativeElevationFactor above.
        if (tmp_z < 0.0)
          tmp_z /= NegativeElevationFactor;

        elev[index][0] = vertx;
        elev[index][1] = verty;
        elev[index][2] = tmp_z;
      }
#ifdef DEBUG_GENERATE_TRAVERSAL
      fprintf(stderr, "\n");
#endif
    }

    // ***** Generate the faces *****
    // Each iteration builds faces like this:
    //  p4 ----- p3
    //   | f2  /  |
    //   |   /    |
    //   | /   f1 |
    //  p1 ----- p2
    //
    // When the whole loop is finished the mesh will look like this:
    // +--+--+--+--+--+--+
    // | /| /| /| /| /| /|
    // |/ |/ |/ |/ |/ |/ |
    // +--+--+--+--+--+--+
    // | /| /| /| /| /| /|
    // |/ |/ |/ |/ |/ |/ |
    // +--+--+--+--+--+--+
    // | /| /| /| /| /| /|
    // |/ |/ |/ |/ |/ |/ |
    // +--+--+--+--+--+--+
    //
    // We traverse "one less" in each direction because each iteration
    // reaches "one more" up and right.
    for (uint row = 0; row < SizeY - 1; ++row) {
      for (uint col = 0; col < SizeX - 1; ++col) {
        int p1 = row * SizeX + col;
        int p2 = p1 + 1;
        int p4 = p1 + SizeX;
        int p3 = p4 + 1;
        mesh.addFace(p1, p2, p3); // f1
        mesh.addFace(p1, p3, p4); // f2
      }
    }

    // ***** Generate the neighbor list *****
    // TODO: Could be implicitly computed.
    mesh.findNeighbours();

    // mesh.saveSMF("mesh_ascii");

    // build quadric error decimator
    etQemMesh qmesh(mesh.getPoints(), mesh.nPoints(),
                    mesh.getFaces(),  mesh.nFaces(),
                    mesh.getNeighbours());
    qmesh.setSizeX(SizeX); // Specific for terrain processing
    qmesh.setSizeY(SizeY); // Specific for terrain processing
    qmesh.setTargetError(error);
    qmesh.setBorderTargetError(borderError);

    // ***** decimating the mesh *****
    if (decimate) {
      qmesh.decimateTile();
    }

    // Extract the points and faces from the decimated tile.  getTile grabs the
    // points from the third argument and assumes they are in the same order as
    // the original points.
    qmesh.getTile(points, faces, &elev[0]);

    // Compress the tmesh into the packet buffer.
    compress(tmeshAddr.level+1,
             ox/180.0, oy/180.0, stepx/180.0, stepy/180.0,
             points, faces, compressed, reserve_size);
  } // foreach quad
}
