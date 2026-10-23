# Fateful Falls (`beggin`): source review and builder questions

The coverage register maps `beggin` as an unregistered current-source component
with zone header #33, two rooms, two mobiles, one object, and one local MA
response. It is absent from the full-area and QST-area manifests, has no story
sidecar, and defines no Q/QA quest contract. See the [coverage row](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L366)
and [candidate dependency row](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L15).

## Source map and limits

The two WLD records form a reciprocal east-west road fragment: #3300 leads east
to #3301, which returns west to #3300. Room #3300 describes the aftermath of
battles against evil races. Room #3301 describes neglected fields, says the road
continues toward a village, and has an extra description saying the field may
yield vegetables. The current local WLD has no further exit east from #3301.
See the [room pair](../../../areas/wld/beggin.wld#L1) and
[field description](../../../areas/wld/beggin.wld#L31).

The zone resets the Tome of Command in #3300, the village girl in #3300, and the
black cat in #3301. The girl has one MA default response telling players to look
at the book; there are no local Q or QA exchanges. The cat's tag says to talk
with its master, Zornade. A text search across `areas/`, `docs/`, and `src/`
found that name only in this tag, and the component defines no Zornade mobile
or local conversation.
See the [resets](../../../areas/zon/beggin.zon#L9),
[girl response](../../../areas/qst/beggin.qst#L1),
[mobile prototypes](../../../areas/mob/beggin.mob#L1), and
[cat tag](../../../areas/wld/beggin.wld#L37).

The Tome gives a short newcomer guide: basic commands, navigation, equipment,
communication, grouping, following, movement, and help. Its pages also describe
a Good/Evil race war. The component contains no harvest object or reward tied to
the field description. See the [Tome prototype](../../../areas/obj/beggin.obj#L1)
and its [pages](../../../areas/obj/beggin.obj#L21).

Checkpoint 877's [typed-source audit](../quest-story-followups/FATEFUL_FALLS_AUDIT_REVIEW.md#inbound-and-outbound-typed-reference-closure) completes the bounded current-source dependency scan. The two-room source has no active or source-only external WLD approach, same-type prototype duplicate, reset/QST/SHP/sidecar consumer, portal/switch destination, or semantically typed compiled VNUM binding. The raw C/C++ numeric matches are unrelated table/category constants. Retain the source while editor/tool references, generated/deployed state, persisted identity, and owner intent remain unqualified. This does not prove the fragment is loaded or reachable through every code path, and grants no migration clearance.

## Open area-owner questions and conservative recommendations

- **FATEFUL-FALLS-Q01 — Is the girl's MA response only an introductory prompt
  to read the Tome, or was a completable interaction intended?** The local QST
  defines a default message directing players to the book but no Q/QA exchange.
  **Conservative recommendation:** describe it as a tutorial prompt and do not
  imply a reward, quest completion, or separate girl interaction. See the
  [MA response](../../../areas/qst/beggin.qst#L1),
  [girl reset](../../../areas/zon/beggin.zon#L11), and
  [Tome reset](../../../areas/zon/beggin.zon#L9).

- **FATEFUL-FALLS-Q02 — Does the cat's tag refer to a specific, currently
  reachable master named Zornade, and if so, where is that contact defined?**
  The tag is the repository's only `Zornade` mention; this component has no such
  mobile or local dialogue. **Conservative recommendation:** retain the tag as
  unresolved source text until the owner identifies the intended contact; do
  not invent a route, identity, or quest. See the
  [tag description](../../../areas/wld/beggin.wld#L37),
  [cat prototype](../../../areas/mob/beggin.mob#L1), and
  [zone resets](../../../areas/zon/beggin.zon#L9).

- **FATEFUL-FALLS-Q03 — Does the field description promise a player action to
  gather corn or vegetables, or is it environmental flavor?** The extra
  description says the field may yield produce, but these local files define no
  harvest item, reset, or quest reward. **Conservative recommendation:** treat
  the text as scenery unless the owner identifies a supported action and
  outcome; do not promise item acquisition. See the
  [field text](../../../areas/wld/beggin.wld#L31),
  [local object file](../../../areas/obj/beggin.obj), and
  [local resets](../../../areas/zon/beggin.zon#L9).

- **FATEFUL-FALLS-Q04 — Is the eastward village in the road description meant
  to be an actual continuation of this route, or only off-screen setting?** The
  only local exits connect #3300 and #3301; the candidate row records no active
  inbound WLD exits, and the component is not in the full-area list.
  **Conservative recommendation:** describe only the authored local fragment;
  do not claim access to a village or an active travel route. See the
  [outbound road text](../../../areas/wld/beggin.wld#L9),
  [return room](../../../areas/wld/beggin.wld#L17), and
  [candidate dependency row](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L15).

- **FATEFUL-FALLS-Q05 — Is the Tome still intended as an authoritative current
  guide for new players, including its command examples and Good/Evil race-war
  framing?** Its pages present broad advice but the source text alone does not
  establish current command behavior or a live conflict. **Conservative
  recommendation:** preserve the book's text as authored guidance and do not
  present it as a verified statement of current mechanics or timeline. See the
  [Tome introduction](../../../areas/obj/beggin.obj#L9),
  [command pages](../../../areas/obj/beggin.obj#L21), and
  [war-scarred road](../../../areas/wld/beggin.wld#L1).

- **FATEFUL-FALLS-Q06 — Is the battle-scarred road intended as historical
  aftermath, or should it connect to an active conflict story?** The room
  describes old battle damage but defines no local conflict objective or
  outcome. **Conservative recommendation:** treat the battle as environmental
  history; do not infer an active campaign, alignment choice, or combat-related
  reward. See the [road description](../../../areas/wld/beggin.wld#L1),
  [zone resets](../../../areas/zon/beggin.zon#L9), and
  [local QST](../../../areas/qst/beggin.qst#L1).

## Current disposition

Retain the component files in place. The source review does not resolve exact
typed dependencies, editor/tool references, duplicate identity, or any runtime
reachability beyond the recorded static exits. Keep those closure items open
before considering a separately approved migration.
