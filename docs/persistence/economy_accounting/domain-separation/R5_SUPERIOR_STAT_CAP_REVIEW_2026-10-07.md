# Superior stat-cap numeric boundary review - 2026-10-07

Published reservation `fca852db9b0d51864f24f415a086e6341c17d931` is approved
for its four-file numeric rule extraction. This is an independent preparation
boundary under the continuing charter, not native enhancement support.

The coordinator inspected the real wrapper, its all-stat plan caller, the
prototype observation/cleanup and existing stat/config regressions. Its complete
cap body remains identical to producer
`275df7f626e12cb396a22da34317a4e7f355e9a1` through R4. Independently authenticated
SHA256 `8ffea94a07b0b2388cacfb04bc010cac7755a8fe35293f6eef66b83a42834c00`;
stat regression `89a60051a4fcbc2cc2299243c24eb0566afe5321b2a017c77e79f5722d26c7ec`;
current config contract
`44e2f6a01d0ac7ebbc7ec2ccedb1090ddfb58302989452105fbe6939f0ee8e24`.

Move only the numeric cap law into `src/economy/enhancement_stat_rules.h`, with
explicit native int base modifier and configured double multiplier. Retain the
original wrapper and capture the configuration at its existing call point after
the original prototype observation/cleanup. Keep all skip, target, catalogue,
remaining-step, material, payment and effect ordering in the producer.

Preserve base<=0 and nonfinite/nonpositive multiplier returning0, exact double
multiplication, upper saturation at SCHAR_MAX and truncation below it. In
particular, a finite positive multiplier whose product overflows to positive
infinity still saturates. A new product-finiteness rejection would change the
original behavior. Tiny positive products may yield0; no ceil, minimum-one,
eligibility change, broader input range or error policy is added.

Touch only the new header, wrapper/include, actual stat-bounds regression and
the relocated cap assertion in the existing configuration contract. Verify the
actual wrapper arguments and real header formula; preserve unrelated fee/config
assertions. Execute identical extended expectations on original and extracted
complete wrappers, retaining ASan/UBSan/float-cast-overflow/no-PIE flags and the
30-second deadline. Cover signed boundaries, finite overflowing product,
fractional/truncated/zero results, invalid multipliers, saturation and repeats.
Retain material-builder/payment/config/all-stat/active-refusal checks and exact
maintained SQL/flat build, formatting, current import/type and source/ELF proof.

Prototype/stat tables, lowest-VNUM/wear/APPLY selection, source identities, RNG,
output effects, debit/refund, retirement, persistence and recovery stay with
their existing owners. The already-shared forge price table requires a separate
concrete gap to justify any work. After this delivery reassess all nine domains
and current owner dependencies; R5 cannot complete the continuing Goal.

## Subsequent primary and import checkpoint

Primary `35298aacdf0b55e5026db98103f4e3e7b63094ad` adds acknowledged room-coin
cold restoration and the 754th maintained provider. Its enhancement/stat test
preimages remain raw unchanged from275; this numeric boundary remains approved.
The coordinator preserves that primary change while publishing documentation.

Implementation `d45402ca508b2f984e612e6dd6699d4355bbe22e` plus include-placement
successor `b31809a0d7f15c9318861d66677a3717c0d9b838` is under qualification.
An independent private-index check against bare352 shows production patch
application passes, but the full source/test delta fails at the configuration
contract's R3-relocated fee context. Declare that test prerequisite or provide
a separately checked minimal cap-contract adaptation preserving the primary's
original fee expectations. Source independence is not whole-bundle independence.
No primary source or completed R3 rule is changed to resolve this packaging issue.
