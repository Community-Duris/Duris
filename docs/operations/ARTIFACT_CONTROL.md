# Artifact control operations

The artifact catalog is the central control surface for the first migration
slice. It records the stable object vnum, uniqueness family, placement/load
point, lifetime, legacy binding, compiled adapter IDs, power limits, and the
variant chosen for a player, wild NPC, or controlled NPC. It does not execute
content from the catalog. Adapter names are descriptive capability IDs matched
to compiled code.

The shipped pilot definitions are Mayhem (21), Symmetry (22), mirrored ioun
(922), Avernus (19730), Tsunami (31514), wand of wonder (41350), and Living
Necroplasm (67243). The source inventory and compatibility report remain in
`docs/research/`. The other source-inventory templates remain on their native
legacy callbacks until an adapter is explicitly registered; they are not
silently assigned a modern implementation.

The shipped catalog revision keeps every player, wild-NPC, and controlled-NPC
holder on `legacy`. Telegraphic adapters are available for explicit canary
testing, but no pilot selects them by default. A telegraphic rollout therefore
requires a deliberate `setmode` draft, preview, review, and publish for the
specific artifact and holder class being tested.

## Runtime administration

After boot, a Forger or higher can use the existing `artifact` command with the
`control` namespace:

```
artifact control status
artifact control list [name|vnum|classification]
artifact control inspect <vnum>
artifact control preview <vnum>
artifact control setmode <vnum> <player|npc|controlled> <legacy|telegraphic>
artifact control enable <vnum> <0|1>
artifact control publish
artifact control discard
artifact control reload
```

`setmode` and `enable` only change an in-memory draft. `preview` shows the
effective draft and its differences from the active catalog. `publish` checks
the candidate again, increments the monotonic revision, computes the catalog
hash, and atomically replaces `lib/artifacts/catalog.json`. `discard` restores
the active snapshot. A failed write or validation leaves the previous active
snapshot in place. The runtime never performs file I/O from an item action; the
catalog is loaded during property initialization and read through immutable
process state during play.

The catalog controls routing and admission together. A disabled definition is
suppressed for both modes. A holder policy selecting `legacy` returns to the
existing native callback. A holder policy selecting `telegraphic` allows the
corresponding compiled action adapter. A failed modern balance/property check
still suppresses that modern action and does not silently spend a second random
roll by falling through to legacy. The current bridge covers Tsunami, mirrored
ioun, Living Necroplasm, Mayhem/Symmetry swords, Avernus, and wand of wonder.

Power entries are enforced at the adapter boundary as well. Setting a power's
`enabled` field to false suppresses that named action while leaving other powers
on the same artifact available. `powerLevel` is consumed by the modern
necroplasm transformation, wand-of-wonder spell selection, Avernus drain cap,
and sword combat selection; the remaining chance, cooldown, windup, and mana
fields are published metadata until the corresponding legacy timer/resource
owner is migrated. This keeps a balance change reviewable without pretending
that a legacy callback has already become catalog-owned.

The active object UID, binding soul, cooldown timers, and mana reserve remain
owned by the existing artifact and item-action services. Changing a catalog
variant does not create another artifact, reset a timer, or grant mana. Existing
objects keep their custody and expiry state; placement edits affect future
loads. The catalog's load fields make those points visible for an operator and
are ready for the spawn/lifecycle service to consume as that service is
migrated.

## No-login CLI workflow

`artifactctl.py` uses the same validation rules and hash material as the C++
loader. From the repository root:

```
python3 scripts/artifactctl.py --catalog lib/artifacts/catalog.json validate
python3 scripts/artifactctl.py --catalog lib/artifacts/catalog.json status
python3 scripts/artifactctl.py --catalog lib/artifacts/catalog.json list tsunami
python3 scripts/artifactctl.py --catalog lib/artifacts/catalog.json inspect 31514
# Explicit canary opt-in; the shipped catalog remains legacy for every holder.
python3 scripts/artifactctl.py --catalog lib/artifacts/catalog.json set-mode 31514 player telegraphic
python3 scripts/artifactctl.py --catalog lib/artifacts/catalog.json publish
```

The first `set-mode` command writes a sibling draft (`catalog.json.draft`);
the active file is unchanged until `publish`. Use `--publish` on `set-mode` or
`enable` only for a deliberate one-step offline publication. `enable` accepts
`0` or `1`. The CLI uses a temporary file plus `os.replace`, so a process crash
cannot leave a partially written active catalog.

For SQL deployments, migration `0029_artifact_control` adds immutable revision
and draft documents, a singleton head, holder and power projections, publish
requests/results, a writer lease, and the `artifact_control_active` view. The
runtime adapter can use these tables as the shared authority; the flat-file
path remains the authority when the server is built without MySQL. The
migration's verifier checks the table/view contract and runs a transactionally
rolled-back request/result smoke test.

## Review and recovery procedure

1. Run `validate` against the exact catalog that will be deployed.
2. Inspect the affected vnums and holder policies. Confirm that a placement
   change is intended for future acquisition and that an existing UID is not
   being recalled accidentally.
3. In-game, stage the same change and use `preview`; or import the reviewed
   CLI draft. Record the reason and the expected revision in the change review.
4. Publish once. Confirm `artifact control status` reports the new revision and
   hash. The baseline path is legacy for every holder; exercise a telegraphic
   holder path only when the draft explicitly opts that artifact and holder
   into the canary.
5. If the candidate is wrong, restore the previous values in a new draft and
   publish a new revision. Do not edit an old revision in place and do not
   reset artifact bind data as part of a mode change.

If the catalog is malformed or the hash is inconsistent, boot logs an
`Artifact control catalog unavailable` message and the compiled legacy routes
remain available. An operator should fix the file and run `properties reload`
or restart, then confirm a successful status before enabling a new mode.

This slice intentionally leaves the remaining legacy-only powers read-only in
the catalog import and keeps the existing `.zon`/artifact ownership authority.
The next implementation phase should migrate fresh-load reservation and
instance/timer operations through the same revisioned service before allowing
catalog placement values to replace those authoritative paths.
