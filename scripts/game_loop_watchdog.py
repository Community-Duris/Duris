#!/usr/bin/env python3
"""Observe only completed world loops of one directly supervised Linux game.

The private datagram socket survives copyover exec; a new game gets a new
socket. Lifecycle records grant bounded grace, never completed-loop credit.
"""

from __future__ import annotations

import argparse
import ctypes
from dataclasses import dataclass
import math
import os
from pathlib import Path
import select
import signal
import socket
import struct
import subprocess
import sys
import time


BOOT_FAILURE = 78
HUNG_REBOOT = 56
DIAGNOSTIC_SECONDS = 2


@dataclass(frozen=True)
class Deadlines:
    startup: float = 300
    stall: float = 30
    copyover: float = 120
    shutdown: float = 60
    abort: float = 10

    @classmethod
    def from_environment(cls) -> Deadlines:
        values = {}
        for name, default in cls().__dict__.items():
            key = f"DURIS_WATCHDOG_{name.upper()}_SECONDS"
            value = float(os.environ.get(key, default))
            if not math.isfinite(value) or not 0 < value <= 86400:
                raise ValueError(f"{key} must be finite and between 0 and 86400 seconds")
            values[name] = value
        return cls(**values)


class Progress:
    def __init__(self, pid: int, started: float, limits: Deadlines):
        self.pid = pid
        self.limits = limits
        self.phase = "startup"
        self.deadline = started + limits.startup
        self.sequence = 0
        self.ever_completed = False
        self.last_completed: float | None = None
        self.last_record = started
        self.lifecycle_sequence = -1

    def receive(self, record: bytes, sender: int, now: float) -> None:
        try:
            version, pid, sequence, timestamp, phase = record.decode("ascii").split()
            sequence = int(sequence)
            emitted = int(timestamp) / 1_000_000
            if (version != "1" or int(pid) != self.pid or sender != self.pid
                    or not 0 <= sequence < 2**64
                    or not self.last_record <= emitted <= now + 0.01):
                return
        except (ValueError, UnicodeError):
            return

        if phase == "P" and sequence > self.sequence:
            self.sequence = sequence
            self.ever_completed = True
            self.last_completed = emitted
            self.phase = "running"
            self.deadline = emitted + self.limits.stall
        elif phase == "B" and self.phase == "copyover":
            self.phase = "copyover startup"
            self.deadline = emitted + self.limits.startup
        elif phase in ("C", "S", "D") and sequence >= self.sequence:
            if self.phase == "running" and sequence > self.lifecycle_sequence:
                self.lifecycle_sequence = sequence
                self.phase = {"C": "copyover", "S": "stopping", "D": "restart drain"}[phase]
                grace = self.limits.copyover if phase == "C" else self.limits.shutdown
                self.deadline = emitted + grace
            elif self.phase in ("startup", "copyover startup") and phase in ("S", "D"):
                self.phase = "stopping" if phase == "S" else "restart drain"
                self.deadline = min(self.deadline, emitted + self.limits.shutdown)
        elif phase == "R" and self.phase in ("copyover", "stopping", "restart drain"):
            self.phase = "running"
            self.deadline = emitted + self.limits.stall
        else:
            return
        self.last_record = emitted


def child_parent_death_signal(parent: int) -> None:
    # Set before exec, including the interval before the game's main() runs.
    # This process is single-threaded when Popen invokes the child hook.
    libc = ctypes.CDLL(None, use_errno=True)
    if libc.prctl(1, signal.SIGKILL, 0, 0, 0) != 0:  # PR_SET_PDEATHSIG
        os._exit(BOOT_FAILURE)
    if os.getppid() != parent:
        os._exit(BOOT_FAILURE)


def delegated_world(record: bytes, sender: int, supervisor: int, started: float,
                    now: float) -> tuple[int, float] | None:
    """Only the launched supervisor may nominate its directly owned world."""
    if sender != supervisor:
        return None
    try:
        version, parent, world, timestamp = record.decode("ascii").split()
        world = int(world)
        emitted = int(timestamp) / 1_000_000
        if (version != "2" or int(parent) != supervisor or world <= 1
                or not started <= emitted <= now + 0.01):
            return None
        status = Path(f"/proc/{world}/status").read_text()
        if f"PPid:\t{supervisor}\n" not in status:
            return None
        return world, emitted
    except (ValueError, UnicodeError, OSError):
        return None


