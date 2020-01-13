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
#
# Simple tool for pounding the server to make sure it properly handles
# concurrent requests for data from the same globe.
# Expects Atlanta globe (Atlanta.glb) to be available
# on the server.
# Checking the data makes sure that the data is consistent on every read,
# not that it is correct in some absolute sense.
#

import sys
import urllib2

DEFAULT_HOST = "hostname.com"

TEST_PACKET0_ADDRESS = ("http://%s/portable/"
                        "Atlanta.glb/flatfile?f1-03103032222212031-i.5")
TEST_PACKET1_ADDRESS = ("http://%s/portable/"
                        "Atlanta.glb/flatfile?f1-03103032222212032-i.5")


def InitGoldData(packet_addresses, gold_data, host):
  """Gold standard is data from first read."""
  packet_addresses.append(TEST_PACKET0_ADDRESS % host)
  packet_addresses.append(TEST_PACKET1_ADDRESS % host)
  gold_data.append(urllib2.urlopen(packet_addresses[0]).read())
  gold_data.append(urllib2.urlopen(packet_addresses[1]).read())


def CheckData(packet1, packet2, gold_data):
  """Check that data matches the gold standard."""
  if packet1 != gold_data[0]:
    raise Exception("Packet1 data does not match gold standard.")
  if packet2 != gold_data[1]:
    raise Exception("Packet2 data does not match gold standard.")


def RunTrial(packet_addresses, check_data, gold_data):
  """Read packets for next trial."""
  packet0 = urllib2.urlopen(packet_addresses[0]).read()
  packet1 = urllib2.urlopen(packet_addresses[1]).read()
  if check_data == "t":
    CheckData(packet0, packet1, gold_data)


def Usage():
  """Show usage information."""
  print "usage: pound.py host [num_trials] [check_data]"
  print "  host: host server (default = hotname)"
  print "  num_trials: number of times data is read (default = 50000)"
  print "  check_data: (t or f) whether data is checked (default = f)"
  print "  E.g. ./pound.py localhost 10000 t"


def main():
  """Main for pounder."""
  num_trials = 50000
  check_data = "f"
  host = DEFAULT_HOST
  if len(sys.argv) == 1:
    Usage()
    return

  if len(sys.argv) > 1:
    arg1 = sys.argv[1]
    if arg1 == "-h" or arg1 == "--help":
      Usage()
      return
    else:
      host = arg1

  if len(sys.argv) > 2:
    num_trials = int(sys.argv[2])

  if len(sys.argv) > 3:
    check_data = sys.argv[3]

  print "Sever:", host
  print "Trials:", num_trials
  print "Checking data:", check_data
  had_error = False
  gold_data = []
  packet_addresses = []
  InitGoldData(packet_addresses, gold_data, host)
  for i in xrange(num_trials):
    try:
      RunTrial(packet_addresses, check_data, gold_data)
    except urllib2.URLError, e:
      had_error = True
      print "Error at", i, ":", e
      break

  if not had_error:
    print "No errors.", num_trials, "trials."

  print "Done."


if __name__ == "__main__":
  main()
