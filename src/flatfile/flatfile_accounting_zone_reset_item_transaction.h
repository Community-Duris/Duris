#ifndef DURIS_FLATFILE_ACCOUNTING_ZONE_RESET_ITEM_TRANSACTION_H
#define DURIS_FLATFILE_ACCOUNTING_ZONE_RESET_ITEM_TRANSACTION_H

#include "flatfile/flatfile_accounting_store.h"
#include "flatfile/flatfile_item_repository.h"
#include "economy/zone_reset_item_origin.h"
#include "economy/zone_reset_item_recovery.h"
#include <memory>

// Passive exact CURRENT initial ROOM birth projection. The actual world owner
// separately authenticates its coordinator/source/stage/publication lifetime.
struct flatfile_zone_reset_item_projection
{
	flatfile_room_item_record room;
	std::vector<flatfile_item_ownership_record> custody;
};
class critical_zone_reset_item_flat_execution_owner;
class flatfile_zone_reset_item_publication_storage;
class flatfile_accounting_zone_reset_item_transaction final
{
    public:
	~flatfile_accounting_zone_reset_item_transaction();
	flatfile_accounting_zone_reset_item_transaction(
		const flatfile_accounting_zone_reset_item_transaction &) = delete;
	flatfile_accounting_zone_reset_item_transaction &
	operator=(const flatfile_accounting_zone_reset_item_transaction &) = delete;

    private:
	friend class critical_zone_reset_item_flat_execution_owner;
	friend class flatfile_zone_reset_item_publication_storage;
	// Sole genuine queued execution owner supplies original full INITIAL carrier,
	// configured root/source/actor/attempt/generation/thread and recovered lock.
	// No caller DTO/public constructor/general executor can acquire this role.
	// Same initial lock through first commit; retain original proposal/ID after
	// possible publication. EALREADY requires authentic complete stored receipt.
	// All preparation refusals leave output untouched. Root reserves full actual
	// retained and transient workspace; allocation catches are not budget proof.
	static unsigned int
	prepare_locked(const std::string &, const flatfile_authority_lock &,
		       const critical_native_recovery_envelope &,
		       std::unique_ptr<flatfile_accounting_zone_reset_item_transaction> *) noexcept;
	critical_apply_result commit_locked(const std::string &,
					    const flatfile_authority_lock &) noexcept;
	critical_apply_result reconcile_locked(const std::string &,
					       const flatfile_authority_lock &) noexcept;
	bool publication_possible() const noexcept;
	bool retained_bytes(size_t *) const noexcept;
	static critical_apply_result
	verify_retained_locked(const std::string &, const flatfile_authority_lock &,
			       const critical_native_recovery_envelope &) noexcept;
	static unsigned int verify_record_locked(const std::string &,
						 const flatfile_authority_lock &,
						 const critical_native_recovery_envelope &,
						 flatfile_accounting_record *) noexcept;
	static unsigned int read_current_locked(const std::string &,
						const flatfile_authority_lock &,
						const critical_native_recovery_envelope &,
						const critical_completion &,
						flatfile_zone_reset_item_projection *) noexcept;
	static unsigned int read_origin_locked(const std::string &, const flatfile_authority_lock &,
					       uint64_t,
					       zone_reset_item_retained_origin *) noexcept;
	// Complete original receipt/current proof with prospective transitive storage.
	// Caller includes original/root/lock/receipt/prior outputs in outer. Returned
	// heap excludes inline output DTO; no execution, source or ACK authority.
	static unsigned int
	verify_record_locked_bounded(const std::string &, const flatfile_authority_lock &,
				     const critical_native_recovery_envelope &,
				     flatfile_accounting_record *, flatfile_scratch_reserve_fn,
				     void *, size_t,
				     size_t *retained_output_heap = nullptr) noexcept;
	static unsigned int read_current_locked_bounded(
		const std::string &, const flatfile_authority_lock &,
		const critical_native_recovery_envelope &, const critical_completion &,
		flatfile_zone_reset_item_projection *, flatfile_scratch_reserve_fn, void *, size_t,
		size_t *retained_output_heap = nullptr) noexcept;
	static unsigned int
	observe_initial_locked(const std::string &, const flatfile_authority_lock &,
			       const critical_native_recovery_envelope &) noexcept;
	// Complete original INITIAL observation with prospective transitive storage.
	// Caller owns original/root/lock/context and all existing scratch in outer.
	// No source/execution/publication/receipt authority is granted; all staged
	// values remain local. Unsupported pinned request ABI refuses ENOTSUP.
	static unsigned int
	observe_initial_locked_bounded(const std::string &, const flatfile_authority_lock &,
				       const critical_native_recovery_envelope &,
				       flatfile_scratch_reserve_fn, void *context,
				       size_t outer_live_scratch) noexcept;
	struct implementation;
	std::unique_ptr<implementation> state_;
	explicit flatfile_accounting_zone_reset_item_transaction(std::unique_ptr<implementation>);
};
class zone_reset_item_owner;
class critical_zone_reset_item_publication_owner;
class flatfile_zone_reset_item_publication_storage final
{
	friend class zone_reset_item_owner;
	friend class critical_zone_reset_item_publication_owner;
	// SAME genuine recovered configured-root lock and original full carrier.
	// Passive stored proof only; no constructor, source/execution/ACK authority,
	// acquisition/recovery/write/commit or live mutation. Strong outputs.
	// Complete passive initial source/catalog/pile absence cut. Staged catalog
	// images stay local and are discarded; no receipt stage, writer or commit.
	static unsigned int
	observe_initial_locked(const std::string &, const flatfile_authority_lock &,
			       const critical_native_recovery_envelope &) noexcept;
	// Complete original INITIAL observation with prospective transitive storage.
	// Caller owns original/root/lock/context and all existing scratch in outer.
	// No source/execution/publication/receipt authority is granted; all staged
	// values remain local. Unsupported pinned request ABI refuses ENOTSUP.
	static unsigned int
	observe_initial_locked_bounded(const std::string &, const flatfile_authority_lock &,
				       const critical_native_recovery_envelope &,
				       flatfile_scratch_reserve_fn, void *context,
				       size_t outer_live_scratch) noexcept;
	static unsigned int read_locked(const std::string &, const flatfile_authority_lock &,
					const critical_native_recovery_envelope &,
					const critical_completion &,
					flatfile_zone_reset_item_projection *) noexcept;
	static unsigned int
	read_locked_bounded(const std::string &, const flatfile_authority_lock &,
			    const critical_native_recovery_envelope &, const critical_completion &,
			    flatfile_zone_reset_item_projection *, flatfile_scratch_reserve_fn,
			    void *, size_t, size_t *retained_output_heap = nullptr) noexcept;
	static unsigned int read_origin_locked(const std::string &, const flatfile_authority_lock &,
					       uint64_t,
					       zone_reset_item_retained_origin *) noexcept;
};
#endif
