# The Caverns of the Worms: exploration and Wilms's commissions

Priority103 of the original220-zone queue. This is a **source-comprehensive map**;
played accounting/source/access/persistence qualification remains pending. The
[schema3/revision1 journal](../../../areas/story/worms.story.json) maps sixteen
independent support services, one contact/twenty addressable aliases and thirty-four
optional current-material rows, without exclusions. Services retain accepted
receipts and add no quest achievements or dailies. Discovery remains separate.
New discovery/encounter/journal/achievement/daily credit requires active, ready
accounting; frozen recovery remains separate.

## Source closure

| Source | Complete review and implication |
| --- | --- |
| [Quest source](../../../areas/qst/worms.qst) | All16 Q/17 addressed M/20 aliases, complete dialogue, repeated ingredients and belt's empty completion reply. Every spoken material/count matches its native recipe. No ambient block, fee, departure, local intermediate exchange or exclusions. |
| [Rooms](../../../areas/wld/worms.wld) | All100 physical rooms6900–6999,21 full prose families/3 headers/1 metadata family,220 exact exits/68 relative patterns/one full empty exit-text family. Registry6870–6999/mode2. Entry/alcove flags13:dark+no-mob+indoors; ordinary burrows flags9:dark+indoors/sector15UnderdarkInside.6984 flags0/sector0; no swimming/flight prerequisite inferred from damp prose. |
| [Mobiles](../../../areas/mob/worms.mob) | All16 full records6900–6915/16 prose families plus ID-paired numeric metadata. Fifteen colored small/middle/large worms carry matching materials; Wilms6915 is a level50 duergar curer with cap1 placement at6907. Sources and caps are declarations, not played availability. |
| [Objects](../../../areas/obj/worms.obj) | All32 full records6900–6931, values/flags/extra prose/applies. Fifteen distinct strips/pieces/hides6900–6914 retain quest flag32768; sixteen equipment rewards6915–6930 and fixed garbage control6931. Rewards and raw materials are separate kinds. No local type25 route. |
| [Resets](../../../areas/zon/worms.zon) | All204 complete argument/location/cap/chance/parent declarations through111 expanded exact families/188 parent-aware signatures:D2/O1/M101/G100, allchance100. Fifteen worm source groups total100 material stocks; one curer; one fixed garbage at6937. |
| Shared execution | Full relevant [quest.c](../../../src/world/quest.c) loader and addressed ASK/GIVE/durable selection reviewed, along with unchanged active-accounting, custody, corpse/source, current have/count and accepted-receipt projection paths. Full setup_dir and D reset decoding reviewed; existing generic item_switch closure retained. No native tradeskill curer assignment or local literal special assignment found. |
| Foreign closure | Global touching recipe/reset/room/object/713-portal scan:all16 touching recipes local, no foreign reset groups or ordinary incoming type25 declaration. Full Underdark811708/connectorzones54211 boundary rooms read. Full Ixarkon96402 prototype/source96524 and entire [illithid_teleport_veil](../../../src/specs/specs.ixarkon.c) reviewed;25 random destinations include6900. Foreign custom travel/restore ownership is distinct. |

## Exact equipment services

| Commission | Exact native input → output | Quantity and overlap |
| --- | --- | --- |
| Helmet, Q93 | I6900+I6908 → I6915;D0 | 1 × a strip of grey wormskin; 1 × a red wormhide |
| Choker, Q99 | I6906+I6909 → I6916;D0 | 1 × a strip of red wormskin; 1 × a strip of brown wormskin |
| Eyepatch, Q105 | I6901 → I6917;D0 | 1 × a piece of grey wormskin |
| Mask, Q110 | I6910+I6910+I6912+I6906 → I6918;D0 | 2 × a piece of brown wormskin; 1 × a strip of purple wormskin; 1 × a strip of red wormskin |
| Ear clasp, Q118 | I6903 → I6919;D0 | 1 × a strip of glowing wormskin |
| Armor, Q123 | I6914+I6911+I6906+I6906+I6906 → I6920;D0 | 1 × a purple wormhide; 1 × a brown wormhide; 3 × a strip of red wormskin |
| Backpack, Q132 | I6900+I6908+I6905 → I6921;D0 | 1 × a strip of grey wormskin; 1 × a red wormhide; 1 × a glowing wormhide |
| Cloak, Q139 | I6908+I6908 → I6922;D0 | 2 × a red wormhide |
| Sleeves, Q145 | I6902+I6910 → I6923;D0 | 1 × a grey wormhide; 1 × a piece of brown wormskin |
| Gloves, Q151 | I6904+I6904 → I6924;D0 | 2 × a piece of glowing wormskin |
| Bracelet, Q157 | I6900+I6909+I6912 → I6925;D0 | 1 × a strip of grey wormskin; 1 × a strip of brown wormskin; 1 × a strip of purple wormskin |
| Ring, Q164 | I6903+I6912 → I6926;D0 | 1 × a strip of glowing wormskin; 1 × a strip of purple wormskin |
| Belt, Q170 | I6902+I6909 → I6927;D0 | 1 × a grey wormhide; 1 × a strip of brown wormskin |
| Leggings, Q175 | I6905+I6911 → I6928;D0 | 1 × a glowing wormhide; 1 × a brown wormhide |
| Boots, Q181 | I6913+I6907 → I6929;D0 | 1 × a piece of purple wormskin; 1 × a piece of red wormskin |
| Shield, Q187 | I6914+I6905+I6907+I6907+I6913 → I6930;D0 | 1 × a purple wormhide; 1 × a glowing wormhide; 2 × a piece of red wormskin; 1 × a piece of purple wormskin |

