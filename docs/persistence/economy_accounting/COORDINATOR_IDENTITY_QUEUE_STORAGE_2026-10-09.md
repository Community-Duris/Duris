# Owning coordinator identity queue storage - 2026-10-09

Pending admission and fence queues now use a data-free owning deque type retaining the original ordinary std::deque algorithms and default allocator. On the pinned GCC13 C++11 ABI, qualified protected-grandparent access observes actual allocated map slots, blocks and string capacities through genuine owning objects. No existing object is cast or assigned a guessed layout. Other platforms retain ordinary queue behavior and refuse the new storage observation.

The next-push profile follows actual GCC13 map recenter/growth and end-block policy. It accounts for old/new map overlap, then release of the old map before block and copied-string allocation. Current observation retains actual storage after pop/clear. Outputs are strong and arithmetic checked.

Independent complete RAW source review passed; final token/preprocessor equivalence and exact removal of the class and two queue-type changes reconstruct the original file. Primary source evidence includes installed GCC13 stl_deque.h and deque.tcc hashes in tmp/coordinator-identity-queue-owner-20261009. Isolated registry preserves 931 policies/399 authenticated pins with zero new or unmapped sites.

Complete bounded submission, persistent coordinator ownership joining, genuine backend support/initial registration and recovery integration remain open. This prerequisite is not full bounded admission. Native compilation and tests remain deferred to major-plan readiness. Accounting inactive/CLOSED; coverage incomplete, release BLOCKED, primary goal ACTIVE.
