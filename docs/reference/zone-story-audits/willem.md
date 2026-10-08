# The Ruins of Turolopolis: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence willem \
  --evidence-format markdown --output docs/reference/zone-story-audits/willem.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 7103 | `give=I:7103;receive=E:25000,I:7100;disappear=1` | story: Corwyck: bring the unicorn horn | [areas/qst/willem.qst:27](../../../areas/qst/willem.qst#L27) |
| 7121 | `give=I:7107,I:7110,I:7113,I:7116,I:7124;receive=E:500000,I:7131;disappear=1` | story: Lothrell: honour the five heroes | [areas/qst/willem.qst:117](../../../areas/qst/willem.qst#L117) |
| 7125 | `give=I:7135;receive=E:50000,I:7137;disappear=1` | story: The drow emissary: recover the timeworn letter | [areas/qst/willem.qst:149](../../../areas/qst/willem.qst#L149) |
| 7126 | `give=I:7131,I:7139;receive=I:7138;disappear=0` | story: Kurtukr: strengthen the bloodsaber | [areas/qst/willem.qst:165](../../../areas/qst/willem.qst#L165) |
| 7130 | `give=I:7141;receive=E:10000,I:7142;disappear=1` | story: The minotaur: supply the blue ooze | [areas/qst/willem.qst:204](../../../areas/qst/willem.qst#L204) |
| 7140 | `give=I:7154;receive=C:200000;disappear=1` | story: The slave: deliver Willem’s named skull | [areas/qst/willem.qst:273](../../../areas/qst/willem.qst#L273) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 7103 | fountain | [areas/qst/willem.qst:2](../../../areas/qst/willem.qst#L2) |
| 7103 | hopeless holy item | [areas/qst/willem.qst:10](../../../areas/qst/willem.qst#L10) |
| 7103 | druid park forest animals | [areas/qst/willem.qst:17](../../../areas/qst/willem.qst#L17) |
| 7120 | willem | [areas/qst/willem.qst:45](../../../areas/qst/willem.qst#L45) |
| 7120 | palace | [areas/qst/willem.qst:51](../../../areas/qst/willem.qst#L51) |
| 7120 | father | [areas/qst/willem.qst:58](../../../areas/qst/willem.qst#L58) |
| 7121 | turol | [areas/qst/willem.qst:69](../../../areas/qst/willem.qst#L69) |
| 7121 | yes story | [areas/qst/willem.qst:76](../../../areas/qst/willem.qst#L76) |
| 7121 | spell | [areas/qst/willem.qst:90](../../../areas/qst/willem.qst#L90) |
| 7121 | favor | [areas/qst/willem.qst:106](../../../areas/qst/willem.qst#L106) |
| 7125 | brooding fate | [areas/qst/willem.qst:141](../../../areas/qst/willem.qst#L141) |
| 7127 | bloodsaber saber | [areas/qst/willem.qst:177](../../../areas/qst/willem.qst#L177) |
| 7130 | horn horns | [areas/qst/willem.qst:194](../../../areas/qst/willem.qst#L194) |
| 7136 | willem | [areas/qst/willem.qst:219](../../../areas/qst/willem.qst#L219) |
| 7137 | willem | [areas/qst/willem.qst:230](../../../areas/qst/willem.qst#L230) |
| 7137 | winterhaven | [areas/qst/willem.qst:238](../../../areas/qst/willem.qst#L238) |
| 7138 | trap fate palace gods tharnadia winterhaven | [areas/qst/willem.qst:246](../../../areas/qst/willem.qst#L246) |
| 7138 | statue ring glory hero heroes | [areas/qst/willem.qst:257](../../../areas/qst/willem.qst#L257) |
| 7140 | slave rest willem | [areas/qst/willem.qst:265](../../../areas/qst/willem.qst#L265) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

199 parsed reset commands: D: 52, E: 29, F: 8, G: 10, M: 77, O: 16, P: 7.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
