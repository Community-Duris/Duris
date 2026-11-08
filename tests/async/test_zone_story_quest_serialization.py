#!/usr/bin/env python3
"""Production quest serialization refuses swallowed stream allocation failures."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="duris-quest-serialization-") as temporary:
    binary = Path(temporary) / "serialization"
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
        "tests/async/zone_story_quest_serialization_harness.cpp",
        "src/world/zone_story_quest_feature.c", "src/world/zone_story_quest_tracking.c",
        "src/world/zone_story_quest_catalog.c", "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=30)