There are40 consumed ingredient occurrences across34 optional kind/count rows.
Strip, piece and whole hide are different objects for each color. Small worms
carry strips, middle-sized worms pieces and large worms hides. Declared source
stocks by color and size are grey8/3/2, glowing12/10/4, red11/6/6,
brown10/6/4 and purple10/4/4. Every material-bearing M is followed by its exact G;
none of these declarations proves an individual player's first recovery.

All sixteen commissions accept the complete loose inventory bundle together.
Repeated goals select distinct object roots; the largest shield bundle uses five,
below durable maximum14. There is no new NPC-held incremental collection state.
Held/worn/nested/consumed/transferred stock is not current preparation. A finished
reward does not stand in for ingredients or prove historical acceptance. Exact
supplied skins fit without earlier personal kills, source visits, dialogue or gates.
The renderer already shows actual have/count; missing quantities are dynamic.

## Recipe dispatch and source provenance

ASK explains the requested equipment, but has no selected-order state. The
loader prepends native Q and G lists. Durable submission finds the first complete
supported bundle with a goal matching the offered kind. Consequently shield,
backpack and ring can precede the overlapping boots, helmet and ear-clasp bundles.
The journal describes competing allocation; a card does not reserve stock or
change acceptance precedence. For a deliberate order the universal plan must bind
an exact request to an admitted transaction, rather than store a UI preference.

Selection walks the actor's loose roots and prevents repeated use within a bundle.
With spare copies, it does not force the specific triggering pointer into the
consumed set. This can be valid bulk-trigger behavior; intended indexed-item
semantics need accounting qualification before calling it broken or selecting a
repair. A personal first-source milestone must use the frozen **actually consumed**
UID roots and source/custody facts. It cannot presume the trigger item was consumed.

All native accepted identities remain unchanged. Historical raw sixteen outcomes
project to zero authored quest achievements/dailies while all sixteen service
receipts survive serialization, replay and cold recovery. The empty belt reply
still has its native accepted outcome. No all-set terminal or campaign achievement
is inferred from equipment rewards or Wilms's merchant prose.

## Exploration and access stories

The Underdark enters6900 from811708; from6900 its return route is SOUTH. The short
duergar corridor leads east to the curer's alcove. Grey smooth/crumbly burrows lead
into glowing phosphorescent, brown slimy, red damp and purple moldy families, with
overlapping routes. These regions provide material exploration rather than a
required ordered personal clearance. Darkness and actual movement policy remain.

Fixed garbage6931 is O cap1 at6937. Its type29 values are PUSH270/6937/SOUTH/value3=1.
Room6937SOUTH points to6984 with raw door state8, and reset D also uses8. setup_dir
masks initial low bits; reset_zone decodes raw8 into runtime EX_BLOCKED128.
Generic item_switch clears BLOCKED. Return6984NORTH→6937 is open, and6984SOUTH
connects to connectorzones54211NORTH. A shared-open route is an alternate access,
not proof of the player's own solve. The control stays fixed and is not a recipe.

The entry's west-to-Underdark wording conflicts with the actual SOUTH exit; east
into6901 is correct. A one-word source-caption correction is proposed below and
requires its own named fix/news treatment if chosen. No native change ships here.

Ixarkon's independently owned object96402 at96524 dispatches exact ENTER argument
` veil`, randomly chooses among25 valid destinations including6900, moves/restores
and imposes its native wait. There is no Wilms quest receipt or required incoming
travel prerequisite. The foreign room's descriptive east is not a local exit;
any owning-zone correction requires its own evidence and scope. Scarcity, arrival,
restoration, account admission and PvP consequences remain native policy.

## Builder follow-ups and expanded universal plan

