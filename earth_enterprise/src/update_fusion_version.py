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


"""Update the Fusion version string.

It takes two flags --long long_version_name and --short short_version_name.
Then it opens all the files need to be updated and changes from current
long_version_name and short_version_name to these new values.

Note: The long version name needs to start with short version name.

Example Usage:
  ./update_fusion_version.py --long "3.2.0" --short "3.2"

"""


import datetime
import fileinput
import os
import sys

from pyglib import app
from pyglib import flags


FLAGS = flags.FLAGS

flags.DEFINE_string('long',
                    '',
                    'Long version string for fusion (e.g 3.2.0')

flags.DEFINE_string('short',
                    '',
                    'Short version string for fusion (e.g 3.2')


def FindUpdateCurrentVersion(fusion_version_file, long_version, short_version,
                             year):
  """Find and update long and short version names in the fusion_version_file.

  Args:
    fusion_version_file: Absolute filename for fusion_version.txt
    long_version: The new long_version to update to.
    short_version: The new short_version to update to.
    year: The current year to be used in copyright statement.

  Returns:
    A couple of string (in a list) representing current long and short
    respectively.

  Raises:
    AssertionError: Whenever anything fails.
  """
  cmd = 'cd %s; g4 open %s' % (os.path.dirname(fusion_version_file),
                               os.path.basename(fusion_version_file))
  if os.system(cmd):
    raise AssertionError('Cannot run command "%s"' % cmd)
  stage = 0  # not yet reached long_version
  for line in fileinput.FileInput(fusion_version_file, inplace=1):
    if stage == 0:
      if not line.startswith('#'):
        stage = 1  # long_version reached
        old_long = line[:-1]
        print long_version
      else:
        # TODO: Create script to do this for all copyrights.
        if line.startswith('# Copyright'):
          print '# Copyright %d Google Inc. All Rights Reserved.' % year
        else:
          print line,
    elif stage == 1:
      old_short = line[:-1]
      print short_version
      stage = 2  # short version reached
    else:
      raise AssertionError('Cannot comprehend line "%s" in %s' % (
          line, fusion_version_file))
  return (old_long, old_short)


def ChangeVersionInInstallerFiles(
    common_prefix, old_long, new_long, old_short, new_short):
  """For installer xml files change from old version to new version.

  Args:
    common_prefix: Common root for all files to change.
    old_long: Current long version string.
    new_long: New long version string.
    old_short: Current short version string.
    new_short: New short version string.

  Raises:
    AssertionError: Whenever anything fails.
  """
  installer_files = ('installer/config/GoogleEarthInstaller.iap_xml',
                     'installer/config/GoogleFusionInstaller.iap_xml',
                     'installer/config/GoogleFusionToolsInstaller.iap_xml')
  old_long_cdata = 'CDATA[%s]' % (old_long)
  old_short_cdata = 'CDATA[%s]' % (old_short)
  new_long_cdata = 'CDATA[%s]' % (new_long)
  new_short_cdata = 'CDATA[%s]' % (new_short)
  long_key = 'CDATA[$LONG_VERSION$]'
  short_key = 'CDATA[$SHORT_VERSION$]'
  for file_name in installer_files:
    file_name = '%s/%s' % (common_prefix, file_name)
    cmd = 'cd %s; g4 open %s' % (os.path.dirname(file_name),
                                 os.path.basename(file_name))
    if os.system(cmd):
      raise AssertionError('Cannot run command "%s"' % cmd)

    in_defered_mode = False
    defered_lines = []
    for line in fileinput.FileInput(file_name, inplace=1):
      if not in_defered_mode:
        if line.find(old_long_cdata) >= 0 or line.find(old_short_cdata) >= 0:
          in_defered_mode = True
          defered_lines.append(line)
        else:
          line = line.replace(old_long, new_long)
          print line,
      else:
        long_key_found = (line.find(long_key) >= 0)
        if long_key_found or (line.find(short_key) >= 0):
          if long_key_found:
            print defered_lines[0].replace(old_long_cdata, new_long_cdata),
          else:
            print defered_lines[0].replace(old_short_cdata, new_short_cdata),
          for index in range(1, len(defered_lines)):
            print defered_lines[index],
          print line,
          defered_lines = []
          in_defered_mode = False
        else:
          defered_lines.append(line)


def main(argv):
  if not (len(argv) == 1 and FLAGS.long and FLAGS.short and
          FLAGS.long.startswith(FLAGS.short)):
    sys.stderr.write('Wrong Usage of the script %s \n\n' % argv[0])
    sys.stderr.write(__doc__)
    sys.exit(-1)
  script_path = os.path.abspath(argv[0])
  common_prefix = os.path.dirname(os.path.dirname(script_path))
  fusion_version_file = '%s/%s' % (common_prefix, 'src/fusion_version.txt')
  (old_long, old_short) = FindUpdateCurrentVersion(fusion_version_file,
                                                   FLAGS.long, FLAGS.short,
                                                   datetime.datetime.now().year)
  ChangeVersionInInstallerFiles(
      common_prefix, old_long, FLAGS.long, old_short, FLAGS.short)


if __name__ == '__main__':
  app.run()
