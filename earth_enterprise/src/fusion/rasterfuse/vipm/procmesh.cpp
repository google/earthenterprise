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

#include "vector.h"
#include "mesh.h"
#include "quadric.h"
#include "procmesh.h"
#include "heap.h"
#include "array.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////
// Function used to reset the remaining valid vertices for a second decimation
// loop.
void
etQemPoint::cleanUp()
{
  resetCandidate();
  quadric.reset();
  resetHepeable();
}


//////////////////////////////////////////////////////////////////////////////
// This function estimates if a candidate (pt) could be the collapse destination
// of the current point.
// To do so, the candidate must have a good enough error and the resulting
// triangles must be good looking.
//////////////////////////////////////////////////////////////////////////////
void
etQemPoint::possibleCandidate2D(etQemInfo *ctx, etQemPoint *can)
{
  assert(can);

  // The vertex cannot be merged if:
  // - it does not have any candidate (but it should always at this point!).
  // - its priority is greater than the candidate's
  // - it is constrained.
  // - it's a border vertex and the candidate is not.
  if (can->getLevel() < getLevel()) return;
  if (!can->isBorder() && isBorder()) return;
  if (isConstrained()) return;

  // Compute the triangle quality ratio.
  //    double quality = getQualityRatio2D(can);
  double quality = getQualityRatio2D(can);

  // If the triangle's quality ratio is really too bad, I discard
  // this edge as a potential collapse.
  if(quality < ctx->triangleQuality) return;

  // Sum the quadrics.
  // Note: The other option would be to test the candidate against the
  // vertex quadric, but the results are not as good.
  etQuadric<double,4> Q(quadric);
  Q += can->getQuadric();

  // Compute the QEM.
  double error = Q.evaluate(can->getVector());

  // The error will be 0 if it's a totally flat area, so I add
  // 1E-6 to have an error even for flat area. This will garantee
  // to tesselate surfaces with a minimum amount of points.
  // Also the minError factor MUST BE different than 0 because the
  // quality factor will not be taken into acct for the approximation
  // and the application will crash.
  double res = -(error + ctx->minError) / quality * weight;

  if(!candidate.point || candidate.error < res)
  {
    candidate.point = can;
    candidate.error = res;
    deviation       = error;
  }
}


void
etQemPoint::possibleCandidate3D(etQemInfo *ctx, etQemPoint *can)
{
  assert(can);

  // The vertex cannot be merged if:
  // - it does not have any candidate (but it should always at this point!).
  // - its priority is greater than the candidate's
  // - it is constrained.
  // - it's a border vertex and the candidate is not.
  if (can->getLevel() < getLevel()) return;
  if (!can->isBorder() && isBorder()) return;
  if (isConstrained()) return;


  // Compute the triangle quality ratio.
  double quality = getQualityRatio3D(can);

  // If the triangle's quality ratio is really too bad, I discard
  // this edge as a potential collapse.
  if(quality < ctx->triangleQuality) return;

  // Don't take the quality ratio into account for the qem
  //    if(!ctx->weightQuality) quality = 1.0;

  // Sum the quadrics.
  etQuadric<double,4> Q(quadric);
  Q += can->getQuadric();

  // Compute the QEM.
  double error = Q.evaluate(can->getVector());

#ifdef WASTE_TIME
  can->getQuadric().evaluate(getVector());
#endif

  // The error will be 0 if it's a totally flat area, so I add
  // 1E-6 to have an error even for flat area. This will garantee
  // to tesselate surfaces with a minimum amount of points.
  // Also the minError factor MUST BE different than 0 because the
  // quality factor will not be taken into acct for the approximation
  // and the application will crash.
  double res = -(error + ctx->minError) / quality * weight;

  if(!candidate.point || candidate.error < res)
  {
    candidate.point = can;
    candidate.error = res;
    deviation       = error;
  }
}


//////////////////////////////////////////////////////////////////////////////
// This function collects the triangle quality ratio for a given edge collapse.
// The formulae is a ratio between the length of the sides of each triangle
// divided by their area. It is equal to 1 for equilateral triangles and less
// otherwise.
double
etQemPoint::getQualityRatio2D(etQemPoint *can)
{
  double quality = 1.0;
  {
    int i;
    for(i=0;i<poly.length;++i)
    {
      etQemTriangle *p = poly[i];

      // Discard invalid triangles if any
      if ( !p->isValid() )
      {
        poly.remove(i--);
        continue;
      }

      etQemPoint *p1 = p->verts[0];
      etQemPoint *p2 = p->verts[1];
      etQemPoint *p3 = p->verts[2];

      if(p1 == this) p1 = can;
      if(p2 == this) p2 = can;
      if(p3 == this) p3 = can;
      if (p1 == p2) continue;
      if (p1 == p3) continue;
      if (p2 == p3) continue;

      double pp1[2];
      double pp2[2];
      double pp3[2];
      p1->getxydoubles(pp1);
      p2->getxydoubles(pp2);
      p3->getxydoubles(pp3);

      // Compute the edges
      double u[2];
      double v[2];
      double w[2];
      u[0] = pp2[0] - pp1[0];
      u[1] = pp2[1] - pp1[1];
      v[0] = pp3[0] - pp1[0];
      v[1] = pp3[1] - pp1[1];
      w[0] = pp3[0] - pp2[0];
      w[1] = pp3[1] - pp2[1];

      // Test if the collapse will create folds.
      double a = 0.5 * (u[0]*v[1]-v[0]*u[1]);
      if(a <= 0.0) return 0.0;

      double len = ((u[0]*u[0]+u[1]*u[1]) +
                    (v[0]*v[0]+v[1]*v[1]) +
                    (w[0]*w[0]+w[1]*w[1]));

      double q = (4 * 1.73205) * a / len;
      if(q < quality) quality = q;
    }
  }

  return quality;
}


