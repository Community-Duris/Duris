# Session Specification

**Session ID**: `phase02-session10-artifact-and-guild-outcome-batching`
**Phase**: 02 - Transactional Gameplay Domains
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `65ae3d71`

## Objectives

1. Capture equipped-artifact feed/bind state and guild prestige/construction deltas from
   hydrated game-thread state without callback-time database reads.
2. Apply every derived artifact and guild effect once under the committed epic/combat
   parent identity, exact revisions, normalized locks, inbox, ledgers, and outbox.
3. Publish artifact timers, guild totals, cache invalidation, and notifications only from
   exact completion or durable outbox delivery.
4. Fail closed per affected domain when hydration or revision state is unavailable.

## Design Boundary

The epic and combat award publishers retain presentation and progression effects but no
longer mutate or save guilds or call artifact SQL. They derive one child identity from the
parent operation and capture a bounded artifact/guild payload from hydrated state. The
worker validates state revisions and commits legacy current rows, immutable ledgers,
inbox result, and outbox in one transaction.

Artifact ownership remains authoritative in the Session 05 item ownership domain. This
session composes feed/bind metadata only and refuses owner-changing artifact deltas that
do not carry an already-authoritative stable owner fence.

## Success Criteria

- [x] Epic/combat callbacks perform no artifact/guild database, Redis, or save calls.
- [x] Parent replay cannot duplicate artifact, prestige, or construction effects.
- [x] Prestige and threshold-derived construction become visible together.
- [x] Hydration/revision failure leaves prior safe state unchanged.
- [x] Focused MySQL, source, format, build, security, and full-suite gates pass.
