#!/opt/google/gepython/Python-2.7.5/bin/python
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


import os
import re
import unittest
import grp


class TestPostgres(unittest.TestCase):

  def testPostgresMaxStackDepth(self):
    """Check needed running services."""

    postgres_config_path = '/var/opt/google/pgsql/data/postgresql.conf'
    # if process user is not root...
    if os.geteuid() != 0:
      groupIds = os.getgroups()
      group = grp.getgrnam('gegroup')[2]
      msg_error = 'Membership in gegroup or root is needed to access %s file.' \
        % postgres_config_path
      self.assertIn(group, groupIds, msg=msg_error)

    try:
      with open(postgres_config_path, 'r') as f:
        for line in f:
          if 'max_stack_depth' in line:
            regex = r'\d+[A-Z-a-z]+'
            max_stack_depth = re.findall(regex, line)
            error_msg = 'Postgres max stack depth: %s' % max_stack_depth[0]
            self.assertIsNotNone(max_stack_depth, msg=error_msg)
            break
    except IOError as e:
      raise AssertionError('Postgres max stack error: %s', e)

if __name__ == '__main__':
  unittest.main()
