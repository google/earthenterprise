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

"""Portable Installer Script."""


import os
import sys
import tkFileDialog
import Tkinter
import tkMessageBox


class InstallerGUI(object):
  """Create the GUI for users to choose the installation directory."""

  def DirectoryInputDialog(self, title, initial_dir):
    """Create the Browse dialogbox for user to input installationdirectory."""
    #Call the tk librery to provide GUI
    root = Tkinter.Tk()
    root.title(title)
    root.withdraw()
    options = {}
    options['mustexist'] = False
    options['title'] = title
    options['initialdir'] = initial_dir
    created_by_us = False
    if not os.path.exists(initial_dir):
      os.makedirs(initial_dir)
      created_by_us = True

    choice = tkFileDialog.askdirectory(**options)
    if not choice:
      tkMessageBox.showerror('Invalid Choice',
                             'The directory name cannot be empty')
      sys.exit(-1)
    if os.path.exists(choice) and not os.path.isdir(choice):
      tkMessageBox.showerror('Invalid Choice',
                             ('There is a file with same name as the chosen'
                              'directory.'))
      sys.exit(-1)
    if created_by_us:
      os.removedirs(os.path.normpath(initial_dir))
    if not os.path.exists(choice):
      os.makedirs(os.path.normpath(choice))
    return choice


def main():
  gui_handler = InstallerGUI()
  bfile = open('geesetvar.bat', 'w')
  home = os.path.expanduser('~')
  server_dir = os.getenv('GEE_PORTABLE_SILENT_INSTALLER_HOME')
  if not server_dir:
    server_dir = os.path.join(home, 'GEE\Portable\Tools')
    server_dir = gui_handler.DirectoryInputDialog(
        'Choose GEE portable tools installation directory:', server_dir)
  data_dir = os.getenv('GEE_PORTABLE_SILENT_INSTALLER_DATA')
  if not data_dir:
    data_dir = os.path.join(home, 'GEE\Portable\Data')
    data_dir = gui_handler.DirectoryInputDialog(
        'Choose GEE portable data installation directory:', data_dir)
  search_dir = os.getenv('GEE_PORTABLE_SILENT_INSTALLER_SEARCH')
  if not search_dir:
    search_dir = os.path.join(home, 'GEE\Portable\Search')
    search_dir = gui_handler.DirectoryInputDialog(
        'Choose GEE portable search installation directory:', search_dir)
  ext_dir = os.getenv('GEE_PORTABLE_SILENT_INSTALLER_EXT')
  if not ext_dir:
    ext_dir = os.path.join(home, 'GEE\Portable\Ext')
    ext_dir = gui_handler.DirectoryInputDialog(
        'Choose GEE portable extension installation directory:', ext_dir)
  server_dir = os.path.normpath(server_dir)
  data_dir = os.path.normpath(data_dir)
  lines = [
      '@Echo off',
      'set GEE_PORTABLE_SILENT_INSTALLER_HOME=%s' % server_dir,
      'set GEE_PORTABLE_SILENT_INSTALLER_DATA=%s' % data_dir,
      'set GEE_PORTABLE_SILENT_INSTALLER_SEARCH=%s' % search_dir,
      'set GEE_PORTABLE_SILENT_INSTALLER_EXT=%s' % ext_dir,
  ]
  for line in lines:
    print >>bfile, line
  bfile.close()

  # Update portable.cfg file.
  fp = open('portable.cfg', 'w')
  lines = [
      'port 9335',
      'globes_directory %s' % data_dir,
      'search_services_directory %s' % search_dir,
      'ext_service_directory %s' % ext_dir,
      'database file',
      'local_override True',
      'globe_name tutorial_pier39.glb'
  ]
  for line in lines:
    print >>fp, line
  fp.close()

  # Update uninstall.bat file.
  current_content = open('uninstall.bat', 'r').read()
  fp = open('uninstall.bat', 'w')
  shortcut_dir = os.getenv('GEE_PORTABLE_SILENT_INSTALLER_DSHORTCUT')
  if not shortcut_dir:
    shortcut_dir = os.path.join(os.getenv('USERPROFILE'), 'Desktop')
  menu_dir = os.getenv('GEE_PORTABLE_SILENT_INSTALLER_SSHORTCUT')
  if not menu_dir:
    menu_dir = os.path.join(os.getenv('USERPROFILE'), 'Start Menu')
  lines = [
      '@Echo off',
      '',
      'REM [1] get all the installation folder to be uninstalled.',
      'set data_dir=%s' % data_dir,
      'set tools_dir=%s' % server_dir,
      'set shortcut_dir=%s' % os.path.normpath(shortcut_dir),
      'set menu_dir=%s' % os.path.normpath(menu_dir),
      '']
  for line in lines:
    print >>fp, line
  print >>fp, current_content
  fp.close()

if __name__ == '__main__':
  main()
