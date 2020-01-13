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


#
# Internal Tool
# Creates KML that can be pasted into the GEE Cutter or Google Earth
# to see the boundaries of a cut based on the quadtree nodes.
#
# The generated kml can be pasted into the left sidebar. Use GetInfo (on Mac)
# to change outline and fill color and opacity.

import os
import re
import sys

g_kml_header = """<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2"
 xmlns:gx="http://www.google.com/kml/ext/2.2"
 xmlns:kml="http://www.opengis.net/kml/2.2"
 xmlns:atom="http://www.w3.org/2005/Atom">
<Document>"""

g_kml_polygon_template = """
<Placemark>
    <name>%s</name>
    <visibility>1</visibility>
    <Polygon>
      <tessellate>1</tessellate>
      <outerBoundaryIs>
        <LinearRing>
          <coordinates>
            %s
          </coordinates>
        </LinearRing>
      </outerBoundaryIs>
    </Polygon>
  </Placemark>
"""

g_kml_footer = """</Document>
</kml>"""


def GetNextQtnodeBounds(qtnode, x, y, size):
  """Calculate next level boundary for qtnode.

  If the qtnode has further precision, call this routine
  recursively.

  Args:
    qtnode: The remaining string of the qtnode.
    x: Current left of the qtnode (degrees).
    y: Current bottom of the qtnode (degrees).
    size: Current size of sides of qtnode (degrees).
  Returns:
    List of lower left and upper right boundary.
  Raises:
    Exception: if qtnode is not well formed.
  """
  if qtnode:
    size /= 2
    if qtnode[0] == "3":
      return GetNextQtnodeBounds(qtnode[1:], x, y + size, size)
    elif qtnode[0] == "2":
      return GetNextQtnodeBounds(qtnode[1:], x + size, y + size, size)
    elif qtnode[0] == "1":
      return GetNextQtnodeBounds(qtnode[1:], x + size, y, size)
    elif qtnode[0] == "0":
      return GetNextQtnodeBounds(qtnode[1:], x, y, size)
    else:
      raise Exception("Error: unexpected qtnode value %s" % qtnode[0])
  else:
    return [x, y, x + size, y + size]


def GetQtnodeBounds(qtnode, ignore_zero):
  """Calculate boundary for qtnode.

  Args:
    qtnode: The entire string of the qtnode.
    ignore_zero: Whether initial zero should be ignored.
  Returns:
    List of lower left and upper right boundary.
  """
  if ignore_zero and qtnode[0] == "0":
    return GetNextQtnodeBounds(qtnode[1:], -180.0, -180.0, 360.0)
  else:
    return GetNextQtnodeBounds(qtnode, -180.0, -180.0, 360.0)


def OutputCoords(name, bounds):
  """Print kml polygon corresponding to bounds rectangle.

  Args:
    name: Name to be given to the polygon.
    bounds: List of lower left and upper right boundary.
  """
  coords = ("%s,%s,0 %s,%s,0 %s,%s,0 %s,%s,0 %s,%s,0" %
            (bounds[0], bounds[1], bounds[2], bounds[1],
             bounds[2], bounds[3], bounds[0], bounds[3],
             bounds[0], bounds[1]))
  print g_kml_polygon_template % (name, coords)


def MergeBounds(bounds1, bounds2):
  """Merge two rectangles to create minimum rectangle that contains both.

  Args:
    bounds1:  List of lower left and upper right boundary of first rectangle.
    bounds2:  List of lower left and upper right boundary of second rectangle.
  Returns:
    List of lower left and upper right boundary containing both rectangles.
  """
  new_bounds = []
  if bounds1[0] < bounds2[0]:
    new_bounds.append(bounds1[0])
  else:
    new_bounds.append(bounds2[0])

  if bounds1[1] < bounds2[1]:
    new_bounds.append(bounds1[1])
  else:
    new_bounds.append(bounds2[1])

  if bounds1[2] > bounds2[2]:
    new_bounds.append(bounds1[2])
  else:
    new_bounds.append(bounds2[2])

  if bounds1[3] > bounds2[3]:
    new_bounds.append(bounds1[3])
  else:
    new_bounds.append(bounds2[3])

  return new_bounds