double etQemPoint::getQualityRatio3D(etQemPoint *can)
{
  double quality = 1.0;
  {
    etVec3d nRef;

    int i;
    for(i=0;i<poly.length;++i)
    {
      etQemTriangle *p = poly[i];

      // Discard invalid triangles if any
      if( !p->isValid() )
      {
        poly.remove(i--);
        continue;
      }

      etQemPoint *p1 = p->verts[0];
      etQemPoint *p2 = p->verts[1];
      etQemPoint *p3 = p->verts[2];
      if(p1 == this) p1 = can;
      if(p2 == this) p2 = can;
      if(p3 == this) p3 = can;

      if(p1 == p2 || p1 == p3 || p2 == p3) continue;

      etVec3d u, v, w, n;
      u[0] = (*p2)[0]; u[1] = (*p2)[1]; u[2] = (*p2)[2];
      u[0]-= (*p1)[0]; u[1]-= (*p1)[1]; u[2]-= (*p1)[2];

      v[0] = (*p3)[0]; v[1] = (*p3)[1]; v[2] = (*p3)[2];
      v[0]-= (*p1)[0]; v[1]-= (*p1)[1]; v[2]-= (*p1)[2];

      w[0] = (*p3)[0]; w[1] = (*p3)[1]; w[2] = (*p3)[2];
      w[0]-= (*p2)[0]; w[1]-= (*p2)[1]; w[2]-= (*p2)[2];

      n.cross(u, v);

      // Test if the collapse will create folds.
      if(i && nRef.dot(n) <= 0.0) return 0.0;
      else nRef = n;

      double a = 0.5 * n.length();
      if(!a)
      {
        quality = 0;
        break;
      }

      double len = u.length2() + v.length2() + w.length2();
      double q = (4 * 1.73205) * a / len;
      if(q < quality) quality = q;
    }
  }

  return quality;
}


//////////////////////////////////////////////////////////////////////////////
bool etQemPoint::canBeMerged()
{
  // This assert is likely to occur is somehow the quality factor
  // was not taken into account and the tesselation has sliver triangles.
  assert(isValid());

  // Cannot merge if:
  // - There is no candidate
  // - The vertex to be collapsed is constrained!
  if(!getPoint() || isConstrained()) return false;

  return true;
}


//////////////////////////////////////////////////////////////////////////////
int etQemPoint::mergeBorder(etArray<etQemPoint*> &ppt)
{
  // Then replace the point by the candidate.
  etQemPoint *can = candidate.point;
  assert(can);

  // Reset the list of surrounding vertices.
  ppt.reset();

  // Collapsing the surrounding triangles connected to the edge.
  int i;
  for(i=0;i<poly.length;++i) {

    etQemTriangle *p = poly[i];

    // Discard invalid triangles if any
    if( !p->isValid() )
    {
      poly.remove(i--);
      continue;
    }

    etQemPoint    *&p1 = p->verts[0];
    etQemPoint    *&p2 = p->verts[1];
    etQemPoint    *&p3 = p->verts[2];
    etQemTriangle *t1  = p->neighbours[0];
    etQemTriangle *t2  = p->neighbours[1];
    etQemTriangle *t3  = p->neighbours[2];

    if(p1 == this)
    {
      if(!t3) ppt.add(&p3);
      if(!t1) ppt.add(&p2);
      p1 = can;
    }

    if(p2 == this)
    {
      if(!t2) ppt.add(&p3);
      if(!t1) ppt.add(&p1);
      p2 = can;
    }

    if(p3 == this)
    {
      if(!t3) ppt.add(&p1);
      if(!t2) ppt.add(&p2);
      p3 = can;
    }
  }

  // Get rid of the 2 collapsed triangles from the collapsed point list.
  // At the same time, I update the connectivity
  // Note: This scheme does not work for non manifold meshes.
  int merged = 0;
  for(i=0;i<poly.length;++i) {

    etQemTriangle *t = poly[i];

    // Discard invalid triangles if any
    if( !t->isValid() )
    {
      poly.remove(i--);
      continue;
    }

    etQemPoint    *p1 = t->verts[0];
    etQemPoint    *p2 = t->verts[1];
    etQemPoint    *p3 = t->verts[2];
    etQemTriangle *t1 = t->neighbours[0];
    etQemTriangle *t2 = t->neighbours[1];
    etQemTriangle *t3 = t->neighbours[2];

    if(p1 == can && p2 == can)
    {
      if(t3) {
        if(t3->neighbours[0] == t) t3->neighbours[0] = t2;
        if(t3->neighbours[1] == t) t3->neighbours[1] = t2;
        if(t3->neighbours[2] == t) t3->neighbours[2] = t2;
      }

      if(t2) {
        if(t2->neighbours[0] == t) t2->neighbours[0] = t3;
        if(t2->neighbours[1] == t) t2->neighbours[1] = t3;
        if(t2->neighbours[2] == t) t2->neighbours[2] = t3;
      }

      t->setCollapsed();
      ++merged;

      continue;
    }

    if(p2 == can && p3 == can)
    {
      if(t1) {
        if(t1->neighbours[0] == t) t1->neighbours[0] = t3;
        if(t1->neighbours[1] == t) t1->neighbours[1] = t3;
        if(t1->neighbours[2] == t) t1->neighbours[2] = t3;
      }

      if(t3) {
        if(t3->neighbours[0] == t) t3->neighbours[0] = t1;
        if(t3->neighbours[1] == t) t3->neighbours[1] = t1;
        if(t3->neighbours[2] == t) t3->neighbours[2] = t1;
      }

      t->setCollapsed();
      ++merged;

      continue;
    }

    if(p3 == can && p1 == can)
    {

      if(t2) {
        if(t2->neighbours[0] == t) t2->neighbours[0] = t1;
        if(t2->neighbours[1] == t) t2->neighbours[1] = t1;
        if(t2->neighbours[2] == t) t2->neighbours[2] = t1;
      }

      if(t1) {
        if(t1->neighbours[0] == t) t1->neighbours[0] = t2;
        if(t1->neighbours[1] == t) t1->neighbours[1] = t2;
        if(t1->neighbours[2] == t) t1->neighbours[2] = t2;
      }

      t->setCollapsed();
      ++merged;

      continue;
    }
  }

  // Concatenate both lists together
  can->poly.merge(poly);

  // Identify this vertex as processed.
  setCollapsed();

  return merged;
}


