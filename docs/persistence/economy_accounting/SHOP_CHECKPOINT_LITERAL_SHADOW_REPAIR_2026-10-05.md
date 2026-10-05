# SHOP checkpoint literal shadow repair — 2026-10-05

The refreshed Plan5 maintained SQL build rejects capture_literal_root's local
vector literal because the enclosing attempt lambda captures a compatible
outer vector with that name. This is a reproduced compile blocker in the
existing warning profile, not a requested warning suppression.

The repair renames only the inner declaration and four uses to tree_literal.
The outer captured vector and completed-checkpoint output remain unchanged.
Literal capture, parent/slot validation, byte budget, original native body,
transactions and refusal semantics stay exact. Reversing the identifier substitution and
changed-line formatting reconstructs the full token source; changed-line clang18 fixed point, source pins,
census and static registration checks are source evidence only.

The exact [peer maintained-build report](PLAN5_7CD9_MAINTAINED_BUILD_HANDOFF_2026-10-05.md)
is imported unchanged from1ad3f71ec. It records a fresh full flat build passing
and fresh SQL build failing on this identifier at its 7cd9/schema56 inputs.
Neither result qualifies later source/schema changes. No local native build,
test, SQL, service or gameplay execution runs in this milestone. Current
combined both-policy builds and checkpoint/native checks remain in the original
major-plan qualification batch. Inactive behavior, wallet item exclusions and
the declined spell path are preserved. Full R1–R8/release remain BLOCKED.
