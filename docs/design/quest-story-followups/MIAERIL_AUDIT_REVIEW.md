# Miaeril Village: active connectivity, references, title history and retention

**Checkpoint 115 · 2026-10-09 · active source `mril`, zone 337.** This repository-wide supplement adds graph, inactive-map, typed-dependency, title/history, and retention evidence to the [comprehensive Miaeril Village story dossier](../zone-stories/MIAERIL_VILLAGE.md). It does not change gameplay or move source files.

Sources: [`areas/AREA`](../../../areas/AREA#L163), [`mril.wld`](../../../areas/wld/mril.wld#L1), [`mril.zon`](../../../areas/zon/mril.zon#L2), [`mril.qst`](../../../areas/qst/mril.qst#L1), [`mril.shp`](../../../areas/shp/mril.shp#L1), [active cross-area exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L453), [cross-area pair summaries](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L306), and the [coverage row](ZONE_COVERAGE_REGISTER.csv#L193).

## Active connectivity and directed source reach

The active `mril.wld` defines 65 rooms and 190 exits. All 190 targets resolve in the active WLD set; 188 exits stay within Miaeril and two leave it. The local undirected projection is one weak component of 65 rooms. Room-range mismatch and negative or unresolved targets are absent. The coverage row records two fall-chance rooms and six rooms with current fields.

Two local exits leave Miaeril for the same Surface room, #522257: village room #33717 south and west-trail room #33764 east. Only the second has a reverse WLD edge: Surface #522257 west returns to #33764. This gives two outbound records, one active inbound record, and one reverse pair; it does not make the #33717 route reciprocal. See [Miaeril outbound rows 453–454](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L453), [Surface inbound row 1365](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1365), [Miaeril-to-Surface pair row 306](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L306), and [Surface-to-Miaeril pair row 554](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L554).

An authored local directed traversal from either entrance candidate, #33700 or #33764, reaches 63/65 rooms when the concealed stump route is unavailable; #33742 and #33743 are the omitted rooms. Reverse traversal shows an authored path from all 65 rooms back to the tested roots. When every authored exit is treated as available, both starts reach all 65. The relevant stump exits are #33741 down to #33742 (D5, closed/secret) and #33742 down to #33743; the room and fall descriptions also mark #33742 and #33744 with fall chance. These are source-graph counts, not a simulation of SEARCH/open success, keys, currents, falls, actor movement, resets, mounts, permissions, or safe travel. See [`mril.wld` stump rooms](../../../areas/wld/mril.wld#L1083) and the [Miaeril dossier's access and hazard analysis](../zone-stories/MIAERIL_VILLAGE.md).

## Inactive-map sweep

All 95 WLD files outside the active `areas/AREA` contributor set were scanned: 527,791 rooms and 2,080,773 exits, with no parse issues. None has a source-room VNUM overlap with Miaeril's 65 rooms, and none has an exit into a Miaeril room. The curated source-only approach total and reviewed Surf subset therefore remain unchanged at 1,529 and 140 respectively. This negative finding is specific to inactive WLD exits; it does not qualify code, transport, staff, or runtime arrival paths. Keep `surf` and `surface2011` **RETAIN** while their broader map provenance and dependency reviews remain open.

## Typed references and dependency closure

The bounded typed scan found one real outside material/reset lead and one unrelated numeric collision:

| Reference | Source record | Classification |
| --- | --- | --- |
| Inbound material through an active reset | [`mril.zon:92`](../../../areas/zon/mril.zon#L92) loads object #31317 at Gorlon #33743. [`dream.obj:242`](../../../areas/obj/dream.obj#L242) defines the skull crystal, and [`dream.qst:53`](../../../areas/qst/dream.qst#L53) uses it as one item in Qin's Q46 exchange for #31315. | A real imported Dream object and cross-zone recipe lead. It is a reset import; Miaeril's three local QA exchanges use only local materials: two accept #33710 frozen hearts, and Falga's accepts five #33709 salmon ([local QST](../../../areas/qst/mril.qst#L1)). The full Dream campaign remains bounded context in the existing dossier, not a new comprehensive review of Dream. |
| External QST numeric match | [`wh.qst:2629`](../../../areas/qst/wh.qst#L2629) has `R I 33719`; [`wh.story.json:1104`](../../../areas/story/wh.story.json#L1104) records a completion key receiving item #33719. | No object prototype #33719 was found anywhere under `areas/obj`; in Miaeril, #33719 is a room ([WLD](../../../areas/wld/mril.wld#L485)) and mobile ([MOB](../../../areas/mob/mril.mob#L291)). The Winterhaven story source itself describes the referenced item prototype as missing. Classify this as an unresolved Winterhaven reference and numeric-namespace collision, not an inbound Miaeril object dependency. |
| Direct source-code numbers | [`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L100) contains Miaeril object numbers #33701, #33702, #33703, #33705, and #33711 in Chaos starter-kit data. | These are typed starter-equipment grants, not native quest inputs or evidence of a Miaeril source episode. The existing story dossier distinguishes possession granted by `starter_grant` from quest completion. |

The bounded scans found no outside active QST item command targeting a local Miaeril object, no external SHP product, no external ZON reset role consuming a local Miaeril prototype, no external MOB/OBJ duplicate for its local mobiles or objects, no typed external story-sidecar `item_vnums` reference, and no target into a Miaeril room among 872 active type-25 declarations. All 231 external story sidecars parsed; the single raw completion-key number hit is the #33719 Winterhaven collision classified above, not a Miaeril item dependency. Local reset line 92 is the only local reset-role reference outside Miaeril prototypes or its room range; no local QST item command or typed sidecar `item_vnums` points outside its local object set. These are direct static scans, not exhaustive runtime, editor/tool, indirect-code, or persisted-player closure.

Miaeril has one shop header, #33714, with products #33708, #33706, and #33707 ([`mril.shp`](../../../areas/shp/mril.shp#L1)); the coverage register now records this shop rather than a zero count. Product and keeper stock are described in the existing dossier. No shop product points outside the local object set.

## Title, duplicate, and replacement history

After Duris color-code removal and normalization, the `.zon` title “Miaeril Village” has one exact owner among 351 non-empty active titles: `mril`. The title scanner found no other active lexical candidate containing “Miaeril.” The followed `mril.wld`, `.zon`, `.mob`, `.obj`, `.qst`, and `.shp` paths remain under the same stem after the 2009 repository-wide `areas/` move (`642da5400`). Their history contains same-stem edits through 2011, 2013, 2014 shop cleanup (`1c59fb690`), a 2015 quest-item correction (`891988052`), and 2025–2026 area updates (`92fa3eb09`, `5a5e92097`, `dc5ca2773`, `428ebfddf`); it contains no alternate-stem rename or explicit predecessor/replacement claim. The story sidecar was added on 2026-10-06 (`82e31e6aa`). The current title and bounded followed history do not prove that no untracked predecessor ever existed.

Preserve active `mril` as its own current identity. There is no evidence-based replacement or merge disposition. Keep it **DO NOT MOVE** as a current, un-commented `areas/AREA` contributor. The source-only `surf` and `surface2011` records remain **RETAIN** pending map ownership and their independent dependency/tool closure.

## Scope and remaining qualification

The [coverage row](ZONE_COVERAGE_REGISTER.csv#L193) now reconciles this checkpoint's graph counts, shop count, title/history result, typed references, and disposition. It links this supplement and the existing source-comprehensive dossier. Gameplay qualification, dynamic arrival, actual search/door/travel outcomes, route safety, reset/shop availability, tooling/editor references, indirect code paths, and persisted-player references remain outside this static checkpoint. No source, area placement, or gameplay behavior changed.

## Checkpoint 295: all-442 title and direct builder questions

A normalized scan of all 442 current .zon files gives “Miaeril Village” one
exact owner: active stem mril. The [comprehensive dossier](../zone-stories/MIAERIL_VILLAGE.md)
and [Checkpoint 115 supplement](MIAERIL_AUDIT_REVIEW.md) preserve its two
outbound Surface edges, concealed stump route, three local exchanges, imported
Dream object, Winterhaven numeric collision, and bounded same-stem history.
These twenty questions make the remaining owner choices and gameplay evidence
explicit without changing any native recipe or route.

1. **Question:** Should active stem mril remain the exact normalized title
   owner for “Miaeril Village”?
   **Recommendation:** Keep the unique title match among all 442 current
   .zon files. Resolve command-target aliases and broader semantic identity
   separately; do not treat similar snowy-village themes as proof of a merge.

2. **Question:** Should the two Miaeril exits to Surface room #522257 keep
   their current asymmetric ownership?
   **Recommendation:** Preserve village #33717 south as one-way and west trail
   #33764 east with Surface #522257 west as the sole reciprocal pair. Do not
   describe the village exit as having a return edge.

3. **Question:** Should the secret stump route remain optional for reaching
   rooms #33742 and #33743?
   **Recommendation:** Preserve the source graph: both entrance roots reach
   63/65 rooms without the concealed route and all 65 when its exits are
   available. Keep search, secret opening, fall admission, actual arrival,
   and safe return as separate qualifications.

4. **Question:** Should Hooded Figure #33700 and Uduku #33701 remain separate
   exchanges even though both accept one frozen heart #33710?
   **Recommendation:** Preserve the exact distinct cloak rewards and
   independent accepted histories. One current heart prepares either
   exchange; it cannot pay both, and no branch exclusivity or earlier
   completion prerequisite is authored.

5. **Question:** Should the cap-one frost giant source remain chance-limited
   rather than guaranteed per player or reset?
   **Recommendation:** Preserve mobile #33720’s cap and spawn chance and the
   separate cap-one heart object source. The line “stopped for good” does not
   establish global retirement, permanent safety, or a live encounter.

6. **Question:** Should successful SEARCH, committed heart recovery, supplied
   proof, and accepted exchange remain distinct events?
   **Recommendation:** Preserve the heart’s SECRET visibility behavior and
   exact local recipe. Qualify actor, source, reveal, custody, and accepted
   GIVE separately; a supplied copy remains valid without claiming a personal
   frost-giant kill or first recovery.

7. **Question:** Should Falga’s recipe continue to require five salmon
   #33709 at once without requiring a personal catch?
   **Recommendation:** Preserve the exact quantity and accepted book reward.
   The six G reset placements define possible stock, not guaranteed
   replenishment; generic fishing, pet feeding, or owning one fish does not
   prove the five-item exchange.

8. **Question:** Should wul #33710’s follower relationship remain separate
   from Falga’s fish exchange and any player-feeding story?
   **Recommendation:** Preserve the native recipe, which does not check
   whether the pet eats or survives. Add a separate actor, food, recipient,
   effect, and recovery authority only if the builder intends a pet-care
   objective.

9. **Question:** Is the lost book #33711 intended to remain a type-33
   spellbook with no encoded spell list in its reviewed extra description?
   **Recommendation:** Confirm whether its blank useful contents are
   intentional. Do not promise spell #300 or ancient spells learned from the
   reward’s name; any content repair should preserve item identity and recipe.

10. **Question:** Should twenty addressed aliases continue to map to three
    response families rather than twenty achievements?
    **Recommendation:** Preserve the grouped responses and actor-specific
    match. Repeated aliases or room echoes must not create extra encounters,
    and no ambient qc_action scene is established.

11. **Question:** Should Gordo #33714’s shop remain distinct from quest
    dialogue about supplies?
    **Recommendation:** Preserve the single shop and its three locally
    produced goods. Qualify keeper/open state, current stock, price, debit,
    grant, and recovery before describing a completed purchase; dialogue
    prose does not create an MA service keyword.

12. **Question:** Should imported Dream skull #31317 at Gorlon #33743 remain
    a reset import that can supply Qin’s separate Q46?
    **Recommendation:** Preserve the typed source link and exact item
    identity. Gorlon’s stock or a Miaeril encounter does not complete a local
    Miaeril card or prove the foreign transaction.

13. **Question:** Should Qin’s five-skull Q46 remain independent from Dream’s
    four-shard Q27?
    **Recommendation:** Preserve the reviewed separate native recipes and
    exact five-skull multiset. Dialogue ordering does not add a Q27
    prerequisite; Dream campaign stages and receipts remain a separately
    scoped owner decision.

14. **Question:** Should Winterhaven’s numeric #33719 completion-key
    reference remain classified as unresolved rather than as a Miaeril item?
    **Recommendation:** Preserve the missing-object finding and namespace
    distinction: #33719 is a Miaeril room and mobile, with no object
    prototype anywhere under areas/obj. Do not add an item dependency or
    candidate route from that number collision.

15. **Question:** Should Chaos starter-kit grants of Miaeril equipment remain
    distinct from native source recovery and accepted quest history?
    **Recommendation:** Keep kit slot values as typed object consumers.
    Qualify kit selection, grant authority, recipient, and custody separately;
    starter equipment is not a quest receipt or proof of personal acquisition.

16. **Question:** Should the river current, waterfall, stump falls, and canopy
    routes remain environmental controls rather than automatic safe travel?
    **Recommendation:** Preserve source values and real movement conditions.
    Ask builders to state any intended safe route or rescue result explicitly;
    static reachability, descriptive prose, and an attempted move do not
    establish arrival or survival.

17. **Question:** Should Tarik’s scouting, Mordat’s lyre, and the nomadic
    family’s trap lore remain descriptive until a task is authored?
    **Recommendation:** Keep these contacts separate from the three existing
    item exchanges. Any help, music, escort, feeding, or trap outcome needs a
    named action, consenting recipient, useful effect, and durable record.

18. **Question:** Should the visible stable placeholder and inconsistent
    “fresh” versus “rotting” salmon wording receive a text-only repair?
    **Recommendation:** Confirm intended names and freshness meaning, then
    scope exact before/after wording separately. Preserve object IDs, reset
    stock, and recipe balance unless a distinct mechanic change is reviewed.

19. **Question:** Should mril remain DO NOT MOVE while Surf and
    surface2011 retain their separate map-owner questions?
    **Recommendation:** Preserve both current outbound edges and the single
    existing reciprocal pair. Keep Surf and surface2011 RETAIN pending their
    own map provenance and tool/dependency review; no inactive WLD approach
    into Miaeril is identified.

20. **Question:** What evidence should qualify any new Miaeril story as
    READY, accepted, saved, and replay-safe?
    **Recommendation:** Require an actual source and actor, successful
    admitted event, exact recipient and outcome, durable receipt, and
    qualified recovery. Keep active READY accounting, reset renewal, current
    inventory, and projection claims separate from played transactions,
    safety, and persistence.

Checkpoint 295 confirms the normalized title among all 442 current .zon
files and adds twenty builder questions. It preserves the current route and
bounded typed/history findings and makes no claim of live availability,
completion, or persistence.