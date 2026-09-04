#!/usr/bin/env python3
"""Account-bound divine rewards use account ownership at every access gate."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, SRC
from contract_text import contains, count, index

reward = (SRC / "account_reward.c").read_text()
actobj = (SRC / "actobj.c").read_text()
actoth = (SRC / "actoth.c").read_text()
harness = Path(__file__).with_name("account_bound_reward_owner_harness.cpp")

owner_start = index(reward, "bool account_bound_reward_owner")
owner_end = index(reward, "#ifndef __NO_MYSQL__", owner_start)
owner = reward[owner_start:owner_end]
assert contains(owner, "ITEM2_ACCOUNT_BOUND")
assert contains(owner, "parse_reward_marker")
assert contains(owner, "strcasecmp")

equip_start = index(actobj, "static bool can_equip_soulbound_item")
equip_end = index(actobj, "#define GETDBG_LOG", equip_start)
equip = actobj[equip_start:equip_end]
assert contains(equip, "ITEM2_SOULBIND")
assert contains(equip, "ITEM2_ACCOUNT_BOUND")
assert contains(equip, "account_bound_reward_owner")
assert contains(equip, "isname(GET_NAME(actor),object->name)")

wear_start = index(actobj, "int wear(P_char")
wear_end = index(actobj, "void do_remove", wear_start)
assert contains(actobj[wear_start:wear_end], "can_equip_soulbound_item")

do_wear_start = index(actobj, "void do_wear(P_char")
do_wear_end = index(actobj, "void do_wield", do_wear_start)
do_wear = actobj[do_wear_start:do_wear_end]
assert do_wear.count("can_equip_soulbound_item") == 2
assert not contains(do_wear, "!isname(GET_NAME(ch),obj_object->name)")

grab_start = index(actobj, "void do_grab(P_char")
grab_end = index(actobj, "void do_remove", grab_start)
grab = actobj[grab_start:grab_end]
assert contains(grab, "can_equip_soulbound_item")
assert not contains(grab, "!isname(GET_NAME(ch),obj_object->name)")

get_start = index(actobj, "void get(P_char")
get_end = index(actobj, "int fight_in_room", get_start)
get_body = actobj[get_start:get_end]
assert get_body.count("ITEM2_NOLOOT") == 3
assert count(get_body, "account_bound_reward_owner(ch,o_obj)") == 4

bulk_start = index(actobj, "static bool select_bulk_get_item")
bulk_end = index(actobj, "static void start_bulk_get", bulk_start)
bulk = actobj[bulk_start:bulk_end]
assert contains(bulk, "ITEM2_NOLOOT")
assert count(bulk, "account_bound_reward_owner(actor,object)") == 2

use_start = index(actoth, "void do_use(P_char")
use_end = index(actoth, "static const char *term_name", use_start)
use = actoth[use_start:use_end]
assert contains(use, "ITEM2_ACCOUNT_BOUND")
assert contains(use, "account_bound_reward_owner(ch,stick)")
assert index(use, "account_bound_reward_owner(ch,stick)") < index(use, "stick->value[2]--")

with tempfile.TemporaryDirectory(prefix="duris-account-reward-owner-") as temporary:
    binary = Path(temporary) / "account_bound_reward_owner"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-D__NO_MYSQL__",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc",
            str(harness),
            "src/account/account_reward.c",
            "-Wl,--gc-sections",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("account-bound reward access contract: ok")
