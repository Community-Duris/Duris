"""Validate the complete inventory before selecting a regression profile."""

from dataclasses import dataclass
import json
import math
from pathlib import Path

PROFILES = ("core", "fast", "native", "journey", "database", "recovery")


@dataclass(frozen=True)
class TestSpec:
    path: Path
    profile: str = "fast"
    mode: str = "script"
    minimum_cases: int = 1
    cpu: int = 1
    memory_mb: int = 256
    locks: tuple[str, ...] = ()
    seconds: float = 1.0
    timeout_seconds: float = 900.0
    manual: bool = False
    reason: str = ""
    purpose: str = ""
    also_profiles: tuple[str, ...] = ()


def inventory(directory: Path, manifest: Path) -> list[TestSpec]:
    document = json.loads(manifest.read_text(encoding="utf-8"))
    if document["version"] != 1:
        raise ValueError("unsupported regression inventory version")
    entries = document["tests"]
    discovered = {path.name: path for path in
                  set(directory.glob("test_*.py")) | set(directory.glob("*_test.py"))}
    missing = discovered.keys() - entries.keys()
    stale = entries.keys() - discovered.keys()
    if missing or stale:
        raise ValueError("regression inventory mismatch; unclassified: "
                         + ", ".join(sorted(missing)) + "; missing files: "
                         + ", ".join(sorted(stale)))
    result = []
    for name, row in sorted(entries.items()):
        if Path(name).name != name or row["profile"] not in PROFILES[1:]:
            raise ValueError(f"invalid regression profile/path: {name}")
        spec = TestSpec(path=discovered[name], **row)
        if (spec.mode not in {"script", "unittest"} or spec.minimum_cases < 1
                or spec.cpu < 1 or spec.memory_mb < 1 or not math.isfinite(spec.seconds) or spec.seconds < 0
                or not math.isfinite(spec.timeout_seconds) or spec.timeout_seconds <= 0
                or len(set(spec.locks)) != len(spec.locks)
                or any(profile not in PROFILES[1:] for profile in spec.also_profiles)
                or not spec.purpose or (spec.manual and not spec.reason)):
            raise ValueError(f"invalid regression metadata: {name}")
        result.append(spec)
    return result


def select(specs: list[TestSpec], profile: str, match: str | None) -> list[TestSpec]:
    return [spec for spec in specs
            if (not spec.manual if profile == "core" else
                (spec.profile == profile or profile in spec.also_profiles)
                and (not spec.manual or profile in {"database", "recovery"}))
            and (not match or match in spec.path.name)]
