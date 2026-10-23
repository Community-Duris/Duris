# `tiamats_lair` legacy-source audit

Canonical key: `component:tiamats_lair`. Coverage data row 456 / CSV line 457;
candidate dependency data row 105 / CSV line 106. Reviewed October 12, 2026,
Checkpoint 543, against checkout
`c500ac51b8cd26540943257e7ca4e9807263ef74`. This is a static source,
identity, graph, and dependency review. It does not verify a generated QST
aggregate or deployed runtime state.

## Replacement history and the two manifests

The current [areas/AREA list](../../../areas/AREA#L388) selects `azhural`
(zone #1352) and excludes `tiamats_lair`. The normalized title `The Lair of
Tiamat` matches the active Azhural ZON title. Commit
`50b400005d0bfc19adb51cda0d74414e940a7b51` (2012-08-07), titled “Swapping
zone files for the new version of Tiamat's Lair,” explicitly comments the old
`tiamats_lair` entry out of `areas/AREA` and adds `azhural` as its replacement.
This is explicit world-source replacement evidence.

The current [areas/qst/AREA list](../../../areas/qst/AREA#L339) still selects
`tiamats_lair.qst`, while it does not select `azhural.qst`. The QST builder
reads that list and selects each named file
([manifest read](../../../areas/src/qst/make_qst.c#L6),
[file selection](../../../areas/src/qst/make_qst.c#L52)); the server reads the
generated `areas/world.qst` aggregate
([runtime input](../../../src/world/quest.c#L79)). The two source QST files
are byte-for-byte identical 97-line files with SHA-256
`5be007ebd8ae737d0647405d23ff58339f222986c29b8ce4541269ace2ead41b` in this
checkout. Thus the legacy QST path is still a current build-input dependency,
though the combined artifact and deployment state were not checked.

The two selected contracts are givers #135219 and #135220. Their text
references collecting eight bone shards for a key to the lair, then collecting
the five dragon-flight essences for passage toward the Dragonspawn Archway
([first contract](../../../areas/qst/tiamats_lair.qst#L1),
[second contract](../../../areas/qst/tiamats_lair.qst#L53)). These are the
same contracts present in `azhural.qst` today. The legacy source has no
separate story sidecar or map JSON; active Azhural has the current
[story sidecar](../../../areas/story/azhural.story.json#L1).

## Current record identity

| Type | Candidate inventory | Current Azhural comparison |
| --- | --- | --- |
| WLD | 83 rooms, #135201–#135283 | All 83 room IDs overlap. 41 complete room bodies match and 42 differ. |
| MOB | 20 records, #135201–#135220 | All 20 IDs overlap. Five complete records match and 15 differ. |
| OBJ | 38 records, #135201–#135238 | All 38 complete object bodies match the current Azhural records. |
| QST | Two contracts, givers #135219/#135220 | Entire 97-line file matches `azhural.qst` byte-for-byte; only the legacy filename is listed by qst/AREA. |
| ZON | Header #1352, reset mode 0 | Seventeen source reset commands: 16 M and one O. The current Azhural ZON has a separate, expanded 129-command reset set. |

These comparisons show shared current data, not a second live map. The source
replacement commit explicitly distinguishes the old and new world files.
Keep same-number records tied to their source files until the manifest and
source owners decide how to consolidate their paths.

## Legacy WLD graph

All 200 source exits resolve inside the candidate's 83-room set. The undirected
weak components contain 74, 8, and 1 rooms; a raw directed walk from #135201
reaches 74 rooms. There are no positive unresolved or negative destinations,
outbound cross-zone exits, fall/current records, or rooms outside header #1352's
room range. The full graph is in the
[legacy WLD](../../../areas/wld/tiamats_lair.wld#L1), with current Azhural
rooms in the [active WLD](../../../areas/wld/azhural.wld#L1).

All 83 legacy room numbers also name active Azhural rooms. The legacy map's
200 exits start in these same-number rooms and target within its own set, so
they describe the predecessor's internal graph. They do not create active
ingress to a second `tiamats_lair` identity. Candidate-specific active inbound
exit count remains zero.

## Reset and portal dependencies

The candidate ZON has 16 M resets that use local mobile IDs and local rooms,
plus one O reset for object #135208 in room #135201
([source resets](../../../areas/zon/tiamats_lair.zon#L9)). It does not reset
its two type-25 portal objects. Candidate objects #135230 and #135231 are
byte-for-byte the active Azhural portal prototypes: #135230 targets room
#135275 and #135231 targets #135202
([legacy declarations](../../../areas/obj/tiamats_lair.obj#L257),
[active declarations](../../../areas/obj/azhural.obj#L257)). Current
Azhural's separate ZON places those portals in rooms #135289 and #135282
([active placements](../../../areas/zon/azhural.zon#L82)). The old declarations
therefore remain source/map evidence; their presence in the legacy OBJ file
does not place them in the active world.

The replacement commit and current manifests support a split disposition:
keep Azhural as the selected world source, and retain the legacy files because
`qst/AREA` still names the old QST path. Reconcile that QST ownership with its
builder and deployment owner before any archive, rename, or manifest change.
No area/QST data was changed in this review. See
[AZHURAL_AUDIT_REVIEW.md](AZHURAL_AUDIT_REVIEW.md) for the broader current map,
portal, source-only approach, and typed-consumer analysis, and
[TIAMATS_LAIR_BUILDER_QUESTIONS.md](TIAMATS_LAIR_BUILDER_QUESTIONS.md) for the
twenty owner questions.

## Disposition

Keep the five `tiamats_lair` source files **RETAIN IN PLACE**. The WLD/MOB/OBJ/ZON
version is an explicit former version of Azhural, while the legacy QST path
remains selected by the current quest manifest. Preserve the identical QST
source until its owner reconciles the manifest; do not archive, rename, move,
or co-load the old source family based only on the world-list replacement.
