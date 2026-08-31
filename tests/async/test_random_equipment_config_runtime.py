#!/usr/bin/env python3
from _paths import SRC
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "core/prototypes.h"
#include <cstdarg>
#include <cstdio>
#include <cmath>
void logit(const char *, const char *, ...) {}
#include "item/random_equipment_config.c"

int main() {
    const random_equipment_config *cfg = random_equipment_config_get();
    if (std::fabs(cfg->drop_luck_divisor - 4.0f) > 0.001f) return 19;
    if (random_equipment_stat_max(61) != 8) return 20;
    boot_random_equipment_config();
    cfg = random_equipment_config_get();
    if (std::fabs(cfg->drop_piece_percentage - 15.0f) > 0.001f) return 1;
    if (std::fabs(cfg->drop_equipment_percentage - 8.0f) > 0.001f) return 2;
    if (std::fabs(cfg->quality_level_multiplier - 1.0f) > 0.001f) return 3;
    if (random_equipment_stat_max(35) != 4) return 4;
    if (random_equipment_stat_max(36) != 5) return 5;
    if (random_equipment_stat_max(49) != 5) return 6;
    if (random_equipment_stat_max(50) != 6) return 7;
    if (random_equipment_stat_max(60) != 6) return 8;
    if (random_equipment_stat_max(61) != 8) return 9;

    FILE *fp = std::fopen("lib/random_equipment.cfg", "w");
    if (!fp) return 10;
    std::fputs("drop.piece.percentage=12.5\nstat.cap.medium.level=30\nstat.cap.high.level=45\nstat.cap.elite.level=55\n", fp);
    std::fclose(fp);
    boot_random_equipment_config();
    cfg = random_equipment_config_get();
    if (std::fabs(cfg->drop_piece_percentage - 12.5f) > 0.001f) return 11;
    if (random_equipment_stat_max(31) != 5) return 12;
    if (random_equipment_stat_max(46) != 6) return 13;
    if (random_equipment_stat_max(56) != 8) return 14;

    fp = std::fopen("lib/random_equipment.cfg", "w");
    if (!fp) return 15;
    std::fputs("drop.luck.divisor=0\nquality.level.multiplier=nan\ndrop.jitter.minimum=8\ndrop.jitter.maximum=-8\nstat.primary.divisor=0\n", fp);
    std::fclose(fp);
    boot_random_equipment_config();
    cfg = random_equipment_config_get();
    if (std::fabs(cfg->drop_luck_divisor - 4.0f) > 0.001f) return 16;
    if (std::fabs(cfg->quality_level_multiplier - 1.0f) > 0.001f) return 21;
    if (cfg->drop_jitter_min != -5 || cfg->drop_jitter_max != 5) return 17;
    if (cfg->stat_primary_divisor != 46) return 18;
    return 0;
}
'''

with tempfile.TemporaryDirectory() as td:
    root = Path(td)
    (root / "lib").mkdir()
    (root / "lib/random_equipment.cfg").write_text((ROOT / "lib/random_equipment.cfg").read_text())
    harness = root / "test.cpp"
    binary = root / "test"
    harness.write_text(HARNESS)
    subprocess.run([
        "g++", "-std=c++20", f"-I{SRC}", f"-I{SRC / 'ships'}",
        str(harness), "-o", str(binary)
    ], check=True, cwd=ROOT)
    subprocess.run([str(binary)], check=True, cwd=root)

print("random equipment runtime config parity passed")
