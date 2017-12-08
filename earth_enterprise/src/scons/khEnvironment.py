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

"""The build environment, builders, actions and helper methods.

Define build environments, builders, actions and helper methods in this
central place and reuse it in all SConscripts as much as possible.
"""

import os
import os.path
import subprocess
import sys
import time
from datetime import datetime
import SCons
from SCons.Environment import Environment


def AppendToFlags(target, env, key, to_add):
  if not SCons.Util.is_List(to_add):
    to_add = [to_add]
  tmp = target.get(key, env.get(key, []))
  if not SCons.Util.is_List(tmp):
    tmp = [tmp]
  target[key] = tmp + to_add


def PrependToFlags(target, env, key, to_add):
  if not SCons.Util.is_List(to_add):
    to_add = [to_add]
  tmp = target.get(key, env.get(key, []))
  if not SCons.Util.is_List(tmp):
    tmp = [tmp]
  target[key] = to_add + tmp


# Qt stuff - yanked from scons-users mailing list archive
def Emitter(env, target, source):
  base = SCons.Util.splitext(str(source[0].name))[0]
  uidir = os.path.join(str(target[0].get_dir()), '.ui')
  hfile = os.path.join(uidir, base+'.h')
  cppfile = os.path.join(uidir, base+'.cpp')
  mocdir = os.path.join(str(target[0].get_dir()), '.moc')
  mocfile = os.path.join(mocdir, 'moc_' + base + '.cpp')
  env.uic_impl(cppfile, [hfile, source])
  env.moc(mocfile, hfile)
  return [hfile], [source]

uic = SCons.Builder.Builder(action='$UIC $SOURCE -o $TARGET',
                            emitter=Emitter)
uic_impl = SCons.Builder.Builder(action='$UIC -o $TARGET -impl $SOURCES')
moc = SCons.Builder.Builder(action='$MOC -o $TARGET $SOURCE')


# pylint: disable=W0104
def CleanupLibFlags(prefix, a_list, suffix, stripprefix, stripsuffix, env):
  a_list = env['_oldstripixes'](prefix, a_list, suffix,
                                stripprefix, stripsuffix, env)
  return a_list


def AddSourceScannerToTargets(target, source, env):
  for t in target:
    if t.source_scanner is None:
      key = t.scanner_key()
      scanner = env.get_scanner(key)
      if scanner:
        t.source_scanner = scanner
  return (target, source)


idl_h_builder = SCons.Builder.Builder(
    action='$KHIDL --hfile $TARGET $SOURCE',
    suffix='.h',
    src_suffix='.idl',
    # emitter=AddSourceScannerToTargets,
)

idl_impl_h_builder = SCons.Builder.Builder(
    action='$KHIDL --impl_hfile $TARGET $SOURCE',
    suffix='_impl.h',
    src_suffix='.idl',
    # emitter=AddSourceScannerToTargets,
)

idl_cpp_builder = SCons.Builder.Builder(
    action='$KHIDL --cppfile $TARGET $SOURCE',
    suffix='.cpp',
    src_suffix='.idl',
    # emitter=AddSourceScannerToTargets,
)


def AliasBuilder(env, target, source):
  (env, target, source) = (env, target, source)  # Silence gpylint


def NoOutput(target, source, env):
  (env, target, source) = (env, target, source)  # Silence gpylint
  return None


my_alias_builder = SCons.Builder.Builder(
    action=SCons.Action.Action(AliasBuilder, NoOutput),
    target_factory=SCons.Node.Alias.default_ans.Alias,
    source_factory=SCons.Node.FS.Entry,
    multi=1,
    is_explicit=None,
    name='my_alias_builder')


def WriteToFileFunc(file_name, strn):
  """Writes strn to file_name.

  Args:
   file_name: The file to which to write
   strn: The string to write
  """
  base_path = os.path.dirname(os.path.abspath(file_name))
  os.system('test -d %s || mkdir -p %s' % (base_path, base_path))
  f = open(file_name, 'w')
  f.write(strn)
  f.close()


def WriteToFileStrfunc(file_name, strn):
  return 'WriteToFile(%s, %s)' % (file_name, strn)


