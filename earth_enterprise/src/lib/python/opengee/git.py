import os
import re
import subprocess

from . import version
from . import environ


def get_git_version(git_path):
  """Extracts the version number of git."""

  process = subprocess.Popen(
    [git_path, '--version'], stdout=subprocess.PIPE)
  (stdout, _) = process.communicate()
  match = re.search('[0-9][0-9.]*', stdout)

  # If match is None we don't know the version.
  if match is None:
    return None

  version = match.group().split('.')

  return version
