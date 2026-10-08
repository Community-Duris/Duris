# Troll Caves: comprehensive source map

Reviewed October 4, 2026. Zone 969, `troll_caves`; roadmap priority 66.
**Source coverage is comprehensive; live gameplay qualification remains pending.**
Active, ready accounting is mandatory for discovery, visible encounters,
journals and new achievement/daily credit. Frozen obligations remain separate.

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
  scanned. The Alatorin gem dealer's exact stock, body, room and full shop
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
underwater96944. All use local live-copy cap one. Foreign Alatorin dealer
120003 at room123215 also declares those exact three input kinds as stocked
goods, with G549–551 caps999 and a separate shop. That is a real source
alternative; declared stock does not prove accepted shopping, availability,
personal mining, original acquisition or daily renewal. Prior Alatorin mapping
and foreign ownership stay unchanged.

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
