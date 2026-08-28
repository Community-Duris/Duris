#!/usr/bin/env python3
"""Delayed ship-volley lifetime and generation regressions under ASan/UBSan."""

import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

HARNESS = r'''
#include "ships/ship_identity.c"

#include <cstdarg>
#include <cstdlib>
#include <type_traits>

static int resolved_impacts = 0;
static uint64_t impact_checksum = 0;

void panic_corruption(const char *, const char *, ...)
{
	std::abort();
}

static void require(bool condition, int code)
{
	if (!condition)
		std::exit(code);
}

struct scheduled_volley
{
	int ticks;
	VolleyData payload;
};

static void advance_clock(scheduled_volley &event, int ticks)
{
	while (ticks-- > 0 && event.ticks > 0)
	{
		if (--event.ticks > 0)
			continue;

		P_ship attacker;
		P_ship target;
		if (!resolve_volley_endpoints(&event.payload, &attacker, &target))
			continue;
		resolved_impacts++;
		impact_checksum ^= attacker->runtime_ref.generation;
		impact_checksum ^= target->runtime_ref.generation;
	}
}

static P_ship make_ship()
{
	auto *ship = new ShipData{};
	const ShipRuntimeRef ref = register_ship_runtime(ship);
	require(ref.slot != 0 && ref.generation != 0, 1);
	return ship;
}

static void destroy_ship(P_ship ship)
{
	unregister_ship_runtime(ship);
	delete ship;
}

static scheduled_volley schedule(P_ship attacker, P_ship target)
{
	return { 2, { target->runtime_ref, attacker->runtime_ref, 0, 100 } };
}

static void test_live_endpoints_resolve()
{
	P_ship attacker = make_ship();
	P_ship target = make_ship();
	scheduled_volley event = schedule(attacker, target);
	advance_clock(event, 2);
	require(resolved_impacts == 1, 2);
	destroy_ship(attacker);
	destroy_ship(target);
}

static void test_deleted_endpoint_is_discarded(bool delete_attacker)
{
	P_ship attacker = make_ship();
	P_ship target = make_ship();
	scheduled_volley event = schedule(attacker, target);
	const int impacts_before = resolved_impacts;

	if (delete_attacker)
	{
		destroy_ship(attacker);
		attacker = nullptr;
	}
	else
	{
		destroy_ship(target);
		target = nullptr;
	}
	advance_clock(event, 2);
	require(resolved_impacts == impacts_before, delete_attacker ? 3 : 4);

	if (attacker)
		destroy_ship(attacker);
	if (target)
		destroy_ship(target);
}

static void test_reused_slot_rejects_stale_generation()
{
	P_ship attacker = make_ship();
	P_ship target = make_ship();
	scheduled_volley event = schedule(attacker, target);
	const ShipRuntimeRef stale = attacker->runtime_ref;
	const int impacts_before = resolved_impacts;

	destroy_ship(attacker);
	P_ship replacement = make_ship();
	require(replacement->runtime_ref.slot == stale.slot, 5);
	require(replacement->runtime_ref.generation != stale.generation, 6);
	require(find_ship_by_runtime_ref(stale) == nullptr, 7);

	advance_clock(event, 2);
	require(resolved_impacts == impacts_before, 8);
	destroy_ship(replacement);
	destroy_ship(target);
}

int main()
{
	static_assert(std::is_trivially_copyable_v<ShipRuntimeRef>);
	static_assert(std::is_trivially_copyable_v<VolleyData>);
	test_live_endpoints_resolve();
	test_deleted_endpoint_is_discarded(true);
	test_deleted_endpoint_is_discarded(false);
	test_reused_slot_rejects_stale_generation();
	return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-nevent-volley-") as directory:
    temp = Path(directory)
    harness = temp / "harness.cpp"
    binary = temp / "harness"
    harness.write_text(HARNESS, encoding="ascii")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-O1",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            f"-I{SRC}",
            str(harness),
            "-o",
            str(binary),
        ],
        check=True,
    )
    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    subprocess.run([str(binary)], check=True, env=environment)

ships = (SRC / "ships" / "ships.h").read_text(encoding="ascii")
base = (SRC / "ships" / "ship_base.c").read_text(encoding="ascii")
combat = (SRC / "ships" / "ship_combat.c").read_text(encoding="ascii")
redis = (SRC / "redis.c").read_text(encoding="ascii")
sql_player = (SRC / "sql_player.c").read_text(encoding="ascii")

volley = ships[ships.index("struct VolleyData") : ships.index("extern struct ShipMap")]
assert "P_ship" not in volley
assert "ShipRuntimeRef target;" in volley
assert "ShipRuntimeRef attacker;" in volley

callback = combat[
    combat.index("void volley_hit_event") : combat.index("int damage_sail")
]
assert callback.index("resolve_volley_endpoints") < callback.index("SHIP_DOCKED(target)")
assert "vd->attacker" not in callback
assert "vd->target" not in callback
volley_schedule = combat[
    combat.index("VolleyData vd = {}") : combat.index(
        "ship->timer[T_BSTATION]", combat.index("VolleyData vd = {}")
    )
]
assert "(void *)&vd" not in volley_schedule
assert "&vd" in volley_schedule

constructor = base[base.index("struct ShipData *new_ship") : base.index("void name_ship")]
destructor = base[base.index("void delete_ship(P_ship") : base.index("void clear_references_to_ship")]
assert "register_ship_runtime(ship);" in constructor
assert destructor.index("unregister_ship_runtime(ship);") < destructor.index("FREE(ship);")

assert "redis_load_ship_snapshot" not in redis
assert "redis_cache_ship_snapshot" not in redis

sql_loader = sql_player[
    sql_player.index("P_ship sql_load_ship(") : sql_player.index(
        "bool sql_load_all_ships()"
    )
]
failure = sql_loader[sql_loader.index('component=dependent_rows outcome=failure') :]
assert failure.index("shipObjHash.erase(ship);") < failure.index("delete_ship(ship, true);")

print("ship volley stable-reference tests passed under ASan/UBSan")
