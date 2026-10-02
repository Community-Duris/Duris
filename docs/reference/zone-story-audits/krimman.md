# Lord Krimeneha's Mansion: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence krimman \
  --evidence-format markdown --output docs/reference/zone-story-audits/krimman.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 16419 | `give=I:16451;receive=I:16451,I:16454;disappear=1` | request: Release the Lady | [areas/qst/krimman.qst:2](../../../areas/qst/krimman.qst#L2) |
| 16420 | `give=I:16451;receive=I:16451;disappear=1` | request: Release the Serving Girl | [areas/qst/krimman.qst:18](../../../areas/qst/krimman.qst#L18) |
| 16421 | `give=I:16451;receive=I:16436,I:16451;disappear=1` | request: Release the Crying Ghost | [areas/qst/krimman.qst:51](../../../areas/qst/krimman.qst#L51) |
| 16422 | `give=I:16450;receive=I:16451;disappear=0` | service: Prepare a Staff Fragment | [areas/qst/krimman.qst:89](../../../areas/qst/krimman.qst#L89) |
| 16422 | `give=I:16452,I:16453,I:16454;receive=I:16445,I:16455;disappear=1` | story: Release the Haunted Family | [areas/qst/krimman.qst:101](../../../areas/qst/krimman.qst#L101) |
| 16424 | `give=I:16451;receive=I:16451,I:16452;disappear=1` | request: Release the Boy | [areas/qst/krimman.qst:118](../../../areas/qst/krimman.qst#L118) |
| 16425 | `give=I:16451;receive=I:16451,I:16453;disappear=1` | request: Release the Young Girl | [areas/qst/krimman.qst:131](../../../areas/qst/krimman.qst#L131) |
| 16426 | `give=I:16451;receive=I:16451;disappear=1` | request: Release the First Gardener | [areas/qst/krimman.qst:165](../../../areas/qst/krimman.qst#L165) |
| 16427 | `give=I:16451;receive=I:16422,I:16451;disappear=1` | request: Release the Second Gardener | [areas/qst/krimman.qst:188](../../../areas/qst/krimman.qst#L188) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 16421 | crying tears sob | [areas/qst/krimman.qst:33](../../../areas/qst/krimman.qst#L33) |
| 16421 | leave exit | [areas/qst/krimman.qst:40](../../../areas/qst/krimman.qst#L40) |
| 16421 | lord krimeneha | [areas/qst/krimman.qst:46](../../../areas/qst/krimman.qst#L46) |
| 16422 | pool | [areas/qst/krimman.qst:64](../../../areas/qst/krimman.qst#L64) |
| 16422 | eckraldu | [areas/qst/krimman.qst:77](../../../areas/qst/krimman.qst#L77) |
| 16426 | pool | [areas/qst/krimman.qst:146](../../../areas/qst/krimman.qst#L146) |
| 16426 | monster monsters | [areas/qst/krimman.qst:154](../../../areas/qst/krimman.qst#L154) |
| 16426 | lord krimeneha | [areas/qst/krimman.qst:160](../../../areas/qst/krimman.qst#L160) |
| 16427 | lord krimeneha | [areas/qst/krimman.qst:178](../../../areas/qst/krimman.qst#L178) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

176 parsed reset commands: D: 48, E: 6, G: 10, M: 58, O: 31, P: 23.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
