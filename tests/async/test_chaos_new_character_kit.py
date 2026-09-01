#!/usr/bin/env python3
"""Contracts for the level-56 class kits granted to new CHAOS characters."""

from pathlib import Path
import re

from _paths import SRC


ROOT = Path(__file__).resolve().parents[2]
NANNY = (SRC / "account" / "nanny.c").read_text()
MOVEMENT = (SRC / "item" / "item_movement_transaction.c").read_text()
DEFINES = (SRC / "core" / "defines.h").read_text()
CLASS_NAMES = [
    "Warrior",
    "Ranger",
    "Psionicist",
    "Paladin",
    "Anti-Paladin",
    "Cleric",
    "Monk",
    "Druid",
    "Shaman",
    "Sorcerer",
    "Necromancer",
    "Conjurer",
    "Rogue",
    "Assassin",
    "Mercenary",
    "Bard",
    "Thief",
    "Warlock",
    "MindFlayer",
    "Alchemist",
    "Berserker",
    "Reaver",
    "Illusionist",
    "Blighter",
    "Dreadlord",
    "Ethermancer",
    "Avenger",
    "Theurgist",
    "Summoner",
    "Dragoon",
]
KIT_ARRAY_NAMES = {
    "chaos_warrior_kit",
    "chaos_ranger_kit",
    "chaos_psionicist_kit",
    "chaos_cleric_kit",
    "chaos_druid_kit",
    "chaos_shaman_kit",
    "chaos_sorcerer_kit",
    "chaos_necromancer_kit",
    "chaos_conjurer_kit",
    "chaos_rogue_kit",
    "chaos_mercenary_kit",
    "chaos_berserker_kit",
    "chaos_illusionist_kit",
    "chaos_ethermancer_kit",
}


# The normal starter path remains intact; only the durable first-entry branch
# selects the CHAOS kit.
creation = NANNY.split("if (!GET_LEVEL(ch))", 1)[1].split(
    "else if (IS_SET(ch->specials.act2", 1
)[0]
assert "if (writeCharacter(ch, 1, NOWHERE))" in creation
assert "if (chaos_mud_enabled())" in creation
assert "load_chaos_new_character_kit(ch);" in creation
assert "else\n\t\t\t\tload_obj_to_newbies(ch);" in creation
assert creation.index("writeCharacter(ch, 1, NOWHERE)") < creation.index(
    "load_chaos_new_character_kit(ch)"
)
assert creation.count("load_chaos_new_character_kit(ch)") == 1
assert creation.count("load_obj_to_newbies(ch)") == 1


slot_values = {
    name: int(value)
    for name, value in re.findall(r"(?m)^#define\s+([A-Z][A-Z0-9_]*)\s+(-?\d+)\b", DEFINES)
}
array_matches = re.findall(
    r"static const chaos_kit_item (chaos_[a-z]+_kit)\[\] = \{(.*?)\};",
    NANNY,
    re.S,
)
assert {name for name, _ in array_matches} == KIT_ARRAY_NAMES

kit_arrays: dict[str, dict[int, int]] = {}
all_vnums = {96443}
for array_name, body in array_matches:
    pairs = re.findall(r"\{\s*([A-Z0-9_]+),\s*(\d+)\s*\}", body)
    assert pairs and pairs[-1] == ("WEAR_NONE", "0")
    items: dict[int, int] = {}
    for slot_name, raw_vnum in pairs[:-1]:
        slot = slot_values[slot_name]
        vnum = int(raw_vnum)
        assert 0 <= slot < 43
        assert vnum >= 0
        assert slot not in items, (array_name, slot)
        items[slot] = vnum
        if vnum:
            all_vnums.add(vnum)
    kit_arrays[array_name] = items


# The warrior sample replaces the five selected slots and explicitly suppresses
# the three unresolved slots so its mercenary fallback cannot reintroduce 1252.
warrior = kit_arrays["chaos_warrior_kit"]
for slot_name, vnum in {
    "WEAR_FINGER_R": 45510,
    "WEAR_FINGER_L": 87511,
    "WEAR_NECK_1": 44192,
    "WEAR_FEET": 44194,
    "WEAR_EARRING_L": 67269,
}.items():
    assert warrior[slot_values[slot_name]] == vnum
