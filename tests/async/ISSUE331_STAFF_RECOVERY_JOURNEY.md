# Issue 331 staff recovery journey

The journey keeps two verification boundaries explicit:

- **Immediate delivery:** strict `verify --plan` runs after the in-game
  submission and before recipient login/materialization. It checks the
  original payloads, native IST1 bytes, delivery identities, ownership,
  affects, descriptions, and artifact timing. The journey also proves that a
  changed native field and a truncated payload with the same IST1 header are
  rejected.
- **After login/save/restart:** the journey does not weaken production
  `verify`. Runtime loading and saving legitimately normalize this fixture:
  bag #391 becomes weight `4 + 1 = 5`, prototype #15 adds affect `19:-10`,
  prototype #67259 adds `13:25` and `20:-2`, and prototype #7 contributes
  inherited extra descriptions. `post_runtime_acceptance` checks those exact
  fixture/prototype values, the immutable receipt/original-payload/delivery
  fingerprint, UID ownership topology, artifact timer projections, and target
  UID uniqueness.

The fingerprint is recorded before recipient login and compared after the
idempotent offline replay. It covers receipt identity/digests/counts, every
delivery identity and original payload, and restitution artifact timing rows;
mutable `player_items` and runtime projections are intentionally outside it.
Runtime save may rematerialize `player_items` rows, so ownership is checked by
UID and revision rather than by mutable database row ID.

The journey seeds the legacy `[601,602]` item-level spellbook description.
The post-save check expects that row together with the prototype #7 master
spellbook marker and inherited textual description; it does not apply the
isolated spellbook test's zero-row assumption to this journey. The separate
zero-row case is exercised by
`run_player_load_repository_spellbook_mysql.sh`.

Run the canonical staff journey with:

```sh
bash tests/async/run_issue331_staff_recovery_mysql.sh
```
