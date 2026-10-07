# Ceothia: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence ceofutur \
  --evidence-format markdown --output docs/reference/zone-story-audits/ceofutur.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 81404 | `give=I:81407;receive=C:500000,I:81406;disappear=1` | request: Bring the thief leader a bluestone vial | [areas/qst/ceofutur.qst:27](../../../areas/qst/ceofutur.qst#L27) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 81404 | hello hi howdy | [areas/qst/ceofutur.qst:2](../../../areas/qst/ceofutur.qst#L2) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

287 parsed reset commands: D: 6, E: 54, F: 6, G: 9, M: 203, O: 9.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