for slot_name in ("WEAR_QUIVER", "GUILD_INSIGNIA", "WEAR_ATTACH_BELT_1"):
    assert warrior[slot_values[slot_name]] == 0
warrior_final = dict(kit_arrays["chaos_mercenary_kit"])
warrior_final.update(warrior)
assert 1252 not in warrior_final.values()


# The 31 ordered profiles map the 30 class IDs plus the unused zero index.
profiles_body = NANNY.split(
    "static const chaos_kit_profile chaos_kit_profiles[CLASS_COUNT + 1] = {", 1
)[1].split("};", 1)[0]
profiles = re.findall(r"\{\s*(NULL|chaos_[a-z]+_kit),\s*(NULL|chaos_[a-z]+_kit)\s*\}", profiles_body)
assert len(profiles) == 31
assert profiles[0] == ("NULL", "NULL")
assert all(fallback != "NULL" for _, fallback in profiles[1:])
for class_id, (items_name, fallback_name) in enumerate(profiles[1:], 1):
    assert fallback_name in kit_arrays
    own = {} if items_name == "NULL" else kit_arrays[items_name]
    fallback = kit_arrays[fallback_name]
    final = dict(fallback)
    final.update(own)
    assert final, CLASS_NAMES[class_id - 1]
    assert len(final) == len(set(final)), CLASS_NAMES[class_id - 1]


# Every static VNUM is installed, and 96443 remains a container prototype.
world_obj = (ROOT / "areas" / "world.obj").read_text(errors="replace")
for vnum in all_vnums:
    assert re.search(rf"(?m)^#{vnum}$", world_obj), vnum
bag_entry = re.search(r"(?ms)^#96443$\n(.*?)(?=^#\d+$)", world_obj)
assert bag_entry
bag_lines = bag_entry.group(1).splitlines()
numeric = [line for line in bag_lines if re.fullmatch(r"[-0-9 ]+", line.strip()) and line.strip()]
assert numeric and numeric[0].split()[0] == "15"


# The bag is queued first. Every gear item is then queued into that bag, and
# command input remains blocked until the serialized grant queue completes.
chaos_preparer = NANNY.split("static void prepare_chaos_kit_item", 1)[1].split(
    "static void load_chaos_new_character_kit", 1
)[0]
assert "REMOVE_BIT(obj->extra_flags, ITEM_SECRET);" in chaos_preparer
chaos_loader = NANNY.split("static void load_chaos_new_character_kit", 1)[1].split(
    "void load_obj_to_newbies", 1
)[0]
assert "bag_vnum = 96443" in chaos_loader
assert chaos_loader.index("item_creation_grant_submit_to_player(ch, bag, ch)") < chaos_loader.index(
    "item_creation_grant_submit_to_player(ch, obj, ch, bag)"
)
assert chaos_loader.index("item_creation_grant_submit_to_player(ch, obj, ch, bag)") < chaos_loader.index(
    "item_creation_grant_mark_blocking(ch)"
)


# A pending root container may be named by later queued grants, but it must be
# the same recipient's earlier top-level container request. Once the child
# starts, its creation command records the durable parent and root.
target_helper = MOVEMENT.split("bool creation_grant_target_available", 1)[1].split(
    "void discard_creation_queue", 1
)[0]
for token in (
    "GET_ITEM_TYPE(container) != ITEM_CONTAINER",
    "OBJ_CARRIED_BY(container, recipient)",
    "OBJ_NOWHERE(container)",
    "!request.target_container_uid",
    "request.item_uid == container->obj_uid",
    "request.recipient_pid == recipient_pid",
):
    assert token in target_helper

payload = MOVEMENT.split("item_transfer_payload payload = {", 1)[1].split("};", 1)[0]
assert "target_container ? target_runtime.root_item_uid" in payload
assert "target_container ? target_container->obj_uid" in payload
assert "target_container ? target_runtime.item_revision" in payload
assert "adopted && target_container" not in payload

queue = MOVEMENT.split("bool queue_creation_grant", 1)[1].split("void account_health", 1)[0]
assert queue.index("creation_grants.try_emplace") < queue.index(
    "creation_grant_target_available"
) < queue.index("queue.requests.push_back")

print("CHAOS new-character class kit contracts passed")
