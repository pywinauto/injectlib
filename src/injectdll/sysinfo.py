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

"""Simple module for checking whether Python and Windows are 32-bit or 64-bit"""
import sys
import os
import platform
import ctypes
import logging
import win32process
import win32api
import win32con

try:
    # Disable 'INFO' logs from comtypes
    log = logging.getLogger('comtypes')
    log.setLevel('WARNING')

    import comtypes  # noqa: E402

    UIA_support = True
except ImportError:
    UIA_support = False


def os_arch():
    architecture_map = {
        'x86': 'x86',
        'i386': 'x86',
        'i486': 'x86',
        'i586': 'x86',
        'i686': 'x86',
        'x64': 'x86_64',
        'AMD64': 'x86_64',
        'amd64': 'x86_64',
        'em64t': 'x86_64',
        'EM64T': 'x86_64',
        'x86_64': 'x86_64',
        'IA64': 'ia64',
        'ia64': 'ia64'
    }
    if sys.platform == 'win32':
        architecture_var = os.environ.get('PROCESSOR_ARCHITEW6432', '')
        if architecture_var == '':
            architecture_var = os.environ.get('PROCESSOR_ARCHITECTURE', '')
        return architecture_map.get(architecture_var, 'Unknown')
    else:
        return architecture_map.get(platform.machine(), '')


def python_bitness():
    return ctypes.sizeof(ctypes.POINTER(ctypes.c_int)) * 8


def is_x64_python():
    return python_bitness() == 64


def is_x64_os():
    return os_arch() in ['x86_64', 'ia64']


def is64_bitprocess(process_id):
    """Return True if the specified process is a 64-bit process on x64

    Return False if it is only a 32-bit process running under Wow64.
    Always return False for x86.
    """

    is32 = True
    if is_x64_os():
        phndl = win32api.OpenProcess(win32con.MAXIMUM_ALLOWED, 0, process_id)
        if phndl:
            is32 = win32process.IsWow64Process(phndl)

    return not is32
