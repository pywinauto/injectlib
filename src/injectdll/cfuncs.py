# Copyright (C) 2023 Mark Mc Mahon and Contributors
# https://github.com/pywinauto/injected/graphs/contributors
# https://pywinauto.readthedocs.io/en/latest/credits.html
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of pywinauto nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Some functions already exists in pywinauto, but for correctly
injector work needs full redefinition with ctypes compatible
"""

import win32con
from ctypes import Structure, sizeof, alignment
from ctypes import c_char_p, c_wchar_p
from ctypes import POINTER
from ctypes import windll
from ctypes.wintypes import BOOL, DWORD, HANDLE, LPVOID, LPCVOID


class SecurityAttributes(Structure):
    _fields_ = [
        ('nLength', DWORD),
        ('lpSecurityDescriptor', LPVOID),
        ('bInheritHandle', BOOL),
    ]


assert sizeof(SecurityAttributes) == 12 or sizeof(SecurityAttributes) == 24, sizeof(SecurityAttributes)
assert alignment(SecurityAttributes) == 4 or alignment(SecurityAttributes) == 8, alignment(SecurityAttributes)

PAGE_READWRITE = win32con.PAGE_READWRITE
WAIT_TIMEOUT = win32con.WAIT_TIMEOUT
PROCESS_ALL_ACCESS = win32con.PROCESS_ALL_ACCESS
VIRTUAL_MEM = (win32con.MEM_RESERVE | win32con.MEM_COMMIT)
LPCSTR = LPCTSTR = c_char_p
LPWTSTR = c_wchar_p
LPDWORD = PDWORD = POINTER(DWORD)
LPTHREAD_START_ROUTINE = LPVOID
LPSECURITY_ATTRIBUTES = POINTER(SecurityAttributes)

OpenProcess = windll.kernel32.OpenProcess
OpenProcess.restype = HANDLE
OpenProcess.argtypes = (DWORD, BOOL, DWORD)

VirtualAllocEx = windll.kernel32.VirtualAllocEx
VirtualAllocEx.restype = LPVOID
VirtualAllocEx.argtypes = (HANDLE, LPVOID, DWORD, DWORD, DWORD)

ReadProcessMemory = windll.kernel32.ReadProcessMemory
ReadProcessMemory.restype = BOOL
ReadProcessMemory.argtypes = (HANDLE, LPCVOID, LPVOID, DWORD, DWORD)

WriteProcessMemory = windll.kernel32.WriteProcessMemory
WriteProcessMemory.restype = BOOL
WriteProcessMemory.argtypes = (HANDLE, LPVOID, LPCVOID, DWORD, DWORD)

CreateRemoteThread = windll.kernel32.CreateRemoteThread
CreateRemoteThread.restype = HANDLE
CreateRemoteThread.argtypes = (HANDLE, LPSECURITY_ATTRIBUTES, DWORD, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD)

GetModuleHandleA = windll.kernel32.GetModuleHandleA
GetModuleHandleA.restype = HANDLE
GetModuleHandleA.argtypes = (LPCTSTR,)

LoadLibraryA = windll.kernel32.LoadLibraryA
LoadLibraryA.restype = HANDLE
LoadLibraryA.argtypes = (LPCTSTR,)

GetModuleHandleW = windll.kernel32.GetModuleHandleW
GetModuleHandleW.restype = HANDLE
GetModuleHandleW.argtypes = (LPWTSTR,)

LoadLibraryW = windll.kernel32.LoadLibraryW
LoadLibraryW.restype = HANDLE
LoadLibraryW.argtypes = (LPWTSTR,)

GetProcAddress = windll.kernel32.GetProcAddress
GetProcAddress.restype = LPVOID
GetProcAddress.argtypes = (HANDLE, LPCTSTR)

WaitForSingleObject = windll.kernel32.WaitForSingleObject
WaitForSingleObject.restype = DWORD
WaitForSingleObject.argtypes = (HANDLE, DWORD)
