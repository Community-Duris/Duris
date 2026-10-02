# Pine Hollow: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence pineholl \
  --evidence-format markdown --output docs/reference/zone-story-audits/pineholl.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 16005 | `give=I:16015,I:16016,I:16076,I:16077;receive=I:16015,I:16081,I:16082,I:16084,I:16085;disappear=1` | story: The Cruel Warrior's Dragon Trophies | [areas/qst/pineholl.qst:35](../../../areas/qst/pineholl.qst#L35) |
| 16006 | `give=I:16013,I:16014,I:16080;receive=I:16015,I:16075;disappear=1` | story: Help the Wounded Gold Dragon | [areas/qst/pineholl.qst:69](../../../areas/qst/pineholl.qst#L69) |
| 16080 | `give=I:16019,I:16019,I:16019;receive=I:16048;disappear=0` | request: A Brown Bear Jacket | [areas/qst/pineholl.qst:124](../../../areas/qst/pineholl.qst#L124) |
| 16080 | `give=I:16020,I:16020,I:16020;receive=I:16049;disappear=0` | request: Black Bear Leggings | [areas/qst/pineholl.qst:134](../../../areas/qst/pineholl.qst#L134) |
| 16080 | `give=I:16021,I:16021;receive=I:16050;disappear=0` | request: A Heavy Bear Coat | [areas/qst/pineholl.qst:146](../../../areas/qst/pineholl.qst#L146) |
| 16080 | `give=I:16024,I:16024,I:16024;receive=I:16051;disappear=0` | request: Badgerskin Boots | [areas/qst/pineholl.qst:158](../../../areas/qst/pineholl.qst#L158) |
| 16080 | `give=I:16025,I:16025,I:16025;receive=I:16052;disappear=0` | request: A Red Fox Stole | [areas/qst/pineholl.qst:169](../../../areas/qst/pineholl.qst#L169) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 16005 | search searching hunt hunting | [areas/qst/pineholl.qst:2](../../../areas/qst/pineholl.qst#L2) |
| 16005 | lance | [areas/qst/pineholl.qst:7](../../../areas/qst/pineholl.qst#L7) |
| 16005 | auriam | [areas/qst/pineholl.qst:14](../../../areas/qst/pineholl.qst#L14) |
| 16005 | children dragons | [areas/qst/pineholl.qst:25](../../../areas/qst/pineholl.qst#L25) |
| 16006 | scar scars battle | [areas/qst/pineholl.qst:55](../../../areas/qst/pineholl.qst#L55) |
| 16080 | jacket | [areas/qst/pineholl.qst:91](../../../areas/qst/pineholl.qst#L91) |
| 16080 | leggings legging | [areas/qst/pineholl.qst:97](../../../areas/qst/pineholl.qst#L97) |
| 16080 | coat | [areas/qst/pineholl.qst:103](../../../areas/qst/pineholl.qst#L103) |
| 16080 | boots boot | [areas/qst/pineholl.qst:111](../../../areas/qst/pineholl.qst#L111) |
| 16080 | stole | [areas/qst/pineholl.qst:118](../../../areas/qst/pineholl.qst#L118) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

352 parsed reset commands: D: 28, E: 95, F: 9, G: 43, M: 163, O: 11, P: 3.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
