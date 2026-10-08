#ifndef QUEST_MOBILE_NATIVE_FLATFILE_H
#define QUEST_MOBILE_NATIVE_FLATFILE_H

#include "flatfile/flatfile_authority_transaction.h"
#include "economy/native_mobile_birth_recovery.h"
#include "world/quest_mobile_native.h"

struct quest_mobile_native_flatfile_row
{
	uint64_t mobile_instance_id = 0;
	bool present = false;
	quest_mobile_native_image image;
};

// Values only: zero proves a bounded canonical read under the supplied exact
// root lock, not authenticated birth/source/epoch/admission or ACK authority.
// The outer owner must first recover the shared authority bundle under this lock.
// A missing file is zero with present=false; corruption never becomes absence.
// All outputs remain unchanged on any nonzero return.
int quest_mobile_native_flatfile_read_locked(const std::string &root,
					     const flatfile_authority_lock &,
					     uint64_t mobile_instance_id,
					     quest_mobile_native_flatfile_row *output) noexcept;

// Rereads the entire original before-image and returns only a canonical domains
// write operation. The caller validates its admitted transition/revision plan
// and commits this together with player removals, custody, original evidence,
// receipt/outbox and journal cleanup in ONE existing authority bundle.
// No recovery, commit, directory creation, IDs, world mutation or ACK occurs.
// RETIRED remains a durable image file, never a remove or implicit revival.
// Cash-aware birth and ordinary transitions require known v2 cash with the
// checked cash/mobile revision policy. Historical unknown cash is read-only here.
int quest_mobile_native_flatfile_prepare_locked(
	const std::string &root, const flatfile_authority_lock &,
	const critical_operation_id &original_parent,
	const quest_mobile_native_flatfile_row &retained_before,
	const quest_mobile_native_image &after, flatfile_authority_operation *output) noexcept;

// Immutable constructor evidence only, not another native/UID/custody catalog.
// Historical absence is explicitly unknown. The full original command is read
// from the exact terminal NMR1 bytes, never reconstructed from current templates.
struct quest_mobile_native_flatfile_origin_row
{
	bool present = false;
	critical_native_recovery_envelope original;
};

// The caller first recovers the existing authority bundle under this root lock.
// Requires the actual current native image to exist and match this exact reference;
// immutable lifetime facts match the retained birth while revisions may advance.
// Values only: receipt/source/current custody/world proof is still caller-owned.
// A missing origin is present=false, not permission to invent constructor values.
// Corruption/nonregular files refuse; every refusal leaves output unchanged.
int quest_mobile_native_flatfile_origin_read_locked(
	const std::string &root, const flatfile_authority_lock &,
	const quest_mobile_native_reference &, quest_mobile_native_flatfile_origin_row *) noexcept;

// Requires the ACTUAL coordinator terminal phase2 envelope and the complete
// current born image. The caller separately authenticates the genuine original
// receipt, root/source/wallet and complete initial custody under this SAME lock.
// Returns only a domains WRITE operation for the existing authority bundle.
// Exact existing bytes/revision are idempotent; conflicting origin refuses.
// Caller must keep advancement gated, confirm bundle commit (or recover/recheck
// after uncertainty/lost reply), then retire the journal. This primitive does
// not recover, commit, issue IDs, authenticate caller evidence, mutate the world
// or grant ACK/retirement authority. Every refusal leaves output unchanged.
int quest_mobile_native_flatfile_origin_prepare_locked(const std::string &root,
						       const flatfile_authority_lock &,
						       const critical_native_recovery_envelope &,
						       flatfile_authority_operation *) noexcept;

#endif
