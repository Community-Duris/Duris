# Twin Towers Forest milestones

**Status: direct owner decision captured October 8, 2026.** This is a proposed
tracking design only. It does not ship a sidecar/runtime/accounting change or
award any character credit.

## QSF-TWF-01 — Gardener belt access and flower delivery

**Priority:** P2. **Confidence:** high. **Classification:** missing durable
milestone tracking. The access challenge itself is intentional.

### Current native facts

Alvinar accepts any one of eight exact plants and gives identify scroll 13572.
The contract does not require the belt, conversation with the gardener, or a
successful garden crossing ([Alvinar contracts](../../../areas/qst/twin_towers_forest.qst#L14)).
The eight accepted plants and returned cut-flower responses are summarized in
the [zone dossier](../zone-stories/TWIN_TOWERS_FOREST.md#L36).

Gardener 13504 is reset in room 13556 with belt 13521 equipped at slot 13
([reset](../../../areas/zon/twin_towers_forest.zon#L371)). The belt object's
identity and text are in its [object record](../../../areas/obj/twin_towers_forest.obj#L182).
The movement special checks for that exact item worn at `WEAR_WAIST` before it
allows the authored directions; merely carrying the belt does not pass the
gate ([movement check](../../../src/specs/specs.twintowers.c#L130)). The current
story sidecar labels its `garden-access` step optional and explicitly says a
valid plant from another player can be handed to Alvinar
([current projection](../../../areas/story/twin_towers_forest.story.json#L48)).

Per the owner's clarification for this review, mortal characters do not have
an active steal route. In the authored route, belt acquisition therefore means
killing the gardener and looting the belt; that challenge is intentional. Do
not replace this with a friendly award, weaken the access check, or make the
challenge a prerequisite for the native flower hand-in.

### Owner-approved durable aggregate

For the fuller Twin Towers Forest achievement, each character must eventually
have all three historical facts:

1. **Acquired belt 13521.** The acquisition method is unrestricted. A valid
   gift, trade, loot, or other accepted custody path can qualify; do not require
   a gardener kill or a first-source provenance event.
2. **Wore belt 13521 at the waist.** This is a historical equipment milestone,
   not a requirement to keep the belt equipped. Preserve the actual wear event
   identity and slot.
3. **Completed Alvinar's native flower delivery.** Any of the eight accepted
   plant kinds qualifies by the exact original Q exchange. Delivery may happen
   before, between, or after acquisition and wear.

The aggregate can complete when all three facts exist, in any order. Acquisition
and wear credit survive later transfer or unwearing; each character must obtain
and wear the belt themselves. Current custody/equipment is a projection of
present state and cannot stand in for durable historical milestones. The
existing `garden-access` step is optional preparation; the full achievement
needs separately durable events and aggregation, not a newly mandatory access
gate. These owner decisions are also summarized in the [zone dossier](../zone-stories/TWIN_TOWERS_FOREST.md#L71).

### Proposed explanation sequence

Use progressive, truthful hints:

- **Story/encounter:** Alvinar's accepted plant alternatives and the broader
  family/garden context; keep the belt route as an optional lead.
- **Blocked movement:** identify the worn-at-waist gardener's belt as the exact
  movement check. Do not claim that Alvinar requires it.
- **Achievement progress:** identify belt acquisition, waist wear, and the
  flower exchange as separate historical milestones. Do not claim that a
  current carried/worn check is durable credit.

Any displayed objective must distinguish “wear the belt to pass these garden
paths” from “Alvinar accepts a valid flower without that path.” Do not suggest
that all eight plants require garden access: the daisy reset is elsewhere, and
the native turn-in accepts supplied items.

### Qualification required before implementation

Keep the achievement disabled until active READY accounting can record accepted
acquisition and waist-wear events durably, across reconnect/replay and item
transfer, and combine those with the existing exact Alvinar completion receipt.
Test that an acquired-then-transferred belt remains historical credit, an
unworn carried belt does not satisfy wear, another character's belt use does
not award this character's wear, a valid flower delivery can be first, each
flower alternative keeps its exact output, and no access event is required for
delivery. Preserve the movement check and all source/reward/balance behavior.

**UNSHIPPED proposal — news wording if the milestone projection is later
qualified:** “Twin Towers Forest now records obtaining the gardener's belt,
wearing it at the waist, and Alvinar's flower exchange as separate milestones.
They may be completed in any order; belt access remains optional for the flower
hand-in, and the garden's existing access challenge is unchanged.”

## Checkpoint 407: broader explicit builder questions

The [zone dossier](../zone-stories/TWIN_TOWERS_FOREST.md#explicit-builder-questions-and-recommended-resolutions)
now contains exactly 20 direct-source builder questions and recommendations.
They retain the owner-approved belt challenge and any-order flower-achievement
aggregate above, and cover the remaining native exchanges, services, reset
limits, animal stories, cross-zone boundaries, and the New Cave city wizard
reset. This checkpoint does not ship the unshipped milestone proposal or change
its owner-approved terms.
