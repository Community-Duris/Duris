#!/usr/bin/env python3
"""Keep the documented server build matrix aligned with the maintained target."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
makefile = (ROOT / "src" / "Makefile").read_text(encoding="ascii")
building = (ROOT / "docs" / "guides" / "BUILDING.md").read_text(encoding="ascii")
help_system = (ROOT / "docs" / "content" / "HELP_SYSTEM.md").read_text(encoding="ascii")
building_words = " ".join(building.split())
help_words = " ".join(help_system.split())

assert "#CFLAGS += -D__NO_MYSQL__" not in makefile
assert "make -C src PERSISTENCE_BACKEND=mariadb" in building
assert "make -C src PERSISTENCE_BACKEND=flatfile" in building
assert "does not add the MySQL include path or client library" in building_words
assert "mixed per-operation authority transfer is not supported" in building_words
assert "Hiredis and OpenSSL remain build dependencies" in building_words
assert "Without MySQL (`-D__NO_MYSQL__` builds)" in help_system
assert "same client-free content path serves" in help_words

print("supported MariaDB and client-free server build contract passed")
