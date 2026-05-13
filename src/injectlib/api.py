import json
import logging
import sys

from .channel import Pipe
if sys.platform == "win32":
    from .injector import Injector
else:
    from .linux_injector import LinuxInjector as Injector

logger = logging.getLogger(__package__)

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
        self._backend_config = {}

    def register_backend(self, pid, backend_name, dll_name):
        """Register backend to use when injecting into pid. Must be called before the first call_action for that pid."""
        self._backend_config[pid] = (backend_name, dll_name)

    def _get_pipe(self, pid):
        if pid not in self._pipes:
            self._pipes[pid] = self._create_pipe(pid)
        return self._pipes[pid]

    def _create_pipe(self, pid):
        pipe_name = 'process_{}'.format(pid)
        pipe = Pipe(pipe_name)
        if pipe.connect(n_attempts=1):
            logger.info('Pipe {} found, looks like dll has been injected already'.format(pipe_name))
            return pipe
        else:
            logger.info('Pipe {} not found, injecting dll to the process'.format(pipe_name))
            # Fallback to dotnet to keep compatibility
            backend_name, dll_name = self._backend_config.get(pid, ('dotnet', 'bootstrap'))
            Injector(pid, backend_name, dll_name)
            if not pipe.connect():
                raise InjectedRuntimeError(
                    "Injected server channel did not appear for pid {}".format(pid))
            return pipe

    def call_action(self, action_name, pid, **params):
        command = {'action': action_name}
        command.update(params)

        reply = self._get_pipe(pid).transact(json.dumps(command))
        reply = json.loads(reply)

        reply_code = reply['status_code']

        if reply_code == UNSUPPORTED_ACTION:
            raise InjectedUnsupportedActionError(reply['message'])
        elif reply_code == RUNTIME_ERROR:
            raise InjectedRuntimeError(reply['message'])
        elif reply_code == NOT_FOUND:
            raise InjectedNotFoundError(reply['message'])
        elif reply_code != OK:
            raise InjectedBaseError()

        return reply