def signal_group(child: subprocess.Popen, signum: int, cleanup: bool = False) -> None:
    # This child is the leader of a new session, never the launcher's group.
    if cleanup or child.poll() is None:
        try:
            os.killpg(child.pid, signum)
        except ProcessLookupError:
            pass


def owned_world_running(world: int, supervisor: int) -> bool:
    try:
        status = Path(f"/proc/{world}/stat").read_text().rsplit(")", 1)[1].split()
        return status[0] != "Z" and os.getpgid(world) == supervisor
    except (OSError, IndexError):
        return False


def diagnostic_snapshot(pid: int, reason: str) -> None:
    directory = Path("logs/watchdog")
    directory.mkdir(mode=0o700, parents=True, exist_ok=True)
    path = directory / f"{time.time_ns()}-{pid}.txt"
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    print(f"GAME LOOP WATCHDOG: diagnostics {path}", file=sys.stderr, flush=True)
    with os.fdopen(fd, "w") as stream:
        stream.write(reason + "\n")
        stream.flush()
        snapshot_procfs(pid, stream)


def snapshot_procfs(pid: int, stream) -> None:
    root = Path(f"/proc/{pid}")
    print(f"game pid={pid} executable={os.readlink(root / 'exe')}", file=stream, flush=True)
    paths = [root / name for name in ("stat", "status", "limits")]
    for task in sorted((root / "task").iterdir(), key=lambda path: int(path.name))[:64]:
        paths.extend(task / name for name in ("stat", "status", "wchan", "syscall", "stack"))
    for path in paths:
        print(f"\n--- {path} ---", file=stream, flush=True)
        try:
            with path.open() as procfile:
                print(procfile.read(16384), file=stream, flush=True)
        except OSError as error:
            print(f"unavailable: {error}", file=stream, flush=True)


def capture_diagnostics(child: subprocess.Popen, reason: str, world_pid: int) -> None:
    print(f"GAME LOOP WATCHDOG: {reason}", file=sys.stderr, flush=True)
    try:
        # Both file writes and procfs inspection run separately with a hard
        # budget; diagnostics cannot hold up the game termination deadline.
        diagnostic = subprocess.Popen(
            [sys.executable, __file__, "--diagnose", str(world_pid), reason],
            start_new_session=True,
        )
        try:
            diagnostic.wait(timeout=DIAGNOSTIC_SECONDS)
        except subprocess.TimeoutExpired:
            diagnostic.kill()
            try:
                diagnostic.wait(timeout=1)
            except subprocess.TimeoutExpired:
                pass
            print("GAME LOOP WATCHDOG: diagnostic budget expired", file=sys.stderr, flush=True)
    except OSError as error:
        print(f"GAME LOOP WATCHDOG: diagnostics unavailable: {error}", file=sys.stderr, flush=True)


