# Genuine coordinator current-storage lender - 2026-10-09

The actual coordinator lender authenticates a held unique_lock on the exact coordinator mutex before observing its complete current physical storage. Registered ROOT callbacks borrow that current ownership, add it once to the actual full caller prefix and end the scalar borrow on both callback outcomes. The original unregistered path retains its complete prefix and original callback behavior.

The real bounded submit relay now uses this lender. Initialization clears passive scalar registration only after the original lifecycle guards and before journal replay; an active guard or borrow prevents that reset. No outside observer or new producer route is selected by this change.

Complete independent RAW and final formatted source review passed. Exact source inverse, changed-line token preservation and three protected-file hashes passed. All 400 source pins authenticate; 931 writer policies remain unchanged with zero new or unmapped sites. Evidence: tmp/coordinator-shared-lender-owner-20261009 and tmp/coordinator-shared-lender-integrated-20261009.

Checkpoint, ACK, retirement and remaining locked companions still need genuine lender integration before full-seven selection. Startup replay, full pulse and native qualification remain open. Native execution stays deferred to major-plan readiness; accounting inactive/CLOSED, coverage incomplete, release BLOCKED, primary goal ACTIVE.
