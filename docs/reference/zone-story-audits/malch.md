# Malch'Hor Ganl the Goblin City: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence malch \
  --evidence-format markdown --output docs/reference/zone-story-audits/malch.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 23623 | `give=I:93901,I:93901,I:93901;receive=C:66666,E:66666;disappear=1` | request: Bring Snarg three shadowy circles | [areas/qst/malch.qst:24](../../../areas/qst/malch.qst#L24) |
| 23624 | `give=I:23622;receive=C:25000,E:25000;disappear=1` | request: Return Threzik's pirate hat | [areas/qst/malch.qst:50](../../../areas/qst/malch.qst#L50) |
| 23626 | `give=I:43131,I:43131,I:43131;receive=C:40000,E:65000;disappear=1` | request: Bring Shablem three lightning charms | [areas/qst/malch.qst:89](../../../areas/qst/malch.qst#L89) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 23623 | hi hello | [areas/qst/malch.qst:15](../../../areas/qst/malch.qst#L15) |
| 23624 | hi hello yes | [areas/qst/malch.qst:38](../../../areas/qst/malch.qst#L38) |
| 23626 | hi hello | [areas/qst/malch.qst:78](../../../areas/qst/malch.qst#L78) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

291 parsed reset commands: D: 18, E: 5, G: 11, M: 240, O: 14, P: 3.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
