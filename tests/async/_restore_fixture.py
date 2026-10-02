"""Build the native restore fixture shared by craft and backup regressions."""

import os
import subprocess
import sys

from _paths import ROOT

sys.path.insert(0, str(ROOT / "scripts"))
import build_restore_qualifier as native


def build(destination):
    # The fixture mutates items; the listener-free qualifier can discard those
    # sections and therefore has a smaller link dependency set.
    names = list(dict.fromkeys(native.SOURCES + [
        "flatfile_collector_repository", "collector_command", "collector_codec",
        "collector_policy", "collector_accounting",
    ]))
    sources = []
    for name in names:
        found = list((ROOT / "src").rglob(name + ".c"))
        if len(found) != 1:
            raise RuntimeError("ambiguous restore fixture source: " + name)
        sources.append(str(found[0]))
    destination.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([
        os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-D__NO_MYSQL__", "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST",
        "-DDURIS_FLATFILE_TRANSACTION_FAULT_TEST", "-DDURIS_FLATFILE_ACCOUNTING_TEST",
        "-Isrc", "-Isrc/no_mysql", "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
        "tests/async/persistence_restore_fixture.cpp", *sources, "-lcrypto", "-lz", "-pthread",
        "-o", str(destination),
    ], cwd=ROOT, check=True)
    return destination
