# Village of the Damned: builder questions

Canonical key: area:yuan_ti2. Physical coverage row 352. The
[source dossier](YUAN_TI2.md) and
[repository-wide audit](../quest-story-followups/YUAN_TI2_AUDIT_REVIEW.md)
document the village's authored conflicts, shop and chest references, reset
doors, neighboring Temple connection, and source-only map approaches. These
recommendations preserve unanswered story details; they are not builder answers
or live-play qualification.

## Explicit builder questions and recommended resolutions

1. **Who are the “invaders” named on the tortured elf's body?** The scene gives
   no faction or perpetrator. **Recommend** keep the message unattributed until
   the builder identifies its speaker and intended target.
   [Corpse message](../../../areas/obj/yuan_ti2.obj#L17)
2. **Do the elf, staked victim, bloody home, and campsite belong to one
   incident?** Their descriptions establish several violent scenes but no shared
   chronology. **Recommend** preserve each as separate evidence unless a builder
   supplies a confirmed relationship.
3. **How does the poor-human mobile's slavery text fit the village's story?**
   Mobile #80619 describes forced slave-gathering but has no local reset.
   **Recommend** treat the text as an unplaced prototype, not an ordinary
   player encounter or proof of a current practice.
   [Mobile source](../../../areas/mob/yuan_ti2.mob#L309)
4. **Is poor-human #80619 intentionally unreset, or is a placement missing?**
   The current ZON has no local placement. **Recommend** preserve the absent
   reset until its intended owner, appearance, and story context are confirmed.
5. **What role should the unarmed human merchant play in this settlement?**
   The mobile text calls him out of place and vulnerable without giving him a
   request or outcome. **Recommend** keep that description as characterization
   unless a builder defines an encounter and a safe, explicit resolution.
   [Merchant](../../../areas/mob/yuan_ti2.mob#L184)
6. **Is the elven bard expected to escape, stay, or be killed?** He doubts he
   will be allowed to leave alive, but no source records the outcome.
   **Recommend** keep his statement as fear and do not promise rescue or
   departure without an actor-state event.
   [Bard](../../../areas/mob/yuan_ti2.mob#L34)
7. **Why are the Githyanki noble and warrior in the mayor's quarters?** Their
   reset grouping and descriptions show tension but do not establish whether
   this is diplomacy, coercion, or an attack. **Recommend** preserve the
   encounter and leave its purpose open.
   [Mayor-room reset](../../../areas/zon/yuan_ti2.zon#L99)
8. **What authority does the “living god” exercise over the appointed mayor?**
   The prose states the appointment but records no current command or dispute.
   **Recommend** retain that political fact without inventing new elections,
   rebellion, or control effects.
9. **What is the intended role of the herb garden and its caustic vines?** The
   garden is tended and unusual, but the sources do not define an effect or
   reward. **Recommend** keep its authored description separate from any
   poison, cure, harvesting, or quest behavior.
   [Garden](../../../areas/wld/yuan_ti2.wld#L509)
10. **Are the crushed orchid and hidden golden eye connected?** Both are
    mysterious local clues without a defined use. **Recommend** preserve their
    independent object and room identities until the intended relationship is
    supplied. [Orchid](../../../areas/obj/yuan_ti2.obj#L37),
    [golden eye](../../../areas/obj/yuan_ti2.obj#L144)
11. **Who wrote the campsite note, and does “Skren” mean Skrentherlog?** The
    note identifies its author as an elf outside the village, but its names and
    history remain uncertain. **Recommend** retain that attribution and leave
    the temple connection unresolved until confirmed.
    [Note](../../../areas/obj/yuan_ti2.obj#L435)
12. **What does the gold key #80612 open, and is that its only intended use?**
    The angry human carries it in the same room as a chest whose object data
    names it; no WLD exit uses that key. **Recommend** document the authored
    chest relation without turning it into a village-gate key or a mandatory
    story step. [Key/chest data](../../../areas/obj/yuan_ti2.obj#L201)
13. **Why do rooms #80677 and #80678 retain load-error messages?** The reset
    places true-blood #80627 with “Snakebite” in #80677, while the rooms have
    no active inbound WLD edge. **Recommend** confirm whether these are
    intentional diagnostics or unfinished encounter data before changing
    placement or text. [Rooms and reset](../quest-story-followups/YUAN_TI2_AUDIT_REVIEW.md#active-wld-graph-and-reset-state-reach)
14. **Is unplaced true-blood #80633 intentional beside placed mobile #80632?**
    Their appearance text is similar, but only #80632 is placed.
    **Recommend** preserve the prototype distinction and current reset set
    unless a builder identifies a missing encounter.
    [Source dossier](YUAN_TI2.md#authored-interactions-and-limits)
15. **Should the village gates be open as their prose says or closed and secret
    after reset?** The two-sided fence door resets closed and one side secret.
    **Recommend** reconcile the descriptions with that reset state while
    preserving both authored boundary exits and the neighboring area owner.
    [Door resets](../../../areas/zon/yuan_ti2.zon#L8)
16. **Is the shop-quarters door intended to close after every reset?** Both
    directions reset closed; the keeper and shop data are in room #80656.
    **Recommend** keep the two directions paired and clarify the door's intended
    access without claiming the closed state was tested live.
    [Shop-door reset](../../../areas/zon/yuan_ti2.zon#L16)
17. **Which buyer does the shop's refusal line mean by “your people”?** The
    shop record supplies the message but does not prove a race or faction
    condition. **Recommend** preserve the text and avoid assigning an
    eligibility rule without the owning builder's evidence.
    [Shop record](../../../areas/shp/yuan_ti2.shp#L1)
18. **Are Heavens objects #364, #347, and #353 the intended imported stock?**
    The local shop and reset file use those foreign prototypes.
    **Recommend** preserve Heavens as their prototype owner; do not copy local
    objects or infer stock availability from the checked-in declarations.
    [Shop and reset review](YUAN_TI2.md#authored-interactions-and-limits)
19. **Why does the QST area index list yuan_ti2 when no local QST source exists?**
    The active contributor has no native Q/QA contract or local story sidecar.
    **Recommend** keep the index/file distinction documented and do not add or
    remove a QST file or entry without confirming the loader intent.
    [QST index](../../../areas/qst/AREA#L255)
20. **Do the Temple note and inactive Surface maps imply a replacement or
    merged area?** The active yuan_ti boundary is reciprocal, while Surf and
    oldmap8 approaches are source-only. **Recommend** retain yuan_ti2 as a
    separate owner and preserve the inactive edges with their map sources
    rather than treating them as active ingress.
    [Boundary and map audit](../quest-story-followups/YUAN_TI2_AUDIT_REVIEW.md#source-only-inactive-wld-approaches)

This is a source-backed recommendation review. It changes no area source,
door/reset state, shop behavior, or player-facing text.
