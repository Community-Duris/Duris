# Authentic refusal cleanup admission - 2026-10-09

The private coordinator now provides complete bounded refusal cleanup. It preserves original full native command/BODY, receipt, generation, delivered never-admitted and lifecycle guards. Actual frozen clone, operation key and canonical comparison allocations are admitted before pinning the operation. Genuine native cleanup runs outside the coordinator mutex with its complete live prefix.

Distinct caller-owned cleanup_called and cleanup_succeeded markers record the real callback effect immediately before later lock/rechecks. A successful native destruction followed by coordinator removal failure therefore cannot justify repeating destruction. Caller marker/context storage must survive native-owner cleanup. Original allocation-free fence/removal and health-notification tail remains intact; no journal rewrite or ACK is added.

Independent complete RAW review passed. The reviewed append was reanchored without semantic changes above the published queue fix; exact insertion inverses and formatted tokens/preprocessing/original C prefix passed. Evidence: tmp/coordinator-refusal-bounded-owner-20261009, tmp/coordinator-refusal-integration-owner-20261009 and tmp/coordinator-refusal-integrated-20261009. Registry: 931 policies/399 authenticated pins, zero new/unmapped sites.

Full native cleanup provider and pulse integration, completion/submission/recovery and paired pool census remain open. Native builds/tests stay deferred to major-plan readiness. Accounting inactive/CLOSED; coverage incomplete, release BLOCKED, primary goal ACTIVE. Source review is not runtime qualification.
