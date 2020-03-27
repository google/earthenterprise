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


"""Tests Portable Server composite unpacker library.

Does basic tests to make sure that it can read files
and 3d and 2d packet data from globes and maps.
"""

import os
import glc_unpacker


class UnpackerError(Exception):
  """Thrown if unable to read from unpacker."""
  pass


class Unpacker(object):
  """Class for getting data from a packed file."""

  def __init__(self, packed_file, is_composite):
    self.file_ = packed_file
    self.reader_ = glc_unpacker.PortableGlcReader(packed_file)
    self.unpacker_ = glc_unpacker.GlcUnpacker(self.reader_, is_composite, False)
    self.data_loc_ = glc_unpacker.PackageFileLoc()
    if not os.path.isfile(self.file_):
      raise UnpackerError("Unable to open %s" % self.file_)

  def GetData(self):
    """Read data from packed file.

    Returns:
      Data from last found packet or file.
    """
    offset = 0L + (self.data_loc_.HighOffset() & 0xffffffff) << 32
    offset += (self.data_loc_.LowOffset() & 0xffffffff)
    size = self.data_loc_.LowSize()

    fp = open(self.file_, "rb")
    fp.seek(offset)
    content = fp.read(size)
    fp.close()
    return content

  def ReadFile(self, packed_file):
    """Reads file from packed file.

    Args:
      packed_file: Local path to packed file.
    Returns:
      File from packed file.
    Raises:
      UnpackerError: if unable to find file.
    """
    found = self.unpacker_.FindFile(packed_file, self.data_loc_)

    if found:
      return self.GetData()
    else:
      raise UnpackerError("Unable to read %s" % file)

  def ReadDataPacket(self, qtpath, packet_type, channel, layer_id):
    """Reads file from packed file.

    Args:
      qtpath: Quadtree path to the packet.
      packet_type: Type of packet being accessed.
      channel: Channel of packet being accessed.
      layer_id: Composite layer being accessed.
    Returns:
      File from packed file.
    Raises:
      UnpackerError: if unable to find packet.
    """
    found = self.unpacker_.FindDataPacket(
        qtpath, packet_type, channel, layer_id, self.data_loc_)

    if found:
      return self.GetData()
    else:
      raise UnpackerError("Unable to find packet.")

  def ReadQtPacket(self, qtpath, layer_id):
    """Reads quadtree packet at given address.

    Args:
      qtpath: Quadtree path to the packet.
      layer_id: Composite layer being accessed.
    Returns:
      Quadtree packet.
    Raises:
      UnpackerError: if unable to find packet.
    """
    found = self.unpacker_.FindQtpPacket(
        qtpath, glc_unpacker.kQtpPacket, 0, layer_id, self.data_loc_)

    if found:
      return self.GetData()
    else:
      raise UnpackerError("Unable to find packet.")

  def ReadDbRoot(self, layer_id):
    """Returns dbroot.

    Args:
      layer_id: Composite layer being accessed.
    Returns:
      DbRoot.
    Raises:
      UnpackerError: if unable to find dbroot.
    """
    found = self.unpacker_.FindQtpPacket(
        "0", glc_unpacker.kDbRootPacket, 0, layer_id, self.data_loc_)

    if found:
      return self.GetData()
    else:
      raise UnpackerError("Unable to find dbroot.")

  def ReadMetaDbRoot(self):
    """Returns meta dbroot.

    Returns:
      DbRoot.
    Raises:
      UnpackerError: if unable to find meta dbroot.
    """
    found = self.unpacker_.FindMetaDbRoot(self.data_loc_)

    if found:
      return self.GetData()
    else:
      raise UnpackerError("Unable to find meta dbroot.")

  def ReadMapDataPacket(self, qtpath, packet_type, channel, layer_id):
    """Returns packet at given address and channel.

    If packet is not found, throws an exception.

    Args:
      qtpath: the quadtree node of the map packet.
      packet_type: the type of data in the map packet.
      channel: the channel of the map packet.
      layer_id: Composite layer being accessed.
    Returns:
      The map packet itself.
    Raises:
      UnpackerError: if unable to find the packet.
    """
    found = self.unpacker_.FindMapDataPacket(
        qtpath, packet_type, channel, layer_id, self.data_loc_)

    if found:
      return self.GetData()
    else:
      raise UnpackerError("Unable to find packet.")


