import logging
import os
import socket
import sys
import time

logger = logging.getLogger(__package__)

if sys.platform == "win32":
    import pywintypes
    import win32file
    import win32pipe
    import winerror


class InjectedBrokenPipeError(Exception):
    pass


if sys.platform == "win32":
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
                    logger.info('Connected to the pipe {}'.format(self.name))
                    break
                except pywintypes.error as e:
                    if e.args[0] == winerror.ERROR_FILE_NOT_FOUND:
                        logger.warning('Attempt {}/{}: failed to connect to the pipe {}'.format(
                            i + 1, n_attempts, self.name))
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

else:
    class Pipe(object):
        def __init__(self, name):
            self.name = name
            self.path = "/tmp/injectlib_qt_{0}_{1}.sock".format(
                os.getuid(), name.rsplit("_", 1)[-1])

        def connect(self, n_attempts=30, delay=1):
            for i in range(n_attempts):
                client = None
                try:
                    client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                    client.connect(self.path)
                    return True
                except OSError:
                    pass
                finally:
                    if client is not None:
                        try:
                            client.close()
                        except Exception:
                            pass

                if i < n_attempts - 1:
                    logger.warning('Attempt {}/{}: failed to connect to the socket {}'.format(
                        i + 1, n_attempts, self.path))
                    time.sleep(delay)
            return False

        def transact(self, string):
            client = None
            try:
                client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                client.connect(self.path)
                client.sendall(string.encode("utf-8"))
                client.shutdown(socket.SHUT_WR)
                chunks = []
                while True:
                    chunk = client.recv(64 * 1024)
                    if not chunk:
                        break
                    chunks.append(chunk)
                return b"".join(chunks).decode("utf-8")
            except OSError as exc:
                raise InjectedBrokenPipeError('Unexpected socket error: {}'.format(exc))
            finally:
                if client is not None:
                    try:
                        client.close()
                    except Exception:
                        pass

        def close(self):
            pass
