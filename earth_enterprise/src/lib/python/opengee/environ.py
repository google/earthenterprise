"""
A module with functions for operating on environment variables.
"""

import os


def prepend_path(value, new_path, if_present='skip', separator=os.pathsep):
  """Adds a component to a PATH variable, where paths are separated by
  `separator`.

    if_present = What to do, if PATH already contains `new_path`.
      'skip': leave PATH as is.
      'move': move `new_path` to the beginning of PATH.
    separator = String that separates path components in the variable value.
  """

  # Check if the PATH variable already contains `new_path`:
  if value == new_path:
    return value
  if value.startswith(new_path + separator):
    if if_present == 'skip' or 'move':
      return value
    else:
      raise ValueError('Invalid if_present argument: {0}'.format(if_present))
  if value.endswith(separator + new_path):
    if if_present == 'skip':
      return value
    elif if_present == 'move':
      return new_path + separator + value[ : -len(new_path) - 1]
    else:
      raise ValueError('Invalid if_present argument: {0}'.format(if_present))
  i = value.find(separator + new_path + separator)
  if i >= 0:
    if if_present == 'skip':
      return value
    elif if_present == 'move':
      return new_path + separator + value[: i] + value[i + len(new_path) + 1:]
    else:
      raise ValueError('Invalid if_present argument: {0}'.format(if_present))

  # PATH doesn't contain `new_path`, prepend it:
  if value == '':
    return new_path

  return new_path + separator + value

def env_prepend_path(
  var_name, new_path, if_present='skip', separator=':'
):
  """Prepend a path component to a PATH-like environment variable (i.e., one
  which contains paths separated by `separator`).

    var_name = Name of the environment variable to add a path component to.
  The variable is created, if it doesn't exist.
    new_path = Path to add to the variable value.
    if_present = What to do, if the variable already contains `new_path`.
      'skip': Leave the variable value as is.
      'move': Move `new_path` to the beginning of the variable value.
    separator = String that separates path components in the variable value.
  """

  try:
    value = os.environ[var_name]
  except KeyError:
    value = ''
  os.environ[var_name] = prepend_path(
    value, new_path, if_present=if_present, separator=separator)
