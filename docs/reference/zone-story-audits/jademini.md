# The Rice Fields: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence jademini \
  --evidence-format markdown --output docs/reference/zone-story-audits/jademini.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 77202 | `give=C:50000;receive=I:76676;disappear=0` | service: Buy a map of Jade | [areas/qst/jademini.qst:2](../../../areas/qst/jademini.qst#L2) |
| 77203 | `give=I:76670;receive=I:77201;disappear=0` | service: Have a wood sprite canned | [areas/qst/jademini.qst:9](../../../areas/qst/jademini.qst#L9) |
| 77214 | `give=I:77204;receive=I:77205;disappear=1` | story: Release the captive princess’s shackles | [areas/qst/jademini.qst:28](../../../areas/qst/jademini.qst#L28) |
| 77218 | `give=I:22622;receive=;disappear=0` | service: Offer sea maps to the helmsman | [areas/qst/jademini.qst:43](../../../areas/qst/jademini.qst#L43) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 77216 | `archer` | [src/specs/specs.assign.c:831](../../../src/specs/specs.assign.c#L831) |

## Reset coverage

76 parsed reset commands: D: 10, E: 2, G: 3, M: 53, O: 3, P: 5.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