def ReadGoldStandard(gold_file):
  """Read in gold standard data."""
  fp = open("../test_data/gold_data/%s" % gold_file, "rb")
  gold_content = fp.read()
  fp.close()
  return gold_content


def RunTest(tally, test, unpacker):
  """Run a test and mark in tally if it is a success or failure."""
  try:
    if test(unpacker):
      tally[0] += 1
    else:
      print "FAIL:", test.func_name
      tally[1] += 1
  except UnpackerError:
    print "FAIL Unpacker Exception:", test.func_name
    tally[1] += 1


def TestGlobeImageryPacket(unpacker):
  """Test globe imagery packet is same as gold standard."""
  content = unpacker.ReadDataPacket("030132031",
                                    glc_unpacker.kImagePacket, 0, 0)
  if content == ReadGoldStandard("gold_globe_imagery_packet"):
    return True
  return False


def TestGlobeTerrainPacket(unpacker):
  """Test globe terrain packet is same as gold standard."""
  content = unpacker.ReadDataPacket("030132031",
                                    glc_unpacker.kTerrainPacket, 1, 0)
  if content == ReadGoldStandard("gold_globe_terrain_packet"):
    return True
  return False


def TestGlobeVectorPacket(unpacker):
  """Test globe vector packet is same as gold standard."""
  content = unpacker.ReadDataPacket("03013203",
                                    glc_unpacker.kVectorPacket, 5, 0)
  if content == ReadGoldStandard("gold_globe_vector_packet"):
    return True
  return False


def TestGlobeQtPacket(unpacker):
  """Test globe qt packet is same as gold standard."""
  content = unpacker.ReadQtPacket("03013203", 0)
  if content == ReadGoldStandard("gold_globe_qt_packet"):
    return True
  return False


def TestGlobeDbRoot(unpacker):
  """Test globe dbroot is same as gold standard."""
  content = unpacker.ReadDbRoot(0)
  if content == ReadGoldStandard("gold_globe_dbroot"):
    return True
  return False


def TestGlobeFile(unpacker):
  """Test globe file is same as gold standard."""
  if (unpacker.ReadFile("earth/info.txt") ==
      ReadGoldStandard("gold_globe_file.txt")):
    return True
  return False


def TestGlcMetaDbRoot(unpacker):
  """Test meta dbroot of glc."""
  if (unpacker.ReadMetaDbRoot() ==
      ReadGoldStandard("gold_glc_meta_dbroot")):
    return True

  return False


def TestGlcFiles(unpacker):
  """Test globe file is same as gold standard."""
  if (unpacker.ReadFile("earth/info.txt") !=
      ReadGoldStandard("gold_glc_info.txt")):
    print "Did not match: earth/info.txt"
    return False

  if (unpacker.ReadFile("earth/layer_info.txt") !=
      ReadGoldStandard("gold_glc_layer_info.txt")):
    print "Did not match: earth/info.txt"
    return False

  return True


def TestGlcLayer1ImageryPacket(unpacker):
  """Test globe imagery packet is same as gold standard."""
  content = unpacker.ReadDataPacket("030132031",
                                    glc_unpacker.kImagePacket, 0, 1)
  if content == ReadGoldStandard("gold_globe_imagery_packet"):
    return True
  return False


def TestGlcLayer1TerrainPacket(unpacker):
  """Test globe terrain packet is same as gold standard."""
  content = unpacker.ReadDataPacket("030132031",
                                    glc_unpacker.kTerrainPacket, 1, 1)
  if content == ReadGoldStandard("gold_globe_terrain_packet"):
    return True
  return False


