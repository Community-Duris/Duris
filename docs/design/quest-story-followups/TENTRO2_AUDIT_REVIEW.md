# Tentro2 audit review

Checkpoint 382 completes the source-story, active-graph, typed-reference,
title/history, and builder-question review for active tentro2. Findings below
come from the current repository files and followed history. No runtime
journey or operational behavior was tested.

The [Tentro source dossier](../zone-stories/TENTRO.md) maps all 294 rooms,
115 local mobile prototypes, 83 local object prototypes, zone resets, key
placements, and the separate pool teleporter.

## Story and local source closure

The source combines a wildlife prairie, hostile Troglodyte cave society,
walled village, covert zoo/prison, and Salol farming cooperative. The room
prose includes a usually fatal Troglodyte virus, human raids, ritual killing
of princesses, smuggling, summoning, and creature surgery. These are
source-authored themes. The local area has no QST file or giver, no story
sidecar, and no SHP file, so it defines no local quest or shop-stock contract.

The stale [QST area list](../../../areas/qst/AREA#L233) names tentro2 even
though areas/qst/tentro2.qst is absent. External [Winterhaven QST text](../../../areas/qst/wh.qst#L1339)
mentions Tentro as one of several places associated with psychomia plants;
it supplies no local prototype reference or Tentro quest step. The 2009
commit af691ce1e, titled "completed a missing Tentro quest," changed only
tentro2.zon by equipping surgeon's mask #69016 on the deranged surgeon
#69075. It did not restore or create a QST file.

## Active connectivity and routes

The active WLD contains 294 rooms and 682/682 resolving authored directions:
678 internal and four outward. Its weak components contain 227 and 67 rooms;
the Salol farm is a separate component with its own Surface access. Five
active inbound records target four rooms. Four outward records have paired
active returns. Winterhaven #55559 up enters Tentro #69249, which has no
ordinary WLD exit back to #55559.

The two raw directed WLD traversals reach 250/294 rooms from inbound targets
and cover 291/294 in reverse from outbound sources. They ignore locks,
secret discovery, hazards, scripts, and item travel. Pool object #69001 is a
type-25 teleporter reset in #69038 that targets underwater room #69041 on
CMD_ENTER. It provides a source-defined item-travel route into the cave, not
a missing WLD exit.

## Typed dependencies, title, and history

Local resets import six ConnectorZones mobiles (#53612, #53619, #53639,
#53642) into prairie rooms. Tentro also places Verz1 object #3097 in its
bank. The object remains Verz1-owned and has a shared
storage_locker_obj_hook binding; its in-room sign describes deposit,
withdrawal, and balance. Active external ZON, QST, story-sidecar, SHP, and
maintained numeric code scans find no consumer of Tentro's local mobile or
object prototypes.

After terminator trimming, whitespace normalization, and case folding, "The
Mountain Village of Tentro" is unique among all 442 current ZON titles. Same-
type active WLD, MOB, and OBJ scans find no exact ID overlap; all 95 inactive
WLD sources also lack an exact Tentro room VNUM. Surface room #557288 is
titled "Tentro" but is not a direct edge into the active area. Followed
history identifies no alternate-stem predecessor or explicit replacement.

The one-way sewer entry is historically traceable: commit 0ac17d14c
(2010-08-05, "Fixed up some missing exits on surface map") removed the
southward WLD record from Tentro #69249 to Winterhaven #55559 while the
Winterhaven inbound remained. This establishes the change, not whether the
one-way design was intentional.

## Builder questions and conservative recommendations

1. **Winterhaven return:** Is the one-way up entry from Winterhaven #55559 to
   Tentro #69249 intentional? The reciprocal exit was removed in 2010. Restore
   a paired route only if the one-way sewer arrival is not intended.
2. **Village boundary:** Are the four current Surface approaches the intended
   public entrances to the walled village? Surface #557288 is labeled Tentro
   but has no direct edge into this area. Keep map label and actual route
   ownership explicit.
3. **Salol separation:** Should Salol remain a distinct weak WLD component
   with its own Surface approach, despite the common church and prose about
   Tentro's support? Preserve the current separation unless the builder
   intends a physical road connection.
4. **Pool arrival:** Is object #69001's enter-to-#69041 route the intended
   underwater access from the prairie pool? Document it as item travel and
   keep it distinct from the WLD census.
5. **Holding rooms:** Are rooms #69289-#69293 deliberately one-way
   mobile-holding or test spaces? Their text and exits do not establish
   player access or staff enforcement; label or route them according to
   their intended audience.
6. **Zoo premise:** Should the public Oddity Park and the hidden smuggling,
   prison, summoning, and surgery story remain this sharply contrasted?
   Preserve both layers if that contrast is intentional.
7. **Bank service:** Is the bank's deposit/withdraw/balance sign meant to
   describe a working bank? Its imported Verz1 object #3097 is bound to a
   shared storage-locker procedure. Confirm the intended service before
   changing ownership or code.
8. **Other named services:** Are the pub, greenhouse, bathhouse, and merchant
   services intended to remain prose/NPC-only, given the absence of a local
   SHP file? Add a local service source only if the builder wants configured
   transactions here.
9. **Quest registration:** Should the stale tentro2 row in areas/qst/AREA be
   backed by a QST file or removed from that list? The historical "missing
   quest" change only added the surgeon's mask reset; do not infer a quest
   from that commit title.
10. **Troglodyte virus:** Is the disease described at #69274 implemented by
    another system, or is the statement only lore? Keep the hazard in prose
    unless a verified source mechanic supports a stronger player-facing
    claim.
11. **Raids and captives:** Are the Troglodyte raids for humans and valuables
    meant as background faction behavior without a local quest or reward
    loop? Keep consequences and rewards unstated until a native contract is
    identified.
12. **Princess ritual:** Is the account of rival princesses being killed to
    appease a strange god intended as canonical Troglodyte religion? Confirm
    the lore before adding quest steps or changing the source descriptions.
13. **Throne key holder:** Should the chieftain's key #69079 stay on a warrior
    in #69078, beside the locked throne door, rather than with the chieftain
    in #69079? Preserve the current reset placement pending intent.
14. **Mayor and zoo access:** Is simple key #69040 inside desk #69025 in the
    mayor's office the intended lead into the zoo security route? Confirm
    whether the mayor's placement implies complicity or merely a puzzle.
15. **Zoo cells:** Should keeper #69060 at #69197 carry the blue cell key
    #69039 for the locked cells? Confirm intended access to the captives and
    any return route before changing this security chain.
16. **Trapdoor key:** Is devil's key #69041 on incubus #69067 at #69212 the
    intended lock source for the hidden trapdoor between #69209 and #69217?
    Keep the prose about concealment aligned with the WLD flags and reset.
17. **Key of Shadows:** Are the two reset copies of #69046 intentional for
    the three keyed doors at #69205/#69208, #69213/#69216, and #69217/#69218?
    Preserve both placements until the builder confirms the intended route.
18. **Secret flags:** Should both directions of the #69166/#69168 and
    #69177/#69178 pairs be secret? Should #69205/#69208 share the same
    secret-flag state? Align WLD flags only after intended discovery behavior
    is confirmed.
19. **Foreign ownership:** Are the six imported ConnectorZones wildlife
    placements and the Verz1-owned bank counter intentional dependencies?
    Keep their prototypes and generic shared binding with their owners.
20. **Park rewards and props:** Is surgeon's mask #69016 intended as a
    placement on the deranged surgeon #69075, with no local QST contract?
    The 2009 change added that reset alone; tie it to a quest only if a
    current native contract is identified.

No change to route, key, reward, service, or migration behavior is implied by
these questions. Keep active tentro2 in place while design intent is
confirmed.

## Checkpoint 837: Current reset, bank, and pool routes rechecked

A current typed-source pass against the unchanged pinned sources confirms the existing Tentro ownership map:

- **Imported mobiles:** Six active `M` rows in [`tentro2.zon`](../../../areas/zon/tentro2.zon#L585) import ConnectorZones mobiles #53612, #53619, #53639, and #53642. Tentro is the reset owner for these placements; their prototypes remain ConnectorZones-owned.
- **Imported bank object:** Tentro resets Verz1 OBJ #3097 at bank room #69149 ([ZON](../../../areas/zon/tentro2.zon#L393)); the prototype remains in [`verz1.obj`](../../../areas/obj/verz1.obj#L1199), and maintained code binds #3097 to `storage_locker_obj_hook` ([assignment](../../../src/specs/specs.assign.c#L1308)). Preserve this shared storage-service binding and object owner.
- **Pool item route:** Local OBJ #69001 is type 25 and targets room #69041 on entry ([object](../../../areas/obj/tentro2.obj#L14)); Tentro places it in room #69038 ([reset](../../../areas/zon/tentro2.zon#L386)). This is a typed item route separate from the WLD graph. Numeric #69001 is also a local prairie-dog MOB placed in prairie rooms ([MOB](../../../areas/mob/tentro2.mob#L17), [resets](../../../areas/zon/tentro2.zon#L427)); keep the MOB and portal OBJ identities distinct.
- **Negative closure and source-only routes:** No active outside typed consumer of Tentro's local prototypes, local QST file, story sidecar, or SHP source was found, and no exact maintained C++ binding to a Tentro-local VNUM was found. This is separate from Verz1 object #3097's shared storage hook. Four inactive Surf approaches remain source-only on existing candidate row 97. Winterhaven's Tentro mention and the stale QST index remain context, not an added local contract.

Keep active Tentro2 **DO NOT MOVE**. This refresh adds no active WLD edge, candidate approach, gameplay change, or migration clearance.

