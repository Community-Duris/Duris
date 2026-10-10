#ifndef DURIS_FLATFILE_ACCOUNTING_NATIVE_MOBILE_BIRTH_ORDINARY_TRANSACTION_H
#define DURIS_FLATFILE_ACCOUNTING_NATIVE_MOBILE_BIRTH_ORDINARY_TRANSACTION_H

#include "flatfile/flatfile_accounting_store.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_native_mobile_wallet.h"
#include "economy/native_mobile_birth_cash_role_result.h"
#include "economy/native_mobile_birth_recovery.h"
#include <memory>

// Informational exact original birth CURRENT cut, never a world/ACK permit.
struct flatfile_ordinary_native_birth_projection
{
	flatfile_native_mobile_wallet_current wallet;
	uint64_t owner_revision = 0;
	std::vector<flatfile_item_ownership_record> custody;
};

class critical_ordinary_native_flat_execution_owner;
class flatfile_native_mobile_birth_ordinary_publication_storage;
class flatfile_accounting_native_mobile_birth_ordinary_transaction final
{
    public:
	~flatfile_accounting_native_mobile_birth_ordinary_transaction();
	flatfile_accounting_native_mobile_birth_ordinary_transaction(
		const flatfile_accounting_native_mobile_birth_ordinary_transaction &) = delete;
	flatfile_accounting_native_mobile_birth_ordinary_transaction &
	operator=(const flatfile_accounting_native_mobile_birth_ordinary_transaction &) = delete;

    private:
	friend class critical_ordinary_native_flat_execution_owner;
	friend class flatfile_native_mobile_birth_ordinary_publication_storage;
	// Sole genuine executing owner authenticates thread/attempt/generation,
	// full immutable original source and live-world exclusion BEFORE entry.
	// It borrows identity THEN authority, recovers original journals BEFORE
	// this cut, and keeps the SAME locks/writer exclusion through first commit.
	// No public constructor, mintable DTO, boolean bypass, lock acquisition,
	// selector, activation, terminal, publication or ACK entry exists here.
	// EALREADY requires the full actual receipt/history, never native alone.
	// Every preparation refusal leaves output unchanged.
	static unsigned int
	prepare_locked(const std::string &, const flatfile_identity_lock &,
		       const flatfile_authority_lock &, const critical_native_recovery_envelope &,
		       std::unique_ptr<flatfile_accounting_native_mobile_birth_ordinary_transaction>
			       *) noexcept;
	// First commit clears BOTH borrowed stack-lock identities before the
	// genuine existing journal cut. Publication uncertainty retains the same
	// full command/attachment/operations/proposal. Never recommit old images.
	critical_apply_result commit_locked(const std::string &, const flatfile_identity_lock &,
					    const flatfile_authority_lock &) noexcept;
	// Fresh genuine recovered identity->authority cut, no stored-lock access
	// or proposal writes. Outer owner rechecks its actual current lifetime.
	critical_apply_result reconcile_locked(const std::string &, const flatfile_identity_lock &,
					       const flatfile_authority_lock &) noexcept;
	bool publication_possible() const noexcept;
	// Original owned capacities only; not transient/native/32MiB qualification.
	// Strong nonallocating output; fixed MBR4 is part of sizeof(state).
	bool retained_bytes(size_t *) const noexcept;
	// Available only after authentic successful CURRENT verification in this
	// same operation. Pure typed values grant no root/world publication right.
	const native_mobile_birth_cash_role_result *ordinary_wallet_result() const noexcept;
	static critical_apply_result
	verify_retained_locked(const std::string &, const flatfile_identity_lock &,
			       const flatfile_authority_lock &,
			       const critical_native_recovery_envelope &) noexcept;
	static unsigned int
	read_current_locked(const std::string &, const flatfile_identity_lock &,
			    const flatfile_authority_lock &,
			    const critical_native_recovery_envelope &, const critical_completion &,
			    flatfile_ordinary_native_birth_projection *) noexcept;
	static unsigned int verify_record_locked(const std::string &,
						 const flatfile_identity_lock &,
						 const flatfile_authority_lock &,
						 const critical_native_recovery_envelope &,
						 flatfile_accounting_record *) noexcept;
	struct implementation;
	std::unique_ptr<implementation> state_;
	explicit flatfile_accounting_native_mobile_birth_ordinary_transaction(
		std::unique_ptr<implementation>);
};

// Private game-owner bridge to the complete original CURRENT storage proof.
// Caller authenticates its original envelope/generation/delivered receipt and
// actual lifecycle, then borrows same-root identity->authority locks, recovers
// the authority journal, and consumes the projection under that SAME interval.
// The projection conveys no world/publication/ACK or execution-lease authority.
// This facade acquires no locks and performs no preparation or financial write.
class flatfile_native_mobile_birth_ordinary_publication_storage final
{
    private:
	friend class quest_mobile_native_birth_owner;
	static unsigned int
	read_current_locked(const std::string &, const flatfile_identity_lock &,
			    const flatfile_authority_lock &,
			    const critical_native_recovery_envelope &, const critical_completion &,
			    flatfile_ordinary_native_birth_projection *) noexcept;
};

#endif