def EmitBuildDateFunc(target, build_date):
  """Emits build date information to target file."""
  fp = open(target, 'w')
  fp.writelines(['// DO NOT MODIFY - auto-generated file\n',
                 'extern const char *const BUILD_DATE = "' +
                 time.strftime('%Y-%m-%d', build_date) + '";\n',
                 'extern const char *const BUILD_YEAR = "' +
                 time.strftime('%Y', build_date) + '";\n',
                 'extern const char *const BUILD_MONTH = "' +
                 time.strftime('%m', build_date) + '";\n',
                 'extern const char *const BUILD_DAY = "' +
                 time.strftime('%d', build_date) + '";\n',
                ])
  fp.close()


def EmitBuildDateStrfunc(target, build_date):
  return 'EmitBuildDate(%s, %s)' % (target, build_date)


def EmitVersionHeaderFunc(target, backupFile):
  """Emit version information to the target file."""

  versionStr = GetVersion(backupFile)
  longVersionStr = GetLongVersion(backupFile)

  fp = open(target, 'w')
  fp.writelines(['// DO NOT MODIFY - auto-generated file\n',
                 'extern const char *const GEE_VERSION = "' +
                 versionStr + '";\n',
                 'extern const char *const GEE_LONG_VERSION = "' +
                 longVersionStr + '";\n'
                ])
  fp.close()


def EmitVersionHeaderStrfunc(target, backupFile):
  return 'EmitVersionHeader(%s, %s)' % (target, backupFile)
  
  
def EmitVersionFunc(target, backupFile):
  """Emit version information to the target file."""

  versionStr = GetVersion(backupFile)

  fp = open(target, 'w')
  fp.write(versionStr)
  fp.close()
  
  fp = open(backupFile, 'w')
  fp.write(versionStr)
  fp.close()


def EmitVersionStrfunc(target, backupFile):
  return 'EmitVersion(%s, %s)' % (target, backupFile)
  
  
def EmitLongVersionFunc(target, backupFile):
  """Emit version information to the target file."""

  versionStr = GetLongVersion(backupFile)

  fp = open(target, 'w')
  fp.write(versionStr)
  fp.close()


def EmitLongVersionStrfunc(target, backupFile):
  return 'EmitLongVersion(%s, %s)' % (target, backupFile)
  

def GetLongVersion(backupFile):
    """Create a detailed version string based on the state of
       the software, as it exists in the repository."""
    
    if CheckGitAvailable():
        return GitGeneratedLongVersion()

    # Without git, must use the backup file to create a string.
    base = ReadBackupVersionFile(backupFile)
    date = datetime.utcnow().strftime("%Y%m%d%H%M")
    
    return '-'.join([base, date])


def GetVersion(backupFile):
    """As getLongVersion(), but only return the leading *.*.* value."""
    try:
        raw = GetLongVersion(backupFile)
        final = raw.split("-")[0]
    except Exception:
        return "Version Error"

    return final
  

def CheckGitAvailable():
    """Try the most basic of git commands, to see if there is
       currently any access to a repository."""
    
    try:
        subprocess.check_output(['git', 'status'])
    except Exception:
        return False
    
    return True


def CheckDirtyRepository():
    """Check to see if the repository is not in a cleanly committed state."""
    
    str = subprocess.check_output(['git', 'status', '--porcelain'])
    
    # Ignore version.txt for this purpose, as a build may modify the file
    # and lead to an erroneous interpretation on repeated consecutive builds.
    if (str == " M earth_enterprise/src/version.txt\n"):
        return False
    
    return (len(str) > 0)
    

def ReadBackupVersionFile(target):
    """There should be a file checked in with the latest version
       information available; if git isn't available to provide
       information, then use this file instead."""
    
    try:
        fp = open(target, 'r')
        line = fp.readline()
        fp.close()
        
        return line
    
    except Exception:
        return "Version Error"


