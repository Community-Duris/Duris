# Durable custody for raised followers

Migration `0028_pet_custody` adds a nullable, unique `player_pets.pet_uid` and
allows owner type 11 in the three item authority tables. Existing pet rows keep
`NULL` and their existing player owned equipment route. The migration and its
verification script are additive and rerunnable on MariaDB 10.11 and MySQL 8.

For a committed player corpse raise that produces a follower, the corpse key
becomes the stable pet UID. The corpse transaction creates the pet record,
transfers every durable nested item to that pet owner, writes its physical item
graph, and records the commit receipt together. A restart can load that graph
before a later player save. The initial pet record includes its charm duration
and absolute charm and death deadlines so an immediate restart does not extend
its lifetime. The existing chance for a summon to be hostile is
resolved before the transaction: hostile creatures have no pet record and keep
the previous caster custody route. Corpse money still follows the spell's
existing wallet rule; transient objects are retired.

A player save verifies every UID, root, parent, and vnum against pet custody
before replacing the pet item projection. A save that omits a pet with a
committed owner revision places its row on `custody_pending` hold instead of
deleting its equipment. A fresh authoritative login is needed before normal
saving resumes after a committed raise whose live publication failed. Repeated
loads and saves must retain one pet record and one copy of each UID.

## Rollback and recovery

Do not restore the old owner type checks or drop `pet_uid` while type 11 rows
exist. An older server cannot interpret those owners. First stop writers and
take a verified database backup. Reconcile each pet's UID graph to an explicit
destination under item authority, checking its physical rows and player
snapshot, then confirm no type 11 owners, pet receipts, or pending holds need
replay. Only then can a separately reviewed down migration remove the new
schema. Retaining the additive schema while rolling back application code is
safe only when there are no pet custody records. Never delete a held pet row or
its items merely to clear a load or save error.
