# Thetis's Realm: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence thetis \
  --evidence-format markdown --output docs/reference/zone-story-audits/thetis.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 38015 | `give=I:38018;receive=I:38028;disappear=1` | story: Return a blue gem to the hermit crab | [areas/qst/thetis.qst:7](../../../areas/qst/thetis.qst#L7) |
| 38025 | `give=I:38001;receive=I:38013;disappear=1` | story: Return the mermaid’s coral earring | [areas/qst/thetis.qst:26](../../../areas/qst/thetis.qst#L26) |
| 38037 | `give=I:77209;receive=I:38037,I:77209;disappear=0` | story: Bring the torn treasure map to Burgadan | [areas/qst/thetis.qst:37](../../../areas/qst/thetis.qst#L37) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 38015 | gem blue stone | [areas/qst/thetis.qst:2](../../../areas/qst/thetis.qst#L2) |
| 38025 | earring | [areas/qst/thetis.qst:19](../../../areas/qst/thetis.qst#L19) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

243 parsed reset commands: D: 18, E: 27, G: 20, M: 166, O: 3, P: 9.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
