# Owning coordinator table storage - 2026-10-09

Operations and fence tables now own the exact underlying table used by the original supported GCC13 std::unordered_map. Default allocator, hash/equality, node-cache, range and prime-rehash policies remain identical. Ordinary methods use those original algorithms, including exact transparent lookup forwarding. Unsupported platforms retain the original unordered_map alias.

The new data-free owner observes actual current bucket/node/key capacities and obtains the genuine current rehash policy through its public owning accessor. Next insertion runs the real policy on a copy, admitting actual node/key/mapped-constructor requests and old/new bucket overlap. No existing object is cast, no primes or hidden policy state are guessed, and no probe allocation occurs. Mapped-value heaps remain owned by their separate census.

Independent complete RAW source review passed; formatting preserves tokens/preprocessing and the exact class/two-type inverse. Evidence: tmp/coordinator-native-map-owner-20261009 and tmp/coordinator-native-map-integrated-20261009. Registry: 931 policies/399 authenticated pins, zero new/unmapped sites.

Full coordinator retained-storage observation, genuine initial callback and bounded native submission/shared-root handoff remain open. This prerequisite is not full submission. Native compilation/tests stay deferred to major-plan readiness. Accounting inactive/CLOSED, coverage incomplete, release BLOCKED, primary goal ACTIVE.
