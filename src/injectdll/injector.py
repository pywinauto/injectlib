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

import ctypes
import locale
import os

from . import sysinfo
from . import cfuncs


class Injector(object):
    """Class for injections dll"""

    def __init__(self, pid, backend_name, dll_name, is_unicode=False):
        """Constructor inject dll (one application - one class instance)."""
        self.is_unicode = is_unicode
        self.pid = pid
        if not sysinfo.is_x64_python() == sysinfo.is64_bitprocess(self.pid):
            raise RuntimeError("Application and Python must be both 32-bit or both 64-bit")
        self.h_process = self._get_process_handle(self.pid)

        self.dll_path = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                                     'libs', backend_name,
                                     'x{}'.format("64" if sysinfo.is64_bitprocess(self.pid) else "86"),
                                     '{}.dll'.format(dll_name)).encode('utf-16' if self.is_unicode
                                                                       else locale.getpreferredencoding())
        self._inject_dll_to_process()

    @staticmethod
    def _get_process_handle(pid):
        return cfuncs.OpenProcess(cfuncs.PROCESS_ALL_ACCESS, False, pid)

    def _create_remote_thread_with_timeout(self, proc_address, arg_address, timeout_ms=1000,
                                           call_err_text="", timeout_err_text=""):
        thread_handle = cfuncs.CreateRemoteThread(self.h_process, None, 0, proc_address, arg_address, 0, None)
        if not thread_handle:
            raise RuntimeError(call_err_text)
        # TODO: add timeout to pywinauto timings and replace
        ret = cfuncs.WaitForSingleObject(thread_handle, ctypes.wintypes.DWORD(timeout_ms))
        if ret == cfuncs.WAIT_TIMEOUT:
            raise TimeoutError(timeout_err_text)

    def _inject_dll_to_process(self):
        # Get dll path length
        c_dll_path = ctypes.create_unicode_buffer(self.dll_path) \
            if self.is_unicode else ctypes.create_string_buffer(self.dll_path)

        # Allocate space for DLL path
        arg_address = cfuncs.VirtualAllocEx(self.h_process, 0, ctypes.sizeof(c_dll_path),
                                            cfuncs.VIRTUAL_MEM, cfuncs.PAGE_READWRITE)

        # Write DLL path to allocated space
        if not cfuncs.WriteProcessMemory(self.h_process, arg_address,
                                         ctypes.byref(c_dll_path), ctypes.sizeof(c_dll_path), 0):
            raise AttributeError("Couldn't write data to process memory, check python access.")

        # Resolve LoadLibraryA(W) Address
        h_kernel32 = cfuncs.GetModuleHandleW(b"kernel32.dll") \
            if self.is_unicode else cfuncs.GetModuleHandleA(b"kernel32.dll")
        h_loadlib = cfuncs.GetProcAddress(h_kernel32, b"LoadLibraryW" if self.is_unicode else b"LoadLibraryA")

        # Now call createRemoteThread with entry point set to LoadLibraryA(W) and pointer to DLL path as param
        self._create_remote_thread_with_timeout(h_loadlib, arg_address, 1000,
                                                "Couldn't create remote thread, application and Python must be "
                                                "both 32-bit or both 64-bit", "Inject time out")

    def _get_dll_proc_address(self, proc_name):
        lib = cfuncs.LoadLibraryW(self.dll_path) if self.is_unicode else cfuncs.LoadLibraryA(self.dll_path)
        return cfuncs.GetProcAddress(lib, proc_name)

    def remote_call_void_func(self, func_name):
        proc_address = self._get_dll_proc_address(func_name)

        if not cfuncs.CreateRemoteThread(self.h_process, None, 0, proc_address, 0, 0, None):
            raise RuntimeError("Couldn't create remote thread, dll not injected, inject and try again!")

    def remote_call_int_param_func(self, func_name, param):
        # Resolve param type for different applications
        a = ctypes.c_int64(param) if sysinfo.is64_bitprocess(self.pid) else ctypes.c_int32(param)

        arg_address = cfuncs.VirtualAllocEx(self.h_process, 0, ctypes.sizeof(a),
                                            cfuncs.VIRTUAL_MEM, cfuncs.PAGE_READWRITE)
        if not cfuncs.WriteProcessMemory(self.h_process, arg_address, ctypes.byref(a), ctypes.sizeof(a), 0):
            raise AttributeError("Couldn't write data to process memory, check python access.")

        proc_address = self._get_dll_proc_address(func_name)

        self._create_remote_thread_with_timeout(proc_address, arg_address, 1000,
                                                "Couldn't create remote thread, dll not injected, "
                                                "inject it and try again!",
                                                "{0}(int) function call time out".format(func_name))
