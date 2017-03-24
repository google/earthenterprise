#! /usr/bin/python2.4
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
#
# parse_khhttpd_access_log.py
#
# Parses an input GEE server log, searching for imagery requests.
# Output is either:
#   1. a CSV containing lat/lon/level (and other info) for each imagery request, or
#   2. a KML file with placemarks at each imagery request location, where the name of
#      the placemark is the number of requests seen at that location.
#
# TODO: separate out KML output code into routine like CSV already is.
# TODO: compile imagery-recognition regexp in KML routine like CSV does.
# TODO: read initially into quad_dict instead of making list and de-duplicating.
# TODO: remove IP logging so Earth users aren't concerned about watching their use.
# TODO: determine output type from extension on output file
# TODO: pass output file into KML class and just my_kml.openDoc() etc.
#

import re
import sys

def Usage():
  '''Tell the user how the program should be invoked.'''
  print 'Usage:\n'
  print '    log_parser.py <input_file> <output_file> <file_type>\n'
  print 'Example: log_parser.py khhttpd_access_log access_log.kml kml\n'
  print '     or: log_parser.py khhttpd_access_log access_log.csv csv\n'

def main():
  if len(sys.argv) < 4:
    Usage()
    sys.exit(1)
  infile = sys.argv[1]
  outfile = sys.argv[2]
  filetype = sys.argv[3].lower()
  input = open(infile).readlines()
  output = open(outfile, 'w')

  if filetype == 'csv':
    MakeCSV(input, output)
    sys.exit(1)
  quad_addrs = []
  for line in input:
    quad_addr = ParseQuad(line)
    if quad_addr:
      quad_addrs.append(quad_addr)
  quad_dict = DeDupeQuads(quad_addrs)
  my_kml = KML()
  output.write(my_kml.openDoc('0'))
  output.write(my_kml.openFolder(infile, '1'))
  for addr in quad_dict.keys():
    xy_coords = ProcessQuad(addr)
    count = quad_dict[addr]
    output.write(my_kml.MakePoint(xy_coords, count))
  output.write(my_kml.closeFolder())
  output.write(my_kml.closeDoc())

#######################################################################################

def ParseQuad(line):
  '''Check for imagery (q2) requests and parse out quad-tree address.'''
  quad_regex = re.compile(r'.*q2-(.*)-')
  quad_match = quad_regex.match(line)
  if quad_match:
    quad_addr = quad_match.group(1)
    return quad_addr

def DeDupeQuads(quad_addrs):
  '''Identify unique quad-tree addresses and keep track of their use.'''
  quad_dict = {}
  for address in quad_addrs:
    if address not in quad_dict:
      quad_dict[address] = 1
    else:
      quad_dict[address] += 1
  return quad_dict

#######################################################################################

def MakeCSV(input, output):
  '''Parse the input log file and create pipe-delimited "|" output.'''
  header = 'ip|date|lon|lat|level|req_code|bytes\n'
  output.write(header)
  image_regex = re.compile(r'.*q2-.*-')
  for line in input:
    line_match = image_regex.match(line)
    if line_match:
      ip_match = re.match(r'^(.+?)\s-', line)
      ip = ip_match.group(1)
      date_match = re.match(r'.*\[(.+?)\]', line)
      date = date_match.group(1)
      quad_match = re.match(r'.*q2-(.*)-', line)
      quad = quad_match.group(1)
      xy_coords = ProcessQuad(quad_match.group(1))
      lon = xy_coords[0]
      lat = xy_coords[1]
      level = len(quad_match.group(1))
      apache_codes_match = re.match(r'.*\s(\d+?\s\d+?)$', line)
      apache_codes = apache_codes_match.group(1)
      req_code = apache_codes.split()[0]
      bytes = apache_codes.split()[1]
      csv_string = '%s|%s|%f|%f|%s|%s|%s\n' % (ip, date, lon, lat, level, req_code, bytes)
      output.write(csv_string)

#######################################################################################

def ProcessQuad(addr):
  '''Convert the quad address string into a list and send it off for coords.'''
  tile_list = list(addr)
  tile_list.reverse()
  tile_list.pop()
  xy_range = 180.0
  x_coord = 0.0
  y_coord = 0.0
  new_coords = Quad2GCS(tile_list, x_coord, y_coord, xy_range)
  return new_coords

def Quad2GCS(addr_list, x_coord, y_coord, xy_range):
  '''Drill down through quad-tree to get final x,y coords.'''
  if not addr_list:
    new_coords = (x_coord, y_coord)
    return new_coords
  else:
    tile_addr = addr_list.pop()
    new_range = xy_range/2
    if tile_addr == '0':
      x_coord -= new_range
      y_coord -= new_range
    if tile_addr == '1':
      x_coord += new_range
      y_coord -= new_range
    if tile_addr == '2':
      x_coord += new_range
      y_coord += new_range
    if tile_addr == '3':
      x_coord -= new_range
      y_coord += new_range
    return Quad2GCS(addr_list, x_coord, y_coord, new_range)

#######################################################################################

class KML:
  '''builds kml objects'''

  def openDoc(self, visibility):
    '''Opens kml file, and creates root level document.
       Takes visibility toggle of "0" or "1" as input and sets Document <open>
       attribute accordingly.
    '''
    self.visibility = visibility
    kml = ('<?xml version="1.0" encoding="UTF-8"?>\n'
           '<kml xmlns="http://earth.google.com/kml/2.1">\n'
           '<Document>\n'
           '<open>%s</open>\n') % (self.visibility)
    return kml

  def openFolder(self, name, visibility):
    '''Creates folder element and sets the "<name>" and "<open>" attributes.
       Takes folder name string and visibility toggle of "0" or "1" as input
       values.
    '''
    kml = '<Folder>\n<open>%s</open>\n<name>%s</name>\n' % (visibility, name)
    return kml

  def MakePoint(self, coords, name):
    '''Create point placemark.'''
    x = coords[0]
    y = coords[1]
    kml = ('<Placemark>\n'
           '<name>%s</name>\n'
           '<visibility>1</visibility>\n'
           '<Point>\n'
           '<coordinates>%f, %f</coordinates>\n'
           '</Point>\n'
           '</Placemark>\n\n') % (name, x, y)
    return kml

  def closeFolder(self):
    '''Closes folder element.'''
    kml = '</Folder>\n'
    return kml

  def closeDoc(self):
    '''Closes KML document'''
    kml = '</Document>\n</kml>\n'
    return kml

#######################################################################################

if __name__ == '__main__':
  main()