def ProcessNode(qtnode, use_kml, ignore_zero):
  """Convert quadtree node to bounds or kml polygon.

  Args:
    qtnode: String representing the quadtree node.
    use_kml: Whether bounds should be converted to a kml polygon.
    ignore_zero: Whether initial zero should be ignored.
  Returns:
    List of lower left and upper right boundary containing quadtree node.
  """
  bounds = GetQtnodeBounds(qtnode, ignore_zero)
  if use_kml:
    OutputCoords(qtnode, bounds)
  return bounds


def ProcessFile(qtnode_file, use_kml, ignore_zero, max_level):
  """Process all qtnodes in the given file.

  Args:
    qtnode_file: Path to file containing quadtree nodes.
    use_kml: Whether bounds should be converted to a kml polygon.
    ignore_zero: Whether initial zero should be ignored.
    max_level: Max level to which quadtree nodes should be considered.
  Returns:
    List of lower left and upper right boundary containing all quadtree nodes.
  """
  # Open file of qtnodes used to cut a globe.
  fp = open(qtnode_file)

  # Create a polygon for each qtnode in the file.
  processed = []
  bounds = [180, 180, -180, -180]
  for next_qtnode in fp:
    qtnode = next_qtnode.strip()[:max_level]
    # Don't reprocess.
    # May get many duplicates as max_level gets small.
    if not qtnode in processed:
      processed.append(qtnode)
      bounds = MergeBounds(bounds, ProcessNode(qtnode, use_kml, ignore_zero))

  fp.close()
  return bounds


def Help():
  """Prints help information for this tool."""
  print """
Usage: ./qtnode_to_kml.py [options] [required_arg]

Outputs bounds or kml for one or more quadtree nodes. The kml includes
polygons that define the boundaries of each quadtree node.

Required arg: qtnode or qtnode file

Options:
 --kml             Output polygons as kml. Otherwise, output bounds.
 --bounds_as_kml   Output bounds for all quadtree nodes as single polygon
                   in kml. I.e. a rectangle that bounds all of of the
                   quadtree nodes.
 --ignore_zero     Ignore leading zero on qtnode addresses.
 --max_level level For quadtree nodes beyond this level, use quadtree
                   node that encompasses this node at this level.
                   Level should be in the range of 1 to 24.
 ?                 Print help information.
 -h                Print help information.
 --help            Print help information.


Examples:
  ./qtnode_to_kml.py --kml --max_level 15 /tmp/globe_builder/my_globe_env/qt_nodes.txt
  ./qtnode_to_kml.py --bounds_as_kml /tmp/globe_builder/my_globe_env/qt_nodes.txt
  ./qtnode_to_kml.py --ignore_zero 030132030222112
"""
  sys.exit(0)


if __name__ == "__main__":
  g_qtnode_file = ""
  g_qtnode = ""

  num_args = len(sys.argv) - 1
  if (num_args < 1
      or "?" in sys.argv or "--help" in sys.argv or "-h" in sys.argv):
    Help()

  arg = sys.argv[num_args]
  if os.path.isfile(arg):
    g_qtnode_file = arg
  elif re.match("^[0123]+$", arg):
    g_qtnode = arg
  else:
    print "ERROR: Argument was not a file or a qtnode."
    Help()

  g_output_kml = "--kml" in sys.argv
  g_bounds_as_kml = "--bounds_as_kml" in sys.argv
  g_ignore_zero = "--ignore_zero" in sys.argv
  if "--max_level" in sys.argv:
    max_level_idx = sys.argv.index("--max_level") + 1
    g_max_level = int(sys.argv[max_level_idx])
  else:
    g_max_level = 24

  # If printing kml, start with the header.
  if g_output_kml or g_bounds_as_kml:
    print g_kml_header

  # Process either the qtnode file or a single qtnode address.
  if g_qtnode_file:
    g_bounds = ProcessFile(g_qtnode_file, g_output_kml,
                           g_ignore_zero, g_max_level)
  else:
    g_bounds = ProcessNode(g_qtnode[:g_max_level], g_output_kml, g_ignore_zero)

  # If display bounds as kml, convert to polygon coordinates.
  if g_bounds_as_kml:
    OutputCoords("bounds", g_bounds)

  # If printing kml, end with the footer.
  if g_output_kml or g_bounds_as_kml:
    print g_kml_footer
  # If not printing kml, show the bounds.
  else:
    print g_bounds
