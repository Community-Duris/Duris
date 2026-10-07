"""Bounded Kord calibration: genuine continuation plus actual player modifiers.

This is deliberately limited to the supported level-one solo human fixture.
Active-native entitlement assertions continue to use their original owner.
"""
import json
import subprocess

from case_data import ROOT, digest
from native_build_artifacts import build_native


def decode(blob):
    binary = build_native(ROOT / "bin/tests/quest-prep-continuation-reader",
        ["tests/async/quest_accounting_prep/read_quest_continuation.cpp",
         "src/item/item_transfer_command.c", "src/world/quest_mobile_native_reference.c",
         "src/economy/economic_source_event.c", "src/item/craft_pouch_mutation.c",
         "src/combat/chaos_pouch_ledger.c", "src/player/player_snapshot_codec.c",
         "src/persistence/critical_command.c", "src/economy/native_quest_cost.c",
         "src/economy/native_quest_coin_give.c", "src/item/lockpick_retirement_continuation.c",
         "src/economy/shop_trade_recovery_manifest.c"],
        ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-Isrc"], ["-lcrypto"],
        name="quest-prep-continuation-reader")
    result = subprocess.run([str(binary)], input=bytes.fromhex(blob),
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10, check=True)
    return json.loads(result.stdout)


def kord(cut, operation):
    import quest_cut_checks as checks
    obligation = checks.index(checks.rows(cut, "obligations"), ("offering_operation_id",)).get((operation,))
    checks.require(obligation is not None, "original legacy XP obligation missing")
    terms = decode(obligation["continuation"])
    checks.require(terms == dict(version=5, pid=1, mobile_vnum=29257, level=1, party_size=1,
                   xp_awards=[dict(pid=1, index=2, amount=200)]), "Kord frozen XP calibration changed")
    player = checks.row(cut, "player")
    checks.require(player["race"] == 1 and player["level"] == 1, "solo human level-one calibration required")
    bonuses = [entry for entry in checks.rows(cut, "player_affects") if entry["type"] in (2107, 2108)]
    checks.require(len(bonuses) == 1 and bonuses[0]["type"] == 2108 and bonuses[0]["duration"] > 0,
                   "actual active well-rested affect required")
    properties = dict(line.split("=", 1) for line in (ROOT / "lib/duris.properties").read_text().splitlines()
                      if "=" in line and not line.startswith("#"))
    checks.require(all(float(properties[key]) == value for key, value in
        (("exp.factor.Human", 1.3), ("exp.required.01", 2000), ("exp.rested.enabled", 1),
         ("difficulty.dial.exp.required", 5), ("difficulty.dial.exp.earned", 5))),
         "maintained fixture XP properties changed")
    # quest.c freezes min(2500, next-level 2000 / 10)=200. limits.c applies
    # WELLRESTED x2, then Human x1.3, neutral difficulty, integer truncation,
    # and the 2000/3 final cap. 520 is below that cap and does not level up.
    checks.require(obligation["xp_applied_mask"] == 4, "original runtime slot-two XP not applied")
    return dict(frozen=200, effective=520, mask=4, continuation=obligation["continuation"],
                properties_sha256=digest(ROOT / "lib/duris.properties"))
