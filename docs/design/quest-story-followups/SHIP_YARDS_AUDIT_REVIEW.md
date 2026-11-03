# Ship Yards source audit

Checkpoint 361 completes the broader source review for physical coverage row 273, `area:shipy`. The existing [Ship Yards story dossier](../zone-stories/SHIP_YARDS.md) maps the local quests, services, room prose, reset stock, code callbacks, and remaining gameplay qualification. This supplement adds the active and inactive WLD reconciliation, exact active typed callers, title/history, and builder decisions. It keeps the player-facing `shipy` port separate from the `ship` runtime room pool.

## Static map and active boundary

The active source contains 229 rooms (#43100-#43328), 102 mobile prototypes, 46 objects, 124 QST blocks with 26 native Q/QA contracts, seven shops, and 719 ZON commands. The WLD has 606 exit directions: 570 resolve locally, 32 resolve to active areas, and four point only into unregistered WLD sources. There are no negative targets, range-mismatch rooms, fall markers, or current markers.

The local weak graph has twelve components sized 86, 64, 59, 10, 3, and seven singletons. The ten active inbounds are nine Surface directions and Malch #23760 up to #43320. Seven distinct local boundary rooms receive them: #43100, #43139, #43140, #43179, #43180, #43219, and #43320. The 32 active outbounds comprise 31 Surface directions and the reciprocal Malch passage at #43320 down to #23760. Nine Surface outbounds are reciprocal with Surface; the other 22 are outward directions without an active reverse. Exact loaded boundary records are in the [cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L584), the [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L438), and the [reverse Malch record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L385).

The raw directed graph treats every authored exit as traversable. From #43100 and #43139, raw reach and return are 59/229 each; from #43140 and #43320 they are 83/229 reachable and 86/229 returnable; from #43180 and #43219 they are 62/229 reachable and 64/229 returnable. These are static graph results, not reset-state, access, safe-return, or played-travel findings.

All 70 flagged exit directions match 70 local D resets at 100% chance: 68 state-1 directions, the reset-locked #43291 east passage at state 2 with key zero, and #43234 west at state 5 with key zero. Preserve reset evidence without treating those doors as open or claiming an available key. The four active exits pruned by normal-world resolution are #43120 west to #157323, #43225 west to #154877, #43228 south to #154978, and #43323 east to #278216; their rows are in the [unresolved-exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L211). The dossier identifies the targets as unregistered Duris3 and Surf/Surface2011 map content; no replacement coordinates are chosen here.

## Inactive map identity and approaches

All 95 inactive WLD files parsed without errors: 527,791 room records and 2,080,773 exit directions. No inactive source other than `aopal.wld` overlaps Ship Yards room headers. Aopal defines the same 28 room VNUMs, #43301-#43328, under Tharnadian Causeway prose. It has 62 directions into Ship Yards VNUMs; 58 originate from those overlapping source VNUMs and are duplicate-number map evidence, not distinct source-room approaches. Four originate in unique Aopal rooms and remain source-only: #43329 down to #43319, #43330 down to #43321, #43334 up to #43324, and #43342 north to #43324. Preserve these with the existing [Aopal candidate row 10](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10); do not infer Aopal is a Ship Yards replacement or load its duplicate room definitions.

Eight non-overlapping `surf.wld` rooms also point into the active port:

| Inactive source | Direction to Ship Yards |
| --- | --- |
| #204864 | south to #43219 |
| #205464 | north to #43180 |
| #228912 | east to #43139 |
| #230412 | west to #43323 |
| #239460 | south to #43100 |
| #239759 | east to #43139 |
| #278921 | east to #43179 |
| #278923 | west to #43140 |

These are source-only, not loaded ingress; retain them on the existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). The combined CP361 addition is twelve reviewed non-overlapping approaches: four Aopal and eight Surf. The curated total becomes 4,529 (Surf 278; ud_map2 subset 23; Underworld2 9). No new candidate row or migration clearance is created.

