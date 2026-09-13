"""Build the production scenery harness for tests, previews, and offline auditing."""
from contextlib import contextmanager
import os
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CONFIG = ROOT / "docs/examples/scenery-profiles-v1.json"


def alternate_config():
    config = json.loads(CONFIG.read_text())
    swaps = {"green": "cyan", "bright_green": "bright_cyan",
             "blue": "cyan", "bright_blue": "bright_cyan", "cyan": "blue",
             "bright_cyan": "bright_blue"}
    for recipe in config["recipes"].values():
        recipe["palette"] = [swaps.get(color, color) for color in recipe["palette"]]
    return config


@contextmanager
def scenery_harness():
    build = ROOT / "bin/tests"
    build.mkdir(parents=True, exist_ok=True)
    flags = (["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
             if os.environ.get("SANITIZE") == "1" else [])
    with tempfile.TemporaryDirectory(prefix="scenery-", dir=build) as directory:
        binary = Path(directory) / "harness"
        subprocess.run([
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Wpedantic", "-Og", "-g",
            "-D__NO_MYSQL__", *flags, f"-I{ROOT / 'src'}", f"-I{ROOT / 'src/no_mysql'}",
            str(ROOT / "tests/async/scenery_animation_harness.cpp"),
            *(str(ROOT / "src" / path) for path in (
                "net/output_profiles.c", "net/output_style.c", "net/ansi.c", "net/unicode.c")),
            "-lcjson", "-pthread", "-o", str(binary)
        ], check=True, timeout=120)
        yield binary
