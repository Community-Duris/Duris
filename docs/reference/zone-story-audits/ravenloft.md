# Castle Ravenloft: source review index

Generated source evidence and current sidecar classification; this is not gameplay qualification.

```bash
python3 scripts/zone_story_quest_zone_inventory.py --area-evidence ravenloft \
  --evidence-format markdown --output docs/reference/zone-story-audits/ravenloft.md
```

## Native exchanges

| Giver | Exact native terms | Classification | Source |
| ---: | --- | --- | --- |
| 58312 | `give=I:58393,I:58407;receive=I:58408;disappear=1` | story: Vey Rallen: two temporal essences | [areas/qst/ravenloft.qst:18](../../../areas/qst/ravenloft.qst#L18) |
| 58343 | `give=I:58427;receive=I:58427;disappear=0` | Excluded: The shocker lizard accepts and returns the same copper mallet with a squeak. This prop is not a distinct reward quest, personal bell solve or first-source acquisition. Preserve its native accepted receipt without adding a quest achievement or daily. | [areas/qst/ravenloft.qst:29](../../../areas/qst/ravenloft.qst#L29) |
| 58347 | `give=I:58370;receive=I:58416;disappear=0` | story: Perganan: a way out | [areas/qst/ravenloft.qst:88](../../../areas/qst/ravenloft.qst#L88) |
| 58367 | `give=I:58369;receive=I:58401;disappear=1` | story: The Sabbath Wizard: release from bondage | [areas/qst/ravenloft.qst:137](../../../areas/qst/ravenloft.qst#L137) |
| 58381 | `give=I:58346,I:58410;receive=I:58411;disappear=1` | story: Megosh: the two forbidden relics | [areas/qst/ravenloft.qst:162](../../../areas/qst/ravenloft.qst#L162) |

## Dialogue responses

Topics are source aliases, not recorded learned objectives.

| Giver | Aliases | Source |
| ---: | --- | --- |
| 58312 | hi hello | [areas/qst/ravenloft.qst:6](../../../areas/qst/ravenloft.qst#L6) |
| 58312 | presence | [areas/qst/ravenloft.qst:10](../../../areas/qst/ravenloft.qst#L10) |
| 58312 | obyrith | [areas/qst/ravenloft.qst:14](../../../areas/qst/ravenloft.qst#L14) |
| 58347 | hi hello | [areas/qst/ravenloft.qst:36](../../../areas/qst/ravenloft.qst#L36) |
| 58347 | vistani | [areas/qst/ravenloft.qst:40](../../../areas/qst/ravenloft.qst#L40) |
| 58347 | witch witches baba zelenna drowned lady madam eva fane fanes witchs | [areas/qst/ravenloft.qst:44](../../../areas/qst/ravenloft.qst#L44) |
| 58347 | vampire vampires | [areas/qst/ravenloft.qst:49](../../../areas/qst/ravenloft.qst#L49) |
| 58347 | strahd zarovich | [areas/qst/ravenloft.qst:53](../../../areas/qst/ravenloft.qst#L53) |
| 58347 | barovia bildrath orbitian parriwimple stram etush danovich mathilda ghorvax ephon arbury mary | [areas/qst/ravenloft.qst:59](../../../areas/qst/ravenloft.qst#L59) |
| 58347 | hossa | [areas/qst/ravenloft.qst:64](../../../areas/qst/ravenloft.qst#L64) |
| 58347 | valesta | [areas/qst/ravenloft.qst:68](../../../areas/qst/ravenloft.qst#L68) |
| 58347 | shresta | [areas/qst/ravenloft.qst:73](../../../areas/qst/ravenloft.qst#L73) |
| 58347 | bram burns captain | [areas/qst/ravenloft.qst:78](../../../areas/qst/ravenloft.qst#L78) |
| 58347 | urik | [areas/qst/ravenloft.qst:83](../../../areas/qst/ravenloft.qst#L83) |
| 58367 | hi hello | [areas/qst/ravenloft.qst:128](../../../areas/qst/ravenloft.qst#L128) |
| 58367 | indulgence lucian | [areas/qst/ravenloft.qst:133](../../../areas/qst/ravenloft.qst#L133) |
| 58381 | hi hello | [areas/qst/ravenloft.qst:155](../../../areas/qst/ravenloft.qst#L155) |

## Literal special assignments

Comments are omitted; preprocessor branches are source leads, not evaluated runtime state.

| Kind | VNUM | Procedure | Source |
| --- | ---: | --- | --- |
| obj | 58424 | `artifact_shadow_shield` | [src/specs/specs.assign.c:2176](../../../src/specs/specs.assign.c#L2176) |
| obj | 58413 | `ravenloft_bell` | [src/specs/specs.assign.c:2179](../../../src/specs/specs.assign.c#L2179) |
| mob | 58387 | `ravenloft_vistani_shout` | [src/specs/specs.assign.c:2180](../../../src/specs/specs.assign.c#L2180) |
| obj | 58400 | `shimmer_shortsword` | [src/specs/specs.assign.c:2182](../../../src/specs/specs.assign.c#L2182) |

## Reset coverage

937 parsed reset commands: D: 110, E: 191, F: 150, G: 78, M: 316, O: 50, P: 42.
Full arguments, source locations, and prototypes are available with `--evidence-format json`.

Reset declarations are possible sources; counts do not prove that admission, random rolls,
population limits, conditional chains, or accounting permit them to execute.
