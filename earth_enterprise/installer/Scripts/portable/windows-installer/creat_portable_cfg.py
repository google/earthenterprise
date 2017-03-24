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


def main():
  # Update portable.cfg file. %YourDirectoryHere% is a place holder to be
  # updated during installation using default directory or from user input.
  fp = open('portable.cfg', 'w')
  lines = [
      'port 9335',
      'globes_directory %YourDataDirectoryHere%',
      'search_services_directory %YourDirectoryHere%Search',
      'ext_service_directory %YourDirectoryHere%Ext',
      'database file',
      'local_override True',
      'fill_missing_map_tiles True',
      'max_missing_map_tile_ancestor 24',
      'globe_name tutorial_pier39.glb'
  ]
  for line in lines:
    print >>fp, line
  fp.close()

if __name__ == '__main__':
  main()
