# Heavens: source story review

**Scope:** source review of the `heavens` zone's nine native Q contracts and
the surrounding room/reset context. It does not establish ordinary access,
runtime quest qualification, or the behavior of assigned special procedures.

## Source shape

`heavens` is an active source-list contributor with zone number `0`, 86 WLD
room records, 220 mobile prototypes, 923 object prototypes, and a QST file
with nine `Q` contracts. It has no `heavens.story.json` mapping. The room set
mixes shared/immortal service spaces, bright-corridor and inn nodes, work and
archive rooms, and themed planar scenes; it is not one continuous mortal
adventure map. The catalog's `discoverable` field is false for zone number 0,
but this source fact is not a conclusion about live access. Static entry,
exit, and reset state are documented separately in
[World connectivity](WORLD_CONNECTIVITY.md).

## Finding 1: Rozkadrath's eight-contract scheme

Eight Q blocks belong to mobile prototype `17`, a huge glabrezu that identifies
itself as Rozkadrath, ambassador of Joukarth Forgath. Its dialogue asks for
proof of obedience, then ties successive elemental artifacts to a plot to
open a portal to the prime plane. The hand-in text progresses through air,
ethereal, water, fire, a demon-prince's helm, and earth; the final text reveals
Joukarth's intended domination and says a portal opens toward the Island of
Darkness. The written arc is clear, but those paragraphs alone do not prove
that one contract mechanically gates the next or opens a usable route.

| QST block | Required items as written | Explicit `R` terms | Story beat named by the dialogue |
| --- | --- | --- | --- |
| [Line 36](../../../areas/qst/heavens.qst#L36) | Four `32011` basalt scimitars | `I 0` | Prove obedience; retrieve the first reagents and return a “seal.” |
| [Line 56](../../../areas/qst/heavens.qst#L56) | `I 0`, `24402` lightning earring, `24403` cloak of swirling mist | `I 0` | Air-plane artifacts. |
| [Line 72](../../../areas/qst/heavens.qst#L72) | `12401` cloak of ethereal mists, `12405` Silencer dagger | None | Ethereal artifacts. |
| [Line 87](../../../areas/qst/heavens.qst#L87) | `23202` black pearl earring, `23203` cloak of the manta ray | None | Water-plane artifacts. |
| [Line 103](../../../areas/qst/heavens.qst#L103) | `31549` glowing necklace | None | A necklace tied to the ruler of the sea. |
| [Line 115](../../../areas/qst/heavens.qst#L115) | `25402` flaming earring, `25403` cloak of eternal fire | None | Fire-plane artifacts. |
| [Line 131](../../../areas/qst/heavens.qst#L131) | `19996` skull-faced helmet | None | The helmet of the demon prince Demogorgon. |
| [Line 149](../../../areas/qst/heavens.qst#L149) | `23809` stone boots, `23808` stone gauntlets, `23807` dusty boots, `23806` dusty gauntlets | None | Earth-plane artifacts and the betrayal reveal. |

The source table follows the native loader's direction: `G` records are offered
inputs and `R` records are received outputs ([`G` parser branch](../../../src/world/quest.c#L2202), [`R` parser branch](../../../src/world/quest.c#L2243)). The literal `I 0` entries are retained as written; this review does not assign them an item identity.

The external object sources give the artifact trail a concrete multi-area
footprint: Avernus (`32011`), Plane Air One (`24402–24403`), Ethereal Main
(`12401`, `12405`), Plane Water One (`23202–23203`), Sea King domain (`31549`),
Plane Fire One (`25402–25403`), Astral Tiamat (`19996`), and Plane Earth One
(`23806–23809`). The source identities are linked at each prototype:
[Avernus](../../../areas/obj/avernus.obj#L133),
[air](../../../areas/obj/plane_air_one.obj#L26),
[ethereal](../../../areas/obj/ethereal_main.obj#L18),
[water](../../../areas/obj/plane_water_one.obj#L26),
[sea](../../../areas/obj/seakngdm.obj#L839),
[fire](../../../areas/obj/plane_fire_one.obj#L26),
[astral](../../../areas/obj/astral_tiamat.obj#L646), and
[earth](../../../areas/obj/plane_earth_one.obj#L72). These are candidate
source dependencies, not proof that the whole chain is currently attainable.

Two consecutive blocks contain literal `I 0` terms: the first Q block has
`R I 0`, and the second both gives and receives `I 0`. The prose calls this a
seal, but no prototype identity is expressed by those terms. None of the eight
Rozkadrath blocks carries the native `D` marker; the final portal/departure is
in prose, not an explicit QST disappearance term. Both details need builder
interpretation before any source change.

There is also a static placement question: mobile prototype `17` exists in
[`heavens.mob`](../../../areas/mob/heavens.mob#L139) and owns the eight Q
blocks, but `heavens.zon` has no reset command for mobile `17`. Its description
says it wanders the prime plane in search of power
([mobile](../../../areas/mob/heavens.mob#L139)); this zone does not statically
place it. A staff/event/scripted spawn could exist elsewhere, so this is an
availability question, not proof the NPC can never appear.

## Finding 2: the Orc sailor is a separate liberation story

Mobile `60`, an Orc sailor, has the ninth Q contract. The player gives six
`362` brittle-alloy piles and one `363` gauze bandage; the recorded rewards are
`I 43` (Token of the Gods) and `C 500000`, followed by the native `D` marker.
Its text says the sailor's people will be free from Humans and that the sailor
will leave through a portal to tell family. This story is separate from
Rozkadrath's plot. Unlike mobile `17`, mobile `60` has a static reset in
`heavens.zon` at room `16` ([reset](../../../areas/zon/heavens.zon#L267)); the
alloy and bandage sources are in
[`heavens.obj`](../../../areas/obj/heavens.obj#L3771), and the token prototype
is at [line 496](../../../areas/obj/heavens.obj#L496).

The room title at VNUM `16` is Haruko's Lair of Forgetfulness, and its room
prose describes an altar; the adjacent orc-sailor placement therefore merits
an owner check for intended tone and location. No route, access, reward, or
placement change is proposed here.

## Builder questions and recommended resolutions

1. **Is Rozkadrath meant to be a player-facing chain in this zone?** Confirm
   the intended spawn source for mobile `17`; keep Heavens' zone-0 status and
   access boundary intact while that decision is pending.
2. **What are the two literal `I 0` terms?** Confirm whether they represent an
   intentional sentinel, a missing seal prototype, or a stale input/output.
   Do not silently replace them with a guessed item VNUM.
3. **Is the final portal only narrative text?** Confirm whether opening it is
   meant to create a usable destination or simply conclude the villain's
   departure. Do not add a route or reorder contracts from prose alone.
4. **Does the Orc sailor belong in room 16?** Confirm the tonal/location intent
   and existing token/currency reward. Preserve the current QST terms until
   that review; do not alter the reward or access as part of story mapping.
5. **Should an authored sidecar map these as two separate stories?** If
   approved, keep the demonic artifact scheme separate from the Orc
   liberation exchange; do not connect them into one campaign.

The source inventory records 134 distinct special-procedure names for this
area. This dossier does not claim those handlers or every dynamic plane
transition have been reviewed; runtime behavior, special-driven access, and
route safety remain open work.

## Safe resolution summary

Record the two QST zero-VNUM terms and mobile-17 reset question for builder
review. Preserve all artifact VNUMs, Q types, the Orc sailor's exact reward,
zone 0 classification, and current room/reset data. No gameplay, access,
combat, currency, item, zone-placement, or migration change is proposed.
