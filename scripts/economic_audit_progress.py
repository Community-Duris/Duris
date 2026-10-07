"""Protected local checkpoint I/O shared by read-only economy audits."""
from contextlib import contextmanager
import json
import os
from pathlib import Path
import stat
import tempfile


def load(path, limit, error, label):
    fd = os.open(path, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0) |
                 getattr(os, "O_NONBLOCK", 0))
    with os.fdopen(fd, "rb") as stream:
        info = os.fstat(stream.fileno())
        if (not stat.S_ISREG(info.st_mode) or Path(path).is_symlink() or info.st_nlink != 1 or
                info.st_size > limit or (os.name == "posix" and
                (info.st_uid != os.getuid() or info.st_mode & 0o077))):
            raise error(label + " requires a protected regular file")
        data = stream.read(limit + 1)
    if len(data) > limit:
        raise error(label + " exceeds byte limit")
    def pairs(rows):
        result = {}
        for key, value in rows:
            if key in result:
                raise error("duplicate " + label + " field")
            result[key] = value
        return result
    try:
        return json.loads(data, object_pairs_hook=pairs)
    except ValueError as failure:
        raise error("invalid " + label + " JSON") from failure


def save(path, value, limit, error, label):
    data = (json.dumps(value, allow_nan=False, sort_keys=True, separators=(",", ":")) + "\n").encode()
    if len(data) > limit:
        raise error(label + " exceeds byte limit")
    path = Path(path)
    fd, temporary = tempfile.mkstemp(prefix="." + path.name + "-", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
        if os.name == "posix":
            directory = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY)
            try:
                os.fsync(directory)
            finally:
                os.close(directory)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


@contextmanager
def lock(path, error, label):
    if os.name != "posix":
        raise error("durable " + label + " CLI requires POSIX file locking")
    import fcntl
    fd = os.open(str(path) + ".lock", os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW | os.O_NONBLOCK, 0o600)
    try:
        info = os.fstat(fd)
        if (not stat.S_ISREG(info.st_mode) or info.st_nlink != 1 or info.st_uid != os.getuid() or
                info.st_mode & 0o077):
            raise error(label + " lock requires a protected regular file")
        try:
            fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as failure:
            raise error(label + " is already in use") from failure
        yield
        named = os.stat(str(path) + ".lock", follow_symlinks=False)
        if (named.st_dev, named.st_ino) != (info.st_dev, info.st_ino):
            raise error(label + " lock path changed")
    finally:
        os.close(fd)