def GitGeneratedLongVersion():
    """Take the raw information parsed by git, and use it to
       generate an appropriate version string for GEE."""
    try:
        raw = subprocess.check_output(['git', 'describe', '--tags',
                                        '--match', '[0-9]*\.[0-9]*\.[0-9]*\-*'])
        raw = raw.rstrip()
    except Exception:
        return "Version Error"

    # Grab the datestamp.
    date = datetime.utcnow().strftime("%Y%m%d%H%M")

    # If this condition hits, then we are currently on a tagged commit.
    if (len(raw.split("-")) < 4):
        if CheckDirtyRepository():
            return '.'.join([raw, date])
        return raw

    # Tear apart the information in the version string.
    components = ParseRawVersionString(raw)
  
    # Determine how to update, since we are *not* on tagged commit.
    if components['isFinal']:
        components['patch'] = 0
        components['patchType'] = "alpha"
        components['revision'] = components['revision'] + 1
    else:
        components['patch'] = components['patch'] + 1
    
    # Rebuild.
    base = '.'.join([str(components[x]) for x in ("major", "minor", "revision")])
    patch = '.'.join([str(components["patch"]), components["patchType"], date])
    if not CheckDirtyRepository():
        patch = '.'.join([patch, components['hash']])
    
    return '-'.join([base, patch])


def ParseRawVersionString(raw):
    """Break apart a raw version string into its various components,
    and return those entries via a dictionary."""

    components = { }    
    rawComponents = raw.split("-")
    
    base = rawComponents[0]
    patchRaw = rawComponents[1]
    components['numCommits'] = rawComponents[2]
    components['hash'] = rawComponents[3]
    components['isFinal'] = ((patchRaw[-5:] == "final") or
                             (patchRaw[-7:] == "release"))
  
    baseComponents = base.split(".")
    components['major'] = int(baseComponents[0])
    components['minor'] = int(baseComponents[1])
    components['revision'] = int(baseComponents[2])
  
    patchComponents = patchRaw.split(".")
    components['patch'] = int(patchComponents[0])
    if (len(patchComponents) < 2):
        components['patchType'] = "alpha"
    else:
        components['patchType'] = patchComponents[1]
        
    return components
  

