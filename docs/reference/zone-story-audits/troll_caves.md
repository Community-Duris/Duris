# Troll Caves: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence troll_caves \
  --evidence-format markdown --output docs/reference/zone-story-audits/troll_caves.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 96916 | `give=C:200000,I:96906;receive=I:96917;disappear=0` | service: Hraaf: commission an obsidian dagger (guarded) | [areas/qst/troll_caves.qst:61](../../../areas/qst/troll_caves.qst#L61) |
| 96916 | `give=C:400000,I:96907;receive=I:96916;disappear=0` | service: Hraaf: commission a ruby longsword (guarded) | [areas/qst/troll_caves.qst:42](../../../areas/qst/troll_caves.qst#L42) |
| 96916 | `give=C:890000,I:96933;receive=I:96915;disappear=0` | service: Hraaf: commission an emerald mace (guarded) | [areas/qst/troll_caves.qst:23](../../../areas/qst/troll_caves.qst#L23) |
| 96925 | `give=I:96915,I:96931;receive=I:96932;disappear=0` | story: Guremgh: return the chalice and bless the mace | [areas/qst/troll_caves.qst:106](../../../areas/qst/troll_caves.qst#L106) |
| 96926 | `give=C:100000,I:96910;receive=I:96933;disappear=0` | service: Farghan: cut the emeralds (guarded) | [areas/qst/troll_caves.qst:148](../../../areas/qst/troll_caves.qst#L148) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 96916 | weapons weapon | [areas/qst/troll_caves.qst:2](../../../areas/qst/troll_caves.qst#L2) |
| 96925 | mace hraaf | [areas/qst/troll_caves.qst:80](../../../areas/qst/troll_caves.qst#L80) |
| 96925 | weapon weapons | [areas/qst/troll_caves.qst:93](../../../areas/qst/troll_caves.qst#L93) |
| 96925 | dagger longsword | [areas/qst/troll_caves.qst:100](../../../areas/qst/troll_caves.qst#L100) |
| 96926 | emeralds | [areas/qst/troll_caves.qst:128](../../../areas/qst/troll_caves.qst#L128) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

193 parsed reset commands: D: 28, E: 20, F: 4, G: 18, M: 111, O: 11, P: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
