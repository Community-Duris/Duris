# Vast Hidden Grove: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence solonar \
  --evidence-format markdown --output docs/reference/zone-story-audits/solonar.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 30600 | `give=C:4000,I:30661;receive=I:30662;disappear=0` | service: Repair a Shattered Diamond | [areas/qst/solonar.qst:8](../../../areas/qst/solonar.qst#L8) |
| 30601 | `give=I:30651,I:30652;receive=C:10000,I:30658;disappear=0` | service: Prepare Mithril Linked Scales | [areas/qst/solonar.qst:28](../../../areas/qst/solonar.qst#L28) |
| 30602 | `give=I:30634,I:30642;receive=C:10000,I:30656;disappear=0` | service: Prepare Cured Hide Robes | [areas/qst/solonar.qst:50](../../../areas/qst/solonar.qst#L50) |
| 30603 | `give=C:1000,I:30640,I:30650,I:30656,I:30657,I:30658;receive=I:30671;disappear=0` | story: Robes of the Arch-Magi | [areas/qst/solonar.qst:88](../../../areas/qst/solonar.qst#L88) |
| 30604 | `give=C:10000,I:30634,I:30635,I:30636,I:30649,I:30666;receive=I:30670;disappear=0` | story: A Piwafwi of Power | [areas/qst/solonar.qst:108](../../../areas/qst/solonar.qst#L108) |
| 30605 | `give=I:30660,I:30663;receive=I:30664;disappear=0` | service: Awaken the Wooden Heart | [areas/qst/solonar.qst:138](../../../areas/qst/solonar.qst#L138) |
| 30606 | `give=I:30635,I:30649;receive=C:10000,I:30657;disappear=0` | service: Prepare Radiant Thread | [areas/qst/solonar.qst:155](../../../areas/qst/solonar.qst#L155) |
| 30615 | `give=I:30653,I:30654,I:30655;receive=C:15000,I:30650;disappear=0` | service: Trade Herbs for Faerie Dust | [areas/qst/solonar.qst:171](../../../areas/qst/solonar.qst#L171) |
| 30617 | `give=I:30639,I:30640,I:30641,I:30642;receive=I:30666,I:30666;disappear=0` | service: Prepare an Ancient Scroll | [areas/qst/solonar.qst:193](../../../areas/qst/solonar.qst#L193) |
| 30620 | `give=I:30632,I:30633,I:30638;receive=I:30636;disappear=0` | service: Trade for Adamantium | [areas/qst/solonar.qst:215](../../../areas/qst/solonar.qst#L215) |
| 30631 | `give=I:30668;receive=C:10000;disappear=1` | request: Feed the Hawk | [areas/qst/solonar.qst:228](../../../areas/qst/solonar.qst#L228) |
| 30632 | `give=I:30669;receive=C:5000;disappear=1` | request: Feed the Swallow | [areas/qst/solonar.qst:239](../../../areas/qst/solonar.qst#L239) |
| 30635 | `give=I:30643,I:30646;receive=C:6000,I:30641;disappear=0` | service: Prepare a Crystalline Quill | [areas/qst/solonar.qst:257](../../../areas/qst/solonar.qst#L257) |
| 30635 | `give=I:30644,I:30645,I:30647,I:30648;receive=I:30640;disappear=0` | service: Prepare a Glowing Orb | [areas/qst/solonar.qst:267](../../../areas/qst/solonar.qst#L267) |
| 30638 | `give=I:30659,I:30662,I:30664;receive=I:30665;disappear=1` | story: Forge Mage Bane | [areas/qst/solonar.qst:287](../../../areas/qst/solonar.qst#L287) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 30600 | diamond help diamonds | [areas/qst/solonar.qst:2](../../../areas/qst/solonar.qst#L2) |
| 30601 | scales scale | [areas/qst/solonar.qst:18](../../../areas/qst/solonar.qst#L18) |
| 30602 | robes robe hides hide | [areas/qst/solonar.qst:41](../../../areas/qst/solonar.qst#L41) |
| 30603 | mage robe quest | [areas/qst/solonar.qst:63](../../../areas/qst/solonar.qst#L63) |
| 30603 | yes items | [areas/qst/solonar.qst:71](../../../areas/qst/solonar.qst#L71) |
| 30605 | heart | [areas/qst/solonar.qst:129](../../../areas/qst/solonar.qst#L129) |
| 30606 | string thread | [areas/qst/solonar.qst:148](../../../areas/qst/solonar.qst#L148) |
| 30615 | dust faerie | [areas/qst/solonar.qst:164](../../../areas/qst/solonar.qst#L164) |
| 30617 | scroll ancient | [areas/qst/solonar.qst:182](../../../areas/qst/solonar.qst#L182) |
| 30620 | stone | [areas/qst/solonar.qst:207](../../../areas/qst/solonar.qst#L207) |
| 30635 | quill orb | [areas/qst/solonar.qst:250](../../../areas/qst/solonar.qst#L250) |
| 30638 | pickaxe axe pick | [areas/qst/solonar.qst:280](../../../areas/qst/solonar.qst#L280) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| room | 30511 | `inn` | [src/specs/specs.assign.c:2462](../../../src/specs/specs.assign.c#L2462) |

## Reset coverage

152 parsed reset commands: D: 12, G: 53, M: 65, O: 22.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
