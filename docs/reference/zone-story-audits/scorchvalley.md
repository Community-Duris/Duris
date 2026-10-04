# The Scorched Valley: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence scorchvalley \
  --evidence-format markdown --output docs/reference/zone-story-audits/scorchvalley.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 71205 | `give=I:71205;receive=I:71218;disappear=0` | story: A family heirloom for Nital's survivor | [areas/qst/scorchvalley.qst:23](../../../areas/qst/scorchvalley.qst#L23) |
| 71236 | `give=I:28980,I:71009,I:71227;receive=I:71226;disappear=0` | story: Three exact heads for Tallin's advisor | [areas/qst/scorchvalley.qst:113](../../../areas/qst/scorchvalley.qst#L113) |
| 71236 | `give=I:71212;receive=I:71213;disappear=0` | request: The turncoat's sack for the advisor | [areas/qst/scorchvalley.qst:89](../../../areas/qst/scorchvalley.qst#L89) |
| 71236 | `give=I:71228;receive=I:71230;disappear=0` | story: Return the bodyguard's essence | [areas/qst/scorchvalley.qst:129](../../../areas/qst/scorchvalley.qst#L129) |
| 71247 | `give=I:71203,I:71204,I:71219;receive=I:71220;disappear=0` | story: Two banners and a hide for history | [areas/qst/scorchvalley.qst:165](../../../areas/qst/scorchvalley.qst#L165) |
| 71247 | `give=I:71250;receive=I:71251;disappear=0` | request: Preserve a Nital dragon souvenir | [areas/qst/scorchvalley.qst:181](../../../areas/qst/scorchvalley.qst#L181) |
| 71253 | `give=I:71240;receive=I:71235;disappear=0` | story: Godly magic for the council advisor | [areas/qst/scorchvalley.qst:206](../../../areas/qst/scorchvalley.qst#L206) |
| 71256 | `give=I:71239;receive=I:71224,I:71244,I:71245,I:71246;disappear=0` | story: Tallin's blood for four perfect rings | [areas/qst/scorchvalley.qst:228](../../../areas/qst/scorchvalley.qst#L228) |
| 71257 | `give=I:71224,I:71244,I:71245,I:71246;receive=I:71248;disappear=0` | story: Four distinct rings for the wildmage | [areas/qst/scorchvalley.qst:256](../../../areas/qst/scorchvalley.qst#L256) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 71205 | horror kill tallin nital village brain divine army betrayed hair turncoat betrayal heirloom ring | [areas/qst/scorchvalley.qst:2](../../../areas/qst/scorchvalley.qst#L2) |
| 71236 | torg grog commander heads head skull skulls dwarven heavenly heaven lord | [areas/qst/scorchvalley.qst:35](../../../areas/qst/scorchvalley.qst#L35) |
| 71236 | yeenoghu thrym council gods miska queen chaos abyss | [areas/qst/scorchvalley.qst:51](../../../areas/qst/scorchvalley.qst#L51) |
| 71236 | bodyguard body guard mount banishment mountain imprison prison essence | [areas/qst/scorchvalley.qst:63](../../../areas/qst/scorchvalley.qst#L63) |
| 71236 | betrayal betrayer sack giant frost wiggly turncoat servant hefty | [areas/qst/scorchvalley.qst:70](../../../areas/qst/scorchvalley.qst#L70) |
| 71236 | tasks duties tallin possiblities worries duties duty task problem problems | [areas/qst/scorchvalley.qst:78](../../../areas/qst/scorchvalley.qst#L78) |
| 71247 | battle sack collecting collect history empty odds ends cataloguing catalogue standards standard hide | [areas/qst/scorchvalley.qst:141](../../../areas/qst/scorchvalley.qst#L141) |
| 71247 | charm dragon nital town souveneir white history historian | [areas/qst/scorchvalley.qst:156](../../../areas/qst/scorchvalley.qst#L156) |
| 71253 | component task spell head commander scorched valley heaven forces force torg council councilor counsel counselor councilors | [areas/qst/scorchvalley.qst:193](../../../areas/qst/scorchvalley.qst#L193) |
| 71256 | something search searching torg council blood vial holy talling grog sacred plot leads seek seeking | [areas/qst/scorchvalley.qst:216](../../../areas/qst/scorchvalley.qst#L216) |
| 71257 | rings ring search quest look my daft wildmage mage | [areas/qst/scorchvalley.qst:246](../../../areas/qst/scorchvalley.qst#L246) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| mob | 71223 | `block_up` | [src/specs/specs.assign.c:184](../../../src/specs/specs.assign.c#L184) |
| mob | 71259 | `yeenoghu` | [src/specs/specs.assign.c:185](../../../src/specs/specs.assign.c#L185) |
| obj | 71231 | `artifact_invisible` | [src/specs/specs.assign.c:1406](../../../src/specs/specs.assign.c#L1406) |

## Reset coverage

211 parsed reset commands: D: 14, E: 22, F: 31, G: 15, M: 114, O: 9, P: 6.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