//////////////////////////////////////////////////////////////////////////////
int etQemPoint::merge(etArray<etQemPoint*> &ppt)
{
  assert(!isConstrained());
  assert(isValid());

  // Then replace the point by the candidate.
  etQemPoint *can = candidate.point;
  assert(can);

  // Reset the list of surrounding vertices.
  ppt.reset();

  // Collapsing the surrounding triangles connected to the edge.
  int i;
  for(i=0;i<poly.length;++i) {

    etQemTriangle *p = poly[i];

    // Discard invalid triangles if any
    if( !p->isValid() )
    {
      poly.remove(i--);
      continue;
    }

    etQemPoint    *&p1 = p->verts[0];
    etQemPoint    *&p2 = p->verts[1];
    etQemPoint    *&p3 = p->verts[2];
    etQemTriangle *t1  = p->neighbours[0];
    etQemTriangle *t2  = p->neighbours[1];
    etQemTriangle *t3  = p->neighbours[2];

    if(p1 == this)
    {
      if(!t3) ppt.add(&p3);
      ppt.add(&p2);
      p1 = can;
    }

    if(p2 == this)
    {
      if(!t1) ppt.add(&p1);
      ppt.add(&p3);
      p2 = can;
    }

    if(p3 == this)
    {
      if(!t2) ppt.add(&p2);
      ppt.add(&p1);
      p3 = can;
    }
  }

  // Get rid of the 2 collapsed triangles from the collapsed point list.
  // At the same time, I update the connectivity
  // Note: This scheme does not work for non manifold meshes.
  int merged = 0;
  for(i=0;i<poly.length;++i) {

    etQemTriangle *t = poly[i];

    // Discard invalid triangles if any
    if( !t->isValid() )
    {
      poly.remove(i--);
      continue;
    }

    etQemPoint    *p1 = t->verts[0];
    etQemPoint    *p2 = t->verts[1];
    etQemPoint    *p3 = t->verts[2];
    etQemTriangle *t1 = t->neighbours[0];
    etQemTriangle *t2 = t->neighbours[1];
    etQemTriangle *t3 = t->neighbours[2];

    if(p1 == can && p2 == can)
    {
      if(t3) {
        if(t3->neighbours[0] == t) t3->neighbours[0] = t2;
        if(t3->neighbours[1] == t) t3->neighbours[1] = t2;
        if(t3->neighbours[2] == t) t3->neighbours[2] = t2;
      }

      if(t2) {
        if(t2->neighbours[0] == t) t2->neighbours[0] = t3;
        if(t2->neighbours[1] == t) t2->neighbours[1] = t3;
        if(t2->neighbours[2] == t) t2->neighbours[2] = t3;
      }

      t->setCollapsed();
      ++merged;

      continue;
    }

    if(p2 == can && p3 == can)
    {
      if(t1) {
        if(t1->neighbours[0] == t) t1->neighbours[0] = t3;
        if(t1->neighbours[1] == t) t1->neighbours[1] = t3;
        if(t1->neighbours[2] == t) t1->neighbours[2] = t3;
      }

      if(t3) {
        if(t3->neighbours[0] == t) t3->neighbours[0] = t1;
        if(t3->neighbours[1] == t) t3->neighbours[1] = t1;
        if(t3->neighbours[2] == t) t3->neighbours[2] = t1;
      }

      t->setCollapsed();
      ++merged;

      continue;
    }

    if(p3 == can && p1 == can)
    {

      if(t2) {
        if(t2->neighbours[0] == t) t2->neighbours[0] = t1;
        if(t2->neighbours[1] == t) t2->neighbours[1] = t1;
        if(t2->neighbours[2] == t) t2->neighbours[2] = t1;
      }

      if(t1) {
        if(t1->neighbours[0] == t) t1->neighbours[0] = t2;
        if(t1->neighbours[1] == t) t1->neighbours[1] = t2;
        if(t1->neighbours[2] == t) t1->neighbours[2] = t2;
      }

      t->setCollapsed();
      merged++;

      continue;
    }
  }

  // Concatenate both lists together
  can->poly.merge(poly);

  // Add the points's quadric to the candidate.
  if(isBorder() == can->isBorder())
  {
    // Average the quadrics
    can->addQuadric(quadric);
    can->getQuadric() *= 0.5;
  }

  // Identify this vertex as processed.
  setCollapsed();

  return merged;
}


//////////////////////////////////////////////////////////////////////////////
void etQemPoint::updateBorderError()
{
  // Find the two other points to compute the error.
  etArray<etQemPoint*> ppt;

  int i;
  for(i=0;i<poly.length;++i)
  {
    etQemTriangle *p = poly[i];

    if( !p->isValid() )
      continue;

    etQemPoint    *p1 = p->verts[0];
    etQemPoint    *p2 = p->verts[1];
    etQemPoint    *p3 = p->verts[2];
    etQemTriangle *t1 = p->neighbours[0];
    etQemTriangle *t2 = p->neighbours[1];
    etQemTriangle *t3 = p->neighbours[2];

    if(t1 == 0)
    {
      if(p1 == this) ppt.add(&p2);
      else if(p2 == this) ppt.add(&p1);
    }

    if(t2 == 0)
    {
      if(p2 == this) ppt.add(&p3);
      else if(p3 == this) ppt.add(&p2);
    }

    if(t3 == 0)
    {
      if(p3 == this) ppt.add(&p1);
      else if(p1 == this) ppt.add(&p3);
    }
  }

  // Now I should have two vertices in the buffer.
  assert(ppt.length == 2);

  // Now I can compute the error.

  etQVector<double,3> u(ppt[0]->getVector()), v(this->getVector()), n;
  u -= ppt[1]->getVector();
  v -= ppt[1]->getVector();
  n.cross(u, v);
  double error = 0.5 * n.norm();

  candidate.point = ppt[0];
  candidate.error = -error;
  deviation       = error;
}


//////////////////////////////////////////////////////////////////////////////
void
etQemPoint::updateQem2D(etQemInfo *ctx)
{
  // compute the qem
  int i;
  for(i=0;i<poly.length;++i)
  {
    etQemTriangle *p = poly[i];

    // Discard invalid triangles if any
    if(!p->isValid())
    {
      poly.remove(i--);
      continue;
    }

    etQemPoint    *p1 = p->verts[0];
    etQemPoint    *p2 = p->verts[1];
    etQemPoint    *p3 = p->verts[2];
    etQemTriangle *t1 = p->neighbours[0];
    etQemTriangle *t2 = p->neighbours[1];
    etQemTriangle *t3 = p->neighbours[2];

    assert(p1 != p2 && p2 != p3 && p3 != p1);

    if(t1 && t2 && t3)
    {
      // 99.9% of the cases.
      if(p1 == this) possibleCandidate2D(ctx, p2);
      if(p2 == this) possibleCandidate2D(ctx, p3);
      if(p3 == this) possibleCandidate2D(ctx, p1);
      continue;
    }

    if(p1 == this)
    {
      if(!t3) possibleCandidate2D(ctx, p3);
      possibleCandidate2D(ctx, p2);
    }

    if(p2 == this)
    {
      if(!t1) possibleCandidate2D(ctx, p1);
      possibleCandidate2D(ctx, p3);
    }

    if(p3 == this)
    {
      if(!t2) possibleCandidate2D(ctx, p2);
      possibleCandidate2D(ctx, p1);
    }
  }
}


