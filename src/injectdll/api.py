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

import json
import six

from .injector import Injector
from .channel import Pipe
from .actionlogger import ActionLogger

# backend exit codes enum
OK = 0
PARSE_ERROR = 1
UNSUPPORTED_ACTION = 2
MISSING_PARAM = 3
RUNTIME_ERROR = 4
NOT_FOUND = 5
UNSUPPORTED_TYPE = 6
INVALID_VALUE = 7


class InjectedBaseError(Exception):
    """Base class for exceptions based on errors returned from injected DLL side"""


class InjectedUnsupportedActionError(InjectedBaseError):
    """The specified action is not supported"""


class InjectedRuntimeError(InjectedBaseError):
    """Runtime exception during code execution inside injected target"""


class InjectedNotFoundError(InjectedBaseError):
    """Requested item not found: control element, property, ..."""


class Singleton(type):
    """
    Singleton metaclass implementation from StackOverflow

    http://stackoverflow.com/q/6760685/3648361
    """

    _instances = {}

    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(Singleton, cls).__call__(*args, **kwargs)
        return cls._instances[cls]


@six.add_metaclass(Singleton)
class ConnectionManager(object):
    def __init__(self):
        self._pipes = {}

    def _get_pipe(self, pid):
        if pid not in self._pipes:
            self._pipes[pid] = self._create_pipe(pid)
        return self._pipes[pid]

    def _create_pipe(self, pid):
        pipe_name = 'process_{}'.format(pid)
        pipe = Pipe(pipe_name)
        if pipe.connect(n_attempts=1):
            ActionLogger().log('Pipe {} found, looks like dll has been injected already'.format(pipe_name))
            return pipe
        else:
            ActionLogger().log('Pipe {} not found, injecting dll to the process'.format(pipe_name))
            Injector(pid, 'dotnet', 'bootstrap')
            pipe.connect()
            return pipe

    def call_action(self, action_name, pid, **params):
        command = {'action': action_name}
        command.update(params)

        reply = self._get_pipe(pid).transact(json.dumps(command))
        reply = json.loads(reply)

        reply_code = reply['status_code']
        reply_message = reply['message']

        if reply_code == UNSUPPORTED_ACTION:
            raise InjectedUnsupportedActionError(reply_message)
        elif reply_code == RUNTIME_ERROR:
            raise InjectedRuntimeError(reply_message)
        elif reply_code == NOT_FOUND:
            raise InjectedNotFoundError(reply_message)
        elif reply_code != OK:
            raise InjectedBaseError()

        return reply