## Title, same-type identity, and tracked history

After stripping color codes and the terminator, trimming whitespace, and folding case, “The Ship Yards” is unique among all 442 current ZON titles and 351 active contributors. No same-type active mobile or object VNUM overlap was found. The only same-type room-header overlap is the 28-room inactive Aopal source described above; it does not establish a rename or replacement. Ship Yards and the runtime `ship` pool remain different active areas.

Followed shipy WLD history reaches the 2009-08-12 area-tree move and a 2015-06-01 map edit; MOB history reaches 2014-08-27; OBJ and ZON history reach 2015-10-13 and 2015-05-27; QST reaches 2014-08-27; SHP reaches 2015-05-27. The structured story sidecar was added separately in 2026. Bounded paths identify no alternate-stem Ship Yards predecessor or explicit whole-zone replacement. Keep active `shipy` DO NOT MOVE.

## Typed callers and prototype dependencies

Active foreign sources contain 30 exact QST item commands in six files that consume nine distinct Ship Yards objects. The source groups are Alatorin (#43101, #43125, #43138, #43141), Malch (#43131), Surface (#43140, #43142), Surface Mini (#43138), Tundra (#43137, #43138), and Valley of Crushk (#43143). Their structured story sidecars also contain exact typed references; Valley's shiv reference is documented in the [Valley audit](VALLEY_CRUSHK_AUDIT_REVIEW.md#typed-item-reset-and-shop-dependencies). Six active foreign sidecars were found to reference Ship Yards prototypes, including Surface Mini's mobile #43193. Preserve these exact dependencies and the port-owned receipts; supplied proofs do not establish where a player obtained them.

Active external ZON files have 23 mobile reset rows for ten Ship Yards mobile prototypes in Alatorin and Surface Keeps, and 79 object reset rows for eight Ship Yards objects across Alatorin, Caertannad, Surface, Surface Keeps, and Vehicles. Active Surface Keeps shop stock also lists Ship Yards objects #43120 and #43124 in shops #120007 and #120009. These imports are static source references, not proof of current availability or accessibility. Exact local catalog and reset roles are mapped in the [source dossier](../zone-stories/SHIP_YARDS.md).

In the other direction, Ship Yards ZON imports active foreign object stock from Verz1, Malch, Heavens, Torrhan, Gold Hall, Charing, Unique, Shadowclave, and Rftjngle. Its three local type-25 teleport objects use the statue and altar to reach local rooms #43276 and #43318, and the moonwell to reach Shaledrieth #54245. No active foreign type-25 object destination into a Ship Yards room was found. The complete exchange, six support services, seven shops, eight code assignments, fishing and teleport dispatch, shipwright/crew callbacks, and remaining transactional boundaries are covered in the existing story dossier; this static audit does not qualify their live operation.

## Builder questions and conservative recommendations

1. **Should the normalized title “The Ship Yards” remain the public title for this shared multi-port area?** Preserve it as a unique active title, and keep it distinct from `ship` runtime templates while the area owner decides any rename.
2. **Are the nineteen independent commissions intended to remain separate rather than form a required campaign?** Keep their accepted receipts independent; add ordering or shared gates only with an explicit episode design.
3. **Should Pol's refundable briefing continue to require no retained payment and no prerequisite role in the final lure trade?** Clarify its hint and note prose to match the source; preserve the currently accepted ten-material finale unless the owner approves a separate recipe change.
4. **Should Fishfetcher remain a lore name without a pole or entitlement output?** Correct the advertised reward text or author a separately costed, accepted product; do not infer an item grant from dialogue.
5. **Should the final lure commission accept five shells and five fire glands without Pol's earlier note?** Preserve the exact implemented proof and supplied-item validity; any new note prerequisite needs an available, durable source and a new receipt rule.
6. **Should Voshen's “fillet” clues describe the implemented six-pike delivery?** Align the hints and the absent fillet output to the exact six-fish contract; do not add a fillet object or cooking result implicitly.
7. **Which quantity is intended for Cairme's shiv recipe, four or six?** Keep the four accepted shivs and required ration proof until builders explicitly choose a compatible alternative, then align the prose and journal.
8. **Should the refunded briefing, two seafood purchases, and three totem recipes stay support services without achievement credit?** Keep them separate from quest receipts unless each service gains a committed, observable outcome.
9. **Are supplied exact proofs intentionally valid without a personal kill, catch, theft, voyage, or vendor purchase?** Preserve current turn-in semantics; require provenance only for a deliberately new objective with accepted source events.
10. **Should the named cross-port and foreign proof sources remain optional?** Preserve the exact local object contracts and list the known external source roles; do not add a mandatory visit or custody chain based only on source prose.
11. **What destinations were intended for the four unloaded exits to #157323, #154877, #154978, and #278216?** Keep their unresolved status and choose coordinates only after map-owner review of loaded topology and ecology.
12. **Is the loaded ocean branch from viper dispersal #43324 west to #507725 intentional?** Preserve its source destination and qualify actual mobile movement before making any safe or reliable source claim.
13. **Should all three loaded land approaches and the nine reciprocal Surface port directions remain?** Keep the recorded boundaries and return edges; do not treat one port encounter as revealing all shared-area actors.
14. **Are the Canderthal barracks lock (state 2, key zero) and hidden Cairme cellar passage (state 5, key zero) intended access controls?** Preserve both reset records pending an owner decision; do not invent a key, special, or garden bypass.
15. **Should the statue, storm altar, and moonwell continue to use ordinary type-25 teleport admission?** Keep their local/foreign destinations distinct and qualify teleport restrictions and walking returns before adding a journal event.
16. **Are reset rooms and mobile caps intended as initial encounter locations only?** Preserve the reset source and describe givers as leads; do not promise current presence, roaming, retirement, or availability.
17. **Should Mazi and Tazi's retired exchange remain only a referral to Royal Bank?** Keep the disabled procedure and bank counter prose separate from a new currency milestone unless an active, settled conversion is deliberately restored.
18. **How should fishing handle a catch when ownership grant is refused after the catch message and pole/XP changes?** Follow the shared fishing settlement plan: freeze the accepted draw, admit ownership/effects recoverably, then publish success once; preserve the current fish table and skill policy until separately reviewed.
19. **How should paid shipwright, crew, repair, reload, summon, and asset-sale operations respond when wallet or epic settlement is refused?** Follow the existing shared ship-service plan: refuse before mutation until the selected operation can settle atomically and recoverably; keep the services outside quest credit.
20. **Do the current cash, item, and capped XP outcomes express the intended reward balance and story terminal?** Preserve admitted continuations and actual reward forms; review asymmetric actor/companion caps and author any allegiance, fishing, sailing, acquisition, cure, siege, or arsenal state explicitly before crediting it.

These recommendations describe review decisions, not gameplay changes. Keep active `shipy` DO NOT MOVE, preserve its external consumers and both source-only candidate families, and do not promote any static route to live safe travel. See [coverage row 273](ZONE_COVERAGE_REGISTER.csv#L273), [connectivity checkpoint 361](WORLD_CONNECTIVITY.md#checkpoint-361-ship-yards-static-connectivity-and-inactive-map-identity), [title/history checkpoint 361](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-361-ship-yards-title-history-and-aopal-identity), [retention checkpoint 361](ATTIC_MIGRATION_PLAN.md#checkpoint-361-ship-yards-in-place-retention), and the [source dossier](../zone-stories/SHIP_YARDS.md).

## Checkpoint 779: Caertannad typed dependency

Active Caer Tannad has three object reset rows for Ship Yards object #43125. See [Caertannad typed audit](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references).
