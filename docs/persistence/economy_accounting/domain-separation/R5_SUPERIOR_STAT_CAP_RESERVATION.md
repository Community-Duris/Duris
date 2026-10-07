# R5 superior stat-cap reservation — 2026-10-07

Proposed next F3 numeric validation/calculation boundary. No code edits before
coordinator review. R3/R4 remain delivered checkpoints with separate evidence;
the continuing Goal stays ACTIVE through the primary integrated finish line.

## Outcome and actual coupling

`src/item/enhance.c:enhance_stat_cap` still reads the mutable global
`enhance_stat_cap_multiplier` while validating/scaling a captured native base
modifier. The actual all-stat plan calls it immediately after
`enhance_base_modifier` loads/observes/extracts the source prototype. This is the
remaining superior numeric cap rule, distinct from the completed fee and tribute
quotes. Separate its rule into `src/economy/enhancement_stat_rules.h`, taking
explicit native int base modifier and double configured multiplier. Retain the
existing native wrapper, capturing configuration at exactly its original call.
The producer still uses the returned cap in its original skip/target/remaining-step
decisions. Existing prototype and catalog authority is unchanged.

Body SHA256 `8ffea94a07b0b2388cacfb04bc010cac7755a8fe35293f6eef66b83a42834c00`
matches producer `275df7f626e12cb396a22da34317a4e7f355e9a1` and owned source
through R4 `0a5084db1`. Existing stat-bounds regression SHA256
`89a60051a4fcbc2cc2299243c24eb0566afe5321b2a017c77e79f5722d26c7ec`;
current configuration contract SHA256
`44e2f6a01d0ac7ebbc7ec2ccedb1090ddfb58302989452105fbe6939f0ee8e24`.
Unknown private overlap remains explicit; optional import never blocks primary.

## Exact semantics and files

Preserve native int input range, base <=0 returning0, finite positive configured
multiplier requirement, exact double multiplication, signed-byte upper saturation
at SCHAR_MAX and truncation below that bound. A finite positive multiplier may
produce an infinite positive product; that product still saturates as originally.
Do not add a nonfinite-product refusal, ceil, minimum-one, new error/eligibility
policy or broader signed range. Tiny positive products may yield0. Keep the
wrapper's existing int return semantics and all caller ordering.

Four files only: new rule header; the include and original wrapper body in
`src/item/enhance.c`; existing
`tests/async/test_superior_enhancement_stat_bounds.py`; and the cap-rule assertion
in existing `tests/async/test_enhance_stat_config_contract.py`. The latter follows
the actual configured wrapper and real header formula, preserving all fee/config
assertions. Remove replaced arithmetic from the wrapper. No config format or
public native API changes.

Extend the stat regression with the actual header and explicit multiplier cases,
retaining the original complete wrapper bodies, all original ASan/UBSan/
float-cast-overflow/no-PIE controls and 30-second execution deadline. Execute
identical extended expectations on original and extracted wrappers. Cover signed
extremes, fractional truncation, saturation, invalid/nonpositive multipliers,
overflowing finite product, tiny positive products and repeated preparation.
Run actual material builder, superior payment, configuration/all-stat and active
refusal controls. Run maintained SQL and flat builds, formatting and exact source/
ELF pins. Independently check patch applicability/current module type dependencies;
components/builds do not establish genuine active enhancement journeys.

## Ablation and remaining owner facts

Keep only the cap rule and real producer/config/test connection. Omit owned
prototype/stat catalogs, target-selection snapshots, linked-table traversal,
eligible stat capture, source effects, RNG, payment, retirement and recovery.
An owned catalog proposal would require a larger reviewed capture boundary and
proof of original lowest-VNUM, wear compatibility, first matching APPLY value and
remaining-step gap semantics. It is not part of this numeric extraction.

Forge table selection already has a shared five-entry table and bounded indices;
wrapping it adds little independent calculation value, so it is not selected.
Cleric service scaling belongs to the previously declined inactive spell-service
path and remains unchanged. Repair mixes prototype mutation and payment/native
effects under the shop owner; it requires that owner's reviewed facts. Mining
ore/gem valuations include RNG and drop publication and are outside this selected
paid-outcome boundary. C/I/S/A/K/Q/G existing owned plans are reused; shared
authority and quest-prep paths remain untouched. Rerank after this proposed
operation or a meaningful producer advance; no fixed extraction cap or overall
completion is implied.
