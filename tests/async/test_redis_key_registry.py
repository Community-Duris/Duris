#!/usr/bin/env python3
"""Require runtime and destructive maintenance to use the Redis registry."""

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REGISTRY = ROOT / "src" / "redis_key_registry.def"
registry = REGISTRY.read_text(encoding="ascii")

surface_names = re.findall(r"^REDIS_SURFACE\(([A-Z0-9_]+),", registry, re.MULTILINE)
store_names = re.findall(r"^REDIS_STORE\(([A-Z0-9_]+),", registry, re.MULTILINE)
owned_patterns = re.findall(
    r'^REDIS_OWNED_PATTERN\([A-Z0-9_]+, "([^"]+)"\)$', registry, re.MULTILINE
)
assert len(surface_names) == len(set(surface_names)) == 42
assert len(store_names) == len(set(store_names)) == 5
assert owned_patterns == ["<namespace>:*", "mud:*", "ship:snapshot:*"]

redis_literals = re.compile(r'"(?:mud|ship):')
for source in sorted((ROOT / "src").rglob("*.[ch]")):
    if source.name in {"redis_key_registry.c", "redis_key_registry.h"}:
        continue
    assert not redis_literals.search(source.read_text(encoding="utf-8")), source

clear_script = (ROOT / "scripts" / "clear-duris-redis-keys.sh").read_text(encoding="ascii")
assert 'PATTERNS=("$REDIS_NAMESPACE:*" \'mud:*\' \'ship:snapshot:*\')' in clear_script
assert "REDIS_NAMESPACE must match duris:local:<deployment>" in clear_script

print("Redis runtime, lifecycle, and destructive-maintenance registry contract passed")
