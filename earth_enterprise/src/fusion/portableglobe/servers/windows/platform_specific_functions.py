#!/usr/bin/env python2.7
#
# Copyright 2020 Open GEE Contributors
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

# Includes defines and functions for setting the console mode in Windows

import msvcrt
import atexit
import ctypes
from ctypes import wintypes

kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)

# input flags
ENABLE_QUICK_EDIT_MODE = 0x0040
ENABLE_EXTENDED_FLAGS  = 0x0080

def check_zero(result, func, args):    
  """
  check_zero is used by the GetConsoleMode and SetConsoleMode functions to 
  verify that the return value of the system call is 0, indicating "no error".
  It's not immediately obvious, but the ctypes.WinError that is raised here
  gets processed by the system and eventually results in the WindowsError
  that is handled in prepare_for_io_loop.
  """
  if not result:
      err = ctypes.get_last_error()
      if err:
          raise ctypes.WinError(err)
  return args

if not hasattr(wintypes, 'LPDWORD'): # PY2
  wintypes.LPDWORD = ctypes.POINTER(wintypes.DWORD)

kernel32.GetConsoleMode.errcheck= check_zero
kernel32.GetConsoleMode.argtypes = (
    wintypes.HANDLE,   # _In_  hConsoleHandle
    wintypes.LPDWORD,) # _Out_ lpMode

kernel32.SetConsoleMode.errcheck= check_zero
kernel32.SetConsoleMode.argtypes = (
    wintypes.HANDLE, # _In_  hConsoleHandle
    wintypes.DWORD,) # _Out_ lpMode

def prepare_for_io_loop():
  """
  On Windows, we want to disable Quick Edit Mode prior to starting the I/O loop
  so users don't inadvertently cause portable to freeze while Python is stuck
  trying to print to the console. This happens if the user clicks in the console
  and causes it to enter "Select" mode. It's very easy to do unintentionally.
  """
  restore = True
  try:
    flags = 0
    mask = ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE
    # Get the current mode. The handle ID of -10 represents the console input
    # handle.
    console_input_handle = kernel32.GetStdHandle(-10)
    current_mode = wintypes.DWORD()
    kernel32.GetConsoleMode(console_input_handle, ctypes.byref(current_mode))
    original_mode = current_mode.value

    # Mask out Quick Edit Mode flag and set the mode
    if original_mode & mask != flags & mask:
      mode = original_mode & ~mask | flags & mask
      kernel32.SetConsoleMode(console_input_handle, mode)
    else:
      # Quick Edit Mode is not enabled. No need to restore the old mode on exit.
      restore = False
  except WindowsError:
    # GetConsoleMode or SetConsoleMode failed. Likely an invalid handle.
    # Make no further attempt to set the mode, and don't restore the mode
    # on exit.
    restore = False

  if restore:
    atexit.register(kernel32.SetConsoleMode, console_input_handle, original_mode)
