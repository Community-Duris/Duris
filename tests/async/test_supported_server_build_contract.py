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
assert "MySQL/MariaDB client support is a mandatory server build dependency" in building_words
assert "do not define or advertise a supported whole-server build" in building_words
assert "Hiredis and OpenSSL remain build dependencies" in building_words
assert "complete server has no supported no-MySQL build" in help_words

print("supported mandatory-MySQL server build contract passed")
