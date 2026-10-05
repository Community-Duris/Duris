# The Cimmerian Nomad Encampment: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence nomads \
  --evidence-format markdown --output docs/reference/zone-story-audits/nomads.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 6220 | `give=I:6217,I:6218;receive=I:6221;disappear=0` | story: Septimus: present the other-planar evidence | [areas/qst/nomads.qst:48](../../../areas/qst/nomads.qst#L48) |
| 6220 | `give=I:6219,I:6220,I:6221;receive=I:6222;disappear=1` | story: Septimus: return both heads and the collateral ring | [areas/qst/nomads.qst:64](../../../areas/qst/nomads.qst#L64) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 6200 | purpose cimmerian | [areas/qst/nomads.qst:2](../../../areas/qst/nomads.qst#L2) |
| 6214 | work | [areas/qst/nomads.qst:17](../../../areas/qst/nomads.qst#L17) |
| 6220 | fate future | [areas/qst/nomads.qst:27](../../../areas/qst/nomads.qst#L27) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

102 parsed reset commands: D: 16, E: 22, G: 4, M: 55, O: 4, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
