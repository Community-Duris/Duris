#ifndef DURIS_FLATFILE_ORDINARY_NATIVE_BIRTH_RECEIPT_H
#define DURIS_FLATFILE_ORDINARY_NATIVE_BIRTH_RECEIPT_H

#include "flatfile/flatfile_accounting_store.h"
#include "economy/native_mobile_birth_recovery.h"

// Separate CLOSED owner: existing store friends do not inherit this access.
// The genuine future ordinary transaction separately authenticates original
// terminal carrier/native publication before sealing any historical origin.
// Unsealed same-cut history counts, never a financial/native/activation permit.
struct flatfile_ordinary_native_birth_economic_history_counts
{
	critical_operation_id lineage = {};
	uint64_t lineage_revision = 0;
	economic_digest epochs_digest = {};
	uint32_t retained_epochs = 0, segments_verified = 0, baseline_indexes_verified = 0;
	uint16_t buckets_verified = 0, initialized_buckets = 0, clear_buckets = 0;
	uint64_t records_verified = 0, successful_records = 0, failed_records = 0,
		 item_events_verified = 0, baseline_witness_rows_verified = 0,
		 baseline_item_reservations_verified = 0, birth_records_verified = 0,
		 creation_events_verified = 0, birth_account_effects_verified = 0,
		 birth_coin_postings_verified = 0;
	size_t born_uids = 0;
};

class flatfile_ordinary_native_birth_receipt_storage final
{
    private:
	friend class flatfile_accounting_native_mobile_birth_ordinary_transaction;
	// Caller resolves original journals BEFORE this SAME borrowed root lock.
	// Full canonical immutable command/intent/EAP1/MBR4/item-reference evidence,
	// selected exact sourceclaim and retained epoch/mapping proof are required.
	// Historical retirement is readable independently of CURRENT selection/cash.
	// Strong output. No recovery/acquire/stage/write/world/publication/ACK.
	// Physical custody/ledger, immutable terminal carrier and cross-sourceclaim
	// global census are separate owner joins; no SQL evidence parity is claimed.
	// Real original allocating readers/codecs; no memory bound is claimed.
	static flatfile_accounting_status
	verify_retained_current_locked(const std::string &, const flatfile_authority_lock &,
				       const critical_operation_id &original_birth,
				       flatfile_accounting_record *, std::string *);

	// Complete INITIAL immutable economic history under the SAME recovered
	// root lock. Genuine canonical ordinary INITIAL, all baseline books and
	// reservations, every initialized sealed/active segment and clear-bit
	// original namespace fences; any retained birth operation/outcome or born
	// UID event/baseline witness refuses. Source claims count zero operation-wide.
	// Strong informational output; no recovery/acquire/write/native selection.
	static flatfile_accounting_status verify_initial_history_absence_locked(
		const std::string &, const flatfile_authority_lock &,
		const critical_native_recovery_envelope &,
		flatfile_ordinary_native_birth_economic_history_counts *, std::string *);

	// Complete canonical retained receipt plus same-cut immutable history and
	// literal creation-event derivation, whole-operation source claims count1,
	// and unique born UID/revision1 creation events across every retained root.
	// Legitimate future baseline before==after witnesses with no event remain
	// readable. Output record comes only from actual storage and is rechecked
	// byte-for-byte during the complete scan. Both outputs remain unchanged on
	// refusal. Original allocating codecs, no full memory/native qualification.
	// No recovery/acquire/write/publication/ACK/activation authority follows.
	static flatfile_accounting_status verify_retained_history_current_locked(
		const std::string &, const flatfile_authority_lock &, const critical_operation_id &,
		flatfile_accounting_record *,
		flatfile_ordinary_native_birth_economic_history_counts *, std::string *);

	// Complete original sourceclaim namespace census, under the SAME recovered
	// root lock: INITIAL requires selected-source absence and operation count0;
	// retained proof requires the exact selected bytes and operation count1.
	// All canonical claims, including foreign lineage/events, are authenticated.
	// These passive leaves do not replace full financial/physical/terminal proof.
	// No recovery/acquire/write/native/publication/ACK or memory-bound claim.
	static flatfile_accounting_status
	verify_source_claim_absent_locked(const std::string &, const flatfile_authority_lock &,
					  const critical_command &, std::string *);
	static flatfile_accounting_status
	verify_source_claim_current_locked(const std::string &, const flatfile_authority_lock &,
					   const flatfile_accounting_record &, std::string *);
};

// Separate CLOSED historical owner: only the actual receipt owner may enter.
// No original authority friendship becomes a historical proof capability.
class flatfile_ordinary_native_birth_history_storage final
{
    private:
	friend class flatfile_ordinary_native_birth_receipt_storage;
	static unsigned int verify_locked(const std::string &, const flatfile_authority_lock &,
					  const economic_account_key &original_wallet,
					  const critical_operation_id &birth_epoch,
					  const critical_operation_id &birth_operation,
					  uint64_t native_id, std::string *);
};

#endif
