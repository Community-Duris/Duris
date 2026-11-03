# Cloud Giant Kingdom — Checkpoint 108 full-area audit

This supplement reconciles the existing comprehensive Cloud Giant Kingdom dossier with the active manifest, authored WLD graph, complete inactive-WLD scan, exact typed quest/story/reset/shop references, compiled numeric bindings, active title inventory, and followed Git history. It preserves the dossier’s separate questions about current stock, the tablet route, the Charisma scroll, and Empower Song. Static records do not prove played access, supply, safe travel, or reward effects.

## Active graph and boundaries

`areas/AREA` lists `cldgt` at line 361 as an active full-area contributor. Its ZON title is “Cloud Giant Kingdom,” zone 995, with 181 physical WLD rooms. The active graph census parsed all 351 listed WLD contributors and found 391 authored Cloud Giant exits, all 391 resolving: 390 local and one outbound. No Cloud Giant room VNUM collides with another active room.

The local undirected graph has one 181-room weak component. Directed authored exits from entrance #99500 reach all 181 room records when ordinary gates are treated as available. This is the authored graph only: the dossier separately records that both directions of the locked boulder gate are needed to reach the selected strap carrier and Nonstros, and that actual door state, key destruction, flight, and safe return are runtime questions.

Cloud Giant #99500 south leads to Surface #553314, and Surface #553314 north returns to Cloud Giant #99500. This is the one active inbound/outbound boundary pair; exact evidence is in [outbound exit row 1187](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1187), [inbound exit row 1450](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1450), and [pair rows 81 and 481](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L81).

## Inactive WLD approach

All 95 inactive WLD files parsed without issues: 527,791 rooms and 2,080,773 exits, with no source-room VNUM overlap against Cloud Giant. One non-overlapping source-only exit enters the active entrance: `surf.wld` room #220537 (“Endless Plains of Grassy Pastures”) west to Cloud Giant #99500. It is not active ingress. This raises the reviewed Surf subset from 133 to 134 and the curated source-only total from 1,520 to 1,521. Preserve this edge separately from the active reciprocal Surface pair. The `surf` / `surface2011` candidate remains **RETAIN**; this route does not clear a move.

## Typed quest and code dependencies

The bounded inbound scan found no outside active QST item command, SHP product, ZON reset-role reference, story-sidecar VNUM, duplicate mobile/object prototype, or target among 872 external type-25 declarations. The all-sidecar scan parsed 231 external story files without errors and found no broad numeric match to a Cloud Giant room, mobile, or object. Local SHP stock and local reset references resolve within this source’s prototype sets.

Cloud Giant’s own Q68 has four explicit outbound object dependencies: it gives the Charisma scroll #405 and accepts Bel’s heart #32490, orb of unmaking #26614, and mystic runed stone tablet #402. Their definitions are external to `cldgt`: #32490 is in `bctdl.obj`, #26614 in `negplane.obj`, and #402/#405 in `heavens.obj`. The existing dossier separately traces the selected heart/orb sources and records that no current tablet producer appears in its reset/compiled inventory. Do not infer a working tablet source, new artifact requirement, Charisma effect, or campaign chain from those item names.

Effective direct code references include the `SKILL_EMPOWER_SONG` teacher-table entry for mobile #99548 in `src/classes/epic_skills.c`, and Chaos druid equipment-table entries for object #99538 in `src/account/chaos_eq_data.h` (two kit variants). The latter confirms the existing dossier’s typed “mist sleeves” kit reference. These bounded code uses do not add quest receipts or establish that training is available under active accounting.

## Title, history, and disposition

The normalized active ZON title “Cloud Giant Kingdom” has one owner among 351 non-empty active titles. Other titles containing “kingdom” include `torg` (“The Kingdom of Torg”), `arcium` (“Arcium, the Plagued Kingdom”), and `seakngdm` (“Sea Kingdom”); none is an exact title or source identity match.

Followed WLD/ZON history reaches the 2009 repository-wide `areas/` path move and same-path edits in 2013; the WLD also has a 2009 spelling update, and the ZON has a 2015 quest-item-flag update. The 2026-10-06 sidecar commit adds Cloud Giant journal and legacy quest mapping. The current active manifest and followed paths show no explicit predecessor, alternate-stem rename, or replacement claim. The active Surface endpoint is a separate contributor, not evidence that Cloud Giant has been superseded.

Cloud Giant Kingdom remains an active contributor and **DO NOT MOVE**. Preserve the active Surface pair, both external recipe object owners, the Empower Song binding, the Chaos-kit object use, and the separate source-only Surf edge. Keep `surf` and `surface2011` **RETAIN** pending their full ownership and dependency closure.

## Builder follow-up for the dossier

The inactive `surf.wld` record #220537 west to #99500 is an additional legacy map approach, while the active route is the reciprocal Surface pair through #553314. Confirm whether the source-only edge is intended surviving geography or a retired map route and who owns its target. Keep it source-only in the meantime; do not promise ordinary player access through the inactive file or retire either candidate based on this single record. Preserve the already separate builder questions about tablet availability, Charisma reward effect, and Empower Song purchase settlement.

## Source references

- [Active manifest line 361](../../../areas/AREA#L361), [Cloud Giant ZON header](../../../areas/zon/cldgt.zon#L1), and [active entrance/outbound target](../../../areas/wld/cldgt.wld#L1).
- Active [Surface-to-Cloud exit row 1187](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1187) and [Cloud-to-Surface exit row 1450](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1450); reciprocal pair [row 81](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L81) and [row 481](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L481); inactive [Surf room #220537](../../../areas/wld/surf.wld#L430845) and [its exit target](../../../areas/wld/surf.wld#L430861).
- [Cloud Giant Q68 commands](../../../areas/qst/cldgt.qst#L72), [Bel heart object](../../../areas/obj/bctdl.obj#L800), [orb of unmaking](../../../areas/obj/negplane.obj#L198), [tablet](../../../areas/obj/heavens.obj#L4199), and [Charisma scroll](../../../areas/obj/heavens.obj#L4243).
- [Nonstros Empower Song teacher entry](../../../src/classes/epic_skills.c#L195), [Chaos kit reference to mist sleeves](../../../src/account/chaos_eq_data.h#L133), and [second kit reference](../../../src/account/chaos_eq_data.h#L383).

Evidence used: the full active graph, all-inactive-WLD, exact QST/SHP/ZON/sidecar, prototype-duplicate, portal-target, local-outbound, direct numeric reference, title inventory, and followed-history scans in `D:\Dev\Temp\checkpoint108-cldgt`. No build, test suite, gameplay, database, migration, or operational action was run for this documentation checkpoint.
