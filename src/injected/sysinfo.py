"""Simple module for checking whether Python and Windows are 32-bit or 64-bit"""
import sys
import os
import platform
import ctypes
import win32process
import win32api
import win32con


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
    """Return True if the specified process is a 64-bit one

    Return False if it is only a 32-bit process running under Wow64.
    Always return False for x86.
    """

    if is_x64_os():
        phndl = win32api.OpenProcess(win32con.MAXIMUM_ALLOWED, 0, process_id)
        if not phndl:
            raise OSError(f'OpenProcess is failed for PID = {process_id}')
        return not win32process.IsWow64Process(phndl)

    return False
