#!/usr/bin/env python3
"""Source contract for the colorful, grouped staff divineclaim listing."""

from _paths import SRC


source = (SRC / "account_reward.c").read_text()
list_start = source.index("static bool list_grants")
list_end = source.index("static std::vector<RewardGrant> grants_for_removal", list_start)
listing = source[list_start:list_end]

assert "&+CDivine Account Rewards &+Y(%zu)&n" in listing
assert "for (size_t group_start = 0; group_start < grants.size();)" in listing
assert "grants[group_end].account == grants[group_start].account" in listing
assert "  &+CID     Reward" in listing
assert "&+W%7s&n &+W%-42s&n &+C%6d&n" in listing
assert "compact_duration(grant.age_seconds)" in listing
assert "divineclaim_lifetime_text(grant)" in listing
assert "std::to_string(instance_count)" in listing
assert "send_divineclaim_instance_lines(ch, instances);" in listing
assert "instance_count < 0" in listing
assert "&+Runavailable&n" in listing

# The old unbounded key/value row should not return.
assert "account=%s reward=%s" not in listing
assert "instances=%s" not in listing

print("account reward grouped/colorful list format contract: ok")
