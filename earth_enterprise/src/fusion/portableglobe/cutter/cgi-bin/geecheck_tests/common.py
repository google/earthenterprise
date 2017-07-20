#!/usr/bin/env python
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


import glob
import os
import platform
import socket

LATEST_VERSION = '5.2.0'

SUPPORTED_OS_LIST = {
    'redhat': {'min_release': '6.0',
               'max_release': '6.5'},
    'Ubuntu': {'min_release': '10.04',
               'max_release': '14.04'},
    'CentOS Linux': {'min_release': '6.5',
                     'max_release': '6.5'}
}

BAD_HOSTNAMES = ['', 'linux', 'localhost', 'dhcp', 'bootp']

SERVICES = ['gesystemmanager',
            'geresourceprovider',
            'gehttpd',
            '/opt/google/bin/postgres']

PROC_PATH = '/proc'

PROCESS_INFO_LIST = ['cmdline']


def GetHostInfo():
  """Get information about the host."""
  hostname = GetFQDN()
  ipaddr = GetIP(hostname)
  host_check, _, _ = socket.gethostbyaddr(ipaddr)
  if hostname in BAD_HOSTNAMES:
    AssertionError('Hostname cannot be one of %s' % ','.join(BAD_HOSTNAMES))
  return hostname, ipaddr, host_check


def IsFusionInstalled():
  """Check if Fusion is installed."""
  fusion_start_script = '/etc/init.d/gefusion'
  return os.path.exists(fusion_start_script)


def IsGeeServerInstalled():
  """Check if GEE Server is installed."""
  gee_server_start_script = '/etc/init.d/gefusion'
  return os.path.exists(gee_server_start_script)


def GetFusionVersion():
  """Get the version of Fusion."""
  fusion_version_file_path = '/etc/opt/google/fusion_version'
  try:
    with open(fusion_version_file_path, 'r') as f:
      version = f.readline().rstrip()
      return version
  except IOError:
    AssertionError('Fusion version not available.')


def GetGeeServerVersion():
  """Get the version of GEE Server."""
  gee_server_version_file_path = '/etc/opt/google/fusion_server_version'

  try:
    with open(gee_server_version_file_path, 'r') as f:
      version = f.readline().rstrip()
      return version
  except IOError:
    AssertionError('GEE Server version not available.')
  return version


def GetLatestVersion():
  """Get the Latest version."""
  return LATEST_VERSION


def GetHostname():
  return socket.gethostname()


def GetFQDN():
  return socket.getfqdn()


def GetIP(hostname):
  return socket.gethostbyname(hostname)


def GetLinuxDetails():
  """Get Linux distribution details."""
  try:
    # eg. ('Ubuntu', '14.04', 'trusty')
    # platform.linux_distribution() and platform.dist() were deprecated, they
    # return 'debian' as the os, didn't find an elegant alternative and the
    # functions use the files in /etc/ to do their magic.
    with open("/etc/issue") as f:
      return f.read().split()
  except IOError:
    raise AssertionError('Linux distribution details not available.')


def GetLinuxArchitecture():
  """Get Linux architecture."""
  try:
    return platform.architecture()[0]
  except IOError:
    raise AssertionError('Linux architecture not available.')


def GetOSDistro():
  """Get OS distribution."""
  try:
    return GetLinuxDetails()[0]
  except IOError:
    raise AssertionError('Linux OS distribution not available.')


def GetOSRelease():
  """Get OS release."""
  try:
    return GetLinuxDetails()[1]
  except IOError:
    raise AssertionError('Linux OS release not available.')


def GetOSPlatform():
  """Get OS platform."""
  try:
    return platform.system().lower()
  except IOError:
    raise AssertionError('Linux OS release not available.')


def GetProcessesDetails():
  """Get details about running processes."""
  processes_details = {}
  # Loop only through the first directory level
  for process_folder in glob.glob('/proc/[0-9]*'):
    process_id = os.path.basename(process_folder)
    process_details = {}

    for info in PROCESS_INFO_LIST:
      process_details[info] = GetProcessDetails(process_id, info)

    if process_details:
      processes_details[process_id] = process_details

  return processes_details


def GetProcessDetails(pid, info):
  """Get the process detail needed."""
  path = os.path.join(PROC_PATH, pid, info)
  if os.path.exists(path):
    with open(path, 'r') as process_file:
      return process_file.readline().replace('\x00', ' ')
  return ''