def observe(command: list[str], limits: Deadlines) -> int:
    persistent_transport = "--persistent-transport" in command
    receiver, sender = socket.socketpair(socket.AF_UNIX, socket.SOCK_DGRAM)
    receiver.setsockopt(socket.SOL_SOCKET, socket.SO_PASSCRED, 1)
    receiver.setblocking(False)
    environment = os.environ.copy()
    environment["DURIS_WATCHDOG_FD"] = str(sender.fileno())
    environment.pop("DURIS_WATCHDOG_SEQUENCE", None)
    started = time.monotonic()
    parent = os.getpid()
    child = subprocess.Popen(
        command, env=environment, pass_fds=(sender.fileno(),), start_new_session=True,
        preexec_fn=lambda: child_parent_death_signal(parent),
    )
    sender.close()
    progress = Progress(child.pid, started, limits)
    stopping_at: float | None = None
    forced_cleanup = False

    def forward(signum: int, _frame) -> None:
        nonlocal stopping_at
        if signum in (signal.SIGTERM, signal.SIGINT, signal.SIGHUP, signal.SIGUSR2):
            if stopping_at is None:
                stopping_at = time.monotonic()
        if persistent_transport:
            # The transport forwards once to its world. Group delivery here
            # would send the same lifecycle request to the world twice.
            try:
                child.send_signal(signum)
            except ProcessLookupError:
                pass
        else:
            signal_group(child, signum)

    handlers = {signum: signal.signal(signum, forward) for signum in (
        signal.SIGTERM, signal.SIGINT, signal.SIGHUP,
        signal.SIGUSR1, signal.SIGUSR2, signal.SIGRTMIN,
    )}
    try:
        while child.poll() is None:
            now = time.monotonic()
            deadline = (stopping_at + limits.shutdown
                        if stopping_at is not None else progress.deadline)
            # Read already-queued completions before deciding a deadline
            # expired. Scheduling delays of the observer are not world stalls.
            if select.select([receiver], [], [], max(0, min(0.1, deadline - now)))[0]:
                # A finite batch still enforces deadlines against flooding or
                # replay; it is larger than the normal socket-buffer backlog.
                for _ in range(4096):
                    try:
                        record, credentials, flags, _address = receiver.recvmsg(
                            256, socket.CMSG_SPACE(struct.calcsize("3i")))
                    except BlockingIOError:
                        break
                    if flags & (socket.MSG_TRUNC | socket.MSG_CTRUNC):
                        continue
                    pid = -1
                    for level, kind, data in credentials:
                        if level == socket.SOL_SOCKET and kind == socket.SCM_CREDENTIALS:
                            pid, _uid, _gid = struct.unpack("3i", data)
                    received_at = time.monotonic()
                    delegation = (delegated_world(record, pid, child.pid, started, received_at)
                                  if persistent_transport else None)
                    if delegation is not None:
                        world, emitted = delegation
                        if world != progress.pid:
                            progress = Progress(world, emitted, limits)
                    else:
                        progress.receive(record, pid, received_at)

            now = time.monotonic()
            deadline = (stopping_at + limits.shutdown
                        if stopping_at is not None else progress.deadline)
            if now >= deadline:
                forced_cleanup = True
                age = "none" if progress.last_completed is None else f"{now - progress.last_completed:.3f}s"
                reason = (f"pid={progress.pid} supervisor={child.pid} phase={progress.phase} deadline expired; "
                          f"completed={progress.sequence} last_completed_age={age}")
                capture_diagnostics(child, reason, progress.pid)
                # SIGABRT creates native thread stacks via a core dump without
                # a cooperative game callback; SIGKILL bounds blocked/ignored aborts.
                if persistent_transport:
                    signal_group(child, signal.SIGABRT)
                else:
                    child.send_signal(signal.SIGABRT)
                try:
                    child.wait(timeout=limits.abort)
                except subprocess.TimeoutExpired:
                    signal_group(child, signal.SIGKILL)
                    child.wait(timeout=1)
                if stopping_at is not None or progress.phase == "stopping":
                    return 0
                return HUNG_REBOOT if progress.ever_completed else BOOT_FAILURE

        if stopping_at is not None:
            return 0
        status = child.returncode
        return status if status >= 0 else 128 - status
    finally:
        receiver.close()
        try:
            # Frontend death closes IPC. Let the surviving world finish its
            # bounded durable shutdown before removing the owned process group.
            world_exit_deadline = time.monotonic() + limits.shutdown
            while (persistent_transport and not forced_cleanup and progress.pid != child.pid
                   and owned_world_running(progress.pid, child.pid)
                   and time.monotonic() < world_exit_deadline):
                time.sleep(0.05)
            signal_group(child, signal.SIGKILL, cleanup=True)
            child.wait(timeout=1)
        except subprocess.TimeoutExpired as error:
            raise RuntimeError(f"pid={child.pid} did not exit after SIGKILL; refusing replacement") from error
        finally:
            for signum, handler in handlers.items():
                signal.signal(signum, handler)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check-config", action="store_true")
    parser.add_argument("--diagnose", nargs=2, help=argparse.SUPPRESS)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if args.diagnose is not None:
        diagnostic_snapshot(int(args.diagnose[0]), args.diagnose[1])
        return 0
    try:
        limits = Deadlines.from_environment()
        if args.check_config:
            return 0
        command = args.command[1:] if args.command[:1] == ["--"] else args.command
        if not command:
            parser.error("a game executable is required after --")
        return observe(command, limits)
    except (ValueError, OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"GAME LOOP WATCHDOG: {error}", file=sys.stderr, flush=True)
        return BOOT_FAILURE


if __name__ == "__main__":
    raise SystemExit(main())
