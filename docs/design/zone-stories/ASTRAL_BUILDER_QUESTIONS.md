# The Horrible Tracts of the Unnamed: builder questions

Canonical key: `component:astral`. Physical coverage row 362. The
[source dossier](ASTRAL.md) and
[technical audit](../quest-story-followups/ASTRAL_AUDIT_REVIEW.md) record the
empty placeholder, reserved room number, loader status, and tracked editor
metadata. These recommendations do not establish that the source is loaded.

## Explicit builder questions and recommended resolutions

1. **What purpose does the one-room `astral` source serve?** Its only room is
   “Unnamed,” with a blank description and no exits or reset content.
   **Recommend** record the intended purpose before changing, moving, or
   archiving any of its files.
2. **Is `astral` intentionally absent from `areas/AREA` and
   `areas/qst/AREA`?** Both manifests list the distinct `astral_main`,
   `astral_areas`, and `astral_tiamat` stems instead. **Recommend** keep those
   source identities separate and ask the map owner to confirm loader intent.
3. **Is “The Horrible Tracts of the Unnamed” the intended title?** Four other
   current ZON sources use the same normalized title. **Recommend** retain the
   authored string until its owner identifies any preferred title or alias.
4. **Who owns room VNUM #1 in this source?** The code names #1 as
   `ROOM_LIMBO_VNUM`, and active `limbo.wld` is its only active WLD owner.
   **Recommend** do not co-load Astral room #1 or change its path until the
   owner resolves the number collision.
5. **Are the byte-identical #1 placeholders in `mtncastl` and `ud-inside`
   intentional copies?** They share the blank WLD block, but are separate
   file stems. **Recommend** preserve each map identity unless tracked or
   owner-provided evidence establishes a merge or replacement.
6. **Does `areas/map/astral.json` belong to a current editor project?** It
   stores `zoneId: astral` and a coordinate for room #1, and its tracked
   history starts in 2025. **Recommend** ask the editor owner whether it is
   current, stale, or required by an external tool before touching it.
7. **Should this component contain any narrative or room description?** No
   prose appears in the room, and the available mobiles, objects, quests,
   shops, and reset commands are absent. **Recommend** treat the source as an
   empty placeholder; do not invent an Astral story from the title.
8. **Is any relationship intended between this placeholder and active Astral
   Main, Astral Areas, or Astral Tiamat?** Their source stems and zone headers
   differ, and this WLD has no exits. **Recommend** keep the current maps
   separate until the owner specifies a source-backed route or shared role.
9. **Are exits intended from room #1?** The current WLD has none, so the
   static map has one room and no WLD route. **Recommend** preserve the
   authored graph and record intended access and return behavior before a
   gameplay edit.
10. **What does reset mode 2 mean for this ZON with no reset commands?** The
    file defines no M/O/G/E/F/D resets. **Recommend** confirm whether the
    header is a retained template, an intentionally empty zone, or stale
    configuration without assuming it loads.
11. **Do `Magetower.zon` or `mavulsk.zon` room-#1 reset records relate to
    this source?** Each has its own WLD room #1; current active ownership
    assigns that number to Limbo. **Recommend** resolve each source against
    its own map rather than treating numeric references as Astral consumers.
12. **Should source-only WLD edges to numeric #1 be treated as Astral routes?**
    Fourteen directions from eight unlisted maps target #1; nine active
    Desert directions target the same number. **Recommend** keep these as
    numeric records whose current active target identity is Limbo, not as
    candidate ingress to Astral.
13. **Are any QST, SHP, mobile, object, or structured story dependencies
    missing from the current source?** None are present in the inspected
    Astral file set. **Recommend** have the owner identify any external
    dependency before considering a move or archive.
14. **Is the same-stem history complete for this placeholder?** Followed
    tracked paths show no explicit whole-zone replacement, but they do not
    cover external editor history. **Recommend** attach any outside rename,
    import, or retirement record before drawing a replacement conclusion.
15. **What evidence is required before a future path or archive decision?**
    The source tree cannot prove editor ownership, persisted instances, or
    live loader state. **Recommend** keep candidate row 11 **RETAIN IN PLACE**
    until the map owner closes those records and decides the intended
    disposition separately.
