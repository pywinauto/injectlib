"""Some functions already exists in pywinauto, but for correctly
injector work needs full redefinition with ctypes compatible
"""

import win32con
from ctypes import Structure, sizeof, alignment
from ctypes import c_char_p, c_wchar_p
from ctypes import POINTER
from ctypes import windll
from ctypes.wintypes import BOOL, DWORD, HANDLE, LPVOID, LPCVOID


class SECURITY_ATTRIBUTES(Structure):
    _fields_ = [
        ('nLength', DWORD),
        ('lpSecurityDescriptor', LPVOID),
        ('bInheritHandle', BOOL),
    ]


assert sizeof(SECURITY_ATTRIBUTES) == 12 or sizeof(SECURITY_ATTRIBUTES) == 24, sizeof(SECURITY_ATTRIBUTES)
assert alignment(SECURITY_ATTRIBUTES) == 4 or alignment(SECURITY_ATTRIBUTES) == 8, alignment(SECURITY_ATTRIBUTES)

PAGE_READWRITE = win32con.PAGE_READWRITE
WAIT_TIMEOUT = win32con.WAIT_TIMEOUT
PROCESS_ALL_ACCESS = win32con.PROCESS_ALL_ACCESS
VIRTUAL_MEM = (win32con.MEM_RESERVE | win32con.MEM_COMMIT)
LPCSTR = LPCTSTR = c_char_p
LPWTSTR = c_wchar_p
LPDWORD = PDWORD = POINTER(DWORD)
LPTHREAD_START_ROUTINE = LPVOID
LPSECURITY_ATTRIBUTES = POINTER(SECURITY_ATTRIBUTES)

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
