#!/usr/bin/env python2.7
#
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


"""Geometric objects to reduce clutter and the risk of typos."""

import utils


def IsNumber(x):
  return isinstance(x, int) or isinstance(x, float)


class Pair(object):
  """A tuple for numbers, with arithmetic operations.

  For neatness & compactness.
  """

  def __init__(self, x, y):
    self.x = x
    self.y = y

  @staticmethod
  def Div(a, b):
    return Pair(a.x / b.x, a.y / b.y)

  def __div__(self, b):
    if isinstance(b, Pair):
      return Pair(self.x / b.x, self.y / b.y)
    else:
      return Pair(self.x / b, self.y / b)

  def __mul__(self, b):
    if isinstance(b, Pair):
      return Pair(self.x * b.x, self.y * b.y)
    else:
      return Pair(self.x * b, self.y * b)

  def __sub__(self, b):
    if isinstance(b, Pair):
      return Pair(self.x - b.x, self.y - b.y)
    else:
      return Pair(self.x - b, self.y - b)

  def AsTuple(self):
    return (self.x, self.y)

  def __str__(self):
    return "pair(%s,%s)" % (str(self.x), str(self.y))


class Rect(object):
  """<Rect> <= we already have a 'Bounds'.

  Has no units, just handy methods.

  Defined by xy of lower left, and upper right.

  Unfortunately this is all interpretation-free; not tuned for ints or
  floats, no fixed orientation, and no policy on whether one side is
  inclusive or exclusive. Is just a collection of numbers.

  Acts immutable, ie operations return copies rather than changing
  <self> in-place.
  """

  def __init__(self, x0, y0, x1, y1):
    utils.Assert(IsNumber(x0))
    utils.Assert(IsNumber(y0))
    utils.Assert(IsNumber(x1))
    utils.Assert(IsNumber(y1))
    self.x0 = x0
    self.y0 = y0
    self.x1 = x1
    self.y1 = y1

  @staticmethod
  def FromLowerLeftAndUpperRight(xy0, xy1):
    """Make a Rect from the lower left and upper right corners.

    Args:
        xy0: One corner (not always lower-left)
        xy1: The other, opposite corner.
    Returns:
        A Rect with the given corners.
    """
    return Rect(xy0.x, xy0.y, xy1.x, xy1.y)

  @staticmethod
  def FromLowerLeftAndExtent(xy0, xy_extent):
    """Make a Rect from lower-left corner and extent.

    Args:
        xy0: Lower-left corner.
        xy_extent: Pair, of width, height of the new Rect.
    Returns:
        A Rect, from the lower-left parameter, and extent.
    """
    # This will be one-past-the-end for integers.
    return Rect(xy0.x, xy0.y, xy0.x + xy_extent.x, xy0.y + xy_extent.y)

  @property
  def xy0(self):
    return Pair(self.x0, self.y0)

  @property
  def xy1(self):
    return Pair(self.x1, self.y1)

  def _Invariant(self):
    return self.x0 <= self.x1 and self.y0 <= self.y1

  def HorzExtent(self):
    return abs(self.x1 - self.x0)

  def VertExtent(self):
    return abs(self.y1 - self.y0)

  def Width(self):
    return self.HorzExtent()

  def Height(self):
    return self.VertExtent()

  def SignedWidth(self):
    return self.x1 - self.x0

  def SignedHeight(self):
    return self.y1 - self.y0

  def Extent(self):
    """Pair of unsigned width, and height. Assumes self is exclusive."""
    return Pair(self.HorzExtent(), self.VertExtent())

  def Offset(self, dx, dy):
    """Returns an offset Rect, does not move it in place.

    Args:
        dx: x-offset (added)
        dy: y-offset (added)
    Returns:
        A new Rect offset by dx, dy.
    """
    return Rect(self.x0 + dx, self.y0 + dy, self.x1 + dx, self.y1 + dy)

  def AsTuple(self):
    return (self.x0, self.y0, self.x1, self.y1)

  def FlippedVertically(self):
    return Rect(self.x0, self.y1, self.x1, self.y0)

  def IsFlippedVertically(self):
    return self.y1 < self.y0

  def IsFlippedHorizontally(self):
    return self.x1 < self.x0

  def Resized(self, dx0, dy0, dx1, dy1):
    return Rect(self.x0 + dx0, self.y0 + dy0,
                self.x1 + dx1, self.y1 + dy1)

  def __str__(self):
    return "rect(%s,%s,%s,%s)" % (
        str(self.x0), str(self.y0), str(self.x1), str(self.y1))

  def __div__(self, b):
    return Rect(self.x0 / b, self.y0 / b, self.x1 / b, self.y1 / b)

  def AsInts(self):
    return Rect(int(self.x0), int(self.y0), int(self.x1), int(self.y1))


def main():
  pair = Pair(10, 20)
  print pair.AsTuple()

if __name__ == "__main__":
  main()
