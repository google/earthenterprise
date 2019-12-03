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


"""This is the top level program for starting all other programs.

We are using this technique of a single top level main, so that py2exe can
create a single binary rather than multiple ones, having same libraries repeated
many times. The consequence will be the call changing like following:
portable_server.exe -> portable.exe server
Note that all jobs will run in background and this top level will be free for
taking other jobs. Whether to continue the job or lunch and quit will be
decissions of child jobs.
"""


import os
import subprocess
import sys

if __name__ == '__main__':
  assert len(sys.argv) > 1, 'Bad usage of "%s", needs atleast 1 argument' % (
      sys.argv[0])
  # Cannot use os.fork for this purpose, as that is not available in windows
  if sys.argv[1] == 'run_in_background':
    # Run this job in background
    new_list = sys.argv
    new_list.pop(1)  # remove the run_in_background parameter
    if os.path.abspath(sys.executable) != os.path.abspath(sys.argv[0]):
      # running with interpreter, so add interpreter
      new_list.insert(0, sys.executable)
    subprocess.Popen(new_list)
    sys.exit(0)
  first_argument = sys.argv[1]
  sys.argv.pop(1)
  if first_argument == 'server':
    import portable_server
    portable_server.main()
  elif first_argument == 'select':
    import globe_selector
    globe_selector.main(sys.argv)
  # The following are for installer
  elif first_argument == 'get_yes_no':
    import get_yes_no
    get_yes_no.main(sys.argv)
  elif first_argument == 'creatshortcut':
    import creatshortcut
    creatshortcut.main()
  elif first_argument == 'get_user_dir_choice':
    import get_user_dir_choice
    get_user_dir_choice.main()
  elif first_argument == 'check_server_running':
    import check_server_running
    check_server_running.main()
  else:
    assert False, 'Bad command line argument "%s" to "%s"' % (first_argument,
                                                              sys.argv[0])
