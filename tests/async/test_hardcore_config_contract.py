#!/usr/bin/env python3
"""Source contract for the dedicated Hardcore policy configuration."""

from _paths import SRC
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
config = ROOT / "lib/hardcore.cfg"
header = SRC / "hardcore_config.h"
source = SRC / "hardcore_config.c"
makefile = (SRC / "Makefile").read_text()
comm = (SRC / "comm.c").read_text()
hardcore_source = (SRC / "hardcore.c").read_text()
actoth = (SRC / "actoth.c").read_text()
fight = (SRC / "fight.c").read_text()
nanny = (SRC / "nanny.c").read_text()
limits = (SRC / "limits.c").read_text()
ws_handlers = (SRC / "ws_handlers.c").read_text()
frag_config = (ROOT / "lib/frag_cap.cfg").read_text()
frag_header = (SRC / "frag_cap_config.h").read_text()
frag_source = (SRC / "frag_cap_config.c").read_text()
help_index = (ROOT / "lib/information/help_index").read_text()
consumer_sources = "\n".join(
    path.read_text()
    for path in SRC.glob("*.c")
    if path.name != "hardcore_config.c"
)

assert config.exists(), "dedicated Hardcore config is missing"
assert header.exists(), "Hardcore config header is missing"
assert source.exists(), "Hardcore config implementation is missing"

c = config.read_text()
h = header.read_text()
s = source.read_text()

# The first slice owns policy values that are currently hard-coded in gameplay.
for key in (
    "creation.enabled=",
    "creation.veterans.only=",
    "death.max.count=",
    "death.count.arena.deaths=",
    "death.permadeath=",
    "death.record.killer=",
    "death.hall.of.fame=",
    "death.messages.enabled=",
    "level.exp.bypass.property.cap=",
    "level.loss.protected.at=",
    "bonus.hp.per.level=",
    "bonus.healing.multiplier=",
    "bonus.damage.outgoing.multiplier=",
    "bonus.damage.incoming.multiplier=",
    "bonus.mass.heal.base=",
    "bonus.skill.notch.multiplier=",
    "bonus.random.equipment.multiplier=",
    "score.level.points=",
    "score.experience.divisor=",
    "score.frag.points=",
    "score.multiclass.multiplier=",
    "score.killer.bonus=",
    "score.death.penalty.points=",
    "score.invalid.frag.threshold=",
    "mode.disable.in.ctf=",
    "mode.disable.in.chaos=",
    "score.display.divisor=",
):
    assert key in c, key

for symbol in (
    "struct hardcore_config",
    "boot_hardcore_config",
    "hardcore_config_get",
    "hardcore_config_death_is_final",
    "hardcore_config_level_loss_allowed",
):
    assert symbol in h, symbol
    assert symbol in s, symbol

assert "hardcore_config.o" in makefile
assert "boot_hardcore_config();" in comm
for symbol in (
    "score_level_points",
    "score_experience_divisor",
    "score_invalid_frag_threshold",
    "score_frag_points",
    "score_death_penalty_points",
):
    assert symbol in s, symbol
assert "pd.secondary_class" in hardcore_source
assert "2147483648" in hardcore_source
assert "pd.numb_deaths * %d" in hardcore_source
assert "config->score_display_divisor" in hardcore_source
assert "atof(row[1]) / 100.0" not in hardcore_source
assert actoth.count("IS_HARDCORE(ch) && hardcore_config_get()->death_hall_of_fame") == 2
assert "IS_PC(ch) && IS_HARDCORE(ch) && hardcore_config_get()->death_messages_enabled" in fight
assert "hardcore_config_get()->creation_enabled" in nanny
assert "hardcore_config_get()->creation_veterans_only" in nanny
assert "hardcore_config_get()->creation_enabled" in ws_handlers
assert "hardcore_config_get()->creation_veterans_only" in ws_handlers
assert "hardcore_config_death_is_final" in fight
assert "death_permadeath" in s
assert "hardcore_config_level_loss_allowed" in limits
assert "level_loss_protected_at" in s
assert "death.max.count=1" in c
assert "configured death limit" in help_index
assert "fifth death" not in help_index.lower()

# Every typed policy field must be consumed by runtime code outside the loader.
for field in (
    "creation_enabled",
    "creation_veterans_only",
    "death_max_count",
    "death_count_arena_deaths",
    "death_record_killer",
    "death_hall_of_fame",
    "death_messages_enabled",
    "level_exp_bypass_property_cap",
    "bonus_hp_per_level",
    "bonus_healing_multiplier",
    "bonus_damage_outgoing_multiplier",
    "bonus_damage_incoming_multiplier",
    "bonus_mass_heal_base",
    "bonus_skill_notch_multiplier",
    "bonus_random_equipment_multiplier",
    "score_level_points",
    "score_experience_divisor",
    "score_frag_points",
    "score_multiclass_multiplier",
    "score_killer_bonus",
    "score_death_penalty_points",
    "score_invalid_frag_threshold",
    "disable_in_ctf",
    "disable_in_chaos",
    "score_display_divisor",
):
    assert consumer_sources.count(field) >= 1, f"unconsumed Hardcore field: {field}"

# The hardcore frag-cap lead must not be duplicated in this config/module.
assert "hardcore.levels.beyond.cap=" in frag_config
assert "hardcore.levels.beyond.cap=" not in c
assert "hardcore_levels_beyond_cap" in frag_header
assert "hardcore_levels_beyond_cap" in frag_source

print("hardcore configuration source contract passed")
