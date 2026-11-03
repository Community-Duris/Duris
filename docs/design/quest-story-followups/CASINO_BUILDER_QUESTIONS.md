# Casino Builder Questions

Checkpoint 460 source-backed questions for the unregistered `casino` component. Recommendations preserve the current files while area, code, and data owners clarify source identity and intended behavior.

| ID | Question | Recommended resolution |
| --- | --- | --- |
| CZ-01 | Who owns the `casino.wld`, `.mob`, `.obj`, and `.zon` source family and any external editor or generator data? | Identify the source and tooling owners before changing the files, manifest membership, or paths. |
| CZ-02 | Is “CasinoZone” the intended player-facing title, and should a canonical area name or alias be recorded? | Confirm the title and identity with the original area owner; retain the raw ZON title until then. |
| CZ-03 | Is the Casino an intentionally unregistered source, a retired map, or a candidate for future activation? | Record the owner's intended status without treating absence from `areas/AREA` as a retirement decision. |
| CZ-04 | How does the Casino map relate to active Hall of the Ancients, which currently owns room VNUMs #77801–#77815 for different rooms? | Ask both owners to settle map and numeric ownership; keep the maps distinct until there is explicit evidence of a relationship. |
| CZ-05 | Should Hall remain the active owner of #77801–#77815, or is there an approved crosswalk for the Casino source? | Preserve Hall's current active identities unless the area and persisted-data owners approve a documented reassignment. |
| CZ-06 | Why do the Casino WLD records use zone field 778 while active Hall uses zone field 777 for the same numeric room range? | Confirm each source's intended editor and zone-numbering convention before any reindexing. |
| CZ-07 | Do the Casino's local exits intend to connect Casino rooms as authored, even though their target VNUMs resolve to Hall rooms in the active world? | Keep source graph findings separate from runtime access; have the map owner define the intended destination identities. |
| CZ-08 | Do Hall's 23 active exits into #77801–#77815 represent only Hall's existing map routes? | Confirm those exits against Hall's map; do not count them as Casino ingress without a source-identity crosswalk. |
| CZ-09 | Is Hall object #77714's destination value #77813 intended to reach Hall's waterfall room or a former Casino room identity? | Preserve the active Hall destination and ask its owner before interpreting it as a Casino reference. |
| CZ-10 | What persisted room, reset, and player-location data may depend on Hall's current #77801–#77815 assignments? | Have the data owner document identity constraints before considering VNUM reassignment or source activation. |
| CZ-11 | Should Casino reset commands continue importing object #45 into rooms #77803 and #77804? | Ask the area owner to confirm both placements and the intended prototype identity before changing the ZON. |
| CZ-12 | Is active Heavens object #45, described as a slot machine, the intended external prototype for those two Casino imports? | Record the Heavens object as a source dependency; do not assume matching names make the reset safe for Casino or Hall. |
| CZ-13 | How should the two external #45 placements be represented if Casino is ever loaded while active Hall owns the destination room numbers? | Resolve room identity, loader membership, and typed reset ownership together before proposing activation. |
| CZ-14 | The Casino entrance D commands have raw state values 2 and 0, while both comments describe the door as closed. Which initial states are intended? | Keep the comments and numeric values distinct until the area owner confirms the door contract. |
| CZ-15 | Should the two entrance-side door resets remain separate records with their current directions and keys? | Confirm the paired-door setup and intended open/close behavior with the builder; do not normalize the values by assumption. |
| CZ-16 | Are the roulette, wheel-of-fortune, slot-machine, and solitaire prototypes meant as decorative objects only? | Confirm whether any playable activity exists outside the reviewed WLD/MOB/OBJ/ZON files before describing or activating mechanics. |
| CZ-17 | What player-facing wager, odds, payout, and reward rules are intended for the high-roller prose? | Document the actual contract with its gameplay owner; do not infer working gambling from room text or fixture names. |
| CZ-18 | Is a Casino operator or attendant intentionally absent from the zero-byte MOB file? | Confirm the intended staffing and whether any separate active service implements the experience. |
| CZ-19 | Should the eight local-object reset commands and two external #45 imports remain at their current rooms? | Review every placement against the approved Casino/Hall room mapping and intended inventory before editing resets. |
| CZ-20 | Is the lack of a same-stem SHP, QST, and story sidecar intentional? | Confirm whether Casino is environmental source only or has a separate shop, quest, or narrative owner and contract. |
| CZ-21 | Is the smoky, tobacco-heavy casino atmosphere still intended for the area? | Obtain an owner decision on the environmental tone before any separate content revision. |
| CZ-22 | Does any tracked source history identify Casino as a predecessor, rename, or replacement for Hall or another area? | Keep current histories separate unless the owners can identify a specific rename or replacement record. |
| CZ-23 | Should the Casino source remain RETAIN IN PLACE while its numeric identity and reset dependencies are open? | Preserve the files and current loader membership pending source-owner, map, reset, tooling, and persisted-identity decisions. |
| CZ-24 | If a future change is approved, who owns the required source, manifest, editor, and runtime review? | Record the named owners and obtain separate authorization for any path move, manifest change, or gameplay implementation. |
