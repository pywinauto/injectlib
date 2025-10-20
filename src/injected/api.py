import json

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


class ConnectionManager(object, metaclass=Singleton):
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
