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

/* TODO: High-level file comment. */
/*  linreg.h */
#ifndef _LINREG_H_
#define _LINREG_H_

// a linear regression analysis class
class LinearRegression
{
 public:
  LinearRegression(double *x = 0, double *y = 0, long size = 0);
  virtual ~LinearRegression(void) { }

  virtual void add(const double& x, const double& y);
  virtual void remove(const double& x, const double& y);
  void reset();

  // Must have at least 3 points to calculate standard error of estimate.
  //   Do we have enough data?
  bool enough() const { return (_count > 2) ? true : false; }
  long count() const { return _count; }

  virtual double a() const { return _a; }
  virtual double b() const { return _b; }

  double coefDeterm() const { return _coefD; }
  double coefCorrel() const { return _coefC; }
  double stdErrorEst() const { return _stdError; }

  virtual double estimate(double x) const { return _a + _b*x; }

 protected:
  long _count;                    // number of data points input

  double _sumX;                   // sum of x[i]
  double _sumY;                   // sum of y[i]
  double _sumXsquared;    // sum of x[i] squared
  double _sumYsquared;    // sum of y[i] squared
  double _sumXY;                  // sum of x[i]*y[i]

  // coefficients of f(x) = a + b*x
  double _a;                              // regression constant
  double _b;                              // regression coefficient

  double _coefD;                  // coefficient of determination
  double _coefC;                  // coefficient of correlation
  double _stdError;               // standard error of estimate

  void calculate();               // calculate coefficients
};

#endif                                                  // end of linreg.h
