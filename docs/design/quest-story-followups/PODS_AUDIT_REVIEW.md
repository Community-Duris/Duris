# Pharr Valley Swamp: repository-wide connectivity and typed-closure supplement

This checkpoint supplements the comprehensive [Pharr Valley Swamp dossier](../zone-stories/PHARR_VALLEY_SWAMP.md). It reconciles the active WLD boundary against every unregistered WLD input, checks typed consumers and code references, and records normalized title and tracked path history. These are static source findings; they do not establish live access or player history.

## Active WLD boundary and source-only scan

Pharr Valley Swamp has 754 of 754 loader-resolving exits across 375 rooms in one local weak component. Twelve rooms have fall fields and twelve have current fields. Its sole active cross-area boundary is reciprocal: Pods #28500 west to Pharr Valley room #40389, and Pharr Valley #40389 east back to Pods #28500 ([Pods outbound row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L412), [Pharr Valley return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L552)). The keys `pods` and `pharrvly` identify distinct active areas.

The refreshed scan parsed all 95 unregistered WLD files: 527,791 source rooms and 2,080,773 exits, with zero parse issues and no source-room VNUM overlap with Pods. No unregistered WLD exit targets rooms #28500–#28874. Pharr adds no source-only route, so the curated source-only total remains 593 and the reviewed `surf.wld` subset remains 31.

## External typed references

The only external native QST consumer found is Surfacemini's QA33 recipe on the humble hermit #97901: it accepts Pharr headdress #28552 and necklace #28553 together and returns recipe #97903 ([QST exchange](../../../areas/qst/surfacemini.qst#L40)). Surfacemini's structured story sidecar records the same contract and both exact material VNUMs ([contract](../../../areas/story/surfacemini.story.json#L427), [necklace](../../../areas/story/surfacemini.story.json#L462), [headdress](../../../areas/story/surfacemini.story.json#L473)). This remains a foreign recipe; the Pharr dossier covers its independent ownership and the fact that supplied matching ornaments do not require personal Pharr craft receipts.

The active Mntcastl story sidecar also contains an optional pointer to that same hermit completion key, as an earlier route into the castle's granular-potion story ([Mntcastl sidecar](../../../areas/story/mntcastl.story.json#L360)). It is a second authored sidecar reference to the existing Surfacemini contract, not an additional native QST exchange. The all-231-sidecar parse had zero errors and five exact numeric-field matches, all in Surfacemini's foreign-ownership records.

One raw numeric match in Surface Keeps is not a prototype reference: `surfacekeeps.qst` uses `R C 28750` as a copper reward amount, and its sidecar repeats `receive=C:28750` ([QST amount](../../../areas/qst/surfacekeeps.qst#L392), [sidecar amount](../../../areas/story/surfacekeeps.story.json#L541)). The matching integer is a room VNUM, but the native field type is currency. No other external QST matched. No external SHP, ZON reset-role, MOB/OBJ prototype definition, type-25 portal destination, or direct C/C++ VNUM reference into Pods matched.

## Title and tracked history

The normalized title scan found one “Pharr Valley Swamp” title among 351 active area contributors. The current key is `pods`, scope `*285-288`, with the matching title in its ZON ([area entry](../../../areas/AREA#L139), [zone title](../../../areas/zon/pods.zon#L2)). The nearby `pharrvly` area is separate and titled “Pharr Valley”; it is not an exact title duplicate ([area entry](../../../areas/AREA#L192), [Pharr Valley title](../../../areas/zon/pharrvly.zon#L2)).

Tracked WLD/ZON history stays on the same `pods` paths from the 2009 area-tree move (`642da5400`) through the 2010 chest-key and quest-key updates (`d6cd9ea64`, `786e1bad3`), 2013 source edits (`71d731c5b`), 2015 item-flag updates (`f18992b8f`), and 2026 WLD ANSI cleanup (`b07177cd5`). It records no title rename or distinct predecessor. The duplicate/replacement register identifies no separate retired or replacement area for Pods ([history register](DUPLICATE_AND_REPLACED_ZONES.md)).

## Audit boundary

This review used repository source and static registers only. No tests, build, server/runtime, gameplay, database, migration, or operational script was run. The source-comprehensive queue is now **98/220 complete, 122 pending**; priority 99, The Citadel (`citadel`), is next. Source mapping remains separate from played source custody, paid-service settlement, reset issuance, room access, and return-safety qualification.
