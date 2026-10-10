#ifndef DURIS_ARTIFACT_NATIVE_BIRTH_H
#define DURIS_ARTIFACT_NATIVE_BIRTH_H
#include "flatfile/flatfile_authority_transaction.h"
struct obj_data;
struct char_data;
// Private accepted warm NPC enrollment companion. Caller holds original ROOT,
// full native forest proof, identity then same authority lock and game thread.
// Current report cache must be registered once with the common ROOT observer.
// No PC/corpse/salvage or SQL backend substitution. Nonzero errno stops owner;
// actual returned/succeeded survive later diagnostic/cache admission refusal.
// Already returned refuses replay, including failed/uncertain rename outcomes.
int artifact_native_birth_location_bounded(const std::string &, const flatfile_authority_lock &,
					   obj_data *, char_data *, bool *, bool *,
					   bool (*)(size_t, void *) noexcept, void *,
					   size_t) noexcept;
// Distinct native-flat companion for the actual shell extractor. Full original
// bool service result stays separate from the real completion witness.
int artifact_native_birth_shell_remove_owned_bounded(obj_data *, int, bool *, bool *,
						     bool (*)(size_t, void *) noexcept, void *,
						     size_t) noexcept;

#endif
