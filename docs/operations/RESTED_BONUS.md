# Rested experience feature switch

`exp.rested.enabled` controls **automatic** rested and well-rested experience bonuses.
Explicit staff grants through `newbsa`/`newbsu` remain effective when it is off.
The shipped value is `1.000`, and a properties file without the key also defaults
to enabled. Upgrading must not turn either tier off accidentally.

```properties
[rested experience bonuses]
exp.rested.enabled=1.000
```

Use `1.000` for enabled and `0.000` for disabled. The tiers retain their existing
1.5x and 2x XP multipliers when enabled; this is not a multiplier-tuning setting.
Resurrection XP remains exempt. Other XP modifiers, ordinary resting/healing,
and other spell-up effects are unchanged.

## Operator controls

The key lives in `lib/duris.properties`. Add it when upgrading an older properties
file before attempting to change it with the in-game command: `properties set`
updates existing keys and does not create a missing one.

An authorized Forger can use the existing property controls:

```text
properties show exp.rested.enabled
properties set exp.rested.enabled 0
properties show exp.rested.enabled
```

The live property is read at each relevant operation, so a successful change
applies to existing online characters without rebooting or reconnecting. Already
printed text is not erased; account menus and score reflect the setting when
next rendered. Web clients receive the capability on authentication/return to
menu and the current enabled state on every rested-bonus request.

To persist the current property configuration, use the existing `properties save`
command. That saves all current property changes, not only this key; inspect
`properties diff` first. Alternatively edit this one key in the file and run
`properties reload` (which reloads the full property configuration), or load it
at the next normal startup.

Rollback is `properties set exp.rested.enabled 1`, with the same persistence
choice. No player-data rewrite or database migration is needed.

If progression telemetry is enabled, review and add the new property-registry
catalog mappings **before rollout**. Both enabled and disabled states need
admitted mappings if operators intend to toggle between them. Preserve historical
mappings and sealed rows; do not copy the example fixture hashes as a substitute
for reviewing the effective server configuration. See
[configuration context](../telemetry/CONFIG_CONTEXT.md).

## Disabled behavior

- Ordinary rested effects (including existing automatic/purchased effects) do not
  increase XP. Explicit staff-granted effects still apply their usual multiplier.
  Progression events report rested modifiers only when actually applied.
- Login does not grant or refresh rested bonuses, including the initial
  well-rested award to new characters.
- The rest spell reports that the feature is disabled, without granting,
  refreshing or upgrading an effect.
- `newbsa` and `newbsu` remain staff overrides: a grant adds rested, the next
  upgrades to well-rested, and subsequent grants refresh the usual duration.
  Both the XP bonus and visible score/expiry messages remain active for these
  grants while automatic bonuses are disabled. Other blessings are unchanged.
- The witch doctor's XP elixir is omitted from the list. Its former number and
  all existing name aliases are refused before any payment or effect grant.
  Other elixirs and their numbers are unchanged.
- Account option `8) Check rested bonus` is hidden. Manually entering `8`, or
  directly calling the check handler, gives a disabled message and returns to
  the ordinary account menu. Other option numbers are unchanged.
- Offline rested summaries are not generated. Ordinary dormant rested effects
  are hidden from score and emit no wear-off message; active staff grants remain
  visible and expire normally.
- The WebSocket rested endpoint returns `enabled: false` and an empty character
  array, without loading characters or computing countdowns. Authentication and
  return-to-menu payloads expose `data.capabilities.restedBonus`.

The browser client is in the separate DurisWeb repository. This server change
provides the capability and stops reporting countdowns; a client must consume
that capability to hide its own hardcoded control. It cannot retroactively remove
a control from an old client. This PR does not claim to change that separate UI.

## Saved effects and re-enabling

Saved effects remain stored and undergo ordinary online/offline expiration.
Ordinary effects are inert and hidden while the switch is disabled, rather than
being permanently deleted or having their timers frozen. Staff grants use the
rested tag's `AFFTYPE_CUSTOM1` flag as persisted provenance and remain effective
without depending on the granting immortal staying connected. A staff refresh or
upgrade marks an existing ordinary effect too. Pre-upgrade saves have no such
provenance and are treated as ordinary effects until explicitly granted again.
Re-enabling can reactivate
an unexpired effect. Subsequent logins use the usual offline-time rules, including
time spent offline while the feature was disabled. There is no compensation,
bulk purge, retroactive XP change or reset of offline timers.

Staff inspection and persistence still retain the real effect data for diagnosis.
The existing GMCP `Char.Affects` serializer already excludes these tag IDs;
this change does not expand or otherwise change that protocol.

## Focused verification

`python3 tests/async/test_rested_bonus_runtime.py` executes production XP/login
branches and production spell, spell-up, merchant and wear-off function bodies
with isolated engine collaborators. It covers missing/explicit-on/explicit-off
settings, both XP tiers and modifier flags, resurrection exclusion, login
thresholds and new-character offline seed, score filtering, no-charge purchase
rejection by name/number, preserved other blessings/elixirs, live off/on
transitions with existing effects, and effective staff grants/upgrades/refreshes
while off (including score, expiry and XP modifier reporting).

`python3 tests/async/test_rested_toggle_ui.py` covers the account and WebSocket
contracts. The telemetry configuration tests cover the appended gate identity
and compatibility with immutable historical catalog mappings.

These executable harnesses are not an end-to-end server/player journey. Before
non-draft acceptance, exercise a disposable local server with the setting missing,
`1`, then `0`: verify XP deltas, account option `8`, score, both developer spell-up
commands, witch-doctor purchases, and an already-rested character through an
off/on transition. No production server is required for this verification.
