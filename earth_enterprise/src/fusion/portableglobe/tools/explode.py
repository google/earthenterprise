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

"""Extracts tiles from a glm or 2d glc.

Tiles are moved into a directory tree of the
form maptiles/<z>/<x>/<y>/<channel>.<sfx> where
x, y, z are the location and LOD, channel is
the channel the tile came from, and sfx is the
tile type (jpg or png).

Most of the work is done by geglxinfo. This wrapper
just provides a way to distribute the job over
multiple processes.

On standard Ubuntu box (12 cores), we didn't see a lot of
improvement after 5 processes. Presumably, it becomes
I/O-bound pretty quickly.
"""

import argparse
import os
import time


GEGLXINFO_PATH = "geglxinfo"


def Run(cmd):
  """Run the command in bash and return the results."""
  fp = os.popen(cmd)
  results = fp.read()
  fp.close()
  return results


def Launch(cmd):
  """Launch the command in bash and don't wait for results."""
  os.popen(cmd)


def GetNumPackets(path):
  """Returns number of packets or list of number of packets in each layer.

  For glms we expect something like:
    6408 packets
  For glcs, we expect something like:
    3d glc
    Number of layers: 2
    layer 0: 685 packets
    layer 1: 5752 packets
    6437 packets

  Args:
      path: path to the glx.

  Returns:
    List of number of packets per layer.
  """
  cmd = "%s --glx %s --number_of_packets" % (GEGLXINFO_PATH, path)
  size = 0
  num_packets = []

  for line in Run(cmd).split("\n"):
    words = line.strip().split(" ")
    if words[-1] == "packets":
      # Use first line to determine expected size of subsequent lines
      # containing layer information.
      if size == 0:
        size = len(words)
        num_packets.append(int(words[-2]))
      # Subsequent layer info should match size of first line.
      elif size == len(words):
        num_packets.append(int(words[-2]))
  return num_packets


def ExtractPackets(path, start_idx, end_idx=None, layer_idx=None):
  """Extract packets for given range of index entries."""
  cmd = "%s --glx %s --extract_packets --start_idx %d" % (
      GEGLXINFO_PATH, path, start_idx)
  if end_idx is not None:
    cmd += " --end_idx %d" % end_idx
  if layer_idx is not None:
    cmd += " --layer_idx %d" % layer_idx
  # Run in background and discard output
  cmd += " 2>&1 > /dev/null &"
  print cmd
  Launch(cmd)
  print "... launched."


def Wait(names):
  """Wait until there are no processes with the given names."""
  cmd = "ps aux"
  for name in names:
    cmd += " | grep '%s'" % name
  cmd += " | grep -v 'grep'"
  while True:
    time.sleep(3)
    results = Run(cmd).strip()
    print "... running ..."
    if not results:
      break


def ExplodeGlm(path, num_processes):
  """Extract all tiles in the glm to disk."""
  print "Exploding %s ..." % path
  num_packets = GetNumPackets(path)[0]
  packets_per_process = num_packets / num_processes
  next_idx = 0
  for unused_i in xrange(num_processes - 1):
    ExtractPackets(path, next_idx, next_idx + packets_per_process)
    next_idx += packets_per_process
  ExtractPackets(path, next_idx)
  Wait(["geglxinfo", path])
  print "Done!"


def ExplodeGlc(path, layer_idx, num_processes):
  """Extract all tiles in the given glc layer to disk."""
  print "Exploding layer %d of %s ..." % (layer_idx, path)
  num_packets = GetNumPackets(path)[layer_idx]
  packets_per_process = num_packets / num_processes
  next_idx = 0
  for unused_i in xrange(num_processes - 1):
    ExtractPackets(path, next_idx, next_idx + packets_per_process,
                   layer_idx=layer_idx)
    next_idx += packets_per_process
  ExtractPackets(path, next_idx, layer_idx=layer_idx)
  Wait(["geglxinfo", path])
  print "Done!"


def main():
  parser = argparse.ArgumentParser(
      description="Extract tiles from a 2d Portable.")
  parser.add_argument("--glx",
                      help="glm or 2d glc to expand.",
                      required=True)
  parser.add_argument("--layer_idx",
                      help="Index of layer in glc to expand starting at 0.",
                      default=0,
                      type=int,
                      required=False)
  parser.add_argument("--num_processes",
                      help="Number of processes to use to explode the glx.",
                      default=1,
                      type=int,
                      required=False)
  args = parser.parse_args()

  if os.path.exists("maptiles"):
    print "** ERROR **: 'maptiles' directory already exists."
    parser.print_help()
    exit()

  if not os.path.exists(args.glx):
    print "** ERROR **: %s was not found." % args.glx
    parser.print_help()
    exit()

  if args.num_processes < 1:
    print "Setting number of processes to 1."
    args.num_processes = 1

  suffix = args.glx[-4:]
  if suffix == ".glm":
    if args.layer_idx:
      print "** WARNING **: Layer idx is ignored. Only used for glcs."
      parser.print_help()
    ExplodeGlm(args.glx, args.num_processes)
  elif suffix == ".glc":
    # Default is the first layer. This is handy for gme cuts which
    # tend to only have a single imagery layer at the moment.
    ExplodeGlc(args.glx, args.layer_idx, args.num_processes)
  else:
    print "** ERROR **: %s is not a glm or glc." % args.glx
    parser.print_help()


if __name__ == "__main__":
  main()