void
etQemPoint::updateQem3D(etQemInfo *ctx)
{
  // compute the qem
  int i;
  for(i=0;i<poly.length;++i)
  {
    etQemTriangle *p = poly[i];

    // Discard invalid triangles if any
    if(!p->isValid())
    {
      poly.remove(i--);
      continue;
    }

    etQemPoint    *p1 = p->verts[0];
    etQemPoint    *p2 = p->verts[1];
    etQemPoint    *p3 = p->verts[2];
    etQemTriangle *t1 = p->neighbours[0];
    etQemTriangle *t2 = p->neighbours[1];
    etQemTriangle *t3 = p->neighbours[2];

    assert(p1 != p2 && p2 != p3 && p3 != p1);

    if(t1 && t2 && t3)
    {
      // 99.9% of the cases.
      if(p1 == this) possibleCandidate3D(ctx, p2);
      if(p2 == this) possibleCandidate3D(ctx, p3);
      if(p3 == this) possibleCandidate3D(ctx, p1);
      continue;
    }

    if(p1 == this)
    {
      if(!t3) possibleCandidate3D(ctx, p3);
      possibleCandidate3D(ctx, p2);
    }

    if(p2 == this)
    {
      if(!t1) possibleCandidate3D(ctx, p1);
      possibleCandidate3D(ctx, p3);
    }

    if(p3 == this)
    {
      if(!t2) possibleCandidate3D(ctx, p2);
      possibleCandidate3D(ctx, p1);
    }
  }
}


//////////////////////////////////////////////////////////////////////////////
void
etQemTriangle::setPoints(etQemPoint *p1, etQemPoint *p2, etQemPoint *p3)
{
  verts[0] = p1; verts[1] = p2; verts[2] = p3;
}

//////////////////////////////////////////////////////////////////////////////
void
etQemTriangle::setNeighbours(etQemTriangle *n1, etQemTriangle *n2, etQemTriangle *n3)
{
  neighbours[0] = n1; neighbours[1] = n2; neighbours[2] = n3;
}

//////////////////////////////////////////////////////////////////////////////
void etQemTriangle::buildQuadric()
{
  etQuadric<double,4> q;
  q.init(verts[0]->getVector(), verts[1]->getVector(), verts[2]->getVector());
  if(!verts[0]->isBorder()) verts[0]->addQuadric(q);
  if(!verts[1]->isBorder()) verts[1]->addQuadric(q);
  if(!verts[2]->isBorder()) verts[2]->addQuadric(q);
}

//////////////////////////////////////////////////////////////////////////////
etQemMesh::etQemMesh(etVec3f  *pts, int npts, etFace3i *tri, int ntri,
                     etFace3i *neigh)
{
  assert(npts && ntri);

  // Allocate points
  int i;
  for(i=0;i<npts;++i)
  {
    etQemPoint tmp(pts[i]);
    points.add(&tmp);
  }

  // Allocate triangles
  for(i=0;i<ntri;++i)
  {
    int p1 = tri[i][0], p2 = tri[i][1], p3 = tri[i][2];
    etQemTriangle t(&(points[p1]), &(points[p2]), &(points[p3]));
    triangles.add(&t);
  }

  for(i=0;i<ntri;++i)
  {
    int p1 = tri[i][0], p2 = tri[i][1], p3 = tri[i][2];
    points[p1].addTriangle(&(triangles[i]));
    points[p2].addTriangle(&(triangles[i]));
    points[p3].addTriangle(&(triangles[i]));
  }

  if(neigh)
    for(i=0;i<ntri;++i)
    {
      const int t1 = neigh[i][0];
      const int t2 = neigh[i][1];
      const int t3 = neigh[i][2];
      assert(t1 == -1 || t2 == -1 || (t1>=0 && t2>=0 && t1 != t2));
      assert(t2 == -1 || t3 == -1 || (t2>=0 && t3>=0 && t2 != t3));
      assert(t3 == -1 || t1 == -1 || (t3>=0 && t1>=0 && t3 != t1));
      triangles[i].setNeighbours((t1 != -1 ? triangles.peek() + t1 : 0),
                                 (t2 != -1 ? triangles.peek() + t2 : 0),
                                 (t3 != -1 ? triangles.peek() + t3 : 0));
    }

  // Allocate the priority list.
  priolist.init(points.length);


  trianglesLeft   = triangles.length;
  pointsLeft      = points.length;
  info.targetError     = 1.0E9;
  info.minTriangles    = 2;
  info.borderWeight    = 1000.0;
  info.triangleQuality = 1E-2;
  info.minError        = 0.0;
  info.sizeX           = 0;
  info.sizeY           = 0;
  //    info.weightQuality   = true;
}

