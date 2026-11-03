# Checkpoint 193 — Black Tunnels source audit

This supplement reconciles the active `black_tunnels` contributor at zone
#107. The [story dossier](../zone-stories/BLACK_TUNNELS.md) records its authored
environment, inhabitants, objects, route clues, and builder questions. This is
a static source review; it does not establish a live visit, present reset
state, player access, or safe return.

## Source scope and registration

`black_tunnels` is an un-commented full-world `areas/AREA` entry and appears in
the separate historical [`areas/qst/AREA`](../../../areas/qst/AREA#L54) list
([full-world entry](../../../areas/AREA#L60)). Its zone header is #107, title
“Black Tunnels,” top #10781, reset mode 2
([zone header](../../../areas/zon/black_tunnels.zon#L3)). The core source set
contains 82 rooms (#10700–#10781), 16 mobile prototypes (#10700–#10715), and 29
objects (#10700–#10728). No local `.qst`, `.shp`, or story sidecar exists; the
coverage inventory records zero native Q/QA contracts and an empty
`areas/RANDOM_AREA` input.

The 84 ZON commands are 36 `M`, 18 `E`, 15 `O`, 10 `D`, four `G`, and one `P`.
All reset prototypes and rooms are local, and the external `.zon` scan found
no imports of Black Tunnels prototypes. The key object #10712 is reset onto
the flaming salamander #10704 in the hot alcove; it is not referenced by a WLD
key VNUM ([object](../../../areas/obj/black_tunnels.obj#L143),
[reset](../../../areas/zon/black_tunnels.zon#L108)).

## WLD census and static directed reach

The active loader scan covered 351 WLD files and 253,262 active rooms. Black
Tunnels has 200 authored exits: 198 local and two cross-area. All 200 resolve;
there are no negative or unresolved targets, and the local map forms one weak
component of 82 rooms. Both active inbound anchors, #10700 and #10721, have
raw authored-exit paths to all 82 rooms and directed return paths from all 82,
assuming authored doors can be discovered or opened.

The two WLD boundaries are reciprocal:

| Boundary | Black Tunnels edge | Reciprocal edge | Register evidence |
| --- | --- | --- | --- |
| Pragtog | #10700 south to #10693 | Pragtog #10693 north to #10700 | [exit rows 202–203](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L202), [pair summary row 47](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L47) |
| Khildarak | #10721 east to #17377 | Khildarak #17377 west to #10721 | [exit rows 204 and 303](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L204), [pair summary row 46](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L46) |

A conservative as-reset static traversal that excludes reset-closed,
secret, or blocked exits reaches 59/82 rooms from either inbound anchor; the
other 23 lie beyond those sections. This calculation does not simulate
searching for secret exits, opening or picking doors, passdoor, changing reset
state, or live player access. The all-95-inactive-WLD scan found no overlapping
room IDs and no inactive exit into Black Tunnels.

## Door states, loader flags, and reset behavior

The ten directed door exits form five reciprocal pairs. Each pair has matching
ZON `D` resets, and all WLD key VNUM fields are zero:

| WLD passage | Reset state | Source reset rows |
| --- | --- | --- |
| #10711 east ↔ #10740 west | Closed and secret | [8](../../../areas/zon/black_tunnels.zon#L8), [12](../../../areas/zon/black_tunnels.zon#L12) |
| #10735 east ↔ #10736 west | Closed, locked, and secret | [16](../../../areas/zon/black_tunnels.zon#L16), [20](../../../areas/zon/black_tunnels.zon#L20) |
| #10767 east ↔ #10773 west | Closed | [24](../../../areas/zon/black_tunnels.zon#L24), [28](../../../areas/zon/black_tunnels.zon#L28) |
| #10775 west ↔ #10776 east | Closed and locked, with key VNUM 0 | [32](../../../areas/zon/black_tunnels.zon#L32), [36](../../../areas/zon/black_tunnels.zon#L36) |
| #10776 north ↔ #10777 south | Closed | [40](../../../areas/zon/black_tunnels.zon#L40), [44](../../../areas/zon/black_tunnels.zon#L44) |

The first two secret passages and the bone-hall locked door have blank WLD
keyword/description and zero key fields. The four raw WLD state-4 edges do not
represent extra closed doors: the loader masks exit flags with `& 3`
([WLD loader](../../../src/world/db.c#L1570)). The zone's mode-2 reset is
scheduled because its mode is nonzero; the reset event permits mode 2 to reset
even while a zone is occupied ([event scheduling](../../../src/world/new_events.c#L2068),
[reset-mode condition](../../../src/world/events.c#L429)). Thus the source can
reapply these closed states, but it does not establish when any live reset
occurs.

## Typed travel and external dependency scan

The Khildarak ZON resets object #17097 into room #17377. Its object prototype
is an `ITEM_TELEPORT` type 25 configured for command 99 (`CMD_OPEN`), target
#10721, and unlimited charges; its text describes a metal gate standing open
before the cave opening. The generic command dispatcher invokes
`check_item_teleport`, whose handler resolves the configured target
([object](../../../areas/obj/khildarak.obj#L1258),
[reset](../../../areas/zon/khildarak.zon#L2156),
[command constant](../../../src/cmd/interp.h#L134),
[dispatcher](../../../src/cmd/interp.c#L2412),
[handler](../../../src/magic/spell_travel.c#L932)). This is a source-declared
arrival mechanism into #10721 in addition to the reciprocal WLD boundary; the
static audit did not test live placement, execution, access, or safe return.

The external typed-reference sweep found:

- no exact external QST action or story-sidecar reference to a Black Tunnels
  mobile, object, or room;
- no external SHP product, ZON reset import, or duplicate external definition
  of a local mobile or object prototype;
- one inbound type-25 declaration among 872 external object declarations:
  Khildarak #17097 to #10721; and
- no external type-29 switch target into the area.

The exact source token scan found only false numeric matches in Unicode
codepoint tables in `src/net/unicode.c`; it found no authored code callback
binding a Black Tunnels VNUM. These are bounded source scans, not claims about
saved player inventories, live database state, or untracked external tools.

## Title, history, and migration disposition

The normalized `Black Tunnels` title is unique across the current `.zon`
title scan. Followed WLD, MOB, OBJ, and ZON histories retain the same stem
through the 2009 move into `areas/`; later bounded history includes the 2013
area edit, the 2015 quest-item flag pass, and same-stem field/text changes.
The histories show no alternate-stem rename, distinct predecessor, or
whole-zone replacement. No prior duplicate/replacement report identifies a
predecessor for this contributor.

Keep active `black_tunnels` **DO NOT MOVE** as a current full-world
contributor. Do not add an attic candidate row. Preserve the reciprocal
Pragtog and Khildarak boundaries and the Khildarak type-25 arrival declaration
while builders resolve the route, secret-door, diamond-key, white-door, pillar
stair, and hazard questions in the [story dossier](../zone-stories/BLACK_TUNNELS.md#story-boundary-and-builder-questions).