| Follow-up | Evidence and intended next step |
| --- | --- |
| ZSQ-WORMS-SELECTED-COMMISSION | ASK is descriptive; Q loading prepends requests, and durable GIVE chooses the first complete supported bundle sharing the offered kind. Shield/backpack/ring can precede overlapping boots/helmet/ear-clasp bundles. Define an explicit selected-contract invocation with frozen exact native binding, actor, distinct consumed roots/quantities and reward; preserve deliberate precedence and historical receipts. A presentation-only selection cannot change settlement. |
| ZSQ-WORMS-CONSUMED-UID-PROVENANCE | Native bundle search chooses distinct loose roots but does not force a surplus same-kind trigger pointer into the consumed set. Qualify intended bulk-trigger/indexed-item semantics before calling this a bug or proposing a separate fix. First-acquisition/source milestones must inspect actual frozen consumed roots and their admitted source/custody, rather than assume that the offered pointer was consumed. |
| ZSQ-WORMS-CURRENT-BUNDLES | All sixteen requests need their full bundle together, with repeated red hides, brown/glowing pieces and three red strips. Shield requires five roots, below durable maximum14. Renderer already shows current have/count. Qualify wrong colors/sizes, incomplete counts, distinct UID selection, competing allocation, loose versus worn/held/nested/spent stock and all-or-nothing settlement; do not invent NPC-held incremental collections. |
| ZSQ-WORMS-SOURCE-AND-SUPPLIED | Fifteen exact material kinds are G on corresponding small/middle/large worms;100 source declarations. Personal kill, death/corpse extraction and first-source acquisition need actor/UID/generation endpoints. Current supplied proof is valid without earlier combat/tunnel/control history. Reward possession and readiness do not prove accepted service or source. |
| ZSQ-WORMS-ACCOUNTING-RENEWAL | Mode2, cap1 Wilms and capped worm stocks do not promise repeatable availability. Accounting-active O/G issuance remains guarded until durable reset generation and legitimate factory admission are qualified. Test accepted source, bundle/output custody, cap/renewal, rejection and cold recovery without free materials or relaxed issuance guards. |
| ZSQ-WORMS-SHARED-GARBAGE-GATE | Fixed type29 object6931 uses PUSH270→6937SOUTH with value3=1. Raw door/reset state8 becomes runtime EX_BLOCKED128; item_switch clears BLOCKED, not a personal quest endpoint. Qualify exact actor/control generation and successful exit transition, remaining bits, shared-open passage and replaced control before personal solve credit. Preserve fixed placement and native command. |
| ZSQ-WORMS-EXPLORATION-ENDPOINTS | Grey/glowing/brown/red/purple burrows, duergar alcove and deeper connector are coherent guidance, not native clearance or boss quests. Darkness and actual sectors/flags govern movement. Builder-selected exploration/combat milestones need successful actor outcomes and renewal scope before a new achievement-bearing campaign. |
| ZSQ-WORMS-ENTRANCE-WORDING | Entry6900 says the Underdark is west, but its reciprocal Underdark811708 route is SOUTH; east into6901 is correct. Proposed repair: replace only west with south, retain all exits/flags/source policy, add original-fails/repaired-passes source-caption coverage and a separate named fix/news commit. No wording repair is selected or applied in this checkpoint. |
| ZSQ-WORMS-FOREIGN-VEIL | Ixarkon object96402@96524 owns illithid_teleport_veil; exact ENTER argument routes randomly among25 valid targets including6900, restores and waits. Qualify successful arrival/restore/accounting and existing travel/PvP policy under the owning zone. It is not required Worms progression or an accepted Wilms contract. No local literal procedure, ordinary incoming type25 declaration or foreign material recipe/reset group was found. |
| ZSQ-WORMS-SERVICE-HISTORY | All sixteen Q are independent supporting commissions. Preserve all native accepted IDs and receipts while raw16 achievement/daily candidates project to authored0; discovery remains separate. Belt's empty written completion reply does not remove its accepted receipt. Any future campaign terminal or crafting achievement requires deliberate builder semantics, rather than deriving it from a full equipment set. |


## Qualification and publication

Focused exact source/schema assertions cover all sixteen service bindings, aliases,
material quantities, source parents, boundaries, fixed control/decoded state and
foreign custom ownership. C++ projection journeys cover wrong sizes, current
partial counts, repeated stock, complete supplied bundles, reward-only and spent
stock, independent accepted receipts, replay/cold recovery and raw16→authored0
reclassification with all receipts retained. These synthetic receipts qualify
projection, not native selection, combat/source issuance or played accounting.
Full production catalog/inventory/audit regression, all120 journal journeys,
maintained build, changed/staged format and exact preservation are required before
publication. No source/reset/accounting gameplay activation or native repair is claimed.
