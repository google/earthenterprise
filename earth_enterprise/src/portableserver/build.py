#! /usr/bin/python
#-*- Python -*-
#
# Copyright 2017 GEE Open Source Team <github.com/google/earthenterprise>
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

"""
Builds Portable server for Linux and Windows.
"""

import argparse
import datetime
import distutils.dir_util
import os
import os.path
import shutil
import sys

SELF_DIR = os.path.dirname(os.path.realpath(__file__))


def ensure_directory(path):
    """Makes sure a given directory exists."""

    if not os.path.isdir(path):
        os.makedirs(path)

def copy_from_dir_to_dir(
    source_dir, destination_dir, entries=None, exclude_entries=None):
    """Copies given directory entries from one directory to another."""

    if entries is None:
        entries = os.listdir(source_dir)
    if exclude_entries is not None:
        entries = [e for e in entries if e not in exclude_entries]
    for entry in entries:
        source_path = os.path.join(source_dir, entry)
        destination_path = os.path.join(destination_dir, entry)
        if os.path.isdir(source_path):
            distutils.dir_util.copy_tree(source_path, destination_path)
        else:
            shutil.copy2(source_path, destination_path)

class Builder(object):
    """Builds Portable server."""

    def __init__(self, build_dir, source_dir, platform):
        self.build_dir = build_dir
        self.source_dir = source_dir
        self.platform = platform

        self.base_version = None
        self.build_date = None
        self.version_string = None
        self.get_version()

        self.package_dir = os.path.join(
            self.build_dir,
            'portableserver-{0}-{1}'.format(self.platform, self.version_string))
        self.resources_dir = os.path.join(
            self.source_dir, 'portableserver', 'resources')
        self.server_dir = os.path.join(self.package_dir, 'server')
        self.tar_package_name = \
            'portableserver-{0}-{1}'.format(self.platform, self.version_string)
        self.zip_package_name = self.tar_package_name
        if self.platform == 'windows':
            self.should_create_tar_package = False
            self.should_create_zip_package = True
        else:
            self.should_create_tar_package = True
            self.should_create_zip_package = False

    def build(self):
        """Builds and packages Portable server."""

        shutil.rmtree(self.build_dir, ignore_errors=True)
        ensure_directory(self.build_dir)
        ensure_directory(self.package_dir)
        ensure_directory(self.server_dir)
        self.obtain_server_resources()
        self.build_fileunpacker()
        self.obtain_sample_globes()
        if self.should_create_tar_package:
            self.create_tar_package()
        if self.should_create_zip_package:
            self.create_zip_package()

    def clean(self):
        """Deletes build files and directories."""

        shutil.rmtree(self.build_dir, ignore_errors=True)

    def obtain_server_resources(self):
        """Builds <server/>."""

        # Copy from <fusion/portableglobe/servers/>:
        servers_dir = os.path.join(
            self.source_dir, 'fusion', 'portableglobe', 'servers')

        exclude_entries = ['linux', 'mac', 'windows', 'fileunpacker']
        entries = [f
                   for f in os.listdir(servers_dir)
                   if f.lower() not in exclude_entries]
        copy_from_dir_to_dir(servers_dir, self.server_dir, entries=entries)
        copy_from_dir_to_dir(
            os.path.join(servers_dir, self.platform), self.server_dir)

        # Copy from <maps/>:
        copy_from_dir_to_dir(
            os.path.join(self.source_dir, 'maps'),
            os.path.join(self.server_dir, 'local', 'maps'),
            exclude_entries=['SConscript'])

        self.create_version_txt()

        # Copy configuration files:
        copy_from_dir_to_dir(self.resources_dir, self.server_dir)

    def create_version_txt(self):
        """Creates <server/local/version.txt>."""

        self.get_version()
        path = os.path.join(self.server_dir, 'local', 'version.txt')
        with open(path, 'w') as output_file:
            output_file.write(self.version_string)

    def get_version(self):
        """Parses version information, and sets member variables."""

        if self.base_version is None:
            path = os.path.join(self.source_dir, 'version.txt')
            with open(path, 'r') as input_file:
                while True:
                    base_version = input_file.readline().strip()
                    if base_version and not base_version.startswith('#'):
                        break
            self.base_version = base_version

        if self.build_date is None:
            self.build_date = datetime.date.today()

        if self.version_string is None:
            self.version_string = '{0}-{1}'.format(
                self.base_version, self.build_date.strftime('%Y%m%d'))

    def build_fileunpacker(self):
        """Builds `fileunpacker` shared libraries, and copies them to
        <server/>."""

        # Keep the source directory clean:
        task_build_dir = os.path.join(self.build_dir, 'fileunpacker-build')
        shutil.rmtree(task_build_dir, ignore_errors=True)
        distutils.dir_util.copy_tree(
            os.path.join(self.source_dir, 'fusion', 'portableglobe', 'servers',
                         'fileunpacker'),
            task_build_dir)

        # Execute 'build_and_test.py':
        os.chdir(task_build_dir)
        old_path = sys.path
        sys.path = [task_build_dir] + sys.path
        import build_and_test
        sys.path = old_path

        build_and_test.main(['build_and_test.py', self.platform])

        # Copy library to package directory:
        exclude_entries = ['test.py', 'util.py']
        dist_dir = os.path.join(task_build_dir, 'dist')
        entries = [
            f
            for f in os.listdir(dist_dir)
            if os.path.splitext(f)[1].lower() in
            ['.so', '.pyd', '.dll', '.py']
        ]
        copy_from_dir_to_dir(dist_dir, self.server_dir, entries=entries,
            exclude_entries=exclude_entries)

    def obtain_sample_globes(self):
        """Copies tutorial globe and map cuts to <data/>."""

        globes_dir = os.path.join(
            self.source_dir, 'fusion', 'portableglobe', 'globes')
        distutils.dir_util.copy_tree(
            globes_dir, os.path.join(self.package_dir, 'data'))

    def create_tar_package(self):
        """Archives and compresses the install directory."""

        # Python 2.6 on RHEL 6 doesn't have `shutil.make_archive()`, and
        # TarFile objects don't seem to support the `with` construct.
        import tarfile

        archive = tarfile.open(
            os.path.join(self.build_dir, self.zip_package_name + '.tar.gz'),
            "w:gz")
        try:
            archive.add(
                os.path.dirname(self.package_dir),
                os.path.basename(self.package_dir))
        finally:
            archive.close()

    def create_zip_package(self):
        """Archives and compresses the install directory."""

        shutil.make_archive(
            os.path.join(self.build_dir, self.zip_package_name),
            'zip',
            os.path.dirname(self.package_dir),
            os.path.basename(self.package_dir))


def main(argv):
    """Parses command-line arguments, detects build environment, builds, and
    packages Portable server."""

    build_dir = os.path.join(SELF_DIR, 'build')
    source_dir = os.path.join(SELF_DIR, '..')

    parser = argparse.ArgumentParser(
        description=__doc__, prog=os.path.basename(argv[0]))
    parser.add_argument('--platform', '-p', action='store', default=None)
    parser.add_argument('--clean', '-c', action='store_true')
    parse_result = parser.parse_args(argv[1:])

    platform = parse_result.platform
    if platform is None:
        platform = sys.platform.lower()
    if platform.startswith('linux'):
        platform = 'linux'
    elif platform.startswith('win'):
        platform = 'windows'
    elif platform.startswith('darwin') or platform.startswith('mac'):
        platform = 'mac'
    else:
        raise ValueError(
            ('Unrecognized platform: {0}, please specify "linux", "windows", '
             'or "mac" on the command line.').format(platform))

    builder = Builder(build_dir, source_dir, platform)
    if parse_result.clean:
        builder.clean()
    else:
        builder.build()

if __name__ == '__main__':
    main(sys.argv)
