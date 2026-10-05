# SQL SHOP current revision cache — 2026-10-05

The private current-SQL publisher rejected every missing shop revision cache
entry. The sole bulk initializer is called by flat shopkeeper restoration;
fresh SQL startup never initializes this cache. An otherwise authenticated SQL
receipt/current projection therefore could not complete its first publication.

## Corrected original owner

Only the private `shop_trade_current_runtime_owner::publish` accepts absence.
A cached revision greater than the current locked native revision still refuses.
After complete custody-batch value checks, it allocates a node containing that
actual current revision and reserves live map capacity before hydration effects.
The node remains private until complete runtime/root census and owner readback,
then the original SQL session/IN_TRANS proof precedes transfer into the live map.
Existing entries retain their original final-update boundary. No observer or
native handler callback intervenes between staging and insertion.

Failure before final transfer destroys the private node without publishing a
missing entry. Hydration progress preserves the existing retry model. Root adds
the explicit utility include for the new node transfer; all other functions and
inactive/flat behavior remain exact. No public setter, historical payload/result
seed, schema change, admission override or activation is introduced. The original
receipt/current SQL/native publisher and guarded ACK remain the authority owners.

## Evidence scope and remaining work

Implementation-owner inverse/raw-preimage/changed-line clang18 checks and primary
independent complete-diff/current-source review passed. The frozen worker proposal
stays unchanged; the installed successor adds only the explicit include. Source
pins, complete unchanged writer policies/census and regenerated matrix record
this actual milestone. No compilation, tests, database, native, gameplay,
persistence or recovery execution occurs; original major-plan testing is deferred.

Fresh SQL publication/retry, newer-cache refusal, allocation/refusal and original
live/cold guarded ACK remain executable qualification work on the complete
candidate. The cold SHOP owner still needs replay registration, unique native
rebind/absence, exact full-world publication and guarded hold/ACK completion.
Coherent0057–0060 remains private with engine metadata unmeasured; native
reset/spawn/birth/stock/restore/retirement and full R1–R8 remain open. Release
and activation remain BLOCKED.
