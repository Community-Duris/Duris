"""Private regression-build supervisor and atomic C/C++ object publisher."""

import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import threading


def checked_path(path, root):
    path, root = Path(path).absolute(), Path(root).absolute()
    if (root.resolve() != root or path.resolve() != path or
            not path.is_relative_to(root) or path == root):
        raise ValueError("compiler output escaped its object workspace")
    for component in (root, *path.relative_to(root).parents):
        candidate = component if component == root else root / component
        if candidate.is_symlink():
            raise ValueError("compiler output has a symlink ancestor")
    if path.is_symlink() or (path.exists() and path.stat().st_nlink != 1):
        raise ValueError("compiler output is linked outside its workspace")
    return path


def compile_atomic(root, command):
    if "-c" not in command:
        return subprocess.call(command)
    rewritten = []
    output = None
    arguments = iter(command)
    for argument in arguments:
        if argument == "-o":
            output = next(arguments)
        elif argument.startswith("-o") and len(argument) > 2:
            output = argument[2:]
        elif argument in ("-MF", "-MT", "-MQ"):
            next(arguments)
        elif any(argument.startswith(option) for option in ("-MF", "-MT", "-MQ")):
            continue
        else:
            rewritten.append(argument)
    if not output:
        raise ValueError("object compilation did not select an output")
    output = checked_path(output, root)
    dependency = checked_path(output.with_suffix(".d"), root)
    output.parent.mkdir(parents=True, exist_ok=True)
    temporaries = []
    try:
        for suffix in (".o", ".d"):
            fd, path = tempfile.mkstemp(prefix=".duris-compile-", suffix=suffix, dir=output.parent)
            os.close(fd)
            temporaries.append(Path(path))
        object_temp, dependency_temp = temporaries
        if not any(argument in ("-MD", "-MMD") for argument in rewritten):
            rewritten.append("-MMD")
        status = subprocess.call(rewritten + ["-o", str(object_temp), "-MF", str(dependency_temp),
                                               "-MT", str(output)])
        if status:
            return status
        if not object_temp.stat().st_size or not dependency_temp.stat().st_size:
            raise ValueError("compiler succeeded without an object and dependency pair")
        checked_path(output, root)
        checked_path(dependency, root)
        # The final object is the completion marker. A killed compiler never
        # publishes it; a crash between these renames leaves the old valid object.
        os.replace(dependency_temp, dependency)
        os.replace(object_temp, output)
        return 0
    finally:
        for path in temporaries:
            path.unlink(missing_ok=True)


def supervise(watch_fd, command):
    # The caller owns the only write end. Even SIGKILL of the caller closes it,
    # so this watchdog kills the entire new build group, including this process.
    def parent_gone():
        try:
            while os.read(watch_fd, 1):
                pass
        finally:
            os.killpg(os.getpgrp(), signal.SIGKILL)

    threading.Thread(target=parent_gone, daemon=True).start()
    return subprocess.call(command)


if __name__ == "__main__":
    mode, value, separator, *command = sys.argv[1:]
    if separator != "--" or not command:
        raise ValueError("invalid private compiler invocation")
    if mode == "--compile":
        status = compile_atomic(value, command)
    elif mode == "--supervise":
        status = supervise(int(value), command)
    else:
        raise ValueError("unknown private compiler mode")
    sys.exit(status if status >= 0 else 128 - status)
