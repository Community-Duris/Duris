# Lylr-Meop: repository-wide route, dependency, and history supplement

This bounded static supplement accompanies the [comprehensive Lylr-Meop
source dossier](../zone-stories/LYLR_MEOP.md). It records active and
source-only WLD edges, raw directed reach, exact cross-area object references,
compiled bindings, title ownership, and bounded same-stem history. Static
source review does not establish that a player can traverse a route or
complete an interaction.

## Active WLD boundaries and directed reach

Active `lylr`, zone 822, has 64 rooms from #82200 through #82263. Its 134
authored exits all resolve: 132 are local and two leave the area. The local
map has weak components of 40 and 24 rooms. Raw directed traversal from
#82200 reaches 40 rooms, and 40 have a path back to #82200. Rooms #82240–#82263
form the other component and are absent from root-forward and root-return
reach. This is a WLD-only result; it does not qualify an alternate runtime
route.

Both active exits lead outward to Surface and have no reverse-direction WLD
record: Lylr #82200 north to Surface #540876, and Lylr #82240 south to Surface
#539281 ([pair summary row 258](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L258),
[first Lylr room](../../../areas/wld/lylr.wld#L1), [second Lylr room](../../../areas/wld/lylr.wld#L761)).
Neither target has an authored Surface exit back into Lylr in the active map.

## Inactive WLD sweep

All 95 current WLD sources outside the active areas/AREA list parsed cleanly:
527,791 rooms and 2,080,773 exits, with no room-VNUM overlap with Lylr. Two
non-overlapping source-only Surf exits target its separate components:
room #222832 north to #82240 and room #224629 south to #82200
([first source room](../../../areas/wld/surf.wld#L478984), [second source
room](../../../areas/wld/surf.wld#L516661)). They are not active ingress.
Curated source-only totals rise to 3,337 overall, 168 for the reviewed Surf
subset, and 15 for `ud_map2`.

## Exact typed and compiled dependencies

Lylr's active reset file contains an exact external object placement and
three external object gifts:

| Source | Typed reference | Target source and status |
| --- | --- | --- |
| Lylr ZON line 82 | O reset places object #132581 in Lylr room #82228. | Active `tharnadia.obj` defines #132581 as a counter ([reset](../../../areas/zon/lylr.zon#L82), [object](../../../areas/obj/tharnadia.obj#L1373)). |
| Lylr ZON lines 125–127 | G resets give objects #363, #368, and #369 to mobile Cionvra #82204 in room #82220. | Active `heavens.obj` defines them as a gauze bandage, gauze tourniquet, and large medicated bandage; active Lylr SHP #82204 lists the same products ([first reset](../../../areas/zon/lylr.zon#L125), [last reset](../../../areas/zon/lylr.zon#L127), [Cionvra](../../../areas/mob/lylr.mob#L54), [shop products](../../../areas/shp/lylr.shp#L1), [bandage](../../../areas/obj/heavens.obj#L3783), [tourniquet](../../../areas/obj/heavens.obj#L3836), [medicated bandage](../../../areas/obj/heavens.obj#L3850)). |

The selected all-zone scans found no exact external QST, SHP, story, or
reset-role reference into a Lylr room, mobile, or object. Lylr's local QST
and sidecar contain no item VNUM outside its local object set. All 231 other
story sidecars parsed without numeric-token hits into Lylr. No external
duplicate mobile or object prototype, or external type-25 target into Lylr,
was found among 872 scanned type-25 declarations.

Compiled assignments bind three separate services: mobile #82224 to
`money_changer`, mobiles #82229 and #82208 to `world_quest`, and room #82217
to `inn` ([assignments](../../../src/specs/specs.assign.c#L718),
[world-quest bindings](../../../src/specs/specs.assign.c#L778),
[inn binding](../../../src/specs/specs.assign.c#L2307)). These code
references are not evidence that the services or their routes were exercised
in a running game.

## Title and bounded history

The normalized title “Lylr-Meop” belongs only to active stem `lylr` among the
442 scanned `.zon` files ([title source](../../../areas/zon/lylr.zon#L2)).
Followed `lylr.wld` and `lylr.zon` histories reach the 2009 areas-tree move
and later same-stem edits, including the 2013 WLD update, 2014 bank change,
2015 quest-item flag pass, and 2016 bottle fix. The reviewed paths establish
no alternate-stem rename, distinct predecessor, or whole-zone replacement.
See the [duplicate and replacement review](DUPLICATE_AND_REPLACED_ZONES.md).

## Migration disposition

Keep active `lylr` DO NOT MOVE. Preserve both one-way Surface exits, the two
source-only Surf approaches, the Tharnadia counter dependency, the Heavens
healing items used by Lylr's active reset and shop, and the compiled service
bindings. Keep `surf` RETAIN IN PLACE and record both approaches in its
existing candidate row. No new candidate row is added. Editor,
administrative, installed-data, persisted-player, indirect-runtime, and
actual traversal closure remain outside this bounded scan; no gameplay,
migration, or source change is proposed.

## Checkpoint 269: direct builder and owner questions

These questions make the remaining source decisions explicit. The
recommendations preserve current ownership and exact native behavior unless a
separate, approved design selects a concrete repair.

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Should Venmar's scalp-for-boots exchange be the only native receipt, or should homecoming become a later accepted outcome? | [native exchange](../../../areas/qst/lylr.qst#L1), [receipt limits](../zone-stories/LYLR_MEOP.md#native-exchange-source-and-retirement) | Keep the exact scalp/boots receipt; define a separate return-home event only if a real recipient, arrival, and saved result are selected. |
| 2 | Should the one Venmir spelling in Venmar's description be corrected? | [Venmar prototype](../../../areas/mob/lylr.mob#L14), [wording note](../zone-stories/LYLR_MEOP.md#native-exchange-source-and-retirement) | Confirm the intended spelling and use a separate cosmetic fix that preserves name keywords and the single M2 response. |
| 3 | Should scalp #82217 remain hidden and non-rentable, and what fair clue should lead players to its tree source? | [scalp flags](../../../areas/obj/lylr.obj#L259), [tree route](../zone-stories/LYLR_MEOP.md#actual-approaches-hidden-tree-and-prison) | Keep the authored flags; select and qualify a clue/search path before claiming ordinary discovery or changing visibility. |
| 4 | Are the scalp's cap-one carried source and the same actor's 20-percent dagger equipment chance intended to share reset availability? | [carrier and source review](../zone-stories/LYLR_MEOP.md#native-exchange-source-and-retirement), [mobile prototype](../../../areas/mob/lylr.mob#L28) | Preserve exact caps and probabilities until admitted resets establish the intended scalp and dagger supply. |
| 5 | Should zone mode 2, global caps, and actor retirement control the potential daily renewal for this exchange? | [reset header](../../../areas/zon/lylr.zon#L1), [renewal review](../zone-stories/LYLR_MEOP.md#accounting-and-qualification) | Decide actor/source generations and READY-accounting renewal before describing the exchange as reliably daily. |
| 6 | Which selected-world entrance is intended when both local cross-area exits point outward and neither has an active reverse entry? | [first boundary](../../../areas/wld/lylr.wld#L1), [second boundary](../../../areas/wld/lylr.wld#L761), [cross-area summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L258) | Qualify actual alternate arrivals first; if ordinary access is intended, choose a fair route with a return path in a separate reviewable change. |
| 7 | Are the forest route and Meopham route meant to remain separate 40-room and 24-room WLD components? | [directed graph review](#active-wld-boundaries-and-directed-reach), [component analysis](../zone-stories/LYLR_MEOP.md#actual-approaches-hidden-tree-and-prison) | Preserve the authored components until builders identify a concrete connection and its intended owner. |
| 8 | Should the hidden tree stay closed and secret, with the upper platform reachable only through the authored fall route? | [tree and fall review](../zone-stories/LYLR_MEOP.md#actual-approaches-hidden-tree-and-prison), [tree room](../../../areas/wld/lylr.wld#L174) | Keep reset flags and movement restrictions; qualify discovery, falling, arrival, and survival separately. |
| 9 | Is the 20-percent fall chance from platform #82210 meant to sample before the attempted command and suppress that command on dispatch? | [falling behavior](../zone-stories/LYLR_MEOP.md#actual-approaches-hidden-tree-and-prison) | Confirm the intended challenge and policy; retain actual flight, mount, trust, movement, impact, and death behavior unless an observed defect justifies a balanced repair. |
| 10 | Is prison door #82238 north intentionally unkeyed while the opposite #82232 south exit names rusty key #82223? | [door/key source](../zone-stories/LYLR_MEOP.md#actual-approaches-hidden-tree-and-prison), [rusty key](../../../areas/obj/lylr.obj#L353) | Preserve the asymmetric source; qualify both reset directions because successful unlock mirrors reciprocal state. |
| 11 | What counts as a real rescue for the five distinct prison captives? | [prisoner and restraint review](../zone-stories/LYLR_MEOP.md#actual-approaches-hidden-tree-and-prison), [guard source](../../../areas/mob/lylr.mob#L318) | Define consent, restraint release, legal destination, surviving movement, beneficiary, and saved freedom before adding rescue credit. |
| 12 | What effect and charge policy should chest #82215 use when its configured damage value is 15 and the current trap dispatcher supports only 0–8? | [vault trap source](../../../areas/obj/lylr.obj#L232), [trap review](../zone-stories/LYLR_MEOP.md#vault-services-effects-and-wider-stories) | Keep the chest locked and trapped; have the builder choose the intended effect and harm, then review a focused before/after separately. |
| 13 | Should Cionvra's one shop keep its local stock and three imported Heavens bandages, and what purchase behavior is intended under active authority? | [Cionvra stock](../../../areas/shp/lylr.shp#L1), [import resets](../../../areas/zon/lylr.zon#L125) | Preserve Heavens as the item owner; qualify actual stock, price, currency, item delivery, and save before advertising an active purchase. |
| 14 | Are the bandage prototypes meant to stay owned by Heavens while Lylr only resets and sells them? | [reset owner boundary](../../../areas/zon/lylr.zon#L125), [typed dependency review](#exact-typed-and-compiled-dependencies) | Keep the imported VNUMs and source owner distinct; do not copy them into Lylr or claim Lylr authored the supply. |
| 15 | Should Hylga's drink-service prose remain an integration gap while her compiled binding is world_quest and she has no local shop? | [Hylga binding](../../../src/specs/specs.assign.c#L778), [mobile](../../../areas/mob/lylr.mob#L400), [service analysis](../zone-stories/LYLR_MEOP.md#vault-services-effects-and-wider-stories) | Confirm whether a separate fair stock/pricing/hours design is wanted; do not infer a working local shop from dialogue. |
| 16 | Should Sakthris's money-changer command continue redirecting LIST/EXCHANGE to the Royal Bank without locally exchanging coins? | [compiled service](../../../src/specs/specs.assign.c#L718), [service analysis](../zone-stories/LYLR_MEOP.md#vault-services-effects-and-wider-stories) | Preserve the current redirect and zero-surcharge boundary; describe an exchange only after a real currency transaction is defined. |
| 17 | Should foreign Tharnadia counter #132581 remain the Lylr room prop and compiled storage hook? | [reset import](../../../areas/zon/lylr.zon#L82), [foreign prototype](../../../areas/obj/tharnadia.obj#L1373), [binding](../../../src/specs/specs.assign.c#L1307) | Keep Tharnadia as the prototype owner and qualify actual locker admission, fee, arrival, and storage independently from counter text. |
| 18 | Should the inn remain available through its explicit compiled room binding even though the room lacks raw INN/SAFE flags? | [inn binding](../../../src/specs/specs.assign.c#L2307), [inn analysis](../zone-stories/LYLR_MEOP.md#vault-services-effects-and-wider-stories) | Preserve the explicit binding; qualify admission, rent, terminal save, and home update without promising safety from prose. |
| 19 | Which actual use and target rules are intended for the earthquake staff, cyan potion, and other authored supplies? | [staff prototype](../../../areas/obj/lylr.obj#L323), [supply effects](../zone-stories/LYLR_MEOP.md#vault-services-effects-and-wider-stories) | Keep the source effects and restrictions; qualify actor, recipient, damage, consumption, and save before making a safety or care claim. |
| 20 | Which town, family, forest, and care stories should gain a concrete beneficiary and lasting result? | [wider-story boundaries](../zone-stories/LYLR_MEOP.md#vault-services-effects-and-wider-stories), [owned follow-ups](../zone-stories/LYLR_MEOP.md#owned-integration-and-fair-repair-follow-ups) | Let builders select specific consent-based contribution and welfare endpoints; keep cleanup, visits, kills, restraint prose, and separate prototypes outside completion credit until then. |
