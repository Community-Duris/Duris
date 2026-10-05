# Guarded original SHOP refusal callback — 2026-10-05

The retained SHOP pipeline cancelled a `never_admitted` obligation before calling
its private native callback. That preserved the live no-effect cancellation
route, but could not safely support cleanup of an original produced tree during
cold recovery. Moving a callback ahead of cancellation would also run it before
the coordinator authenticated the exact stored refusal.

The private cancellation interface now proves the complete original command and
admission-failure receipt under the coordinator mutex, pins the coordinator
generation and existing publication counters, then invokes the private cleanup
callback outside both owner mutexes. It rechecks the exact original state and
held reservation before releasing fences and consuming the hold. A false return
or failed post-callback proof retains the operation, refusal and all fences;
allocation failure clears only the transient pin/counters. Shutdown/cutover
exclusion continues through hold consumption. No execution receipt, journal
checkpoint, completed-operation cache or successful-execution count is invented.

Independent review rejected the first four-file draft because the existing live
`no_native_effect` callback returns false and had previously been bypassed. The
accepted five-file successor adds a separate private
`original_refusal_no_native_effect` callback, restricted to a valid original
never-admitted completion, matching operation and null context. Only the existing
live retirement uses it; its retained produced-item/notification continuation
still follows cancellation. The original false callback remains unchanged.
Cold recovery must supply its actual cleanup callback and cannot use this route
as physical cleanup proof.

Accepted private source pins: `88f38201a79c9c822b5d62f180129a7ae183897121c051b2d001e1bb52d7af70`.
Exact forward/inverse patch: `c0ae92cb7bf1a962d68819afe66ca4e9543c6cf242f674859cdf71c9da9cdf3b`.
All five original Git preimages, source pins and changed-line clang18 fixed
points were checked. Writer classifications are unchanged; their existing
locations are remapped through byte-identical source lines. Static contract and
matrix results are recorded in the milestone receipt.

No native build/test, gameplay, SQL/service, persistence or recovery execution
ran. Qualification stays in the original major-plan batch. The nine-file cold
SHOP draft remains private: its exact SQL never-admitted BEFORE reader, original
cleanup caller, sealed procedure-binding closure, flat parity and native
recovery/ACK qualification remain unfinished. This shared interface does not
complete Plan4, R1–R8 or any release/activation gate. Inactive behavior, current
admission refusal and the declined spell path remain.
