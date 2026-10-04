# The Motte and Bailey of Duke Delwyn: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence delwyn \
  --evidence-format markdown --output docs/reference/zone-story-audits/delwyn.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 82807 | `give=C:1000;receive=;disappear=0` | service: The pub's paid alley encounter | [areas/qst/delwyn.qst:54](../../../areas/qst/delwyn.qst#L54) |
| 82808 | `give=I:82829;receive=I:82830;disappear=0` | story: The miller's missing gear | [areas/qst/delwyn.qst:74](../../../areas/qst/delwyn.qst#L74) |
| 82815 | `give=I:82828;receive=I:82829;disappear=0` | story: A bell for the bailey clock | [areas/qst/delwyn.qst:134](../../../areas/qst/delwyn.qst#L134) |
| 82821 | `give=C:10000,I:82817;receive=I:82818;disappear=0` | service: Dye the white yarn crimson | [areas/qst/delwyn.qst:160](../../../areas/qst/delwyn.qst#L160) |
| 82823 | `give=C:35000,I:82819;receive=I:82820;disappear=0` | service: Sew the replacement banner | [areas/qst/delwyn.qst:174](../../../areas/qst/delwyn.qst#L174) |
| 82824 | `give=C:10000,I:82818;receive=I:82819;disappear=0` | service: Weave the crimson yarn | [areas/qst/delwyn.qst:189](../../../areas/qst/delwyn.qst#L189) |
| 82825 | `give=C:5000,I:82816;receive=I:82817;disappear=0` | service: Spin a fleece into white yarn | [areas/qst/delwyn.qst:203](../../../areas/qst/delwyn.qst#L203) |
| 82874 | `give=I:82826;receive=I:82827;disappear=0` | story: The captain's missing chess piece | [areas/qst/delwyn.qst:276](../../../areas/qst/delwyn.qst#L276) |
| 82878 | `give=I:82823,I:82824;receive=I:82825;disappear=0` | story: Warn the Duke with two pieces of evidence | [areas/qst/delwyn.qst:285](../../../areas/qst/delwyn.qst#L285) |
| 82890 | `give=I:82822;receive=I:82824;disappear=0` | story: The papers in the Duke's study | [areas/qst/delwyn.qst:315](../../../areas/qst/delwyn.qst#L315) |
| 82895 | `give=I:82820;receive=I:82821;disappear=0` | story: A banner for the keep | [areas/qst/delwyn.qst:356](../../../areas/qst/delwyn.qst#L356) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 82808 | gearwork fix part cog grumbling mill | [areas/qst/delwyn.qst:67](../../../areas/qst/delwyn.qst#L67) |
| 82815 | part clock bell | [areas/qst/delwyn.qst:128](../../../areas/qst/delwyn.qst#L128) |
| 82821 | spool yarn wool dye | [areas/qst/delwyn.qst:155](../../../areas/qst/delwyn.qst#L155) |
| 82823 | flag banner wool bolt fabric | [areas/qst/delwyn.qst:168](../../../areas/qst/delwyn.qst#L168) |
| 82824 | spool yarn wool | [areas/qst/delwyn.qst:182](../../../areas/qst/delwyn.qst#L182) |
| 82825 | fleece | [areas/qst/delwyn.qst:197](../../../areas/qst/delwyn.qst#L197) |
| 82874 | mutter mutters muttering chess something | [areas/qst/delwyn.qst:267](../../../areas/qst/delwyn.qst#L267) |
| 82890 | search searching study duke something | [areas/qst/delwyn.qst:308](../../../areas/qst/delwyn.qst#L308) |
| 82895 | flagpole frown flag | [areas/qst/delwyn.qst:345](../../../areas/qst/delwyn.qst#L345) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

278 parsed reset commands: D: 38, E: 67, F: 5, G: 4, M: 135, O: 28, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