# our derived class
class khEnvironment(Environment):
  """The derived environment class used in all of Fusion SConscripts."""

  WriteToFile = SCons.Action.ActionFactory(WriteToFileFunc,
                                           WriteToFileStrfunc)
  EmitBuildDate = SCons.Action.ActionFactory(EmitBuildDateFunc,
                                             EmitBuildDateStrfunc)
  EmitVersion = SCons.Action.ActionFactory(EmitVersionFunc,
                                           EmitVersionStrfunc)
  EmitLongVersion = SCons.Action.ActionFactory(EmitLongVersionFunc,
                                           EmitLongVersionStrfunc)
  EmitVersionHeader = SCons.Action.ActionFactory(EmitVersionHeaderFunc,
                                           EmitVersionHeaderStrfunc)
  
  rsync_cmd = 'rsync -rltpvu %s %s'
  rsync_excl_cmd = 'rsync -rltpvu --exclude %s %s %s'

  def __init__(self,
               exportdirs,
               installdirs,
               platform=SCons.Platform.Platform(),
               tools=None,
               toolpath=None,
               options=None,
               **kw):
    if toolpath is None:
      toolpath = []
    args = (self, platform, tools, toolpath, options)
    Environment.__init__(*args, **kw)
    self.exportdirs = exportdirs
    self.installdirs = installdirs
    self['BUILDERS']['uic'] = uic
    self['BUILDERS']['uic_impl'] = uic_impl
    self['BUILDERS']['moc'] = moc
    self['BUILDERS']['IDLH'] = idl_h_builder
    self['BUILDERS']['IDLIMPLH'] = idl_impl_h_builder
    self['BUILDERS']['IDLCPP'] = idl_cpp_builder
    self['_oldstripixes'] = self['_stripixes']
    self['_stripixes'] = CleanupLibFlags

    DefineProtocolBufferBuilder(self)

  def DeepCopy(self):
    other = self.Clone()
    other.MultiCommand = SCons.Action.ActionFactory(other.MultiCommandFunc,
                                                    other.MultiCommandStrfunc)
    return other

  def MultiCommandFunc(self, cmd):
    """Runs multiple commands in a single shell.

    Args:
      cmd: The bash commands (may be multiple lines)
    Returns:
      The return status of executing the command.
    """
    return self.Execute('set -x && %s' % cmd.replace('\n', ' && '))

  def MultiCommandStrfunc(self, cmd):
    if SCons.SConf.dryrun:
      return '+ %s' % cmd.replace('\n', '\n+ ')
    else:
      return ''

  # Defines a Phony target that doesn't depend on anything and is always
  # executed.
  def PhonyTargets(self, **kw):
    ret_val = []
    for target, actions in kw.items():
      ret_val.append(self.AlwaysBuild(self.Alias(target, [], actions)))
    return ret_val

  # Install the file or directory as a part of install target.
  # Do this only after dependency is built.
  def InstallFileOrDir(self, source, destination, dependency, alias_name):
    base_path = os.path.dirname(os.path.abspath(destination))
    actions = ['test -d %s || mkdir -p %s' % (base_path, base_path),
               self.rsync_cmd % (source, destination)]
    if dependency:
      self.Depends(self.Alias(alias_name), dependency)
    this_dict = {alias_name: actions}
    return self.PhonyTargets(**this_dict)

  # TODO: looking for removal of this by
  # env.Clean(depends_on, list_of_files_to_remove)
  # The following is an work around as the above doesn't work for symbolic
  # links due to scons bug. The suggested patch to scons is as in
  # http://osdir.com/ml/programming.tools.scons.devel/2008-07/msg00100.html
  def ExecuteOnClean(self, cmd):
    if self.GetOption('clean'):
      self.Execute(self.MultiCommand(cmd))

  def UpdateCppflagsForSkia(self):
    """Update c++ flags for Skia code compilation."""
    if self['release']:
      self['CPPFLAGS'] += ['-DSK_RELEASE', '-DGR_RELEASE',
                           '-DSkDebugf="(void)"']
    elif self['optimize']:
      self['CPPFLAGS'] += ['-DSK_RELEASE', '-DGR_RELEASE',
                           '-DSkDebugf="(void)"']
    else:
      self['CPPFLAGS'] += ['-DSK_DEBUG', '-DGR_DEBUG']
    if sys.byteorder == 'little':
      self['CPPFLAGS'] += ['-DSK_R32_SHIFT=16', '-DSK_G32_SHIFT=8',
                           '-DSK_B32_SHIFT=0', '-DSK_A32_SHIFT=24']
    else:
      self['CPPFLAGS'] += ['-DSK_R32_SHIFT=8', '-DSK_G32_SHIFT=16',
                           '-DSK_B32_SHIFT=24', '-DSK_A32_SHIFT=0']
    self['CPPFLAGS'] += [
        '-DSK_SCALAR_IS_FLOAT', '-DSkUserConfig_DEFINED',
        '-I' + os.path.join(self.exportdirs['root'], self['skia_rel_dir'],
                            'include/config'),
        '-I' + os.path.join(self.exportdirs['root'], self['skia_rel_dir'],
                            'include/core'),
        '-I' + os.path.join(self.exportdirs['root'], self['skia_rel_dir'],
                            'include/effects'),
        '-I' + os.path.join(self.exportdirs['root'], self['skia_rel_dir'],
                            'include/images'),
        '-I' + os.path.join(self.exportdirs['root'], self['skia_rel_dir'],
                            'include/lazy')
    ]

  def staticLib(self, target, source, **kw):
    # path to the target in the srcdir (not builddir)
    target_src_node = self.arg2nodes(target)[0].srcnode()

    base = os.path.basename(target)
    target = os.path.join(self.exportdirs['lib'], base)
    args = (target, source)
    ret = self.StaticLibrary(*args, **kw)

    self.Default(self.alias(target_src_node, ret))
    return ret

  def sharedLib(self, target, source, **kw):
    # path to the target in the srcdir (not builddir)
    target_src_node = self.arg2nodes(target)[0].srcnode()

    base = os.path.basename(target)
    target = os.path.join(self.exportdirs['lib'], base)
    args = (target, source)
    ret = self.SharedLibrary(*args, **kw)

    self.Default(self.alias(target_src_node, ret))
    return ret

  def executable(self, target, source, **kw):
    # path to the target in the srcdir (not builddir)
    target_src_node = self.arg2nodes(target)[0].srcnode()

    base = os.path.basename(target)
    newtarget = os.path.join(self.exportdirs['bin'], base)
    args = (newtarget, source)
    ret = self.Program(*args, **kw)

    self.Default(self.alias(target_src_node, ret))
    return ret

  def test(self, target, source, **kw):
    # path to the target in the srcdir (not builddir)
    target_src_node = self.arg2nodes(target)[0].srcnode()

    base = os.path.basename(target)
    newtarget = os.path.join(self.exportdirs['bin'], 'tests', base)
    args = (newtarget, source)
    ret = self.Program(*args, **kw)

    self.Default(self.alias(target_src_node, ret))
    return ret

  def executableLink(self, dest, target, source, **unused_kw):
    """path to the target in the srcdir (not builddir)."""
    target_src_node = self.arg2nodes(target)[0].srcnode()

    targetbase = os.path.basename(target)
    newtarget = os.path.join(self.exportdirs['bin'], targetbase)
    sourcebase = os.path.basename(source)
    newsource = os.path.join(self.exportdirs['bin'], sourcebase)

    ret = self.Command(newtarget, [newsource],
                       ['ln -sf ${SOURCE.file} $TARGET'])
    self.Command(self.fs.File(targetbase, self.installdirs[dest]),
                 [self.fs.File(sourcebase, self.installdirs[dest])],
                 ['ln $SOURCE $TARGET',
                  'chmod a+x $TARGET'])

    self.Default(self.alias(target_src_node, ret))
    return ret

  def installedExecutableSymlink(self, dest, target, source, **unused_kw):
    """path to the target in the srcdir (not builddir)."""
    targetbase = os.path.basename(target)
    sourcebase = os.path.basename(source)

    return self.Command(
        self.fs.File(targetbase, self.installdirs[dest]),
        [self.fs.File(sourcebase, self.installdirs[dest])],
        ['ln -sf ${SOURCE.file} $TARGET'])

  def install(self, dest, target, subdir=''):
    instdir = self.fs.Dir(subdir, self.installdirs[dest])
    if not SCons.Util.is_List(target):
      target = [target]
    self.Install(instdir, target)

  def installAs(self, dest, src, newname, subdir=''):
    instdir = self.fs.Dir(subdir, self.installdirs[dest])
    if not SCons.Util.is_List(src):
      src = [src]
    if not SCons.Util.is_List(newname):
      newname = [newname]
    self.InstallAs([self.fs.File(i, instdir) for i in newname], src)

  def installDirExcluding(self, dest, target_dir, excluded_list, subdir=''):
    instdir = self.fs.Dir(subdir, self.installdirs[dest])
    self.installDirExcludingInternal(instdir, target_dir, excluded_list)

  def installDirExcludingInternal(self, instdir, target_dir, excluded_list):
    """Get contents of target_dir and install in instdir."""

    contents = os.listdir(target_dir)
    target_dir += '/'
    if not os.path.exists(instdir.get_abspath()):
      os.makedirs(instdir.get_abspath())
    for file_name in contents:
      if file_name in excluded_list:
        continue
      target_file = target_dir + file_name
      if os.path.isdir(target_file):
        subdir = self.fs.Dir(file_name, instdir)
        self.installDirExcludingInternal(subdir, target_file,
                                         excluded_list)
      else:
        self.Install(instdir, target_file)

  def copyfile(self, destdir, target, subdir=''):
    instdir = self.fs.Dir(subdir, destdir)
    ret = self.Install(instdir, target)
    self.Default(self.alias(self.arg2nodes('all')[0].srcnode(), ret))
    return ret

  def qtFiles(self, uifiles, hfiles, imgfiles, prjbase):
    for ui in uifiles:
      self.uic(ui)

    # now strip extentions from .ui & .h files
    uifiles = [os.path.splitext(str(i))[0] for i in uifiles]
    hfiles = [os.path.splitext(str(i))[0] for i in hfiles]

    for h in hfiles:
      self.moc('.moc/moc_'+h+'.cpp', h+'.h')

    if imgfiles:
      imgcollect = [self.Command('.ui/image_collection.cpp', imgfiles,
                                 '$UIC -embed %s $SOURCES -o $TARGET' % (
                                     prjbase))
                   ]
    else:
      imgcollect = []

    uicpps = ['.ui/' + u + '.cpp' for u in uifiles]
    uimoccpps = ['.moc/moc_' + u + '.cpp' for u in uifiles]
    hmoccpps = ['.moc/moc_' + h + '.cpp' for h in hfiles]
    return uicpps + uimoccpps + hmoccpps + imgcollect

  def idl(self, sources):
    for idlfile in sources:
      base = os.path.splitext(str(idlfile))[0]
      self.IDLH('.idl/%s.h' % base, [idlfile, self['KHIDL']])
      self.IDLIMPLH('.idl/%s_impl.h' % base, [idlfile, self['KHIDL']])
      self.IDLCPP('.idl/%s.cpp' % base, [idlfile, self['KHIDL']])

  def alias(self, target, source=None):
    if source is None:
      source = []
    tlist = self.arg2nodes(target, self.ans.Alias)
    if not SCons.Util.is_List(source):
      source = [source]
    source = filter(None, source)

    # Re-call all the target  builders to add the sources to each target.
    result = []
    for t in tlist:
      bld = t.get_builder() or my_alias_builder
      result.extend(bld(self, t, source))
    return result

  def ObjFromOtherDir(self, sources):
    if not SCons.Util.is_List(sources):
      sources = [sources]

    root_dir = self.exportdirs['root']
    shobj_suffix = self['SHOBJSUFFIX']
    return [root_dir + p + shobj_suffix for p in sources if p]


