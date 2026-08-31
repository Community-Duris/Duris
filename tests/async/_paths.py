"""Resolve `src/` paths independently of the subdirectory layout.

Tests historically hardcoded `source("foo.c")`, which pins every test to
the physical location of a source file.  Source files now live in topical
subdirectories (`src/core/`, `src/world/`, ...), so tests address them by bare
filename and let this module find them.

Usage:

    from _paths import ROOT, SRC, source

    text = source("comm.c").read_text(encoding="utf-8")
    text = (SRC / "comm.c").read_text(encoding="utf-8")
"""

from __future__ import annotations

import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC_ROOT = ROOT / "src"

_index: dict[str, Path] | None = None


def _build_index() -> dict[str, Path]:
    """Map each basename under src/ to its path, shallowest match winning."""
    index: dict[str, Path] = {}
    for path in sorted(SRC_ROOT.rglob("*")):
        if not path.is_file():
            continue
        name = path.name
        if name not in index:
            index[name] = path
        else:
            # Prefer the shallower file so a top-level name is never shadowed.
            if len(path.relative_to(SRC_ROOT).parts) < len(
                index[name].relative_to(SRC_ROOT).parts
            ):
                index[name] = path
    return index


def source(name: str | os.PathLike[str]) -> Path:
    """Resolve a source file by bare name, or by path relative to src/.

    A name containing a separator is treated as already-located and returned
    relative to src/.  An unknown name resolves to src/<name> so callers still
    produce a meaningful path in failure messages.
    """
    global _index
    text = os.fspath(name)
    if "/" in text or os.sep in text:
        return SRC_ROOT / text
    if _index is None:
        _index = _build_index()
    found = _index.get(text)
    return found if found is not None else SRC_ROOT / text


def invalidate_cache() -> None:
    """Drop the cached index; for tests that create files under src/."""
    global _index
    _index = None


class SourceDir(os.PathLike):
    """Stand-in for the src/ directory that resolves children by name.

    Behaves like `Path(src)` for globbing and string conversion, but `/`
    resolves a bare filename anywhere in the tree.
    """

    __slots__ = ()

    def __truediv__(self, other: str | os.PathLike[str]) -> Path:
        return source(other)

    def __fspath__(self) -> str:
        return str(SRC_ROOT)

    def __str__(self) -> str:
        return str(SRC_ROOT)

    def __repr__(self) -> str:
        return f"SourceDir({str(SRC_ROOT)!r})"

    def glob(self, pattern: str):
        """Glob across the whole tree when the pattern names no directory.

        `glob("*.c")` historically meant "every source file"; keep that meaning
        now that sources sit one level down.
        """
        if "/" in pattern:
            return SRC_ROOT.glob(pattern)
        return SRC_ROOT.rglob(pattern)

    def rglob(self, pattern: str):
        return SRC_ROOT.rglob(pattern)

    def iterdir(self):
        return SRC_ROOT.iterdir()

    def is_dir(self) -> bool:
        return SRC_ROOT.is_dir()

    def exists(self) -> bool:
        return SRC_ROOT.exists()

    @property
    def path(self) -> Path:
        return SRC_ROOT


SRC = SourceDir()


def rel(name: str | os.PathLike[str]) -> str:
    """Repo-relative POSIX path for a source file, e.g. "src/core/comm.c".

    Use where a test passes a path to a subprocess run from the repo root, or
    compares against tool output that prints repo-relative paths.
    """
    return source(name).relative_to(ROOT).as_posix()
