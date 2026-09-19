# Artifact control catalog

`catalog.json` is the validated control catalog for the supported native and telegraphic artifact adapters. It is intentionally separate from `areas/obj` and `areas/zon`: prototype text and reset commands remain world inputs, while this catalog owns variant policy, safe tuning metadata, and operator-visible provenance.

The server loads this file at boot and on `properties reload`. The current catalog contains the seven supported pilot definitions: Mayhem, Symmetry, mirrored ioun, Avernus, Tsunami, wand of wonder, and living necroplasm. The source inventory remains the authority for auditing all 169 flagged templates; legacy-only templates are not silently assigned powers here. Adding another definition requires an adapter capability and a focused contract test.

Revision 2 deliberately keeps every `player`, `wildNpc`, and `controlledNpc`
holder policy on `legacy`. Telegraphic adapters are registered and validated,
but are opt-in canaries until an artifact has a focused behavior comparison and
is explicitly changed through the draft/publish workflow.

Do not edit live state in this file. The in-game control command, `scripts/artifactctl.py`, and the restricted SQL publication procedures all validate a candidate and atomically replace the catalog. An invalid file leaves the previous in-memory catalog active.
