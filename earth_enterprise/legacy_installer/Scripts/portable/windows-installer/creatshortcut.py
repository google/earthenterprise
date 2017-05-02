#!/usr/bin/python2.4
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

"""Create Shortcut Scripts."""


import optparse
import os
import tempfile


def CreateShortCuts(name, target, wdir, icon, command_line, desktop_path,
                    menu_path):
  """This Module create the portable server shortcuts on dektop and showmenu.
  This module will create the temprory vbscripts files and run these
  scripts using dos command cscript /b to generate shortcuts. After execution
  scripts will be deleted."""

  (fd, fname) = tempfile.mkstemp(suffix='.vbs',
                                 prefix='gee', dir=None, text=True)
  fp = open(fname, 'w')
  start_in = wdir
  if target.endswith('.py'):
    python_exe = os.path.join(wdir, 'gepython\python.exe')
    if os.path.exists(python_exe):
      dt_target = '"%s"' % python_exe
      dt_arguments = '"""%s"" %s"' % (os.path.join(wdir, target), command_line)
    else:
      dt_target = '"%s"' % os.path.join(wdir, 'portable.exe')
      dt_arguments = '"%s"' % (command_line)

  lines = ['Set oWS = WScript.CreateObject("WScript.Shell")']
  # During uninstall we need to remove the directory containing the currently
  # running script also.But:
  # 1. deleting a script while running it, or
  # 2. deleting a directory when inside it are not possible in Windows..
  # So we do the workaround as to copy the install batch
  # file to temprory location and run the sciript. once it is done unistall
  # batch file is removed from temprory location also.
  if target.lower() == 'uninstall.bat':
    dt_target = '"%COMSPEC%"'
    dt_arguments = ('"/c copy ""%s"" %%TMP%%\GEE_UNINSTALL.bat &&'
                    '%%TMP%%\GEE_UNINSTALL.bat"') % (
                        os.path.join(wdir, target))
    start_in = '%TMP%'
  else:  # Don't create desktop short-cut for uninstall
    # look for user provided inputs if any for desktop shortcut.
    linkfilepath = 'sLinkFile = oWs.SpecialFolders("Desktop") + "\%s"' % name
    if desktop_path:
      linkfilepath = 'sLinkFile = "%s" + "\%s"' % (desktop_path, name)
    if command_line == 'select':
      # Create a batch file which can be used for file association
      exe_name = os.path.join(wdir, 'select.bat')
      exe_fp = open(exe_name, 'w')
      print >>exe_fp, 'cd "%s"' % start_in
      # Need not do the special quotes that we did for VB script, as
      # we are directly printing to a batch file
      if os.path.exists(python_exe):
        dt_arguments = '"%s" %s' % (os.path.join(wdir, target), command_line)
      else:
        dt_arguments = '%s' % (command_line)
      print >>exe_fp, '%s %s %%*' % (dt_target, dt_arguments)
      exe_fp.close()
      dt_target = '"%s"' % exe_name
      dt_arguments = '""'

      #Adding windows file association for .glb files.
      lines.extend([
          'fext = "glb"',
          'prgid = "glb File"',
          'des = "glb File"',
          'extanm = "%s"' % exe_name,
          'oWs.regwrite "HKCR\." + fext + "\\" , prgid , "REG_SZ"',
          'oWs.regwrite "HKCR\\" + prgid + "\\", des , "REG_SZ"',
          ('oWs.regwrite "HKCR\\" + prgid + "\DefaultIcon\\", '
           '"""%s"", 0", "REG_SZ"' % os.path.join(wdir, 'icons\GLB.ico')),
          'oWs.regwrite "HKCR\\" + prgid + "\Shell\\","Open"',
          ('oWs.regwrite "HKCR\\" + prgid + "\Shell\Open\Command\\", """" + '
           'extanm + """ ""%1""", "REG_SZ"')])

      #Adding windows file association for .glm files.
      lines.extend([
          'fext = "glm"',
          'prgid = "glm File"',
          'des = "glm File"',
          'extanm = "%s"' % exe_name,
          'oWs.regwrite "HKCR\." + fext + "\\" , prgid , "REG_SZ"',
          'oWs.regwrite "HKCR\\" + prgid + "\\", des , "REG_SZ"',
          ('oWs.regwrite "HKCR\\" + prgid + "\DefaultIcon\\", '
           '"""%s"", 0", "REG_SZ"' % os.path.join(wdir, 'icons\GLM.ico')),
          'oWs.regwrite "HKCR\\" + prgid + "\Shell\\","Open"',
          ('oWs.regwrite "HKCR\\" + prgid + "\Shell\Open\Command\\", """" + '
           'extanm + """ ""%1""", "REG_SZ"')])

      #Adding windows file association for .glc files.
      lines.extend([
          'fext = "glc"',
          'prgid = "glc File"',
          'des = "glc File"',
          'extanm = "%s"' % exe_name,
          'oWs.regwrite "HKCR\." + fext + "\\" , prgid , "REG_SZ"',
          'oWs.regwrite "HKCR\\" + prgid + "\\", des , "REG_SZ"',
          ('oWs.regwrite "HKCR\\" + prgid + "\DefaultIcon\\", '
           '"""%s"", 0", "REG_SZ"' % os.path.join(wdir, 'icons\GLC.ico')),
          'oWs.regwrite "HKCR\\" + prgid + "\Shell\\","Open"',
          ('oWs.regwrite "HKCR\\" + prgid + "\Shell\Open\Command\\", """" + '
           'extanm + """ ""%1""", "REG_SZ"')])

      #Creating shortcuts
      lines.extend([
          linkfilepath,
          'Set oLink = oWS.CreateShortcut(sLinkFile)',
          'oLink.TargetPath = %s' % dt_target,
          'oLink.Arguments = %s' % dt_arguments,
          'oLink.IconLocation = "%s, 0"' % os.path.join(wdir, icon),
          'oLink.WorkingDirectory = "%s"' % start_in,
          'oLink.Save'])

  # look for user provided inputs if any for show-menu shortcut.
  showmenu_path = ('strDirectory = oWs.SpecialFolders("StartMenu")'
                   '+ "\gee_portable"')
  if menu_path:
    showmenu_path = 'strDirectory = "%s" + "\gee_portable"' % menu_path
  lines.extend([
      'Set objFSO = CreateObject("Scripting.FileSystemObject")',
      'Dim strDirectory',
      showmenu_path,
      'If Not objFSO.FolderExists(strDirectory) Then',
      '  objFSO.CreateFolder(strDirectory)',
      'End If',
      'sLinkFile = strDirectory + "\%s"' % name,
      'Set oLink = oWS.CreateShortcut(sLinkFile)',
      'oLink.TargetPath = %s' % dt_target,
      'oLink.Arguments = %s' % dt_arguments,
      'oLink.IconLocation = "%s, 0"' % os.path.join(wdir, icon),
      'oLink.WorkingDirectory = "%s"' % start_in,
      'oLink.Save'])
  for line in lines:
    print >>fp, line
  fp.close()
  os.close(fd)
  os.system('CSCRIPT //B %s' % fname)
  os.remove(fname)
  if command_line == 'select':
    os.system('assoc .glb=glb file')
    os.system('assoc .glm=glm file')
    os.system('assoc .glc=glc file')


def main():
  """Creates desktop shortcut and start menu for:
     start portable server, start remote server and
     uninstall portable server."""
  usage = 'usage: %prog[options] -help or -h'
  parser = optparse.OptionParser(usage=usage, version='%prog 1.0')
  parser.add_option('-D', '--Desktop_Shorcut', action='store', type='string',
                    dest='desktop_path',
                    default=None,
                    help='Dektop path for creating shortcuts.'
                    '[default: %default]')
  parser.add_option('-S', '--ShowMenu_Shorcut', action='store', type='string',
                    dest='menu_path',
                    default=None,
                    help='Main Menu path for adding Menu options.'
                    '[default: %default]')
  (options, _) = parser.parse_args()

  #Create list of Desktop shourtcuts to be added.
  shortcut_list = [
      ('Uninstall.BAT', 'PortableUninstaller.lnk', 'icons\uninstall.ico', None),
      ('portable.py', 'StartServer.lnk', 'icons\Portable_Server.ico', 'select')
  ]

  for v in shortcut_list:
    print 'Creating windows shortcut for %s'% v[1]
    CreateShortCuts(v[1], v[0], os.getcwd(), v[2], v[3],
                    options.desktop_path, options.menu_path)

if __name__ == '__main__':
  main()