//////////////////////////////////////////////////////////////////////////////
etQemMesh::etQemMesh(etVec3f  *pts, int npts, etVec3f  *normals,
                     etFace3i *tri, int ntri, etFace3i *neigh)
{
  assert(npts && ntri);

  // Allocate points
  int i;
  for(i=0;i<npts;++i)
    points.add(new etQemPoint(pts[i]));

  // Allocate triangles
  for(i=0;i<ntri;++i)
  {
    int p1 = tri[i][0], p2 = tri[i][1], p3 = tri[i][2];
    triangles.add(new etQemTriangle(&(points[p1]), &(points[p2]), &(points[p3])));
  }

  for(i=0;i<ntri;++i)
  {
    int p1 = tri[i][0], p2 = tri[i][1], p3 = tri[i][2];
    points[p1].addTriangle(&(triangles[i]));
    points[p2].addTriangle(&(triangles[i]));
    points[p3].addTriangle(&(triangles[i]));
  }

  if(neigh)
    for(i=0;i<ntri;++i)
    {
      const int t1 = neigh[i][0];
      const int t2 = neigh[i][1];
      const int t3 = neigh[i][2];
      assert(t1 == -1 || t2 == -1 || (t1>=0 && t2>=0 && t1 != t2));
      assert(t2 == -1 || t3 == -1 || (t2>=0 && t3>=0 && t2 != t3));
      assert(t3 == -1 || t1 == -1 || (t3>=0 && t1>=0 && t3 != t1));
      triangles[i].setNeighbours((t1 != -1 ? triangles.peek() + t1 : 0),
                                 (t2 != -1 ? triangles.peek() + t2 : 0),
                                 (t3 != -1 ? triangles.peek() + t3 : 0));
    }

  // Allocate the priority list.
  priolist.init(points.length);


  trianglesLeft   = triangles.length;
  pointsLeft      = points.length;
  info.targetError     = 1.0E9;
  info.minTriangles    = 2;
  info.borderWeight    = 1000.0;
  info.triangleQuality = 1E-2;
  info.minError        = 0.0;
  info.sizeX           = 0;
  info.sizeY           = 0;
  //    info.weightQuality   = true;
}


//////////////////////////////////////////////////////////////////////////////
etQemMesh::~etQemMesh()
{
}


//////////////////////////////////////////////////////////////////////////////
// This function will compute the quadrics per vertex.
// It will only update the vertices that are not constrained and those that
// are not of type border.
void etQemMesh::computeQuadrics(etVec3f *normals)
{
  if(!points.length || !triangles.length) return;

  if(normals)
  {
    etQVector<double,4> v;

    int i;
    for(i=0;i<points.length;++i)
    {
      v[0] = normals[i][0]; v[1] = normals[i][1]; v[2] = normals[i][2];
      v[3] = -(points[i][0] * v[0] + points[i][1] * v[1] + points[i][2] * v[2]);
      etQuadric<double,4> &q = points[i].getQuadric();
      q.init(v, 1.0);
    }
  }
  else
  {
    int i;
    for(i=0;i<triangles.length;++i)
      if(triangles[i].isValid())
        triangles[i].buildQuadric();
  }
}

//////////////////////////////////////////////////////////////////////////////
// This function will tag all border vertices
//
void etQemMesh::tagBorderVertices()
{
  int i;
  for(i=0;i<triangles.length;++i)
  {
    if(!triangles[i].neighbours[0]) {
      triangles[i].verts[0]->setBorder();
      triangles[i].verts[1]->setBorder();
    }
    if(!triangles[i].neighbours[1]) {
      triangles[i].verts[1]->setBorder();
      triangles[i].verts[2]->setBorder();
    }
    if(!triangles[i].neighbours[2]) {
      triangles[i].verts[2]->setBorder();
      triangles[i].verts[0]->setBorder();
    }
  }
}


