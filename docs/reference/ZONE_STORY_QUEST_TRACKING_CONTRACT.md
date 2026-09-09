# Zone-story quest tracking contract

## Scope

The initial tracker is seasonal and character/PID-based. It covers zone/story/lore quests, not bartender quests or random world quests.

The initial quest model has no assigned-run lifecycle. There is no acceptance, sharing, expiry, or abandonment record. A successful completion produces one completion transaction.

## Definitions

A quest definition is an eligible, stable piece of zone content. It has a stable definition ID, zone, source, completion identity, and content revision.

A completion transaction is the atomic fact that one definition was completed and a set of characters received credit in the room at that moment.

All definitions are repeatable. The first completion of a definition for a PID in a season contributes to zone percentage; later completions remain activity history but do not increase the distinct-definition numerator.

## Credit modes

- `PERSONAL`: direct completer.
- `SOLO`: the transaction had exactly one credited PID.
- `GROUP_PARTICIPANT`: a credited recipient on a multi-recipient transaction.
- `LEADERSHIP`: the direct completer on a multi-recipient transaction.

Credit is per character PID. Account-wide and lifetime aggregation are future derived views, not initial storage semantics.

## Progress

```text
distinct definitions credited to PID in season
/
eligible definitions in zone and content revision
```

The denominator comes from the catalog, not from completion rows.

## Transaction boundary

The recipient set must be captured at completion time. Later code must not infer recipients by scanning the room again or consulting a changed group leader pointer.
