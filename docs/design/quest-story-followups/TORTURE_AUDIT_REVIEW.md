# Troll Slave Compound audit review

Checkpoint 394 completes the source-level connectivity, title/history, typed
dependency, and builder-question review for active `torture`. The detailed
[Troll Slave Compound dossier](../zone-stories/TROLL_SLAVE_COMPOUND.md)
records room stories, local actors and objects, resets, hazards, and
source-backed builder decisions. No native source was changed.

## Active WLD graph and Ghore boundary

The active [Torture WLD](../../../areas/wld/torture.wld#L1) contains 31 rooms
and 61 resolving exit records: 60 internal records and one outbound edge.
Those internal records form one weak component. In the raw directed WLD graph,
all 31 rooms are reachable from entrance room #95400, and all 31 can return to
#95400 and its Ghore outbound. These counts treat every authored exit as an
edge; they do not apply closed-door, fall, mobile, or runtime rules.

| Direction | Source declaration | Reverse record | Source interpretation |
| --- | --- | --- | --- |
| Inbound | Ghore #11529 D2 south → Torture #95400 ([exit ledger](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L210)) | Torture #95400 D0 north → Ghore #11529 ([exit ledger](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1146)) | Both boundary records have exit flags 0 and key 0: neither is a door. |

The exterior boundary is separate from the inner gate: Torture #95400 D2 to
#95401 and its reverse are doors reset closed by Torture's ZON file
([door resets](../../../areas/zon/torture.zon#L8)). The existing
[cross-area pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L151) already
records the reciprocal Ghore pair. Correct the older dossier statement that
called the north boundary a door; the source shows the gate on the south exit
inside room #95400.

The four authored fall fields are in #95402 (10%), #95426 (20%), #95428 (10%),
and #95429 (5%). The reset file also closes the special-treatment hatch and
corpse-storage grate in both directions. Raw graph reach therefore does not
prove that a character can traverse those reset states, avoid falls, or return
safely. The existing dossier documents the relevant [fall source and code
paths](../zone-stories/TROLL_SLAVE_COMPOUND.md#fall-and-fire-plane-behavior)
without claiming live outcomes.

## Inactive WLD approaches

The repository contains 446 WLD files: 351 active contributors and 95
inactive sources. The inactive sources parsed without WLD-record errors; none
defines a room number in #95400–#95430 or contains an exit targeting one of
those rooms. There is no inactive source-only inbound edge and no numeric room
overlap to add to an attic candidate. The source-only totals remain 9,479
overall and 297 for Surf; no candidate row is changed.

## Typed dependencies

The active `areas/AREA` entry is `torture *954` ([membership](../../../areas/AREA#L341)).
The four zone files are WLD, MOB, OBJ, and ZON. `areas/qst/AREA` also contains
`torture *954` ([manifest entry](../../../areas/qst/AREA#L301)), but no
`areas/qst/torture.qst` exists. There is no Torture SHP file or story sidecar.
The manifest line alone is not a quest contract.

The bounded scan covered all 446 current WLD files (351 active, 95 inactive),
445 MOB files, 442 OBJ files, 442 ZON files, 265 QST files, 180 SHP files,
232 story sidecars, and maintained source files. It found:

- no external active or inactive ZON reset reference to a Torture room, mobile,
  or object, and no Torture reset import of a nonlocal prototype;
- no external QST item command or active giver-header collision for a local
  prototype;
- no external SHP product or typed story-sidecar reference to a Torture
  prototype;
- no external type-25 object destination targeting a Torture room;
- no exact room, mobile, or object ID overlap in mini-area sources;
- no exact maintained-code VNUM binding to a Torture room, mobile, or object;
- no active or inactive external same-type WLD, MOB, or OBJ prototype
  duplicate.

All 232 current story JSON sidecars parsed. These results bound authored
references in the scanned checkout; they do not establish indirect runtime,
editor, or persisted references.

## Title, map identity, and followed history

After color-code and spacing normalization, “Troll Slave Compound” occurs once
among all 442 current ZON titles and has one active owner. Its source stem is
`torture`, with rooms #95400–#95430, mobiles #95400–#95418, and objects
#95400–#95416. No active or inactive external same-type definition overlaps
those local identities.

The followed WLD, MOB, OBJ, and ZON histories first appear at the 2009-08-12
tracked area-tree import under their current `torture` stems; later recorded
changes continue on those paths. The available history shows no alternate-stem
rename, distinct predecessor, or whole-zone replacement. It does not establish
what existed before that tracked import. Preserve this source identity and
keep the active area **DO NOT MOVE**. No Torture attic candidate or migration
is proposed.

The twenty explicit builder questions and conservative recommendations are in
the [zone dossier](../zone-stories/TROLL_SLAVE_COMPOUND.md#explicit-builder-questions-and-recommended-resolutions).
Source declarations do not qualify live gate entry, hatch or grate access,
mobile behavior, fall damage, fire-plane safety, or captive outcomes.

## Checkpoint 849: Current source boundary and negative typed closure refreshed

The active source set remains WLD, MOB, OBJ, and ZON only. `areas/qst/AREA`
still lists `torture *954`, but there is no local Torture QST, SHP, or story
sidecar. Preserve the index mismatch as a source-list fact; it does not define
a rescue contract or shop.

The bounded current-source closure remains as documented at Checkpoint 394:
no external active/inactive ZON reset reference, QST item or giver reference,
SHP product, typed story-sidecar VNUM, type-25 destination, same-type
prototype, or exact maintained-code VNUM binding targets a Torture room,
mobile, or object. Torture's own resets contain no nonlocal prototype. The
negative scan covers the current tracked files only and does not claim absence
of editor, persisted, or indirect runtime relationships. See the [typed
closure](#typed-dependencies), [source dossier](../zone-stories/TROLL_SLAVE_COMPOUND.md),
and [QST index entry](../../../areas/qst/AREA#L301).

The 31-room WLD graph remains 60 local directions and one reciprocal Ghore
boundary, in one weak component; the all-95 inactive scan still finds no room
overlap or approach. The inner gate, hatch, locked grate, and four fall fields
remain separate authored route state and safety evidence. Keep active Torture
**DO NOT MOVE**; this refresh adds no candidate, activation, or migration
clearance.
