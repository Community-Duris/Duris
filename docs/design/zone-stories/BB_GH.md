# BB GH

Canonical source key: component:bb_gh. The five source files are not listed
in areas/AREA and have no full-area membership, local quest file, map JSON, or
story sidecar. This dossier records the source text and the structures it
describes; it does not qualify current loading or live gameplay.

## The hall in the crack

The entrance description places the hall in a natural crack in a cave in the
Underdark. Two rows of obsidian pedestals guide visitors toward the larger
cave. Blood-painted runes and faerie fire turn the approach into a deliberate
ritual space. In the central chamber a large obsidian pedestal stands among
flames. Four small bridges cross a lake of fire, and a pentagram on the
pedestal is filled with boiling blood. The prose makes the hall feel
ceremonial, martial, and dangerous at once
([entrance and central chamber](../../../areas/wld/bb_gh.wld#L1)).

Nearby rooms fill in everyday guild activity. A goblin alchemist works among
shelves, old chests, potions, and mixtures; the local shop is keyed to mobile
#30301 and is recorded for room #30302. Other descriptions include a war
room, vaults, pools, moss, glowing mushrooms, and a resting cavern containing
battle plans and a history of “the clan.” The local mobile source provides
stone golems, a goblin alchemist, and shadow wraiths. None of these names
expands the zone title “BB GH” into a particular guild or clan identity.

## Four separated pockets

The WLD divides #30300–#30343 into five local walking components. The
entrance-side component contains #30300–#30309. The other four pockets are
#30310–#30319, #30320–#30329, #30330–#30339, and #30340–#30343. Their
descriptions evoke fiery rifts and shafts, blackened and cold rock, and
lightning-charged air. The last group includes a cold black surface beneath
a fiery shaft.

Eight locally defined glowing portals are reset as four pairs between the
entrance-side rooms and the four separated pockets. The object source uses
teleport type 25 and CMD_ENTER value 7, with a fixed room VNUM in each
direction; the code contains a matching item-teleport handler. These are
source-level links. The WLD graph by itself leaves the pockets separate, and
the candidate has no full-area registration. The numeric destinations also
overlap unregistered vodgh and venom WLD sources. The source therefore does
not establish which map definitions or portal destinations would be active.

## Identity and continuity

The raw ZON title is “BB GH.” Room prose names “the clan” but does not identify
it. The code binds an inn procedure directly to numeric room #30303; that
procedure handles player rent requests. This numeric hook is evidence of a
code dependency, not proof that the bb_gh definition of #30303 is loaded.

The source rooms #30300–#30343 overlap unregistered vodgh and venom records.
Their mobile and object VNUMs also overlap, and both bb_gh.shp and vodgh.shp
contain distinct shop records keyed to #30301. The source history does not
establish that these are alternate drafts or replacements. Preserve all
three identities until their owners resolve the relationship and room
ownership.

For the complete graph, source dependency, title-history, and retention
review, see the [BB GH technical audit](../quest-story-followups/BB_GH_AUDIT_REVIEW.md).
Explicit questions and recommended resolutions are in the
[builder review](BB_GH_BUILDER_QUESTIONS.md).
