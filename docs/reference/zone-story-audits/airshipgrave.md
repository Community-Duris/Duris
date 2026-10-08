# The Mountain Valley of Dawndale: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence airshipgrave \
  --evidence-format markdown --output docs/reference/zone-story-audits/airshipgrave.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 77501 | `give=C:250000;receive=I:77501;disappear=0` | service: The exile's city-key purchase | [areas/qst/airshipgrave.qst:2](../../../areas/qst/airshipgrave.qst#L2) |
| 77502 | `give=I:77550;receive=E:250000,I:77562;disappear=1` | story: Plunder for the orc treasure-hunter | [areas/qst/airshipgrave.qst:8](../../../areas/qst/airshipgrave.qst#L8) |
| 77504 | `give=I:77508;receive=I:77508;disappear=0` | Excluded: The fossil exchange is a same-kind replacement and referral to an unnamed Ultarium cousin, with no reviewed exact follow-up consumer. It is not a quest achievement, unchanged item identity, new source supply or required campaign stage. | [areas/qst/airshipgrave.qst:21](../../../areas/qst/airshipgrave.qst#L21) |
| 77504 | `give=I:77523;receive=C:250000,I:77524;disappear=0` | service: Prepare the rock cutter's combustible dust | [areas/qst/airshipgrave.qst:30](../../../areas/qst/airshipgrave.qst#L30) |
| 77505 | `give=I:77517;receive=I:77516;disappear=0` | story: A hunter's bow for Keenfeather | [areas/qst/airshipgrave.qst:41](../../../areas/qst/airshipgrave.qst#L41) |
| 77511 | `give=I:77501,I:77525;receive=I:34464,I:77563;disappear=1` | story: A city key and device for a new home | [areas/qst/airshipgrave.qst:60](../../../areas/qst/airshipgrave.qst#L60) |
| 77511 | `give=I:77522;receive=I:77523;disappear=0` | story: A portrait for the refugees | [areas/qst/airshipgrave.qst:49](../../../areas/qst/airshipgrave.qst#L49) |
| 77514 | `give=I:77524;receive=I:77525;disappear=1` | story: An engineer's final explosive device | [areas/qst/airshipgrave.qst:82](../../../areas/qst/airshipgrave.qst#L82) |
| 77517 | `give=I:77515,I:77515,I:77524,I:77549,I:77551,I:77556,I:77557;receive=I:77548;disappear=0` | story: The Astral Dancer captain's supplies | [areas/qst/airshipgrave.qst:101](../../../areas/qst/airshipgrave.qst#L101) |
| 77518 | `give=I:77515,I:77515,I:77524,I:77549,I:77551,I:77556,I:77557;receive=I:77558;disappear=0` | story: The Dlalgarvara captain's supplies | [areas/qst/airshipgrave.qst:113](../../../areas/qst/airshipgrave.qst#L113) |
| 77527 | `give=I:77513;receive=E:250000,I:77552;disappear=0` | story: Rock-worm larvae for Drandagger | [areas/qst/airshipgrave.qst:131](../../../areas/qst/airshipgrave.qst#L131) |
| 77543 | `give=C:25000,I:21673;receive=I:77566;disappear=0` | service: The smith's fine-sand lens commission | [areas/qst/airshipgrave.qst:159](../../../areas/qst/airshipgrave.qst#L159) |
| 77558 | `give=I:40778;receive=I:40779;disappear=1` | story: Return Emition's flute to Whetstone | [areas/qst/airshipgrave.qst:180](../../../areas/qst/airshipgrave.qst#L180) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 77527 | hi hello help assistance | [areas/qst/airshipgrave.qst:125](../../../areas/qst/airshipgrave.qst#L125) |
| 77543 | lens telescope | [areas/qst/airshipgrave.qst:144](../../../areas/qst/airshipgrave.qst#L144) |
| 77543 | pile fine sand | [areas/qst/airshipgrave.qst:151](../../../areas/qst/airshipgrave.qst#L151) |
| 77558 | metal | [areas/qst/airshipgrave.qst:171](../../../areas/qst/airshipgrave.qst#L171) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |

## Reset coverage

321 parsed reset commands: D: 38, E: 54, F: 7, G: 35, M: 161, O: 20, P: 6.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
