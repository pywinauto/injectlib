import os
import platform
import struct

from . import cfuncs


class LinuxInjector(object):
    """Load an injectlib shared object into a same-user Linux process."""

    def __init__(self, pid, backend_name, dll_name):
        self.pid = int(pid)
        self.backend_name = backend_name
        self.dll_name = dll_name
        self._validate_process()
        self.library_path = self._library_path()
        self._inject_shared_object()

    def _validate_process(self):
        proc_dir = "/proc/{}".format(self.pid)
        if not os.path.isdir(proc_dir):
            raise RuntimeError("Target process does not exist: pid={}".format(self.pid))
        if platform.machine().lower() not in ("x86_64", "amd64"):
            raise RuntimeError("Linux Qt attach currently supports x86_64 only")

    def _library_path(self):
        arch = "x64"
        path = os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            "libs",
            self.backend_name,
            arch,
            "{}.so".format(self.dll_name),
        )
        if not os.path.isfile(path):
            raise RuntimeError("Linux Qt server library is missing: {}".format(path))
        return path

    def _inject_shared_object(self):
        remote_dlopen = cfuncs.remote_symbol_address(self.pid, cfuncs.dlopen)
        if not remote_dlopen:
            raise RuntimeError("Cannot resolve remote dlopen address for pid {}".format(self.pid))

        cfuncs.ptrace_attach(self.pid)
        saved_payload = None
        saved_code = None
        saved_regs = None
        stack_addr = None
        code_len = None
        try:
            regs = cfuncs.ptrace_getregs(self.pid)
            saved_regs = cfuncs.UserRegsStruct()
            for field, _ in cfuncs.UserRegsStruct._fields_:
                setattr(saved_regs, field, getattr(regs, field))

            code, path_addr = self._remote_dlopen_stub(remote_dlopen, regs.rsp)
            code_len = len(code)
            payload = self.library_path.encode("utf-8") + b"\0"
            payload_len = ((128 + len(payload) + 7) // 8) * 8
            stack_addr = (regs.rsp - payload_len - 128) & ~0xF
            path_addr = stack_addr + 128
            code, _ = self._remote_dlopen_stub(remote_dlopen, regs.rsp, path_addr)

            saved_payload = cfuncs.read_target(self.pid, stack_addr, payload_len)
            saved_code = cfuncs.read_target(self.pid, regs.rip, code_len)

            stack_payload = bytearray(payload_len)
            stack_payload[128:128 + len(payload)] = payload
            cfuncs.write_target(self.pid, stack_addr, bytes(stack_payload))
            cfuncs.write_target(self.pid, regs.rip, code)

            regs.rip = saved_regs.rip
            regs.rsp = stack_addr
            cfuncs.ptrace_setregs(self.pid, regs)
            stop_signal = cfuncs.ptrace_continue(self.pid)
            if stop_signal != cfuncs.SIGTRAP:
                raise RuntimeError("Remote dlopen stopped with unexpected signal {}".format(stop_signal))

            after = cfuncs.ptrace_getregs(self.pid)
            if after.rax == 0:
                raise RuntimeError("Remote dlopen returned NULL for {}".format(self.library_path))
        finally:
            if saved_payload is not None and stack_addr is not None:
                cfuncs.write_target(self.pid, stack_addr, saved_payload)
            if saved_code is not None and saved_regs is not None and code_len is not None:
                cfuncs.write_target(self.pid, saved_regs.rip, saved_code[:code_len])
            if saved_regs is not None:
                cfuncs.ptrace_setregs(self.pid, saved_regs)
            cfuncs.ptrace_detach(self.pid)

        if not self._target_has_library():
            raise RuntimeError("Remote dlopen did not map {} in pid {}".format(
                self.library_path, self.pid))

    def _remote_dlopen_stub(self, remote_dlopen, stack_pointer, path_addr=None):
        if path_addr is None:
            path_addr = stack_pointer
        code = bytearray()
        code.extend(b"\x48\xb8")
        code.extend(struct.pack("<Q", remote_dlopen))
        code.extend(b"\x48\xbf")
        code.extend(struct.pack("<Q", path_addr))
        code.extend(b"\xbe")
        code.extend(struct.pack("<I", cfuncs.RTLD_NOW | cfuncs.RTLD_GLOBAL))
        code.extend(b"\xff\xd0")
        code.extend(b"\xcc")
        return bytes(code), path_addr

    def _target_has_library(self):
        real_library = os.path.realpath(self.library_path)
        with open("/proc/{}/maps".format(self.pid), "r") as maps_file:
            return any(real_library in line for line in maps_file)
