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

def get_console_mode(output=False):
    '''Get the mode of the active console input or output
       buffer. Note that if the process isn't attached to a
       console, this function raises an EBADF IOError.
    '''
    device = r'\\.\CONOUT$' if output else r'\\.\CONIN$'
    with open(device, 'r+') as con:
        mode = wintypes.DWORD()
        hCon = msvcrt.get_osfhandle(con.fileno())
        kernel32.GetConsoleMode(hCon, ctypes.byref(mode))
        return mode.value

def set_console_mode(mode, output=False):
    '''Set the mode of the active console input or output
       buffer. Note that if the process isn't attached to a
       console, this function raises an EBADF IOError.
    '''
    device = r'\\.\CONOUT$' if output else r'\\.\CONIN$'
    with open(device, 'r+') as con:
        hCon = msvcrt.get_osfhandle(con.fileno())
        kernel32.SetConsoleMode(hCon, mode)

def update_console_mode(flags, mask, output=False, restore=False):
    '''Update a masked subset of the current mode of the active
       console input or output buffer. Note that if the process
       isn't attached to a console, this function raises an
       EBADF IOError.
    '''
    current_mode = get_console_mode(output)
    if current_mode & mask != flags & mask:
        mode = current_mode & ~mask | flags & mask
        set_console_mode(mode, output)
    else:
        restore = False
    if restore:
        atexit.register(set_console_mode, current_mode, output)