"""Some functions already exists in pywinauto, but for correctly
injector work needs full redefinition with ctypes compatible
"""

import os
import sys
from ctypes import Structure, sizeof, alignment
from ctypes import c_char_p, c_wchar_p
from ctypes import POINTER

if sys.platform == "win32":
    import win32con
    from ctypes import windll
    from ctypes.wintypes import BOOL, DWORD, HANDLE, LPVOID, LPCVOID
else:
    import ctypes
    from ctypes import c_int, c_long, c_ulonglong, c_void_p, byref


if sys.platform == "win32":
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
else:
    PTRACE_PEEKDATA = 2
    PTRACE_POKEDATA = 5
    PTRACE_CONT = 7
    PTRACE_GETREGS = 12
    PTRACE_SETREGS = 13
    PTRACE_ATTACH = 16
    PTRACE_DETACH = 17
    RTLD_NOW = 2
    RTLD_GLOBAL = 0x100
    SIGTRAP = 5

    libc = ctypes.CDLL(None, use_errno=True)
    try:
        libdl = ctypes.CDLL("libdl.so.2", use_errno=True)
    except OSError:
        libdl = libc

    ptrace = libc.ptrace
    ptrace.restype = c_long
    ptrace.argtypes = (c_int, c_int, c_void_p, c_void_p)

    waitpid = libc.waitpid
    waitpid.restype = c_int
    waitpid.argtypes = (c_int, POINTER(c_int), c_int)

    dlopen = libdl.dlopen
    dlopen.restype = c_void_p
    dlopen.argtypes = (c_char_p, c_int)

    class UserRegsStruct(Structure):
        _fields_ = [
            ("r15", c_ulonglong),
            ("r14", c_ulonglong),
            ("r13", c_ulonglong),
            ("r12", c_ulonglong),
            ("rbp", c_ulonglong),
            ("rbx", c_ulonglong),
            ("r11", c_ulonglong),
            ("r10", c_ulonglong),
            ("r9", c_ulonglong),
            ("r8", c_ulonglong),
            ("rax", c_ulonglong),
            ("rcx", c_ulonglong),
            ("rdx", c_ulonglong),
            ("rsi", c_ulonglong),
            ("rdi", c_ulonglong),
            ("orig_rax", c_ulonglong),
            ("rip", c_ulonglong),
            ("cs", c_ulonglong),
            ("eflags", c_ulonglong),
            ("rsp", c_ulonglong),
            ("ss", c_ulonglong),
            ("fs_base", c_ulonglong),
            ("gs_base", c_ulonglong),
            ("ds", c_ulonglong),
            ("es", c_ulonglong),
            ("fs", c_ulonglong),
            ("gs", c_ulonglong),
        ]

    def _wifstopped(status):
        return (status & 0xff) == 0x7f

    def _wstopsig(status):
        return (status >> 8) & 0xff

    def _ptrace(request, pid, address=0, data=0):
        ctypes.set_errno(0)
        result = ptrace(request, pid, c_void_p(address), c_void_p(data))
        if result == -1 and ctypes.get_errno() != 0:
            raise OSError(ctypes.get_errno(), os.strerror(ctypes.get_errno()))
        return result

    def ptrace_attach(pid):
        _ptrace(PTRACE_ATTACH, pid)
        status = c_int()
        if waitpid(pid, byref(status), 0) < 0:
            raise OSError(ctypes.get_errno(), os.strerror(ctypes.get_errno()))
        if not _wifstopped(status.value):
            raise RuntimeError("target did not stop after ptrace attach")

    def ptrace_detach(pid):
        _ptrace(PTRACE_DETACH, pid)

    def ptrace_continue(pid):
        _ptrace(PTRACE_CONT, pid)
        status = c_int()
        if waitpid(pid, byref(status), 0) < 0:
            raise OSError(ctypes.get_errno(), os.strerror(ctypes.get_errno()))
        if not _wifstopped(status.value):
            raise RuntimeError("target did not stop after remote call")
        return _wstopsig(status.value)

    def ptrace_getregs(pid):
        regs = UserRegsStruct()
        _ptrace(PTRACE_GETREGS, pid, 0, ctypes.addressof(regs))
        return regs

    def ptrace_setregs(pid, regs):
        _ptrace(PTRACE_SETREGS, pid, 0, ctypes.addressof(regs))

    def read_target(pid, address, size):
        data = bytearray()
        word_size = sizeof(c_long)
        offset = 0
        while offset < size:
            word = _ptrace(PTRACE_PEEKDATA, pid, address + offset, 0)
            chunk = ctypes.string_at(byref(c_long(word)), word_size)
            data.extend(chunk[:min(word_size, size - offset)])
            offset += word_size
        return bytes(data)

    def write_target(pid, address, data):
        word_size = sizeof(c_long)
        offset = 0
        while offset < len(data):
            chunk = data[offset:offset + word_size]
            if len(chunk) != word_size:
                current = bytearray(read_target(pid, address + offset, word_size))
                current[:len(chunk)] = chunk
                chunk = bytes(current)
            word = c_long.from_buffer_copy(chunk)
            _ptrace(PTRACE_POKEDATA, pid, address + offset, word.value & ((1 << (word_size * 8)) - 1))
            offset += word_size

    def module_base_from_maps(pid, module_basename):
        with open("/proc/{}/maps".format(pid), "r") as maps_file:
            for line in maps_file:
                parts = line.split()
                if len(parts) < 6 or "x" not in parts[1]:
                    continue
                path = parts[-1]
                if os.path.basename(path) != module_basename:
                    continue
                start = int(parts[0].split("-", 1)[0], 16)
                offset = int(parts[2], 16)
                return start - offset
        return None

    def local_module_for_address(address):
        with open("/proc/{}/maps".format(os.getpid()), "r") as maps_file:
            for line in maps_file:
                parts = line.split()
                if len(parts) < 6:
                    continue
                start_s, end_s = parts[0].split("-", 1)
                start = int(start_s, 16)
                end = int(end_s, 16)
                if start <= address < end:
                    return os.path.basename(parts[-1]), start - int(parts[2], 16)
        return None, None

    def remote_symbol_address(pid, local_symbol):
        local_address = ctypes.cast(local_symbol, c_void_p).value
        module_name, local_base = local_module_for_address(local_address)
        if not module_name or not local_base:
            return None
        remote_base = module_base_from_maps(pid, module_name)
        if not remote_base:
            return None
        return remote_base + (local_address - local_base)
