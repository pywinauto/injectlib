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

import pywintypes
import time
import win32file
import win32pipe
import winerror
from .actionlogger import ActionLogger


class InjectedBrokenPipeError(Exception):
    pass


class Pipe(object):
    def __init__(self, name):
        self.name = name
        self.handle = None

    def connect(self, n_attempts=30, delay=1):
        for i in range(n_attempts):
            try:
                self.handle = win32file.CreateFile(
                    r'\\.\pipe\{}'.format(self.name),
                    win32file.GENERIC_READ | win32file.GENERIC_WRITE,
                    0,
                    None,
                    win32file.OPEN_EXISTING,
                    0,
                    None
                )
                win32pipe.SetNamedPipeHandleState(
                    self.handle,
                    win32pipe.PIPE_READMODE_MESSAGE | win32pipe.PIPE_WAIT,
                    None,
                    None,
                )
                ActionLogger().log('Connected to the pipe {}'.format(self.name))
                break
            except pywintypes.error as e:
                if e.args[0] == winerror.ERROR_FILE_NOT_FOUND:
                    ActionLogger().log('Attempt {}/{}: failed to connect to the pipe {}'.format(i + 1, n_attempts,
                                                                                                self.name))
                    time.sleep(delay)
                else:
                    raise InjectedBrokenPipeError('Unexpected pipe error: {}'.format(e))
        if self.handle is not None:
            return True
        return False

    def transact(self, string):
        try:
            # TODO get preferred encoding from application
            self.write(string, 'utf-8')
            resp = win32file.ReadFile(self.handle, 64 * 1024)
            return resp[1].decode('utf-8')
        except pywintypes.error as e:
            if e.args[0] == winerror.ERROR_BROKEN_PIPE:
                raise InjectedBrokenPipeError("Broken pipe")
            else:
                raise InjectedBrokenPipeError('Unexpected pipe error: {}'.format(e))

    def close(self):
        win32file.CloseHandle(self.handle)

    def write(self, string, encoding='utf-8'):
        """Write string with the specified encoding to the named pipe."""
        win32file.WriteFile(self.handle, string.encode(encoding))
        win32file.FlushFileBuffers(self.handle)
