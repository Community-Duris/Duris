# Tribal Oasis: repository-wide connectivity and dependency supplement

**Checkpoint 72.** This supplement adds the repository-wide WLD,
typed-reference, title, and bounded-history review for the comprehensive
[Tribal Oasis dossier](../zone-stories/TRIBAL_OASIS.md). The findings describe
source declarations and compiled references; they do not establish played
access, stock, custody, or completion.

## Active connectivity and inactive WLD scan

The active scan parsed all 351 active WLD files without issues. Tribal Oasis
has 304 rooms and 685 authored exits: 684 are internal and one leaves the area.
All 685 targets resolve to loaded rooms in one 304-room weak component. Its
single active boundary is reciprocal:

| Pair | Source records |
| --- | --- |
| Surface | [Tribal Oasis #78000 west to Surface #611692](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L894); [Surface #611692 east return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1602) |

The all-95 inactive WLD scan parsed 527,791 rooms and 2,080,773 exits with no
issues. It found no source-room VNUM overlap and one exit target into Tribal
Oasis: unregistered `surf.wld` room #246122 exits east (`D1`) to #78000
([source exit](../../../areas/wld/surf.wld#L967758)). This is a source-only
map declaration, not an active entrance. It raises the curated source-only
inbound total from 603 to 604. The global active-room scan still finds only
the known duplicate #51800 in Hell Three and Hell Two; Tribal Oasis has no
active-room VNUM collision.

## External typed references and code bindings

No external QST item command, SHP product, ZON reset-role target, exact story
sidecar VNUM, mobile/object prototype definition, or type-25 destination
references Tribal Oasis. All 231 other story sidecars parsed without issues
and had zero exact structured VNUM matches. The all-source scan found 872
external type-25 declarations and no target into this area.

Direct C/C++ numeric matches are already classified in the dossier. CHAOS
equipment arrays reference local object prototypes #78013, #78020, #78024,
#78035-#78037, and #78064-#78065 in
[`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h); these are starter-kit
gear references, not native QST receipts. The epic-skill table also contains
Bargor #78006 on its legacy ENCHANT row
([binding](../../../src/classes/epic_skills.c#L199)). The dossier records why
that row does not currently provide a usable training reward. No new native
journal dependency is identified.

## Title, history, and disposition

The normalized active title scan found one exact “Tribal Oasis” title among
351 active contributors. The color-coded title is in
[`oasis.zon`](../../../areas/zon/oasis.zon#L2), and the active contributor is
listed in [`areas/AREA`](../../../areas/AREA#L281). Bounded tracked WLD and ZON
history reaches the 2009 area-tree move, followed by WLD edits in 2011 and 2013
and ZON edits in 2013, 2015, and 2017. It identifies no distinct predecessor
or replacement. Keep Tribal Oasis **DO NOT MOVE**.

This checkpoint adds no route, reset, shop, or source change. It used repository
source and static registers only; no tests, build, server/runtime, gameplay,
database, migration, or operational script was run.
