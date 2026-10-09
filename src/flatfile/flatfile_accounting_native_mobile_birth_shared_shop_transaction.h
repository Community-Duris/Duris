#ifndef DURIS_FLATFILE_ACCOUNTING_NATIVE_MOBILE_BIRTH_SHARED_SHOP_TRANSACTION_H
#define DURIS_FLATFILE_ACCOUNTING_NATIVE_MOBILE_BIRTH_SHARED_SHOP_TRANSACTION_H

#include "flatfile/flatfile_accounting_store.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_shopkeeper_repository.h"
#include "flatfile/quest_mobile_native_flatfile.h"
#include "economy/native_mobile_birth_recovery.h"
#include <memory>

// Passive exact CURRENT initial birth cut, distinct from retained receipt proof.
// No source, native lifetime, admission, world publication or ACK permission.
struct flatfile_shared_native_birth_projection
{
	quest_mobile_native_image native;
	flatfile_shopkeeper_record keeper;
	bool owner_present = false;
	uint64_t owner_revision = 0;
	std::vector<flatfile_item_ownership_record> custody;
};

class critical_shared_native_flat_execution_owner;
class flatfile_shared_native_birth_publication_storage;
class flatfile_accounting_native_mobile_birth_shared_shop_transaction final
{
    public:
	~flatfile_accounting_native_mobile_birth_shared_shop_transaction();
	flatfile_accounting_native_mobile_birth_shared_shop_transaction(
		const flatfile_accounting_native_mobile_birth_shared_shop_transaction &) = delete;
	flatfile_accounting_native_mobile_birth_shared_shop_transaction &
	operator=(const flatfile_accounting_native_mobile_birth_shared_shop_transaction &) = delete;

    private:
	friend class critical_shared_native_flat_execution_owner;
	friend class flatfile_shared_native_birth_publication_storage;
	// Only the genuine original executing owner calls these methods after
	// current thread/attempt/generation/source/stage validation. It owns root
	// selection and journal recovery BEFORE staging, and retains this SAME
	// borrowed exclusive lock through first commit. Later reconciliation uses
	// a fresh genuine recovered same-root lock, never a stored stack lock.
	// No constructor/value capability, local acquisition or general dispatch.
	// EALREADY requires a complete authentic retained receipt, never SHOP alone.
	// Every preparation refusal preserves the caller's output.
	static unsigned int prepare_locked(
		const std::string &, const flatfile_authority_lock &,
		const critical_native_recovery_envelope &,
		std::unique_ptr<flatfile_accounting_native_mobile_birth_shared_shop_transaction>
			*) noexcept;
	// Retains original ID/envelope/proposal across uncertainty. A subsequent
	// ambiguous call only reconciles original storage; never resubmits stale
	// images, resets IDs or infers rollback. Outer owner rechecks current().
	critical_apply_result commit_locked(const std::string &,
					    const flatfile_authority_lock &) noexcept;
	// Possible publication retains the exact original proposal in the owning
	// queued operation. A later authentic worker borrows a FRESH recovered lock;
	// no old lock pointer is accessed and no old after-image is recommitted.
	critical_apply_result reconcile_locked(const std::string &,
					       const flatfile_authority_lock &) noexcept;
	// Original genuine outcome only. External owner serializes access and
	// never discards a possibly published proposal on any allocation/refusal.
	bool publication_possible() const noexcept;
	// Nonallocating prospective retained-capacity census, strong output. Fixed
	// transaction/state fields and every owned dynamic capacity are included;
	// conservative string capacity+terminator includes SSO. Allocator overhead
	// is not portable; root separately reserves transient execution copies.
	bool retained_bytes(size_t *) const noexcept;
	static critical_apply_result
	verify_retained_locked(const std::string &, const flatfile_authority_lock &,
			       const critical_native_recovery_envelope &) noexcept;
	static unsigned int read_current_locked(const std::string &,
						const flatfile_authority_lock &,
						const critical_native_recovery_envelope &,
						const critical_completion &,
						flatfile_shared_native_birth_projection *) noexcept;
	static unsigned int verify_record_locked(const std::string &,
						 const flatfile_authority_lock &,
						 const critical_native_recovery_envelope &,
						 flatfile_accounting_record *) noexcept;
	struct implementation;
	std::unique_ptr<implementation> state_;
	explicit flatfile_accounting_native_mobile_birth_shared_shop_transaction(
		std::unique_ptr<implementation>);
};

// Passive locked native/keeper/custody observation, never a publication permit.
// The original world owner separately authenticates its actual coordinator,
// stage, source, runtime and once-only publication/ACK lifetime.
struct flatfile_shared_native_birth_publication_projection
{
	quest_mobile_native_image native;
	flatfile_shopkeeper_record keeper;
	bool owner_present = false;
	uint64_t owner_revision = 0;
	std::vector<item_ownership_runtime_entry> custody;
};

class quest_mobile_native_birth_owner;
class shop_trade_current_runtime_owner;
class flatfile_shared_native_birth_publication_storage final
{
	friend class quest_mobile_native_birth_owner;
	friend class shop_trade_current_runtime_owner;
	// Borrow the SAME genuine recovered root lock and full original carrier.
	// Reauthenticates receipt/source/epoch/native/keeper/whole-catalog custody;
	// no acquire/recovery/write/commit, execution-owner mint, world or ACK.
	// Every refusal preserves output.
	static unsigned int
	read_locked(const std::string &, const flatfile_authority_lock &,
		    const critical_native_recovery_envelope &, const critical_completion &,
		    flatfile_shared_native_birth_publication_projection *) noexcept;
};

#endif