def ProtocolBufferGenerator(source, target, env, for_signature):
  """Protocol buffer generator builder.

  Args:
    source: List of source nodes
    target: List of target nodes
    env: Environment in which to build
    for_signature: Just generate command for build signature; don't actually
        run it.

  Returns:
    protocol buffer generator.
  """
  (env, target, source) = (env, target, source)  # Silence gpylint
  for_signature = for_signature   # Silence gpylint

  # Must run the protocol buffer compiler from the source directory!
  command = ('cd ${SOURCES.dir}; '
             '${TOOLS_BIN.abspath}/${PROTOBUF_COMPILER} '
             '--cpp_out $PROTOBUF_OUT_ROOT ${SOURCES.file}')

  return [command]


def ProtocolBufferEmitter(target, source, env):
  """Protocol buffer emitter.

  Args:
    target: List of target nodes
    source: List of source nodes
    env: Environment in which to build

  Returns:
    New (target, source).
  """
  env = env     # Silence gpylint

  # regardless of where the source comes from, we want to put the output files
  # (.pb.cc and .pb.h) into the PROTOBUF_OUT_ROOT directory.
  out_dir = env['PROTOBUF_OUT_ROOT'] + '/'

  # get the basename (non directory) of our source
  sourcebase = os.path.basename(str(source[0]))

  # strip off source extension and replace it with the two we want
  targetcc = out_dir + os.path.splitext(sourcebase)[0] + '.pb.cc'
  targeth = out_dir + os.path.splitext(sourcebase)[0] + '.pb.h'

  # build a new list of targets (ignoring anything that scons already has there)
  target = [targetcc, targeth]
  return target, source


def DefineProtocolBufferBuilder(env):
  # Note: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool.

  Args:
    env: Environment to modify.
  """

  # All protocol buffer generated files will be placed in the export directory
  # under protobuf.
  # To include them, the caller need only include "protobuf/xxx.pb.h"
  out_dir = os.path.join(env.exportdirs['root'], 'protobuf')
  out_dir = out_dir.strip('#')
  out_dir = os.path.abspath(out_dir)

  env.Replace(
      # Root of output; files will be placed in subdirs of this mirroring the
      # source tree.
      PROTOBUF_OUT_ROOT=out_dir
  )

  # Set tool based on local platform
  env['TOOLS_BIN'] = env.fs.Dir('../tools/bin/')
  env['PROTOBUF_COMPILER'] = 'protoc'

  # Add protocol buffer builder
  bld = SCons.Script.Builder(generator=ProtocolBufferGenerator,
                             emitter=ProtocolBufferEmitter,
                             single_source=1,
                             suffix='.pb.cc')
  env.Append(BUILDERS={'ProtocolBuffer': bld})
