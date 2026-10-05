#ifndef QUEST_MOBILE_NATIVE_FLATFILE_H
#define QUEST_MOBILE_NATIVE_FLATFILE_H

#include "flatfile/flatfile_authority_transaction.h"
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

#endif