//////////////////////////////////////////////////////////////////////////////
// This function will compute the quadrics to the border vertices
// Note: I don't use the neighbours to detect edges to rely on the vertex
//       information only. Currently the adjacency information supports only
//       manifold meshes. Using some vertex information makes this function
//       working with non manifold meshes as well.
void
etQemMesh::processBorderVertices(etVec3f *normal)
{
  // Processing the borders in the tangent space.
  if(normal)
  {
    etQVector<double,3> n;
    n[0] = (*normal)[0]; n[1] = (*normal)[1]; n[2] = (*normal)[2];

    // Computing the quadric for the border vertices.
    int i;
    for(i=0;i<triangles.length;++i)
    {
      // This is true 99% of the time
      etQemTriangle *t1 = triangles[i].neighbours[0];
      etQemTriangle *t2 = triangles[i].neighbours[1];
      etQemTriangle *t3 = triangles[i].neighbours[2];
      if(t1 && t2 && t3) continue;

      // Happens if the borders vertices were not tagged
      etQemPoint *p1 = triangles[i].verts[0];
      etQemPoint *p2 = triangles[i].verts[1];
      etQemPoint *p3 = triangles[i].verts[2];
      if(!p1->isBorder() && !p2->isBorder() && !p3->isBorder()) continue;

      // Process border edges only.
      if(!t1)
      {
        assert(p1->isBorder() && p2->isBorder());
        borderQuadric(n, p1, p2);
      }
      if(!t2)
      {
        assert(p2->isBorder() && p3->isBorder());
        borderQuadric(n, p2, p3);
      }
      if(!t3)
      {
        assert(p3->isBorder() && p1->isBorder());
        borderQuadric(n, p3, p1);
      }
    }
  }
  else
  {
    etQVector<double,3> n;

    // Computing the quadric for the border vertices.
    int i;
    for(i=0;i<triangles.length;++i)
    {
      // This is true 99% of the time
      etQemTriangle *t1 = triangles[i].neighbours[0];
      etQemTriangle *t2 = triangles[i].neighbours[1];
      etQemTriangle *t3 = triangles[i].neighbours[2];
      if(t1 && t2 && t3) continue;

      // Happens if the borders vertices were not tagged
      etQemPoint *p1 = triangles[i].verts[0];
      etQemPoint *p2 = triangles[i].verts[1];
      etQemPoint *p3 = triangles[i].verts[2];
      if(!p1->isBorder() && !p2->isBorder() && !p3->isBorder()) continue;

      {
        etQVector<double,3> u(p2->getVector());
        etQVector<double,3> v(p3->getVector());
        u -= p1->getVector();
        v -= p1->getVector();
        n.cross(u,v);
        n.normalize();
      }

      // Process border edges only.
      if(!t1 && p1->isBorder() && p2->isBorder()) borderQuadric(n, p1, p2);
      if(!t2 && p2->isBorder() && p3->isBorder()) borderQuadric(n, p2, p3);
      if(!t3 && p3->isBorder() && p1->isBorder()) borderQuadric(n, p3, p1);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// Given an edge and a  normal, I compute a quadric that is perpendicular to
// the plane made by the edge and the normal.
void
etQemMesh::borderQuadric(const etQVector<double,3> &n,
                         etQemPoint *p1, etQemPoint *p2)
{
  etQVector<double,3> n2;
  etQuadric<double,4> q;

  n2 = p2->getVector();
  n2 += n;
  q.init(p1->getVector(), p2->getVector(), n2);
  q *= info.borderWeight;

  p1->addQuadric(q);
  p2->addQuadric(q);
}


//////////////////////////////////////////////////////////////////////////////
// This function simply inserts a set of vertices to the priority list.
// If bordersOnly is set, then all unconstrained border vertices will be
// inserted into the list.
void etQemMesh::sortVertices(bool bordersOnly)
{
  // insert all the valid vertices.
  priolist.reset();

  if(!bordersOnly)
  {
    int i;
    for(i=0;i<points.length;++i)
    {
      if(!points[i].isValid()     ||
         !points[i].canBeMerged() ||
         points[i].isConstrained()) continue;
      priolist.insert(&(points[i]));
    }
  }
  else
  {
    int i;
    for(i=0;i<points.length;++i)
    {
      if(!points[i].isValid()     ||
         !points[i].canBeMerged() ||
         !points[i].isBorder()    ||
         points[i].isConstrained()) continue;
      priolist.insert(&(points[i]));
    }
  }
}


//////////////////////////////////////////////////////////////////////////////
// Mesh decimation function: The process will stop once the minimum number of
// triangles is reached or when the targetted error is reached.
//
int
etQemMesh::collapse2D()
{
  // The loop will break automatically if no more point is found or if the
  // deviation is larger than minError.
  while(true)
  {
    if(trianglesLeft <= info.minTriangles)
    {
      break;
    }

    // Fetch the point to collapse from the priority list.
    etQemPoint *pt = (etQemPoint*) priolist.getFirst();

    // I stop the iteration if:
    // - the list is empty
    // - the geometric deviation exceeds the fixed limit.
    if(!pt || (pt->getDeviation() > info.targetError))
    {
      break;
    }

    // Processing the vertex.
    // If the vertex cannot be merged, it is simply taken out of the list.
    // It will be reinserted if necessary when one of its neighbours will
    // be merged.
    if(!pt->canBeMerged()) continue;

    etArray<etQemPoint*> ppt;

    // Merge the vertex with its candidate.
    trianglesLeft -= pt->merge(ppt);
    pointsLeft--;

    // Recompute the qem for the candidate and its surroundings
    // Then reinsert the vertices into the priority list.
    // I remove them if they don't have any candidate.
    if(ppt.length)
    {
      int i;
      for(i=0;i<ppt.length;++i)
      {
        assert(ppt[i]->isValid());
        ppt[i]->resetCandidate();
        ppt[i]->updateQem2D(&info);
        assert(ppt[i]->isValid());
        if(ppt[i]->getPoint())
        {
          if(ppt[i]->isInHeap())
            priolist.update(ppt[i]);
          else
            priolist.insert(ppt[i]);
        }
        else
          if(ppt[i]->isInHeap())
            priolist.remove(ppt[i]);
      }
    }
  }

  // etQemPoint::merge can leave points that are connected to no
  // triangles. It's faster to just find these rare cases after the fact
  // and clean them up.
  FixDanglingPoints();

  return pointsLeft;
}


int
etQemMesh::collapse3D()
{
  // The loop will break automatically if no more point is found or if the
  // deviation is larger than minError.
  while(true)
  {
    if(trianglesLeft <= info.minTriangles)
    {
      break;
    }

    // Fetch the point to collapse from the priority list.
    etQemPoint *pt = (etQemPoint*) priolist.getFirst();

    // I stop the iteration if:
    // - the list is empty
    // - the geometric deviation exceeds the fixed limit.
    if(!pt || (pt->getDeviation() > info.targetError))
    {
      break;
    }

    // Processing the vertex.
    // If the vertex cannot be merged, it is simply taken out of the list.
    // It will be reinserted if necessary when one of its neighbours will
    // be merged.
    if(!pt->canBeMerged()) continue;

    etArray<etQemPoint*> ppt;

    // Merge the vertex with its candidate.
    trianglesLeft -= pt->merge(ppt);
    pointsLeft--;

    // Recompute the qem for the candidate and its surroundings
    // Then reinsert the vertices into the priority list.
    // I remove them if they don't have any candidate.
    if(ppt.length)
    {
      int i;
      for(i=0;i<ppt.length;++i)
      {
        assert(ppt[i]->isValid());
        ppt[i]->resetCandidate();
        ppt[i]->updateQem3D(&info);
        assert(ppt[i]->isValid());
        if(ppt[i]->getPoint())
        {
          if(ppt[i]->isInHeap())
            priolist.update(ppt[i]);
          else
            priolist.insert(ppt[i]);
        }
        else
          if(ppt[i]->isInHeap())
            priolist.remove(ppt[i]);
      }
    }
  }

  // etQemPoint::merge can leave points that are connected to no
  // triangles. It's faster to just find these rare cases after the fact
  // and clean them up.
  FixDanglingPoints();

  return pointsLeft;
}


//////////////////////////////////////////////////////////////////////////////
int etQemMesh::collapseBorders()
{
  // First of all, I need to mark the corner of the mesh as constrained.
  int c1 = 0, c2 = info.sizeX-1, c3 = info.sizeX * (info.sizeY-1), c4 = c3 + c2;

  points[c1].setConstrained();
  points[c2].setConstrained();
  points[c3].setConstrained();
  points[c4].setConstrained();

  // Reset the priority list.
  priolist.reset();


  // Then I need to compute the decimation value per vertex.
  // This is done by computing the deviation error caused by the vertex
  // collapse. The best candidate is whatever neigbouring vertex.
  int i;
  for(i=0;i<points.length;++i)
  {
    etQemPoint &pt = points[i];

    if(pt.isBorder() && !pt.isConstrained())
    {
      pt.updateBorderError();
      priolist.insert(&pt);
    }
  }

  // Then I iteratively decimate the borders until I get a sufficient
  // error.
  while(true)
  {
    // Fetch the point to collapse from the priority list.
    etQemPoint *pt = (etQemPoint*) priolist.getFirst();

    // I can collapse if the priority list is empty
    if(!pt) break;

    // Sanity check
    assert(pt->isValid() && pt->isBorder());
    assert(!pt->isConstrained());

    // Stop decimating if the point has a sufficient error
    if(pt->getDeviation() > info.borderTargetError) break;

    // Merge the vertex and collect the 2 border neighbours that will need
    // to be updated.
    etArray<etQemPoint*> ppt;

    // Merge the vertex with its candidate.
    trianglesLeft -= pt->mergeBorder(ppt);
    pointsLeft--;

    if(ppt.length)
    {
      int i;
      for(i=0;i<ppt.length;++i)
      {
        assert(ppt[i]->isValid());
        assert(ppt[i]->isBorder());

        if(!ppt[i]->isConstrained())
        {
          ppt[i]->updateBorderError();

          if(ppt[i]->getPoint())
          {
            if(ppt[i]->isInHeap())
              priolist.update(ppt[i]);
            else
              priolist.insert(ppt[i]);
          }
          else
            if(ppt[i]->isInHeap())
              priolist.remove(ppt[i]);
        }
      }
    }
  }

  return pointsLeft;
}



//////////////////////////////////////////////////////////////////////////////
// This function assumes that the buffer were already allocated.
void
etQemMesh::getMesh(etVec3f  *pts, int &npts, etFace3i *tri, int &ntri,
                   etFace3i *neigh)
{
  // First I need to renumerote the triangles and vertices.
  int *ptIdx = new int[points.length];
  int *trIdx = new int[triangles.length];

  npts = 0;
  int i;
  for(i=0;i<points.length;++i)
  {
    if(!points[i].isValid()) continue;
    pts[npts][0] = (float)points[i][0];
    pts[npts][1] = (float)points[i][1];
    pts[npts][2] = (float)points[i][2];
    ptIdx[i]     = npts++;
  }

  ntri = 0;
  for(i=0;i<triangles.length;++i)
  {
    if(!triangles[i].isValid()) continue;
    int p1 = triangles[i].getPoint(0) - points.peek();
    int p2 = triangles[i].getPoint(1) - points.peek();
    int p3 = triangles[i].getPoint(2) - points.peek();

    tri[ntri][0] = ptIdx[p1];
    tri[ntri][1] = ptIdx[p2];
    tri[ntri][2] = ptIdx[p3];
    trIdx[i] = ntri++;
  }

  if(neigh)
  {
    ntri = 0;
    for(i=0;i<triangles.length;++i)
    {
      if(!triangles[i].isValid()) continue;

      if(triangles[i].getNeighbour(0))
        neigh[ntri][0] = trIdx[triangles[i].getNeighbour(0) - triangles.peek()];
      else
        neigh[ntri][0] = -1;

      if(triangles[i].getNeighbour(1))
        neigh[ntri][1] = trIdx[triangles[i].getNeighbour(1) - triangles.peek()];
      else
        neigh[ntri][1] = -1;

      if(triangles[i].getNeighbour(2))
        neigh[ntri][2] = trIdx[triangles[i].getNeighbour(2) - triangles.peek()];
      else
        neigh[ntri][2] = -1;

      ntri++;
    }
  }

  delete [] ptIdx;
  delete [] trIdx;
}


//////////////////////////////////////////////////////////////////////////////
void
etQemMesh::getMesh(etMesh<etVec3f, etFace3i> &msh)
{
  // First I need to renumerote the triangles and vertices.
  int *ptIdx = new int[points.length];

  int i;
  for(i=0;i<points.length;++i)
  {
    if(points[i].isValid())
      ptIdx[i] = msh.addPoint((float)points[i][0], (float)points[i][1], (float)points[i][2]);
    else ptIdx[i] = -1;
  }

  for(i=0;i<triangles.length;++i)
  {
    if(!triangles[i].isValid()) continue;
    int p1 = ptIdx[triangles[i].getPoint(0) - points.peek()];
    int p2 = ptIdx[triangles[i].getPoint(1) - points.peek()];
    int p3 = ptIdx[triangles[i].getPoint(2) - points.peek()];
    assert(p1 >= 0 && p2 >= 0 && p3 >= 0);
    // assert(p1 <= msh.nPoints() && p2 <= msh.nPoints() && p3 <= msh.nPoints());
    if(p1 >= 0 && p2 >= 0 && p3 >= 0) msh.addFace(p1, p2, p3);
  }

  //    fprintf(stderr, "%d vertices and %d triangles\n", msh.nPoints(), msh.nFaces());

  delete [] ptIdx;
}


//////////////////////////////////////////////////////////////////////////////
void
etQemMesh::findCandidates2D()
{
  // Cleaning up the candidates.
  int i;
  for(i=0;i<points.length;++i)
    if(points[i].isValid() && !points[i].isConstrained())
      points[i].resetCandidate();

  // Computing the qems
  for(i=0;i<points.length;++i)
    if(points[i].isValid() && !points[i].isConstrained())
      points[i].updateQem2D(&info);
}


void
etQemMesh::findCandidates3D()
{
  // Cleaning up the candidates.
  int i;
  for(i=0;i<points.length;++i)
    if(points[i].isValid() && !points[i].isConstrained())
      points[i].resetCandidate();

  // Computing the qems
  for(i=0;i<points.length;++i)
    if(points[i].isValid() && !points[i].isConstrained())
      points[i].updateQem3D(&info);
}


//////////////////////////////////////////////////////////////////////////////
// This function is responsible for decimating generalized manifold meshes.
void etQemMesh::decimate()
{
  // Compute the quadrics to all the vertices of the mesh.
  computeQuadrics();

  // Tag all the border vertices
  tagBorderVertices();

  // Weight the borders with quadrics
  processBorderVertices();

  // Find now the best candidates for all vertices.
  findCandidates3D();

  // Insert the vertices to the priority list
  sortVertices();

  // Collapse the mesh.
  collapse3D();
}


//////////////////////////////////////////////////////////////////////////////
// This function is specialized in terrain tile decimation.
void etQemMesh::decimateTile()
{
  // PRELIMINARY: Find the main direction vectors of the tile
  //==========================================================
  const etQVector<double,3> &p1 = points[0].getVector();
  const etQVector<double,3> &p2 = points[info.sizeX-1].getVector();
  const etQVector<double,3> &p3 = points[(info.sizeY-1) * info.sizeX].getVector();
  etVec3d u, v, n;
  u[0]  = p2[0]; u[1]  = p2[1]; u[2]  = p2[2];
  u[0] -= p1[0]; u[1] -= p1[1]; u[2] -= p1[2];
  u.normalize();

  v[0]  = p3[0]; v[1]  = p3[1]; v[2]  = p3[2];
  v[0] -= p1[0]; v[1] -= p1[1]; v[2] -= p1[2];
  v.normalize();

  n.cross(u,v);

  // Transforming the mesh into the normal space, so that I can evaluate
  // the triangle quality in 2d.
  etMat3d mat;
  mat.setRow(u,0);
  mat.setRow(v,1);
  mat.setRow(n,2);

  int i;
  for(i=0;i<points.length;++i)
  {
    etVec3d &v = *(etVec3d*)points[i].getVector().elem;
    mat.transform(v,v);
  }


  // I also constrain the middle edge vertices and also the center vertex.
  // In order to improve the tesselation and reduce the pops.
  int midX = getSizeX() / 2;
  int midY = getSizeY() / 2;

  points[midX].setConstrained();
  points[midY * getSizeX()].setConstrained();
  points[midY * getSizeX() + midX].setConstrained();
  points[((midY+1) * getSizeX()) - 1].setConstrained();
  points[(getSizeY()-1) * getSizeX() + midX].setConstrained();


  // FIRST PART: I DECIMATE THE BORDERS
  //===================================

  // Tag all the border vertices
  tagBorderVertices();

  // Collapse the mesh.
  collapseBorders();

  // Constrain the border vertices.
  constrainBorderVertices();


  // SECOND PART: I DECIMATE THE INTERIOR OF THE TILE
  //=================================================

  setTriangleQuality(0.001);

  for(i=0;i<points.length;++i)
    if(points[i].isValid()) points[i].cleanUp();

  // The quadrics will be computed for all valid & unconstrained vertices.
  computeQuadrics();

  // Find now the best candidates for all vertices.
  findCandidates2D();

  // Insert the vertices to the priority list
  sortVertices();

  // Collapse the mesh.
  collapse2D();

  // TODO: Add an iteration to decimate the mesh until the targetted amount
  // of triangles & vertices is reached (important for the compression).
}


//////////////////////////////////////////////////////////////////////////////
// Function used to constrain the border vertices so that a second decimation
// loop cannot get rid of them.
void etQemMesh::constrainBorderVertices()
{
  int i;
  for(i=0;i<points.length;++i)
  {
    if(points[i].isValid() && points[i].isBorder())
      points[i].setConstrained();
  }
}


#include <set>
void
etQemMesh::FixDanglingPoints(void)
{
  std::set<etQemPoint*> usedPoints;
  for(int i=0;i<triangles.length;++i)
  {
    if(!triangles[i].isValid()) continue;
    usedPoints.insert(triangles[i].getPoint(0));
    usedPoints.insert(triangles[i].getPoint(1));
    usedPoints.insert(triangles[i].getPoint(2));
  }

  for (int i = 0; i < points.length; ++i) {
    if (points[i].isValid()) {
      if (usedPoints.find(&points[i]) == usedPoints.end()) {
        points[i].setCollapsed();
        --pointsLeft;
      }
    }
  }
}


//////////////////////////////////////////////////////////////////////////////
//
void etQemMesh::getTile(etArray<etVec3f> &pts, etArray<etFace3i> &fcs, etVec3f *verts)
{
  pts.reset();
  fcs.reset();

  // First I need to renumerote the triangles and vertices.
  int *ptIdx = new int[points.length];

  for (int idx = 0; idx < points.length; ++idx) {
    if(points[idx].isValid()) {
      ptIdx[idx] = pts.add(&(verts[idx]));
    } else {
      ptIdx[idx] = -1;
    }
  }

  for(int i=0;i<triangles.length;++i)
  {
    if(!triangles[i].isValid()) continue;
    int p1 = ptIdx[triangles[i].getPoint(0) - points.peek()];
    int p2 = ptIdx[triangles[i].getPoint(1) - points.peek()];
    int p3 = ptIdx[triangles[i].getPoint(2) - points.peek()];
    assert(p1 >= 0 && p2 >= 0 && p3 >= 0);
    assert(p1 < pts.length && p2 < pts.length && p3 < pts.length);
    etFace3i tmp(p1, p2, p3);
    if(p1 >= 0 && p2 >= 0 && p3 >= 0) fcs.add(&tmp);
  }

  // Reodering the triangle indices and sorting the triangles east to
  // west. This will be helpful to get a fast hit test function in the
  // client.
  optimizeIndices(pts, fcs);

  delete [] ptIdx;
}

int etQemMesh::compareTriInfo( const void *arg1, const void *arg2 )
{
  typedef struct {
    float     west;
    etFace3i  face;
  } triInfo;

  triInfo *inf1 = (triInfo*)arg1;
  triInfo *inf2 = (triInfo*)arg2;

  if(inf1->west < inf2->west) return 1;
  else if(inf1->west > inf2->west) return -1;
  return 0;
}


void etQemMesh::optimizeIndices(etArray<etVec3f> &pts, etArray<etFace3i> &fcs)
{
  typedef struct {
    float    west;
    etFace3i face;
  } triInfo;

  triInfo *infos;
  infos = new triInfo[fcs.length];

  // First pass: I order the indices within the triangle
  //             so that the first index is the west most vertex.
  {
    int i;
    int p1, p2, p3;
    for(i=0;i<fcs.length;++i)
    {
      p1 = fcs[i][0]; p2 = fcs[i][1]; p3 = fcs[i][2];

      infos[i].west  = pts[p1][0];

      if(pts[p2][0] < infos[i].west)
      {
        infos[i].west = pts[p2][0];
        fcs[i][0] = p2;
        fcs[i][1] = p3;
        fcs[i][2] = p1;
      }

      if(pts[p3][0] < infos[i].west)
      {
        infos[i].west = pts[p3][0];
        fcs[i][0] = p3;
        fcs[i][1] = p1;
        fcs[i][2] = p2;
      }

      infos[i].face = fcs[i];
    }
  }

  // Second pass: I reorder the triangles so that they are sorted
  //              east to west, based on their westmost coordinate.
  {
    qsort(infos, fcs.length, sizeof(triInfo), etQemMesh::compareTriInfo);

    // Now I swap the triangles.
    int i;
    for(i=0;i<fcs.length;++i)
      fcs[i] = infos[i].face;
  }

  delete [] infos;
}

