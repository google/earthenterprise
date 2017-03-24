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

// TODO: High-level file comment.
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "linreg.h"

LinearRegression::LinearRegression(double *x, double *y, long size)
{
  reset();

  if (size > 0L && x != NULL && y != NULL)
  {
    for (long i = 0L; i < size; i++)
      add(x[i], y[i]);
  }
}

void LinearRegression::reset()
{
  _count = 0L;

  _a = 0.0;
  _b = 0.0;

  _sumX = 0.0;
  _sumY = 0.0;
  _sumXY = 0.0;
  _sumXsquared = 0.0;
  _sumYsquared = 0.0;

  _coefD = 0.0;
  _coefC = 0.0;
  _stdError = 0.0;
}

void LinearRegression::add(const double& x, const double& y)
{
  _count++;

  _sumX += x;
  _sumY += y;
  _sumXY += x * y;
  _sumXsquared += x * x;
  _sumYsquared += y * y;

  calculate();
}

void LinearRegression::remove(const double& x, const double& y)
{
  _count--;


  if (_count <= 0)
    reset();
  else
  {
    _sumX -= x;
    _sumY -= y;
    _sumXY -= x * y;
    _sumXsquared -= x * x;
    _sumYsquared -= y * y;
  }

  calculate();
}

void LinearRegression::calculate()
{
  if (enough() && fabs(double(_count)*_sumXsquared - _sumX*_sumX) > DBL_EPSILON)
  {
    _b = (double(_count)*_sumXY - _sumY*_sumX)/(double(_count)*_sumXsquared - _sumX*_sumX);
    _a = (_sumY - _b*_sumX)/double(_count);

    double sx  = _b*(_sumXY - _sumX*_sumY/double(_count));
    double sy2 = _sumYsquared - _sumY*_sumY/double(_count);
    double sy  = sy2 - sx;

    _coefD = sx/sy2;
    _coefC = sqrt(_coefD);
    _stdError = sqrt(sy/double(_count - 2));
  }
  else
  {
    _a = 0.0;
    _b = 0.0;

    _coefD = 0.0;
    _coefC = 0.0;
    _stdError = 0.0;
  }
}
