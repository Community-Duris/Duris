"""Focused source contract checks for issue #299 world activity policy."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
ACTIVITY = (ROOT / "src/world/world_activity.c").read_text(encoding="utf-8")
HEADER = (ROOT / "src/world/world_activity.h").read_text(encoding="utf-8")
STRUCTS = (ROOT / "src/core/structs.h").read_text(encoding="utf-8")
MOB = (ROOT / "src/mob/mobact.c").read_text(encoding="utf-8")
ACTOFF = (ROOT / "src/cmd/actoff.c").read_text(encoding="utf-8")
HANDLER = (ROOT / "src/world/handler.c").read_text(encoding="utf-8")
PROPERTIES = (ROOT / "lib/duris.properties").read_text(encoding="utf-8")
MAKEFILE = (ROOT / "src/Makefile").read_text(encoding="utf-8")


def test_policy_has_active_nearby_and_distant_tiers():
    assert "world_activity_tier::active" in ACTIVITY
    assert "world_activity_tier::nearby" in ACTIVITY
    assert "world_activity_tier::distant" in ACTIVITY
    assert "PULSE_MOBILE * config.nearby_multiplier" in ACTIVITY
    assert "config.distant_pulses" in ACTIVITY


def test_disabled_path_preserves_legacy_occupied_and_playerless_cadence():
    assert "legacy_zone_occupied ? PULSE_MOBILE" in ACTIVITY
    assert "PULSE_MOBILE * PLAYERLESS_ZONE_SPEED_MODIFIER" in ACTIVITY
    assert "world_activity_schedule_mundane(ch, false" in MOB
    assert "remember_array[world[ch->in_room].zone] != NULL" in MOB


def test_corpse_contract_excludes_npc_corpses_and_tracks_subtrees():
    assert "object->type == ITEM_CORPSE" in ACTIVITY
    assert "IS_SET(object->value[1], PC_CORPSE)" in ACTIVITY
    assert "!IS_SET(object->value[1], NPC_CORPSE)" in ACTIVITY
    assert "walk_corpse_subtree" in ACTIVITY
    for hook in (
        "world_activity_object_enter",
        "world_activity_object_leave",
        "world_activity_character_enter",
        "world_activity_character_leave",
    ):
        assert hook in HANDLER


def test_promotion_is_indexed_and_rebuilt_after_recovery():
    assert "std::unordered_set<P_char> npcs" in ACTIVITY
    assert "world_activity_mundane_event(mob)" in ACTIVITY
    assert "get_scheduled(mob, event_mob_mundane)" not in ACTIVITY
    assert "nevent_advance_by" in ACTIVITY
    assert "world_activity_rebuild();" in (ROOT / "src/net/comm.c").read_text(encoding="utf-8")
    assert "world_activity_promote_character" in HEADER


def test_mundane_handle_is_sequence_validated_and_corpse_walk_has_no_heap_visitor():
    assert "world_activity_mundane_event_sequence" in STRUCTS
    assert "nevent_handle_is_active(event)" in ACTIVITY
    assert "event.event->func != event_mob_mundane" in ACTIVITY
    assert "depth > 1024" in ACTIVITY
    assert "std::unordered_set<P_obj>" not in ACTIVITY
    assert "world_activity_schedule_mundane_after(was_fighting, WAIT_SEC / 2)" in ACTOFF


def test_runtime_switch_and_build_registration_exist():
    for key in (
        "world.activity.enabled",
        "world.activity.distant.seconds",
        "world.activity.grace.seconds",
        "world.activity.nearby.multiplier",
        "world.activity.wake.enabled",
    ):
        assert key in PROPERTIES
    assert "world/world_activity.o" in MAKEFILE
    assert "bootstrapped && enabled_changed && config.enabled" in ACTIVITY


if __name__ == "__main__":
    for test in (
        test_policy_has_active_nearby_and_distant_tiers,
        test_disabled_path_preserves_legacy_occupied_and_playerless_cadence,
        test_corpse_contract_excludes_npc_corpses_and_tracks_subtrees,
        test_promotion_is_indexed_and_rebuilt_after_recovery,
        test_mundane_handle_is_sequence_validated_and_corpse_walk_has_no_heap_visitor,
        test_runtime_switch_and_build_registration_exist,
    ):
        test()
    print("world activity contract checks passed")
