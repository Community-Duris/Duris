# Undead Outpost static dependency review

Checkpoint 871 closes the bounded tracked-source dependency scan for active unoutpst against source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697. The active contributor is listed as *987-988 in areas/AREA and qst/AREA. The source story and builder questions remain in [UNDEAD_OUTPOST.md](../zone-stories/UNDEAD_OUTPOST.md); the map identity and followed history remain as recorded in [checkpoint 766](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-766-undead-outpost-and-human-outpost-identities).

## Active graph and code-bound arrival

The active WLD contains 121 rooms and 250 authored exits: 249 resolve, including 247 local records and the paired external records. The static graph has one weak component, one F 10 field at #98815, and no current fields. It has two reciprocal WLD boundaries: Surface #605489 with Outpost #98700, and Crypt #14300 with Outpost #98737. One north exit from #98737 has negative target -1 and is not a loaded route. The all-95 inactive-WLD scan found no separate target into this map or room-number overlap.

There is also a compiled inbound destination that is not represented as a WLD edge. The shift_prime innate only works while the character is in Astral; for RACE_PHANTOM, do_shift_prime selects a room from its phantom_room list, which includes Outpost #98731, the Main Passage. This is one possible code-selected arrival, not a universal WLD entrance or a guaranteed safe return. See [room source](../../../areas/wld/unoutpst.wld#L633) and [innate destination and branch](../../../src/classes/innates.c#L2012).

A separate #98735 direct-room literal in do_descend is behind the function's unconditional “command has been disabled” return at [actoth.c lines 7887–7888](../../../src/cmd/actoth.c#L7887). Do not count that unreachable lower-code block as a current arrival.

## Typed reset and prototype ownership

A full current ZON search found no external active reset of unoutpst's 19 mobile prototypes or 11 local object prototypes. unoutpst.zon resets its own residents and items, and has one external-object import:

| Source record | Typed target | Owner and evidence |
| --- | --- | --- |
| unoutpst.zon line 54, O reset in room #98820 | OBJ #132581, sign counter | Active Tharnadia prototype at [tharnadia.obj](../../../areas/obj/tharnadia.obj#L1373); the consumer is also present in the [Tharnadia audit](THARNADIA_AUDIT_REVIEW.md). |

The same source reset is not a local OBJ definition and should remain attached to Tharnadia. Its native object callback is storage_locker_obj_hook at [specs.assign.c line 1307](../../../src/specs/specs.assign.c#L1307); the sign counter's location is checked by test_atm_present and the balance/deposit/withdraw handlers in [actoth.c](../../../src/cmd/actoth.c#L2433). This identifies a source-level bank-counter and locker integration, not a local SHP shop or proof of deployed availability.

Within the active Outpost prototypes, OBJ #98704 is a container whose key VNUM field points to local key #98705. The ZON resets the chest at #98750 and the key on the zombie merchant, preserving this local pair. No extra candidate or duplicate prototype is needed.

Current tracked QST and SHP files contain no external numeric reference to Outpost rooms, mobiles, or objects. The active source has no local QST, SHP, or story sidecar. No external object portal destination to an Outpost room was found. The raw room-range hits in earth.obj are #98756/#98765 in the condition slots of unrelated objects #43541/#43564; the object reader stores that numeric field as condition, not as a room destination ([db.c](../../../src/world/db.c#L4682)). No same-type active WLD/MOB/OBJ identity overlap was found.

## Compiled banking and mobile behavior

Tarn is active mobile #98718 and has the money_changer callback in [specs.assign.c line 717](../../../src/specs/specs.assign.c#L717). The assigned procedure explicitly says coin exchange has moved to the Royal Bank and directs players there; it does not perform exchanges at Tarn; see [currency_exchange_proc.c](../../../src/economy/currency_exchange_proc.c#L15). Separately, the imported Tharnadia sign counter #132581 is recognized by the native bank commands and has a storage-locker object hook. Keep those source contracts distinct: the bank counter/object path is present in tracked code, while coin exchange at Tarn is expressly retired.

The source review did not run the server. Current command availability, town eligibility, reset deployment, banking persistence, and actual locker entry remain runtime questions. Reset records alone do not establish current NPC, item, door, or service state.

## Identity and disposition

The Undead Outpost and Human Outpost are distinct active source identities; the Crypt is its reciprocal neighbor, not a replacement. Followed WLD/ZON histories show no distinct predecessor or replacement. Keep active unoutpst DO NOT MOVE and add no attic candidate. Preserve the #132581 Tharnadia import, both WLD boundary pairs, the #98731 code-selected phantom arrival, and the negative unresolved north target as separate dependencies. No map movement, service promise, or gameplay change is authorized by this source review.

