# Ixarkon: source-only boundaries and typed-closure supplement

Status: repository-wide audit supplement to the existing source-comprehensive
[Ixarkon dossier](../zone-stories/IXARKON.md). It reconciles the active WLD
census, all current unregistered WLD inputs, the active typed consumers, and
title/history evidence. Native zone content and gameplay were not changed or
qualified.

## Local directed reachability

An inline parse of the active Ixarkon WLD found 201 rooms and 468 exits: 464
local and four outbound. Every target resolves. The local graph has three weak
components and three strongly connected components, with sizes 199, 1, and 1.
Each of the four unique active inbound arrival rooms (#96471, #96474, #96588,
and #96600) lies in the 199-room strongly connected core: from each arrival,
all 199 core rooms are authored-reachable, and all 199 can return to it. The
five inbound boundary records include two distinct source edges into #96471;
the four outbound records form four reverse pairs with four of those inbound
records ([inbound rows 69, 276, 390, 730, and 1755](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L69),
[outbound rows 1151–1154](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1151),
[pair summaries 203–206](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L203)).

The two WLD-isolated singleton rooms are #96524, the veil chamber, and #96550,
the pet-shop storage backend. This result describes authored local exits only;
it does not rule on their code-defined routes, reset state, service behavior,
or live access. In particular, #96524 has a code assignment and #96550 is
referenced by the pet-shop procedure. The graph also does not model door
discovery, object travel, or actual player traversal.

## Active boundaries and source-only map edges

Ixarkon has 468 authored and loader-resolving WLD exits, three local weak
components (the largest has 199 rooms), no fall or current fields, five active
inbound and four active outbound cross-zone records, and four reverse-paired
boundaries. The existing dossier maps all five foreign boundary rooms and all
nine directed records. The additional records below come from unregistered
source and do not alter those active census totals.

A scan of all 95 current unregistered WLD files parsed 2,080,773 authored
D0–D9 exits and found two records targeting active Ixarkon room 96492:

| Source-only record | Direction and target | Evidence |
| --- | --- | --- |
| ud_map2 room 1120305 | West (D3) to active Ixarkon room 96492 | [exit target](../../../areas/wld/ud_map2.wld#L26) |
| ud_map2 room 1120404 | North (D0) to active Ixarkon room 96492 | [exit target](../../../areas/wld/ud_map2.wld#L140) |

Both source rooms belong to unregistered ud_map2.wld, which is not part of the
current full-area WLD input. These two authored records are not active-world
entrances. They add two individually reviewed records to the curated
source-only list, bringing it from 360 to 362; they do not establish ordinary
access or change the active count of 468 resolving exits.

## Typed consumers and dependencies

The existing Ixarkon dossier reviews all 15 local quest contracts, five local
shops, the full custom procedures and bindings, and the nine active WLD boundary
records. It also records that no foreign recipe produces or consumes Ixarkon
items and that all 713 active type-25 portal declarations avoid its local
rooms. This supplement confirms the external typed references:

- Two active Ixxillikor reset commands load Ixarkon mobile 96433 at room 4305.
  The dossier identifies the placed actor as a slave fighter; these resets do
  not provide a quest material or a local reward.
  [Ixxillikor reset rows](../../../areas/zon/ixxillikor.zon#L249) ·
  [full Ixxillikor closure in the dossier](../zone-stories/IXARKON.md#complete-source-closure)
- No external MOB, OBJ, QST, SHP, or story-sidecar source had a direct numeric
  reference to the Ixarkon prototype ranges in the exact scan. The two
  Ixxillikor resets are the active external numeric consumer found in the
  corresponding active ZON scan. The two ud_map2 records above are WLD
  references only.

The existing dossier also records its actual object-mediated travel,
including the assigned illithid veil with 25 present targets, the inactive
GithyankiCave room assignment, and the six direct procedure assignments. The
portal review is distinct from the source-only WLD references here.

## Title and replacement-history boundary

The normalized active title Ixarkon has one owner, area ixarkon, zone 964.
Tracked history records a 2010 change titled “Connected the new Faang portion
for Pillithid to exit in ixarkon.” That edit adds one north exit from Ixarkon
room 96474 to Faang room 15412.
This is evidence of an inter-area connection, not a claim that Ixarkon was
replaced by Faang or that either area supersedes the other. The reviewed path
history contains no manifest or commit claim identifying a distinct
predecessor or replacement.
The case-insensitive tracked-path review found no zone or area source named
Pillithid. In the related global history, the wording is consistent with the
Pillithid race reference; it does not establish another Ixarkon predecessor.
The active Ixxillikor and Illithid Arena contributors have separate names and
prototype ranges, with no source evidence that either replaces Ixarkon.
[Active title](../../../areas/zon/ixarkon.zon#L2) ·
[active-area entry](../../../areas/AREA#L345) ·
[Ixarkon exit to Faang](../../../areas/wld/ixarkon.wld#L1851) ·
[2010 Faang connection commit](https://github.com/Community-Duris/Duris/commit/0643ece38e135efb6e1a6fe731f5c0883c9ec74a) ·
[duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md)

## Audit boundary

These are static source findings. The ud_map2 records are not an active route
qualification; dynamic veil travel, exit safety/return, hidden doors, reset
availability, pet/inn/locker outcomes, quest settlement and persisted player
references remain separate. Keep ud_map2 in place while its generator/map
ownership and shaft references are unresolved.
