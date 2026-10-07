# Domain of Lost Souls: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence dlsc \
  --evidence-format markdown --output docs/reference/zone-story-audits/dlsc.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 36822 | `give=I:36856,I:36860;receive=E:75500,I:36859;disappear=1` | request: Return the contract and wedding band to Sir Boadwyn | [areas/qst/dlsc.qst:17](../../../areas/qst/dlsc.qst#L17) |
| 36837 | `give=I:55037,I:55037,I:55037;receive=I:55038;disappear=0` | request: Bring three seasoned-warrior skulls to Bal Sagoth | [areas/qst/dlsc.qst:47](../../../areas/qst/dlsc.qst#L47) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 36822 | contract curse wife | [areas/qst/dlsc.qst:2](../../../areas/qst/dlsc.qst#L2) |
| 36837 | hero heroes | [areas/qst/dlsc.qst:37](../../../areas/qst/dlsc.qst#L37) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 36884 | `kvasir_dagger` | [src/specs/specs.assign.c:1950](../../../src/specs/specs.assign.c#L1950) |
| obj | 36894 | `critical_attack_proc` | [src/specs/specs.assign.c:2588](../../../src/specs/specs.assign.c#L2588) |

## Reset coverage

488 parsed reset commands: D: 30, E: 158, F: 36, G: 71, M: 145, O: 32, P: 16.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
