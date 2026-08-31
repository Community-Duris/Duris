#!/usr/bin/env python3
"""Source contract for configurable frag-cap behavior."""
from _paths import SRC
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LIB = ROOT / "lib"

config = (LIB / "frag_cap.cfg").read_text()
header = (SRC / "frag_cap_config.h").read_text()
impl = (SRC / "frag_cap_config.c").read_text()
sql = (SRC / "sql.c").read_text()
sql_h = (SRC / "sql.h").read_text()
comm = (SRC / "comm.c").read_text()
redis = (SRC / "redis.c").read_text()
report_cache = (SRC / "redis_report_cache.c").read_text()
fraglist = (SRC / "fraglist.c").read_text()
limits = (SRC / "limits.c").read_text()
makefile = (SRC / "Makefile").read_text()
maintenance = (SRC / "maintenance_repository.c").read_text()

assert "frag_cap_config.o" in makefile
assert "boot_frag_cap_config" in comm
assert "frag_cap_config.h" in sql
assert "frag_cap_config_get" in sql
assert "FRAG_LEVEL_RATIO" not in sql_h
assert "FRAGS_TO_LEVEL" not in sql_h

for key in (
    "cap.floor.level",
    "cap.frag.base.level",
    "cap.reset.level",
    "cap.maximum.level",
    "cap.level.step",
    "cap.frags.per.level",
    "timer.default.days",
    "timer.circle.level.35.days",
    "timer.circle.level.40.days",
    "timer.circle.level.45.days",
    "timer.level.50.days",
    "timer.level.51.days",
    "timer.level.52.days",
    "timer.level.53.days",
    "timer.level.54.days",
    "timer.level.55.days",
    "hardcore.levels.beyond.cap",
    "boon.duration.minutes",
    "boon.bonus",
):
    assert key in config, key
    assert key in impl, key

for token in (
    "cap_floor_level",
    "cap_frag_base_level",
    "cap_reset_level",
    "cap_maximum_level",
    "cap_level_step",
    "cap_frags_per_level",
    "timer_default_days",
    "timer_circle_level_35_days",
    "timer_circle_level_40_days",
    "timer_circle_level_45_days",
    "timer_level_50_days",
    "timer_level_51_days",
    "timer_level_52_days",
    "timer_level_53_days",
    "timer_level_54_days",
    "timer_level_55_days",
    "hardcore_levels_beyond_cap",
    "boon_duration_minutes",
    "boon_bonus",
):
    assert token in impl, token

assert "frag_cap_config_cap_level_from_frags" in impl
assert "frag_cap_config_frags_for_level" in fraglist
assert "frag_cap_config_frags_for_level" in report_cache
assert "frag_cap_config_frags_for_level" not in redis
assert "frag_cap_config_timer_days" in impl
assert "timer.circle.level.35" in config
assert "cap.level.step=1" in config
assert "timer.default.days=5" in config
assert "hardcore.levels.beyond.cap=2" in config
assert "timer.level.56.days" not in config
assert "timer_level_56_days" not in header
assert "timer_level_56_days" not in impl
assert "frag_cap_config_hardcore_level_cap" in limits
assert "IS_HARDCORE(ch)" in limits
assert "frag_cap_config_boon_duration_minutes" in impl
assert "frag_cap_config_boon_bonus" in impl
assert "sql_check_level_cap_periodic" in sql
assert "sql_check_level_cap_periodic" in sql_h
assert "sql_check_level_cap_periodic" not in comm
assert "maintenance_job_id::level_cap" in comm
assert "frag_cap_config_cap_level_from_frags" in maintenance
assert "frag_cap_config_timer_days" in maintenance
assert "COALESCE(SUM(total_frags), 0)" in sql
assert "if (gain >= 0)" in sql
assert "FROM_UNIXTIME" in sql
assert "frag_cap_config_reset_level" in sql
assert "frag_cap_config_reset_timer_days" in sql

print("frag-cap configuration source contract passed")
