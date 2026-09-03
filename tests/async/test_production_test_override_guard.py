#!/usr/bin/env python3
"""Regression contracts for production rejection of local-only overrides."""

from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
from pathlib import Path

from _paths import SRC


ROOT = Path(__file__).resolve().parents[2]
CHAOS_CONFIG = SRC / "combat/chaos_config.c"
CHAOS_HEADER = SRC / "combat/chaos_config.h"
ENV_FILE = SRC / "core/env_file.c"
COMM = SRC / "net/comm.c"
CYCLE = ROOT / "scripts/cycle_mud.sh"
KINGDOM = ROOT / "lib/kingdom.cfg"
SWITCHES = ("CHAOS_MUD", "CREATION_ALL_RACES", "CREATION_ALL_CLASSES")


def run_launcher(script: Path, env: dict[str, str], *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["bash", str(script), *args],
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=30,
    )


HARNESS = r'''
#include "combat/chaos_config.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

namespace
{
const char *switches[] = {
    "CHAOS_MUD", "CREATION_ALL_RACES", "CREATION_ALL_CLASSES"
};

void clear_switches()
{
    unsetenv("CHAOS_MUD");
    unsetenv("CREATION_ALL_RACES");
    unsetenv("CREATION_ALL_CLASSES");
}

bool validate(bool expected, const char *enabled_switch)
{
    clear_switches();
    if (enabled_switch)
        setenv(enabled_switch, "TRUE", 1);
    char error[256] = {};
    const bool allowed = chaos_config_validate_environment(error, sizeof(error));
    if (allowed != expected)
    {
        std::cerr << "unexpected validator result for "
                  << (enabled_switch ? enabled_switch : "baseline") << "\n";
        return false;
    }
    if (!expected && (!std::strstr(error, enabled_switch) ||
                      !std::strstr(error, "Production environment")))
    {
        std::cerr << "error did not identify the production override\n";
        return false;
    }
    return true;
}
}

int main()
{
    setenv("ENVIRONMENT", "production", 1);
    if (!validate(true, nullptr))
        return 1;
    for (const char *enabled_switch : switches)
        if (!validate(false, enabled_switch))
            return 1;

    setenv("ENVIRONMENT", "local", 1);
    setenv("CHAOS_MUD", "TRUE", 1);
    setenv("CREATION_ALL_RACES", "TRUE", 1);
    setenv("CREATION_ALL_CLASSES", "TRUE", 1);
    if (!validate(true, nullptr))
        return 1;

    std::cout << "compiled production override validator passed\n";
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-production-override-") as temporary:
    temporary_path = Path(temporary)
    harness = temporary_path / "validator.cpp"
    binary = temporary_path / "validator"
    harness.write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            str(harness),
            str(CHAOS_CONFIG),
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True, text=True)

    launcher_root = temporary_path / "launcher"
    scripts = launcher_root / "scripts"
    scripts.mkdir(parents=True)
    script = scripts / "cycle_mud.sh"
    shutil.copy2(CYCLE, script)
    shutil.copy2(ROOT / "scripts/backup_pfiles.sh", scripts / "backup_pfiles.sh")

    base_env = {
        "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "ENVIRONMENT": "local",
        "PERSISTENCE_MODE": "flatfile-primary",
        "FLATFILE_STATE_DIR": str(launcher_root / "state"),
        "FLATFILE_BACKUP_DIR": str(launcher_root / "backups"),
        "DURIS_DEV_PORT": "4000",
        "REDIS": "FALSE",
    }
    local = dict(base_env)
    for switch in SWITCHES:
        local[switch] = "TRUE"
    accepted = run_launcher(script, local, "--dev", "--check-config")
    assert accepted.returncode == 0, accepted.stdout
    assert "database-independent configuration" in accepted.stdout

    production = dict(base_env)
    production["ENVIRONMENT"] = "production"
    for switch in SWITCHES:
        rejected = dict(production)
        rejected[switch] = "TRUE"
        result = run_launcher(script, rejected, "--production", "--check-config")
        assert result.returncode != 0, f"{switch} was accepted:\n{result.stdout}"
        assert switch in result.stdout
        assert "production" in result.stdout.lower()

    clean_production = dict(production)
    accepted = run_launcher(script, clean_production, "--production", "--check-config")
    assert accepted.returncode == 0, accepted.stdout
    assert "database-independent configuration" in accepted.stdout

header = CHAOS_HEADER.read_text(encoding="utf-8")
config = CHAOS_CONFIG.read_text(encoding="utf-8")
env_file = ENV_FILE.read_text(encoding="utf-8")
comm = COMM.read_text(encoding="utf-8")
cycle = CYCLE.read_text(encoding="utf-8")
kingdom = KINGDOM.read_text(encoding="utf-8")
assert "chaos_config_validate_environment" in header
assert "ENVIRONMENT" in config
assert "CREATION_ALL_RACES" in config
assert "CREATION_ALL_CLASSES" in config
assert "chaos_config_validate_environment" in env_file
load_env = env_file[env_file.index("int load_env_file") :]
assert load_env.index("fclose(f);") < load_env.rindex("validate_environment_overrides()")
assert "chaos_config_validate_environment" not in comm
assert "Production environment cannot enable" in cycle
assert cycle.index("Production environment cannot enable") < cycle.index("if (( DATABASE_REQUIRED")
assert cycle.index("Production environment cannot enable") < cycle.index("if (( CONFIG_CHECK_ONLY" )
assert "kingdom.enabled = 1" in kingdom

print("production test-only override contracts passed")
