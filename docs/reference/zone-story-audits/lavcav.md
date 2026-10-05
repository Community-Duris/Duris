# The Underground Lava Caves: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence lavcav \
  --evidence-format markdown --output docs/reference/zone-story-audits/lavcav.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 35517 | `give=I:35515;receive=E:50000,I:35525;disappear=0` | story: Lieutenant: return the platinum horns | [areas/qst/lavcav.qst:43](../../../areas/qst/lavcav.qst#L43) |
| 35535 | `give=C:100000;receive=I:35515;disappear=1` | Excluded: The coin-only adventurer purchase is refused while active accounting is enabled; its horns issuance and D retirement need atomic wallet debit, selected NPC identity and reward/recovery qualification. Keep it excluded until supported. It is the sole identified native horns producer, not an ordinary floor or loot source. | [areas/qst/lavcav.qst:62](../../../areas/qst/lavcav.qst#L62) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 35515 | tresure secret | [areas/qst/lavcav.qst:2](../../../areas/qst/lavcav.qst#L2) |
| 35517 | person | [areas/qst/lavcav.qst:21](../../../areas/qst/lavcav.qst#L21) |
| 35517 | horns | [areas/qst/lavcav.qst:38](../../../areas/qst/lavcav.qst#L38) |
| 35535 | horns horn | [areas/qst/lavcav.qst:57](../../../areas/qst/lavcav.qst#L57) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

163 parsed reset commands: D: 20, E: 48, F: 2, G: 5, M: 87, O: 1.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