def TestGlcLayer1VectorPacket(unpacker):
  """Test globe vector packet is same as gold standard."""
  content = unpacker.ReadDataPacket("03013203",
                                    glc_unpacker.kVectorPacket, 5, 1)
  if content == ReadGoldStandard("gold_globe_vector_packet"):
    return True
  return False


def TestGlcLayer1QtPacket(unpacker):
  """Test globe qt packet is same as gold standard."""
  content = unpacker.ReadQtPacket("03013203", 1)
  if content == ReadGoldStandard("gold_globe_qt_packet"):
    return True
  return False


def TestGlcLayer1DbRoot(unpacker):
  """Test globe dbroot is same as gold standard."""
  content = unpacker.ReadDbRoot(1)
  if content == ReadGoldStandard("gold_globe_dbroot"):
    return True
  return False


def TestMapImageryPacket(unpacker):
  """Test globe imagery packet is same as gold standard."""
  content = unpacker.ReadMapDataPacket("030132031",
                                       glc_unpacker.kImagePacket,
                                       1000, 0)
  if content == ReadGoldStandard("gold_map_imagery_packet.jpg"):
    return True
  return False


def TestMapVectorPacket(unpacker):
  """Test map vector packet is same as gold standard."""
  content = unpacker.ReadMapDataPacket("030132031",
                                       glc_unpacker.kVectorPacket,
                                       1002, 0)
  if content == ReadGoldStandard("gold_map_vector_packet.png"):
    return True
  return False


def TestMapFile(unpacker):
  """Test map imagery packet is same as gold standard."""
  if (unpacker.ReadFile("earth/info.txt") ==
      ReadGoldStandard("gold_map_file.txt")):
    return True
  return False


# The composite glc is made up of test.glb as layer 1
# and test.glm as layer 2.


def RunCompositeTests(tally):
  """Run all of the composite tests on a single Globe."""
  unpacker = Unpacker("../test_data/test.glc", True)
  RunTest(tally, TestGlcMetaDbRoot, unpacker)
  RunTest(tally, TestGlcFiles, unpacker)
  RunTest(tally, TestGlcLayer1ImageryPacket, unpacker)
  RunTest(tally, TestGlcLayer1TerrainPacket, unpacker)
  RunTest(tally, TestGlcLayer1VectorPacket, unpacker)
  RunTest(tally, TestGlcLayer1QtPacket, unpacker)
  RunTest(tally, TestGlcLayer1DbRoot, unpacker)


def Run3dTests(tally):
  """Run all of the 3d tests on a single Globe."""
  unpacker = Unpacker("../test_data/test.glb", False)
  RunTest(tally, TestGlobeImageryPacket, unpacker)
  RunTest(tally, TestGlobeTerrainPacket, unpacker)
  RunTest(tally, TestGlobeVectorPacket, unpacker)
  RunTest(tally, TestGlobeQtPacket, unpacker)
  RunTest(tally, TestGlobeDbRoot, unpacker)
  RunTest(tally, TestGlobeFile, unpacker)


def Run2dTests(tally):
  """Run all of the 2d tests on a single Map."""
  unpacker = Unpacker("../test_data/test.glm", False)
  RunTest(tally, TestMapVectorPacket, unpacker)
  RunTest(tally, TestMapImageryPacket, unpacker)
  RunTest(tally, TestMapFile, unpacker)


def main():
  """Main for test."""
  tally = [0, 0]
  RunCompositeTests(tally)
  Run3dTests(tally)
  Run2dTests(tally)
  if tally[1] == 0:
    print "SUCCESS:",
  else:
    print "FAIL:",

  print "%d tests passed. %d tests failed." % (tally[0], tally[1])
if __name__ == "__main__":
  main()
