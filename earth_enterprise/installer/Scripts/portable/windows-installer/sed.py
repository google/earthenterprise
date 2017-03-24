#!/usr/bin/python2.4
#
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

"""Script to create .SED file for creating Window self extracting installer.

   Ussage: sed.py <flatten-dir> <SED-tamplateFile> <destination-dir>
   flatten-dir :      path of flatten directory
   SED-tamplateFile : the basic tamplete file to which the data
   is appended.
   destination-dir :  path where the final SED file will be stored.
"""


import os
import sys


class Walker(object):
  """Walks the directory tree and prints the details."""

  def __init__(self, directory):
    self.str1_ = ''
    self.str2_ = '[SourceFiles]\n'
    self.str3_ = ''
    self.file_cnt_ = 0
    self.dir_cnt_ = 0
    os.path.walk(directory, self.Callback, '')

  def Callback(self, _, directory, files):
    """This module will add the file and directory details to the lists."""
    contains_files = False
    if not len(files) > 1:
      return
    for portablefile in files:
      if not os.path.isdir(os.path.join(directory, portablefile)):
        contains_files = True
    if not contains_files:
      return
    self.str2_ += 'SourceFiles%s=%s\\\n' %(self.dir_cnt_, os.path.abspath(
        directory))
    self.str3_ += '[SourceFiles%s]\n' % self.dir_cnt_
    self.dir_cnt_ += 1
    for portablefile in files:
      if os.path.isdir(os.path.join(directory, portablefile)):
        continue
      self.str1_ += 'FILE%s="%s"\n' % (self.file_cnt_, portablefile)
      self.str3_ += '%%FILE%s%%=\n' % self.file_cnt_
      self.file_cnt_ += 1

if __name__ == '__main__':
  walker = Walker(sys.argv[1])
  template = sys.argv[2]
  filename = sys.argv[3]
  fp = open(filename, 'w')
  fp.write(open(template, 'r').read())
  fp.write(walker.str1_)
  fp.write(walker.str2_)
  fp.write(walker.str3_)
  fp.close()
  print 'Finished preparing SED file'
