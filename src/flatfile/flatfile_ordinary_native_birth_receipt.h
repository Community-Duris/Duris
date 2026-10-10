#ifndef DURIS_FLATFILE_ORDINARY_NATIVE_BIRTH_RECEIPT_H
#define DURIS_FLATFILE_ORDINARY_NATIVE_BIRTH_RECEIPT_H

#include "flatfile/flatfile_accounting_store.h"

// Separate CLOSED owner: existing store friends do not inherit this access.
// The genuine future ordinary transaction separately authenticates original
// terminal carrier/native publication before sealing any historical origin.
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
