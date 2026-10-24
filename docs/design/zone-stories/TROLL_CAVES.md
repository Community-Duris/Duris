# Troll Caves: comprehensive source map

Reviewed October 11, 2026. Zone 969, `troll_caves`; roadmap priority 66.
**Source coverage is comprehensive; live gameplay qualification remains pending.**
Active, ready accounting is mandatory for discovery, visible encounters,
journals and new achievement/daily credit. Frozen obligations remain separate.
Checkpoint 31 adds repository-wide source-only WLD and typed-reference
reconciliation in the [audit supplement](../quest-story-followups/TROLL_CAVES_AUDIT_REVIEW.md),
including the Surface Keeps stock attribution and the unchanged story-sidecar wording.

The [schema-three journal](../../../areas/story/troll_caves.story.json)
represents all five native exchanges: four supporting paid commissions and
one chalice-and-mace story. Eight contacts preserve all nine addressed aliases.
Eight optional checks comprise six current-material checks and two historical
producer receipts. Supplied exact items skip personal crafting, gathering,
source, kill and producer history. The four services leave the zone-completion
denominator; the blessing retains one achievement and potential daily candidate.
This classification does not qualify its actual supply, offering or renewal.
Discovering the zone remains its own achievement.

**No native zone or quest repair ships in this checkpoint.** The wand mismatch,
wording, switch-return policy, foreign sign and guarded legacy forge findings
below are plans. The earlier Kobold guardian repair remains in its separate
`fix:` commit and prominent PR/news entry. Any later actual repair must likewise
state the trigger, before/after behavior, validation, limits and news sentence
in a separate fix commit and the PR.

## Reviewed evidence

- All ten [native blocks](../../../areas/qst/troll_caves.qst): five M and
  five Q, all D0. Five addressed response families contain nine aliases:
  Hraaf's `weapons/weapon`; Guremgh's `mace/hraaf`, `weapon/weapons`,
  `dagger/longsword`; Farghan's `emeralds`. These are conversation choices,
  without nine independently accepted quest achievements.
- All eighty-two [rooms](../../../areas/wld/troll_caves.wld), 96900–96981:
  seventy-six exact prose groups, nineteen headers, twenty-six non-exit
  metadata groups, sixty-nine numeric exit families and 119 text/keyword pairs.
  Full properties and every membership were reviewed. Registry range
  96886–96981 is broader than these physical rooms; the zone header remains
  `96981 2 0 19 29 1`, reset mode two. Nineteen rooms declare chance-fall
  values; none declares a current. Water-flow and heat prose do not create
  current or thermal-damage quest events.
- All twenty-eight [mobiles](../../../areas/mob/troll_caves.mob), 96900–96927,
  and thirty-seven [objects](../../../areas/obj/troll_caves.obj), 96900–96936:
  full flags, properties, effects and extras. Ruby-sword trailing numerals are
  supported optional affect-mask fields in the actual object parser, rather
  than proof of a corrupt prototype. Same aliases do not establish same kinds.
- All 193 [resets](../../../areas/zon/troll_caves.zon): M111/D28/E20/G18/O11/
  F4/P1, 142 exact argument families and 152 parent-aware families. All raw
  rows, memberships, source caps, equipment, container contents and F followers
  were reviewed. F followers retain the last M leader; successive slaves do
  not become each other's leader. P uses an actual matching container found
  by kind, rather than guaranteeing the last declared container's identity.
  Local references resolve; no positive exit-key requirement appeared.
- No local shop file or literal local special assignment exists. That does
  **not** mean no runtime behavior: `read_object` dynamically binds both
  type-29 switches; generic ITEM_TELEPORT dispatch handles the sign/orb;
  tradeskill initialization binds Hraaf's smith; Guremgh's ACT_TEACHER flag
  binds the shared teacher. All eleven smith menu rows and relevant full
  switch, teleport, teacher, forge, falling/arrival and reset execution were
  reviewed. No local inn flag or literal rent assignment appeared.
