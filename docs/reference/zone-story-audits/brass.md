# Plane of Fire, Brass: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence brass \
  --evidence-format markdown --output docs/reference/zone-story-audits/brass.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 139083 | `give=I:139128;receive=C:1000000;disappear=0` | story: Herl’s exotic blood request | [areas/qst/brass.qst:22](../../../areas/qst/brass.qst#L22) |
| 139110 | `give=I:139018;receive=;disappear=0` | Excluded: The dying djinn consumes one cold-blue-flame vial in a native recipe with empty response, no reward, no disappearance and no reviewed active placement. No rescue or second-task endpoint is established. Preserve native binding/recovery evidence but exclude it from named story, achievement and daily credit until a builder specifies its intended outcome. | [areas/qst/brass.qst:33](../../../areas/qst/brass.qst#L33) |
| 139119 | `give=I:139026,I:139028,I:139031,I:139033;receive=I:139070;disappear=0` | story: The tax collector’s palace key | [areas/qst/brass.qst:89](../../../areas/qst/brass.qst#L89) |
| 139121 | `give=C:7500000,I:139127,I:139142;receive=I:139137;disappear=0` | service: The armorer’s pyrohydra bracer | [areas/qst/brass.qst:131](../../../areas/qst/brass.qst#L131) |
| 139121 | `give=C:7500000,I:139127;receive=C:7500000,I:139127;disappear=0` | Excluded: The armorer’s smaller elder-scales-plus-7500-platinum recipe is a narrated refusal returning the same item kind and fee. It is not a prerequisite, successful commission, achievement or daily. Preserve native settlement evidence and the existing coin guard; replacement kind does not prove identical physical UID. | [areas/qst/brass.qst:118](../../../areas/qst/brass.qst#L118) |
| 139124 | `give=I:139011,I:139016,I:139017;receive=I:139018,I:139018;disappear=0` | story: Yodono’s three imposters | [areas/qst/brass.qst:159](../../../areas/qst/brass.qst#L159) |
| 139132 | `give=I:139016,I:139017,I:139139,I:139140,I:139141,I:139144;receive=I:139143;disappear=1` | story: The hidden spy’s six palace heads | [areas/qst/brass.qst:182](../../../areas/qst/brass.qst#L182) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 139072 | hi hello | [areas/qst/brass.qst:2](../../../areas/qst/brass.qst#L2) |
| 139083 | hi hello hey | [areas/qst/brass.qst:9](../../../areas/qst/brass.qst#L9) |
| 139083 | rare chimaera drop exotic | [areas/qst/brass.qst:16](../../../areas/qst/brass.qst#L16) |
| 139110 | hi hello hey howdy | [areas/qst/brass.qst:30](../../../areas/qst/brass.qst#L30) |
| 139119 | hi hello hey | [areas/qst/brass.qst:38](../../../areas/qst/brass.qst#L38) |
| 139119 | quest key palace | [areas/qst/brass.qst:45](../../../areas/qst/brass.qst#L45) |
| 139119 | slave slavery slaver | [areas/qst/brass.qst:61](../../../areas/qst/brass.qst#L61) |
| 139119 | shop shops shopping stores | [areas/qst/brass.qst:68](../../../areas/qst/brass.qst#L68) |
| 139119 | explore explorer explorers wander wanderers | [areas/qst/brass.qst:80](../../../areas/qst/brass.qst#L80) |
| 139121 | hydra pyrohydra scales scale | [areas/qst/brass.qst:108](../../../areas/qst/brass.qst#L108) |
| 139124 | hi hello hey howdy | [areas/qst/brass.qst:152](../../../areas/qst/brass.qst#L152) |
| 139132 | hi hello hey quest | [areas/qst/brass.qst:170](../../../areas/qst/brass.qst#L170) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 139138 | `artifact_hide` | [src/specs/specs.assign.c:1392](../../../src/specs/specs.assign.c#L1392) |
| obj | 139004 | `holy_mace` | [src/specs/specs.assign.c:1963](../../../src/specs/specs.assign.c#L1963) |
| room | 139078 | `inn` | [src/specs/specs.assign.c:2313](../../../src/specs/specs.assign.c#L2313) |

## Reset coverage

779 parsed reset commands: D: 112, E: 78, F: 25, G: 133, M: 408, O: 13, P: 1, R: 9.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
