#!/usr/bin/env python3
"""Runtime contract for the boot-time Hardcore configuration loader."""

from _paths import SRC
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = SRC / "hardcore_config.c"
HEADER_DIR = SRC
CONFIG = ROOT / "lib/hardcore.cfg"

PROBE = r'''
#include <stdio.h>
#include "world/hardcore_config.h"

void logit(const char *file, const char *format, ...)
{
    (void)file;
    (void)format;
}

int main(int argc, char **argv)
{
    const struct hardcore_config *config;
    boot_hardcore_config();
    config = hardcore_config_get();

    if (argc == 1)
        return config->death_max_count == 1 &&
               config->bonus_hp_per_level == 2 &&
               config->score_death_penalty_points == 25 &&
               config->score_display_divisor == 100 &&
               config->disable_in_ctf &&
               hardcore_config_death_is_final(1) &&
               !hardcore_config_death_is_final(0) &&
               hardcore_config_level_loss_allowed(49) &&
               !hardcore_config_level_loss_allowed(50) ? 0 : 1;

    if (argc == 2)
        return config->death_max_count == 5 &&
           config->bonus_hp_per_level == 7 &&
           config->score_death_penalty_points == 40 &&
           config->score_display_divisor == 250 &&
           !config->disable_in_ctf &&
           !hardcore_config_death_is_final(1) &&
           hardcore_config_death_is_final(5) &&
           hardcore_config_level_loss_allowed(51) &&
           !hardcore_config_level_loss_allowed(52) ? 0 : 1;

    return config->death_max_count == 1 &&
           config->bonus_hp_per_level == 2 &&
           config->score_display_divisor == 100 &&
           hardcore_config_death_is_final(1) ? 0 : 1;
}
'''

with tempfile.TemporaryDirectory(prefix="hardcore-config-runtime-") as directory:
    root = Path(directory)
    (root / "lib").mkdir()
    (root / "prototypes.h").write_text(
        'void logit(const char *, const char *, ...);\n'
        '#define LOG_STATUS "status"\n'
    )
    (root / "probe.cpp").write_text(PROBE)
    shutil.copy(CONFIG, root / "lib/hardcore.cfg")

    binary = root / "probe"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            f"-I{root}",
            f"-I{HEADER_DIR}",
            str(SOURCE),
            str(root / "probe.cpp"),
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], cwd=root, check=True)

    overridden = (root / "lib/hardcore.cfg").read_text()
    overridden = overridden.replace("death.max.count=1", "death.max.count=5")
    overridden = overridden.replace("bonus.hp.per.level=2", "bonus.hp.per.level=7")
    overridden = overridden.replace("score.death.penalty.points=25", "score.death.penalty.points=40")
    overridden = overridden.replace("score.display.divisor=100", "score.display.divisor=250")
    overridden = overridden.replace("mode.disable.in.ctf=true", "mode.disable.in.ctf=false")
    overridden = overridden.replace("level.loss.protected.at=50", "level.loss.protected.at=52")
    (root / "lib/hardcore.cfg").write_text(overridden)
    subprocess.run([str(binary), "override"], cwd=root, check=True)

    invalid = overridden.replace("death.max.count=5", "death.max.count=0")
    invalid = invalid.replace("bonus.hp.per.level=7", "bonus.hp.per.level=101")
    invalid = invalid.replace("score.display.divisor=250", "score.display.divisor=0")
    (root / "lib/hardcore.cfg").write_text(invalid)
    subprocess.run([str(binary), "invalid", "bounds"], cwd=root, check=True)

print("hardcore configuration runtime contract passed")
