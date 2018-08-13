import os
import re
import subprocess

from . import version
from . import environ


def get_cc_version(cc_path):
  """Extracts the version number of a C/C++ compiler (like GCC) as a list of
  strings.  (Like ['5', '4', '0'].)"""

  process = subprocess.Popen(
    [cc_path, '--version'], stdout=subprocess.PIPE)
  (stdout, _) = process.communicate()
  match = re.search('[0-9][0-9.]*', stdout)

  # If match is None we don't know the version.
  if match is None:
    return None

  version = match.group().split('.')

  return version