- Both full ordinary foreign boundary rooms were reviewed: Surface 622564
  down ↔ 96900 up; foreign tunnel 23741 east ↔ 96976 west. Imported bat12210
  and guano5041 bodies were reviewed. Bounded global native consumers,
  producers, resets, shops, assignments and 713 teleport prototypes were
  scanned. The Surface Keeps gem dealer's exact stock, body, room and full shop
  were reviewed. Graves sign19510 is the one foreign fixed teleport into
  these physical rooms, but no reviewed Graves reset supplies it. No other
  native consumer of the local request inputs appeared in this closure.
  A separate full custom-code scan also found Ixarkon's veil96402, reset at
  96524 and bound to `illithid_teleport_veil`; its complete prototype, room,
  placement, assignment, procedure and normal command/object dispatch were
  reviewed. This is a custom random destination route, outside the type-25
  fixed-teleport inventory. Its command mismatch below prevents promising it
  as a currently usable entrance.

The [generated review index](../../reference/zone-story-audits/troll_caves.md)
retains exact contracts, classifications and source references. Dynamic
bindings and admitted outcomes take precedence over a literal-only inventory,
names, narration or a suggested graph.

## Native progression and present accounting limits

| Native block | Exact terms | Journal role |
| --- | --- | --- |
| [Q148](../../../areas/qst/troll_caves.qst#L148), Farghan96926 at96960 | C100000 + I96910 → I96933 | Guarded emerald-cutting service. The actual inputs omit the wand demanded by M128. |
| [Q23](../../../areas/qst/troll_caves.qst#L23), Hraaf96916 at96925 | I96933 + C890000 → I96915 | Guarded pre-blessing mace commission. Cutting is an optional source, not personal history. |
| [Q42](../../../areas/qst/troll_caves.qst#L42), Hraaf | I96907 + C400000 → I96916 | Independent guarded ruby-sword service. Not an alternative blessing input. |
| [Q61](../../../areas/qst/troll_caves.qst#L61), Hraaf | I96906 + C200000 → I96917 | Independent guarded obsidian-dagger service. Not an alternative blessing input. |
| [Q106](../../../areas/qst/troll_caves.qst#L106), Guremgh96925 at96959 | I96915 + I96931 → I96932 | One story/achievement/potential daily candidate. Two exact items; no fee or personal-kill condition. |

Fees are copper: cutting100000 =100 platinum; mace890000 =890 platinum;
sword400000 =400 platinum; dagger200000 =200 platinum. Producing a mace
from raw emeralds takes two separate commissions totaling990 platinum.
The durable offering selector does not accept coin goals, and coin GIVE and
legacy payment remain guarded while accounting is active. Current-material
readiness does not certify funds, accept fees, reserve items or authorize
new credit. Frozen accepted historical receipts can be retained/recovered
without enabling unsupported fresh payments.

The base mace's only reviewed native producer is Hraaf's paid commission.
That producer guard does not make legitimately supplied existing base maces
invalid for the item-only blessing, nor prove a fresh daily route possible.
Ordinary source/reset/shop admission, actual ownership settlement, reward
publication, restart and renewal need gameplay and persistence qualification.
The journal does not silently replace that gap with a legacy payment route.

Uncut emeralds96910 and cut emeralds96933 share `emeralds`, but only the
cut kind satisfies Hraaf. Base mace96915 and enhanced mace96932 have exactly
the same displayed name and keywords. Their fixed properties differ: native
hit/damage applies change from1/2 to2/3, with different dice/flags and weight.
This is not evidence of a generic tenfold multiplier, a permanently tracked
blessing buff or a heartbeat effect. Enhanced mace, ruby sword and dagger
cannot substitute for the base-kind input. Names must not select quest roots.

## Material, access and exploration routes

Rubies96907 and obsidian96906 are loose O125/O126 resets at chasm bottom96942
(the exact order is obsidian then rubies). Uncut emeralds96910 are O130 at
underwater96944. All use local live-copy cap one. The Surface Keeps gem dealer
120003 at room123215 has active resets for those exact three input kinds, each
with cap999, and the dealer's separate shop lists them as stock. This is a real
foreign source alternative; reset declarations and shop stock do not prove
current availability, accepted purchase, personal mining, original acquisition
or daily renewal. [Shop stock](../../../areas/shp/surfacekeeps.shp#L3) ·
[dealer reset](../../../areas/zon/surfacekeeps.zon#L548) ·
[shopkeeper](../../../areas/mob/surfacekeeps.mob#L48).

The student shaman96915 is M166 at room96920, wearing chalice96931 in E169
held slot18. Guremgh's dialogue hopes the student was killed, but Q106 only
requires base mace and chalice. A legitimate supplied chalice skips a personal
kill/theft/recovery story. Farghan's thin black wand96930 is O124 loose in
waterfall room96940, with gnome96905, pouch/jasper/staff/shield and two
following slave96906 resets. It is not declared on the gnome. Finding or
handing over that wand has no native accepted recipe here. A gnome's death,
follower state or visit does not record a slave rescue.

From Surface622564 descend to96900, travel east/north through96901–96903,
then navigate the hot and slimy holes. Hraaf's cave96925 is behind the secret
rock east of lava-bottom96924; the student is behind the southern curtain
from96918 to96920. Guremgh's clearing96959 is east of forest96955 through
secret trees; Farghan's cave96960 is west of96958 through secret trees.
These are ordinary door/movement routes, without declared positive keys or
automatic story acceptance. Healing room96919 has ROOM_HEAL; hot-bottom96909,
Hraaf96925 and lower lava rooms96923/96924/96926 have ROOM_NO_HEAL. These
flags do not create healing, heat-survival or rent achievements.

The two controls are genuine dynamically assigned type-29 switches:

- Boulder96900 at96935: **PUSH** (command270), target96935 south→96936,
  value3=1; D80/D84 reset both sides closed/secret/blocked13.
- Rock pile96908 at96942: **PUSH**, target96942 south→96943, value3=1;
  D96/D100 reset both sides open/secret/blocked12.

The shared switch clears only the target EX_BLOCKED for a secret exit;
it leaves secret/closed bits and does not clear the reverse block in that
branch. It can announce a revealed wall without creating a traversable or
returnable passage. These local targets have valid reverse exits, so no
missing-reverse crash is established. Qualify commands, door state, traversal,
alternative routes and return before choosing a reciprocal-switch policy fix.
Do not advertise a reciprocal opening as already repaired.

Waterfall96939 north↔96940 south starts as a secret/closed world door but
resets open0. The exact warning sign96911 at96939 uses **PUNCH** (189),
destination96937, charges−1. READ examines its warning rather than dispatching
that teleport. Orb96912 at reservoir96943 uses **TOUCH** (320), destination
96940, charges−1. Generic selection searches inventory, equipment and room;
the handler checks exact command/type, destination and arena compatibility.
Unlimited charges do not hit the final-charge accounting guard. Actual
arrival still needs confirmation; messages or a true handler return are not
accepted travel/survival evidence. Graves sign19510 duplicates the PUNCH
destination96937, without a reviewed local reset; builder intent is unresolved.

Ixarkon's [custom veil](../../../src/specs/specs.ixarkon.c#L15), object96402
at exitless room96524, includes Troll room96909 among twenty-five random
destination rooms. It compares ENTER's argument with literal `" veil"`, while
normal [command dispatch](../../../src/cmd/interp.c#L2405) strips leading
blanks and passes the argument unchanged through object invocation. This is
a concrete normal-command mismatch, rather than an unbound portal: actual
placement and assignment exist. Qualify the exact command/selection and
repair that comparison separately, with played arrival and post-arrival
restore/CharWait and interruption behavior. The handler moves the actor and
restores afterward without a recorded journal episode. No veil repair ships,
and this source map does not promise a usable random entrance or claim the
whole foreign zone reviewed.

Nineteen chance-fall declarations are96904–96908,96913,96916–96918,96923,
96926–96929,96941 and96965–96968. The no-ground chasm96941 has F100; its
downward route reaches96942. Shared command dispatch can schedule falling
before a requested command; no-ground arrival can also schedule it. Flight,
levitation, a flying mount or active climb can affect admission; actual
movement, impact, death/removal, interruption and surviving escape remain
separate outcomes. Falls can damage/remove actors and items can fall too.
Prose ropes, water drains and heat do not establish protected arrival or a
current. Reservoir96943 leads to underwater96944 with actual underwater flag.

The deep branch96903 north→96961, shafts96964–96969, circular caves/pool96974,
secret scrag lair96980 and bat cave96981 connects west from96976 to23741.
The static armor egg96909 at96944 calls itself a fish in its extra text;
no reviewed native hatch/fish-transform outcome exists. Miners, fungus,
followers, bones and relics remain exploration guidance without invented
mining, healing, rescue or bandit-campaign receipts.

## Builder decisions and capability additions

| Finding or need | Balanced next action |
| --- | --- |
| Wand demanded by M128 but absent from Q148 | Choose actual wand+emeralds+fee terms or update the demand. Qualify material allocation, wand spell/custody and atomic paid output/recovery before changing the native recipe. Do not label a current wand turn-in implemented. |
| Cut emerald's extra says uncut; Farghan Q narration says Faraghan; egg extra says fish | Review intended wording/identity and keep any eventual text repairs in a separate fix commit with exact before/after and news treatment. No wording repair ships here. |
| Secret controls clear only the forward block; no local reverse control declared | Qualify each ordinary/alternate journey and return. Builder chooses intended one-way versus reciprocal opening and closed/secret state. Add accepted control/state/arrival evidence before journal access objectives. |
| Warning sign PUNCH teleport, plus unplaced foreign Graves sign | Confirm intentional escape verbs/destination and whether the foreign sign is retained unused data or should have a source. Do not change to READ, seed it, or promise an entrance without that decision. |
| Bound Ixarkon veil expects a leading-space ENTER argument removed by normal dispatch | Qualify the actual placed veil, argument/selected-object behavior, random destination, admitted arrival and post-arrival restore/wait. Repair comparison separately with focused executable evidence and prominent foreign-zone news; no usable random entrance or native repair is claimed here. |
| Four paid services plus item-only continuation | Add atomic fee/material/output/NPC settlement, source UID/generation/custody and recovery. Keep payment guards; supplied valid items skip personal production history. Exact all-stage parent summaries must not inflate support services or force ruby/obsidian branches. |
| Same-name different kinds; narrated tenfold blessing/student kill | Preserve exact immutable input/output kinds and accepted offering outcome. Builder can choose any future source/kill/blessing-effect objectives only after dedicated typed events and gameplay proof. |
| Nineteen fall rooms, underwater sources and teleport returns | Capture movement cause/control/item identity, admitted arrival, fall episode, injury/removal, protection, interruption/reset, surviving exit and any party entitlement. Visits/attempts/room text do not complete survival. |
| Dynamic smith and teacher absent from literal inventory | Review actual type/flag/table dispatch; avoid claiming inert controls, no special behavior or extra quest endpoints from the literal-only index. Teacher ASK level remains separate class guidance. |
| Guarded legacy forge menu | Hraaf's table index12 is compared with choice despite eleven rows; choice12 reaches sentinel−1. Ore scan uses NPC inventory. Keep forge disabled; separately qualify bounds/player allocation, paid generated-item output and recovery before enabling or repairing it. |

Hraaf's eleven forge rows57/58/59/27/29/97/101/102/107/109/112 use mithril,
gold, iron, copper and silver ore and generated object1255. They are armor/
accessory recipes, not the three native gem-weapon commissions. The smith's
periodic hum does not dispatch a crafted item. Guremgh's computed `teacher`
only answers qualifying class/ASK-level guidance with currently present epic
stones; it is not a new local quest or proof of learning a skill.

## Explicit builder questions and recommended resolutions

These questions are source-backed decisions, not implemented repairs. Any
approved data or gameplay change needs its own scoped fix, exact before/after
behavior, focused validation, and publication note.

| # | Builder question | Recommended resolution from current source |
| --- | --- | --- |
| 1 | Does Farghan's emerald-cutting service require the thin black wand? | Reconcile M128's wand demand with Q148's actual fee-and-emerald inputs before claiming or implementing a wand turn-in. The declared Q148 recipe omits wand 96930. [M128](../../../areas/qst/troll_caves.qst#L127) · [Q148](../../../areas/qst/troll_caves.qst#L148) |
| 2 | Are the four paid cutting/weapon commissions separate support services from the final blessing story? | Keep Q148, Q23, Q42, and Q61 outside Q106 story completion and any inferred personal crafting history. The five blocks encode four paid exchanges and one distinct blessing. [Q148](../../../areas/qst/troll_caves.qst#L148) · [Q23](../../../areas/qst/troll_caves.qst#L23) · [Q42](../../../areas/qst/troll_caves.qst#L42) · [Q61](../../../areas/qst/troll_caves.qst#L61) · [Q106](../../../areas/qst/troll_caves.qst#L106) |
| 3 | Should each commission retain its exact copper fee? | Preserve 100000 for cutting, 890000 for the base mace, 400000 for the ruby sword, and 200000 for the obsidian dagger; do not infer a coin objective or active-accounting payment path. [Q148](../../../areas/qst/troll_caves.qst#L148) · [Q23](../../../areas/qst/troll_caves.qst#L23) · [Q42](../../../areas/qst/troll_caves.qst#L42) · [Q61](../../../areas/qst/troll_caves.qst#L61) |
| 4 | Does Q106's blessing require only base mace 96915 and chalice 96931, without a personal kill? | Keep the two exact item inputs as the source-defined receipt; do not turn Guremgh's hope that the student was killed into a kill condition. Treat his teacher guidance as a separate command behavior. [Q106](../../../areas/qst/troll_caves.qst#L106) · [Guremgh](../../../areas/mob/troll_caves.mob#L351) |
| 5 | Should base mace 96915 and enhanced mace 96932 remain distinct exact-kind items despite sharing a display name? | Preserve prototype identity in any future objective or text; the recipe consumes 96915 and returns 96932, with different fixed properties. [Objects 96932–96933](../../../areas/obj/troll_caves.obj#L496) · [Q106](../../../areas/qst/troll_caves.qst#L106) |
| 6 | Is Farghan's cutting commission an optional producer for the blessing's base mace? | Keep supplied valid 96915 eligible without requiring personal cutting history; Q23 can produce it, but Q106 asks for the item rather than its provenance. [Q23](../../../areas/qst/troll_caves.qst#L23) · [Q106](../../../areas/qst/troll_caves.qst#L106) |
| 7 | Should the Surface Keeps gem dealer remain a possible raw-material source rather than a guaranteed supply? | Record the separate shop stock and reset placement, but leave actual stock, purchase, and admission unqualified until gameplay evidence exists. [Shop](../../../areas/shp/surfacekeeps.shp#L3) · [reset](../../../areas/zon/surfacekeeps.zon#L548) |
| 8 | Should the story sidecar's Alatorin dealer label be corrected to Surface Keeps? | The reviewed shop and room belong to Surface Keeps. Keep this audit documentation-only; make any sidecar edit separately after the content owner confirms the intended wording. [overview](../../../areas/story/troll_caves.story.json#L19) · [dealer reset](../../../areas/zon/surfacekeeps.zon#L548) |
| 9 | Should the boulder and rock-pile PUSH controls open only their targeted exits, or a reciprocal route? | Keep current source behavior distinct: both objects bind to the shared switch handler, and the local resets can leave reverse doors closed/secret/blocked. Choose a return policy only after qualifying both journeys. [switch resets](../../../areas/zon/troll_caves.zon#L76) · [chasm door resets](../../../areas/zon/troll_caves.zon#L96) · [switch handler](../../../src/specs/specs.object.c#L309) |
| 10 | Is the waterfall passage meant to reset open even though its WLD exit is secret/closed in authored data? | Preserve the observed source distinction: waterfall exit flags are reset to open. Confirm intended discovery and return behavior in a separate gameplay qualification. [WLD rooms](../../../areas/wld/troll_caves.wld#L834) · [north-exit reset](../../../areas/zon/troll_caves.zon#L88) · [return reset](../../../areas/zon/troll_caves.zon#L92) |
| 11 | Is PUNCH the intended command for the warning sign's escape teleport? | Keep the declared verb and destination 96937 visible in the review; do not silently change it to READ or infer that foreign sign 19510 is placed. [sign object](../../../areas/obj/troll_caves.obj#L154) · [foreign sign](../../../areas/obj/graves.obj#L134) |
| 12 | Is TOUCH on orb 96912 the intended arrival at room 96940? | Retain the type-25 destination and command as separate static evidence; qualify actor admission and return before calling it a safe route. [orb](../../../areas/obj/troll_caves.obj#L168) · [teleport dispatch](../../../src/magic/spell_travel.c#L932) |
| 13 | Should Ixarkon's custom veil count as an entrance to Troll Caves while its normal ENTER argument does not match the handler? | Keep it conditional and unusable in access claims until the placed veil, command dispatch, destination, admission, and return are separately verified or repaired. [veil reset](../../../areas/zon/ixarkon.zon#L156) · [custom target list](../../../src/specs/specs.ixarkon.c#L21) |
| 14 | Should Hraaf's legacy forge stay disabled while its menu bounds can reach the sentinel? | Keep the accounting guard; source lists eleven recipes, but the selection bound uses the smith-table index. Any bounds or allocation repair belongs in a separate focused code change. [smith table](../../../src/economy/tradeskill.c#L214) · [accounting guard](../../../src/economy/tradeskill.c#L793) · [selector](../../../src/economy/tradeskill.c#L832) |
| 15 | What safety and return behavior should be promised for the nineteen chance-fall rooms? | Make no safe-route or survival claim from fall percentages alone. Qualify movement cause, actor protection, interruption, damage/removal, return, and any party credit before adding objectives. [room census](TROLL_CAVES.md#reviewed-evidence) · [example fall declaration](../../../areas/wld/troll_caves.wld#L307) |
| 16 | Is room 96941's no-ground exit with a 100 percent fall intended to lead directly to room 96942? | Preserve the source path and keep fall outcome, survival, and escape as separate unqualified behaviors; do not imply the water or room text guarantees recovery. [chasm rooms](../../../areas/wld/troll_caves.wld#L875) |
| 17 | Should water-flow and heat prose create current, temperature, or quest events? | No such current or thermal-damage flag is authored in the reviewed zone. Add a mechanic only as an explicit new design with its own source and validation. [zone flag census](TROLL_CAVES.md#reviewed-evidence) · [waterfall room](../../../areas/wld/troll_caves.wld#L834) · [chasm room](../../../areas/wld/troll_caves.wld#L875) |
| 18 | Should the three old Surf/Underdark exits be treated as active entrances? | Keep them in the source-only register as inactive unregistered WLD records; do not use them for active access or route claims. [source-only records](../quest-story-followups/TROLL_CAVES_AUDIT_REVIEW.md#active-and-source-only-wld-boundaries) |
| 19 | Should Graves sign 19510 be treated as a current Troll Caves arrival source? | Keep only its declared PUNCH destination in the dependency record; no reviewed reset places it, so do not seed it or promise an arrival without an owner decision. [sign declaration](../../../areas/obj/graves.obj#L134) |
| 20 | Should imported bat 12210 and guano 5041 be locally duplicated or treated as external reset dependencies? | Preserve the referenced prototypes with their owning areas and do not create local copies or claim local ownership from Troll Caves' reset rows alone. [guano reset](../../../areas/zon/troll_caves.zon#L131) · [bat resets](../../../areas/zon/troll_caves.zon#L277) |

## Validation and publication scope

Focused source fixtures verify all five exact recipes, classification, aliases,
sources/foreign stock, same-name distinct inputs, dynamic switches, configured
teleports, fall fields and full census. Existing actual C++ journal journeys
verify unseen-recipient gating, supplied optional history, wrong-kind/worn
materials, service exclusion, retained receipts, spent supplies, immutable
owner, replay and cold recovery. Injected historical paid receipts do not
qualify live coin acceptance. Full production/source and all-map/schema/file
loader regressions, relevant accounting gates, maintained build, changed/staged
formatting, whitespace, native/prior-map preservation and local/source links
are checked before publication.

Catalog:87 maps/1618 achievement units/1459 potential dailies/2207 rows.
All2668 native definitions, fingerprint/content revision two, registry and
prior86 mappings remain unchanged. Four former support units leave the
achievement denominator deliberately. Original queue:66/220 comprehensive,
154 pending; Centaur Villages next. Live reset/shop acquisition, handoff,
paid/native offerings, reward publication, switches/doors/travel/falling/death/
survival, effect application, persistence and renewal remain unqualified.
No DB/account/server operation, migration, deployment or merge occurred.
