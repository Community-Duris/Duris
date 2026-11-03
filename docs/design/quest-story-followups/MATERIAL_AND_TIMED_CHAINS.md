# Material, random, and timed chains

**Research record — proposed decisions only; no recipe, timer, reward, or
accounting change has shipped.** All quantities and prototypes below are exact
native values. Do not infer personal source history from a later item-only
contract.

## QSF-JADE-02 — Freed turtle and competing uses

**Priority:** P2. **Confidence:** high. **Classification:** intentional
scarcity/challenge, with source-generation history not currently durably
tracked.

### Exact route and fork

The Jade release exchange accepts utility knife 76623 at the entangled turtle
76658 and gives one turtle item 76624; its text says the animal is freed and the
recipient retires ([release Q](../../../areas/qst/jade.qst#L118), [knife
description](../../../areas/obj/jade.obj#L322)). The same exact turtle item can
be consumed by the chef for soup 76652 ([chef Q](../../../areas/qst/jade.qst#L56))
or by the tamer for two inferior silver ore items 222 ([tamer Q](../../../areas/qst/jade.qst#L165)). One current copy cannot be submitted to both
consumers.

The entangled turtle is a cap-one reset in the cited room; this creates
competition for current stock, not evidence of a permanent one-time global
choice. See the [reset](../../../areas/zon/jade.zon#L552) and the existing
[Jade source/turn-in analysis](../zone-stories/THE_JADE_EMPIRE.md#L67). The
recipe identities, quantities, and outputs are already described in the
[dossier's competing-supplies table](../zone-stories/THE_JADE_EMPIRE.md#L93).

### Proposed disposition

Record turtle release, soup preparation, and ore exchange as three separate
receipts if those receipts are qualified. Do not mark soup and ore as mutually
exclusive for the character across future resets. Do not automatically credit
the release because the character later holds or cooks a turtle; the later
supplied item proves only the exact downstream exchange. Keep the knife, turtle
and recipe quantities, outputs, and source cap unchanged.

Active READY qualification must show that accepted events are committed at the
correct exchanges and that ordinary later reset stock can produce a second
valid copy when the world provides one. No journal milestone should claim
personal animal rescue based solely on receiving the downstream turtle item.

**UNSHIPPED proposal — news wording if projection is later qualified:** “The
Jade record now distinguishes freeing the turtle from the later soup or ore
exchange. Those services still consume the same exact turtle item, and later
available reset supply may support another exchange.”

## QSF-WH-01 — Random attribute scroll and nine earrings

**Priority:** P2. **Confidence:** high for the native recipe family and random
handler; full source-by-source qualification for the 49 upstream memories is
queued. **Classification:** missing tracking capability; random settlement
requires separate failure/recovery qualification.

### Exact chain

The Winterhaven dossier describes 49 distinct ambassador-memory deliveries,
each awarding one shopkeepers token, one strange enchantment scroll, and
1,000,000 copper. Forty-eight have declared reset sources; Ultarium's memory is
a separate engineer reward. This is dossier evidence, not a fresh exhaustive
recount of all source placements in this task
([memory and jewelry matrix](../zone-stories/WINTERHAVEN.md#L72)).

Reciting a strange enchantment scroll invokes the `attribute_scroll` handler.
The reviewed handler extracts the generic input and chooses one of nine exact
attribute-scroll prototypes, 55352–55360. The operation has a concrete
settlement concern: the input is removed before the selected output is
published, and the output publication is unchecked
([handler](../../../src/specs/specs.winterhaven.c#L3816)). A durable journal
receipt must not report an output that was not committed.

Branay accepts two scrolls of the same exact attribute kind plus one electrum
ingot for its corresponding earring; all nine outcomes are separate exact Q
contracts. The finale consumes all nine distinct earring identities plus
500,000 copper and returns the wondrous-power earring
([Branay contracts](../../../areas/qst/wh.qst#L1426)). The jewelry synthesis
and fee boundaries are also recorded in the [Winterhaven dossier](../zone-stories/WINTERHAVEN.md#L171).

### Proposed disposition

Keep the random nine-way outcome, each pair-match requirement, electrum cost,
and final exact set plus 500,000-copper fee. Do not turn the nine alternatives
into a single exclusive selection: repeated upstream memories and later scrolls
can support more outcomes. A journal may show known results only after a
successfully committed random-output event, not merely after recitation was
attempted.

Any implementation needs a committed attempt/receipt that binds the selected
scroll UID and source context to a frozen random outcome and published output.
Before broad credit, qualify duplicate command attempts, same-name scroll
selection, missing output prototypes, inventory capacity, interrupted
publication, retries, reconnect/replay, and exact coin debit/output identity.
The handler risk and mixed coin settlement should be addressed only in an
explicitly scoped fix; until then, the existing active-accounting guard remains
in force. This review does not authorize changing prices or random odds.

**UNSHIPPED proposal — news wording if the handler is later qualified:**
“Winterhaven now records the exact attribute scroll produced by a completed
recitation and the matching earring exchange. Random results, exact ingredients,
and the 500,000-copper final fee are unchanged.”

## QSF-WH-02 — Three dragon hearts and the third day

**Priority:** P2. **Confidence:** high for the text/code discrepancy and
item-only reward; whether the dialogue means a shared deadline or a simplifying
story clock is unresolved. **Classification:** timing mismatch with builder
intent unresolved.

### Text, runtime, and receipts

Drahknalloth's `three` answer says the hearts, seals, and hand-in must be
completed by noon of the third day after the first dragon's death
([dialogue](../../../areas/qst/wh.qst#L3015)). The three named boss death
handlers instead create their own heart objects and set a three-mud-day decay
timer on each one; the Dragonnia handler shows the timer directly
([death/timer handler](../../../src/specs/specs.winterhaven.c#L4669)). The
shared decay assignment is bound to the three exact heart kinds
([heart decay assignment](../../../src/specs/specs.assign.c#L2157)). Thus the
reviewed mechanic is per-heart lifetime from each heart's own creation, not a
single timer persisted from the first boss death.

The native reward contact accepts the three-heart result and returns ring
55073, 2,000,000 copper, and 250,000 experience
([Xola reward contract](../../../areas/qst/wh.qst#L1215)). Item acceptance does
not prove the player personally killed any dragon, unlocked any seal, or freed
the prisoner unless a separate successful door/use/actor event is actually
recorded. The full sequence and independent clock finding are summarized in
the [Winterhaven dossier](../zone-stories/WINTERHAVEN.md#L71).

### Proposed disposition

Until the builder resolves intent, describe the native item decay accurately:
each generated heart has its own three-mud-day timer. Do not replace those
timers with a shared countdown or extend them to make a story sentence appear
true. Do not add personal-boss-kill requirements to an exact-heart contract.
Keep door unlocks, heart hand-in, and any actual release as separate facts.

If the author intended a shared campaign deadline, that is a native mechanic
change with substantial timing and recovery effects. It needs an explicit
design, exact start/end event, qualification for recovered/stored hearts, and
failure behavior before any fix proposal. Source availability, decay replacement
failure, and live carry/equipment edge cases still require active qualification
before new durable credit.

**UNSHIPPED proposal — news wording if the existing mechanic is documented:**
“Winterhaven's record now explains that each dragon heart decays three mud days
after it is created. The current heart recipes, reward, and door requirements
are unchanged.”

## QSF-ASH-01 — Ashrumite silver-to-magical-necklace story

**Priority:** P1. **Confidence:** high for the source contract mismatch; the
intended recipe/story path is ambiguous. **Classification:** ambiguous authored
intent.

### Actual native contracts

The smith's dialogue describes silver ore refining and a plain necklace, but
the first contract consumes raw gem 66032 plus 10 copper and gives different
raw gem 66033. The next consumes five 66033 plus 2,500 copper and gives magical
necklace 66049. This exact necklace is a charged rainbow staff, not the plain
necklace described in the dialogue. The [two smith contracts](../../../areas/qst/ashrumite.qst#L132)
and source matrix in the [Ashrumite dossier](../zone-stories/ASHRUMITE_VILLAGE.md#L64)
establish current terms and outputs.

Some neighboring contracts do match their gem-cutting intent: the exact raw
gem 66039, 66040, or 66041 costs 20 copper for a cut stone. Sapphire 66042
plus 20 copper returns plain necklace 66047; emerald 66043 plus 20 returns
ordinary gem-set necklace 66048. The separate earring/ring recipes request
items 66065/66064 and 66048 for 100 copper but name absent reward prototypes
66067/66066. The six-item exchange consumes magical necklace 66049, three
specified cut gems, plain necklace 66047, and ordinary necklace 66048 plus
1,000 copper for real gold ore 66050. Mage 66017 then requires real gold ore
66050, missing disc 4372, and 25,000 copper for pyrite 66051. Silver ore 66030
and pure bar 66031 are distinct prototypes without the promised active refining
recipe in the reviewed contract set. The [full exact matrix](../zone-stories/ASHRUMITE_VILLAGE.md#L75)
also records conflicting sign/dialogue fee statements and the exact item
identities.

These adjacent recipes are not one mechanically validated pipeline merely
because the dialogue groups them into one story. Names, native output types,
prices, stock, and absent prototypes all differ. A visually similar name or
neighboring Q is not an acceptable substitute.

### Proposed disposition and builder decision

Default to accurate explanatory text: say what each exact current contract
accepts and returns; do not promise a silver bar, plain necklace, missing
earring/ring, or magical transformation that the native Q does not produce.
Keep exact prices, offers, output prototypes, and reward values unchanged.

If the intended author goal is instead a continuous silver-to-necklace-to-gold
story, the builder must select the recipe set first: exact producer and
consumer IDs, item quantities, copper fees, valid source routes for 66030/66031,
meaning of the 66049 charged staff, disposition of missing 66066/66067, and
whether/how the missing disc 4372 is supplied. Then review balance and active
accounting admission for each paid/mixed exchange. No blanket offset, generic
“disc,” guessed ingredient substitution, or reward repair is approved here.

**UNSHIPPED proposal — news wording if only explanatory text is corrected:**
“Ashrumite descriptions now identify the exact items and outputs used by the
existing recipes. Ingredient counts, copper costs, and rewards are unchanged;
the silver-bar and missing-disc story remains pending a builder decision.”
