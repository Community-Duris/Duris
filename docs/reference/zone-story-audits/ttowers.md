# The Twin Towers: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence ttowers \
  --evidence-format markdown --output docs/reference/zone-story-audits/ttowers.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 13206 | `give=I:13223;receive=C:30000;disappear=0` | story: Blaevyna: the affair | [areas/qst/ttowers.qst:14](../../../areas/qst/ttowers.qst#L14) |
| 13207 | `give=I:13223;receive=C:30000;disappear=0` | story: Mixt: former love | [areas/qst/ttowers.qst:29](../../../areas/qst/ttowers.qst#L29) |
| 13213 | `give=I:13221,I:13222;receive=C:100000;disappear=0` | story: Lyena: two hearts | [areas/qst/ttowers.qst:54](../../../areas/qst/ttowers.qst#L54) |
| 13216 | `give=I:13221;receive=I:13224;disappear=0` | story: Priest: one heart for the temple | [areas/qst/ttowers.qst:80](../../../areas/qst/ttowers.qst#L80) |
| 13216 | `give=I:13222;receive=I:13224;disappear=0` | story: Priest: one heart for the temple | [areas/qst/ttowers.qst:90](../../../areas/qst/ttowers.qst#L90) |
| 13216 | `give=I:13223;receive=I:13224;disappear=0` | story: Priest: one heart for the temple | [areas/qst/ttowers.qst:85](../../../areas/qst/ttowers.qst#L85) |
| 13229 | `give=I:13221,I:13222,I:13223;receive=E:100000,I:13236;disappear=1` | story: Talfyn: harvest of three hearts | [areas/qst/ttowers.qst:112](../../../areas/qst/ttowers.qst#L112) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 13202 | mixt lyena | [areas/qst/ttowers.qst:2](../../../areas/qst/ttowers.qst#L2) |
| 13206 | mixt | [areas/qst/ttowers.qst:10](../../../areas/qst/ttowers.qst#L10) |
| 13207 | lyena southern tower | [areas/qst/ttowers.qst:23](../../../areas/qst/ttowers.qst#L23) |
| 13209 | lyena | [areas/qst/ttowers.qst:40](../../../areas/qst/ttowers.qst#L40) |
| 13213 | mixt blaevyna | [areas/qst/ttowers.qst:48](../../../areas/qst/ttowers.qst#L48) |
| 13214 | lyena | [areas/qst/ttowers.qst:63](../../../areas/qst/ttowers.qst#L63) |
| 13216 | owners mixt lyena blaevyna | [areas/qst/ttowers.qst:71](../../../areas/qst/ttowers.qst#L71) |
| 13229 | deal hello | [areas/qst/ttowers.qst:97](../../../areas/qst/ttowers.qst#L97) |
| 13229 | harvest | [areas/qst/ttowers.qst:102](../../../areas/qst/ttowers.qst#L102) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

156 parsed reset commands: D: 34, E: 17, G: 7, M: 87, O: 10, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
