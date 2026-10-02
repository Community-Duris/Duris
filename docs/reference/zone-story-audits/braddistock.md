# Braddistock Mansion: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence braddistock \
  --evidence-format markdown --output docs/reference/zone-story-audits/braddistock.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 1314 | `give=I:1340;receive=E:150000,I:1375;disappear=1` | story: Quiet the mansion | [areas/qst/braddistock.qst:30](../../../areas/qst/braddistock.qst#L30) |
| 1316 | `give=I:1334;receive=I:1340;disappear=1` | service: Release Slippers | [areas/qst/braddistock.qst:53](../../../areas/qst/braddistock.qst#L53) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 1314 | hi hello help | [areas/qst/braddistock.qst:2](../../../areas/qst/braddistock.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 1372 | `jet_black_maul` | [src/specs/specs.assign.c:1291](../../../src/specs/specs.assign.c#L1291) |

## Reset coverage

186 parsed reset commands: D: 10, E: 25, F: 7, G: 1, M: 64, O: 52, P: 27.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
