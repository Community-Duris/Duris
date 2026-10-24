# Home of the Enlightened builder questions and recommendations

These questions preserve the source-authored story and the distinction from
active Longhollow. They do not authorize a load, merge, or path change. See
[TEMPLE2_AUDIT_REVIEW.md](TEMPLE2_AUDIT_REVIEW.md).

1. **Source owner:** Who owns the `temple2` WLD, MOB, OBJ, ZON, and QST files
   and their editor/export path? **Recommendation:** keep the five files
   RETAIN IN PLACE until the map and quest owners confirm the source of truth.
2. **Manifest history:** Why are the files outside `areas/AREA` and
   `areas/qst/AREA` today? **Recommendation:** require an owner-supported
   manifest history before activation or archival.
3. **Zone identity:** Is `Home of the Enlightened` the intended identity for
   zone #344 alongside active `Defense of Longhollow`, also zone #344?
   **Recommendation:** preserve separate names and area ownership.
4. **Room records:** How should all 89 differently titled `temple2` rooms
   #34400–#34488 remain distinct from Longhollow's same-number rooms?
   **Recommendation:** keep every source record separate and prohibit
   VNUM-only remapping.
5. **Active approaches:** Should the two Surface exits into #34401 continue
   to resolve to Longhollow's active room? **Recommendation:** preserve the
   current Longhollow route identity; do not claim either exit enters
   `temple2`.
6. **Direction codes:** What do the 14 WLD exits with direction values 6–9
   mean in this map format? **Recommendation:** verify the map and loader
   contract before using the full 196-record graph as playable reachability.
7. **Map completeness:** Is a directed walk from #34400 reaching all 89
   numeric rooms the intended topology? **Recommendation:** have the mapper
   review exits and reverse paths; do not infer safe travel from connectivity.
8. **Door/reset state:** Are reset mode 1 and the 60 `D` zone commands still
   intended? **Recommendation:** preserve authored reset and door records
   until the area owner confirms load behavior.
9. **Mobile identities:** How should the 30 candidate mobiles retain their
   identity beside Longhollow's different same-number prototypes?
   **Recommendation:** maintain strict source ownership for every #34400–29
   mobile before any activation proposal.
10. **Black hood #34405:** Is the `temple2` black hood distinct from
    Longhollow's active Selune token? **Recommendation:** preserve both
    definitions as different prototypes despite the shared number.
11. **Shop consumer:** Does unregistered player_guilds shop #34402 intend
    product #34405 to be Longhollow's token or this candidate's hood?
    **Recommendation:** retain the source-only row on candidate 84 and
    resolve its owner and loader exposure before describing stock.
12. **Priest and boy:** Are QST headers #34426/#34427 intended to bind the
    scared priest and Jared, respectively? **Recommendation:** preserve the
    QST and MOB source IDs together while confirming that the quest records
    have an intended current owner.
13. **Letter object:** Is object #34427 the letter used by the priest-and-son
    story, despite #34427 also being the boy's mobile/QST header number?
    **Recommendation:** keep mobile, object, and QST namespaces distinct and
    verify the `R I`/`G I` command roles.
14. **Encoded experience:** What does QST `R E 100000` at line 36 mean for
    this contract? **Recommendation:** confirm parser and owner intent before
    treating 100000 as an implemented requirement or reward.
15. **Story preservation:** Should the rescue-and-letter prose remain a
    source-authored quest thread without a story sidecar? **Recommendation:**
    preserve the wording and avoid inventing extra objectives, reward logic,
    or aftermath.
16. **Local resets:** Are the 65 `M`, 58 `O`, 41 `P`, 13 `E`, one `G`, and 60
    `D` records still intended to populate this separate map?
    **Recommendation:** retain them as source-only placements and do not
    co-load them with Longhollow resets.
17. **Mount code:** Should `read_mobile(34429)` continue to return the active
    Longhollow prototype even though `temple2` #34429 is a slave?
    **Recommendation:** keep code ownership with active Longhollow and
    reconcile the duplicate separately before any renumbering discussion.
18. **Old commented binding:** Is the commented `real_object0(34404)` entry
    still intentionally inactive? **Recommendation:** treat it as a comment,
    not an active candidate dependency, until its owner confirms otherwise.
19. **Quest endpoint:** What is intended for the boy's response and the
    letter's receipt/consumption after the authored QST commands?
    **Recommendation:** review the QST parser contract and preserve the
    current source text until the feature owner confirms the endpoint.
20. **Disposition authority:** Who can confirm the source, loader, identity,
    and persisted-use status of `temple2`? **Recommendation:** retain all
    files in place and require a separate owner decision before activation,
    co-loading, merging, moving, or archiving.
