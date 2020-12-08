"""
A module collecting functions for operating on versions.
"""


def is_version_ge(version_components, comparand_components):
  """Tests if a version number is greater than, or equal to another version
  number.  Both version numbers are specified as lists of version components.
  None is trated as an unknown version, and comparisons with it test as False.
  """

  if version_components is None or comparand_components is None:
    return False

  for i in range(len(comparand_components)):
    if i >= len(version_components):
      version_component = 0
    else:
      version_component = int(version_components[i])
    if int(comparand_components[i]) > version_component:
      return False
    if int(comparand_components[i]) < version_component:
      return True

  return True
