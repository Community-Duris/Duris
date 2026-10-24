# Abandoned Elven Homestead title and history audit supplement

This supplement records the current normalized-title comparison and followed path history for the active `elvish` contributor alongside the [source-comprehensive story dossier](../zone-stories/ABANDONED_ELVEN_HOMESTEAD.md). It changes no native area source.

## Title, history, and disposition

A fresh normalized scan of all 442 current `.zon` title lines finds one exact “Abandoned Elven Homestead” owner: `elvish`. Both current indexes list `elvish *358-359` ([full-area entry](../../../areas/AREA#L172), [QST entry](../../../areas/qst/AREA#L159)); the current title is in [elvish.zon](../../../areas/zon/elvish.zon#L2). `elftomb` is a separate current contributor at `*206-208` with the distinct title “Barrow of the Quiosho” ([full-area entry](../../../areas/AREA#L107), [title source](../../../areas/zon/elftomb.zon#L2), [Barrow history review](ELFTOMB_AUDIT_REVIEW.md)). It is not the Homestead's title owner or predecessor.

Followed WLD, MOB, OBJ, ZON, and QST histories stay on the `elvish` stem from the 2009 area-tree move. The first tracked `elvish.zon` in that move displays “Abandoned Elven Homestead.” with a final period; commit `1d093edde` removes that punctuation while updating the editor marker, leaving the normalized title unchanged. Later edits remain on the same paths. The story sidecar was added on 2026-10-02. No alternate-stem rename, distinct predecessor, retirement, or whole-zone replacement was established in the reviewed history.

The source dossier's title question now links to `elvish.zon` and retains the `elvish` stem. Keep active `elvish` **DO NOT MOVE**. Command-target alias ownership remains the separate open question in the [dossier](../zone-stories/ABANDONED_ELVEN_HOMESTEAD.md#direct-area-owner-questions-and-conservative-recommendations). See [coverage row 87](ZONE_COVERAGE_REGISTER.csv#L88) and the [duplicate/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-725-abandoned-elven-homestead-title-and-history).

## Checkpoint 795: current typed census refresh

### ZON, QST, and story references

The active elvish.zon has 118 prototype-reset commands: 65 M, 12 F, 11 O,
16 P, six E, and eight G. Each P command names both child and container
objects, so the commands carry 134 typed prototype VNUM operands. Every
operand resolves to a local Elvish mobile or object; there is no outbound
ZON reset import and no unresolved reset reference.

The local QST has 14 R/G item fields, all local
([QST](../../../areas/qst/elvish.qst#L1)). The current story sidecar has 20
typed VNUM values: two mobile, 11 giver, and seven item values, all local
([sidecar](../../../areas/story/elvish.story.json#L23)). None of the 231
other current story sidecars references an Elvish room, mobile, or object.
No external native QST item/giver header, ZON reset role, SHP product, or
external MOB/OBJ definition targets an Elvish prototype. The area has no
local shop and no same-type duplicate mobile/object prototype.

### Typed item travel and compiled reference

Elvish objects #35801 and #35803 have type-25 destinations to local room
#35800. Object #35804 targets Solonar room #30648
([object](../../../areas/obj/elvish.obj#L63),
[room](../../../areas/wld/solonar.wld#L1474)). Solonar's object #30604
provides the reverse type-25 destination to Elvish room #35810
([Solonar object](../../../areas/obj/solonar.obj#L58),
[reset](../../../areas/zon/solonar.zon#L65),
[reciprocal audit](SOLONAR_AUDIT_REVIEW.md)). The global scan of 872
external type-25 declarations found this single target into Elvish:
Solonar #30604 to #35810. The scan of 380 external type-29 declarations found
no switch-room target in Elvish. These item routes do not add authored WLD
exits.

The shared Chaos equipment tables reference local object #35808 in both
starter kits ([standard](../../../src/account/chaos_eq_data.h#L135),
[enhanceable](../../../src/account/chaos_eq_data.h#L385)). No other direct
C/C++ prototype reference was found. Reset declarations and item travel do not
establish live availability, successful transport, editor/deployment state,
or persisted player identity. No area source, candidate disposition,
gameplay, or WLD total changed.
