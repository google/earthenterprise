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
# Quick and dirty tool to check data and quadtree bundles before
# they are packed into a glb.  It checks that the crc for each
# bundle, which is kept in a side file, matches the crc it calculates
# for each bundle. It also checks that number of packed data packets
# jibes with the index size. The crc is maintained in memory while
# the data and qtp bundles are generated, so if there is a mismatch
# between what is loaded to be written and what is actually written,
# it should appear as a crc error.
#
# Takes as arguments the path to the globe directory (should
# contain "data" and "qtp" directories) and the number of imagery,
# terrain, and vector packets (top line of output from
# geportableglobebuilder).

import os
import sys

CRC_SIZE = 4
INDEX_ENTRY_SIZE = 24
PERCENT_PROGRESS_STEP = 5


def CalculateCrc(fname):
  """Returns calculated crc for the globe."""
  size = os.path.getsize(fname) / CRC_SIZE
  fp = open(fname, "rb")

  crc = [0, 0, 0, 0]
  step = size / 100.0 * PERCENT_PROGRESS_STEP
  percent = 0
  next_progress_indication = 0.0
  for i in xrange(size):
    word = fp.read(CRC_SIZE)
    for j in xrange(CRC_SIZE):
      crc[j] ^= ord(word[j])
    if i >= next_progress_indication:
      print "%d%%" % percent
      next_progress_indication += step
      percent += PERCENT_PROGRESS_STEP
  fp.close()
  return crc


def GetCrc(fname):
  """Get crc values from crc file."""
  fp = open(fname, "rb")
  crc = []
  for i in xrange(CRC_SIZE):
    byte = ord(fp.read(1))
    crc.append(byte)
  fp.close()
  return crc


def CheckCrc(path):
  """Check if crc in crc file matches calculated crc for data file."""
  calculated = CalculateCrc("%s/pbundle_0000" % path)
  existing = GetCrc("%s/pbundle_0000.crc" % path)
  if calculated == existing:
    print "Good crc in %s." % path
  else:
    print "** BAD CRC **"
    print path
    print "Existing: ", existing
    print "Calculated: ", calculated


def CheckIndex(path, total_packets):
  """Check if number of packets looks correct."""
  actual = os.path.getsize("%s/index" % path)
  expected = total_packets * INDEX_ENTRY_SIZE
  if actual == expected:
    print "Number of packets in %s/index looks good." % path
  else:
    print "** BAD INDEX SIZE **"
    print path
    print "Expected: ", expected
    print "Actual: ", actual


def main():
  """Main for check crc."""
  if len(sys.argv) != 5:
    print ("usage: %s <globe_path> <num_imagery_packets> <num_terrain_packets>"
           " <num_vector_packets>") % sys.argv[0]
    print ("  Packet numbers are first line of output from "
           "geportableglobebuilder.")
    print "  E.g. %s globe 32838 23881 2821" % sys.argv[0]
    sys.exit(0)

  base_dir = sys.argv[1]
  print "Please be patient, can take a while for large globes ..."
  CheckCrc("%s/data" % base_dir)
  CheckCrc("%s/qtp" % base_dir)
  total_packets = int(sys.argv[2]) + int(sys.argv[3]) + int(sys.argv[4])
  CheckIndex("%s/data" % base_dir, total_packets)


if __name__ == "__main__":
  main()
