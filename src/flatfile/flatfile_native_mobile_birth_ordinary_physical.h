#ifndef DURIS_FLATFILE_NATIVE_MOBILE_BIRTH_ORDINARY_PHYSICAL_H
#define DURIS_FLATFILE_NATIVE_MOBILE_BIRTH_ORDINARY_PHYSICAL_H

#include "flatfile/flatfile_identity_repository.h"
#include "economy/native_mobile_birth_recovery.h"

struct flatfile_native_mobile_birth_ordinary_identity_current
{
	uint64_t catalog_revision = 0;
	std::vector<flatfile_identity_record> records;
};

class flatfile_native_mobile_birth_ordinary_identity_storage final
{
    private:
	friend class flatfile_native_mobile_birth_ordinary_player_physical_storage;
	// Caller already holds identity THEN recovered authority for this root.
	// Complete original catalog, including all retired records; no recovery.
	// Strong local output, unsealed values and no new identity/write permit.
	static flatfile_identity_result
	read_locked(const std::string &, const flatfile_identity_lock &,
		    const flatfile_authority_lock &,
		    flatfile_native_mobile_birth_ordinary_identity_current *,
		    std::string *) noexcept;

	// Distinct complete retained catalog reader. Borrow identity then authority;
	// true missing FILE remains empty, missing/unsafe directory remains error.
	// Caller retains root/locks/prior output in outer. Strong full rows and heap
	// scalar; no diagnostics, new permit, acquisition, recovery or write.
	static flatfile_identity_result
	read_locked_bounded(const std::string &, const flatfile_identity_lock &,
			    const flatfile_authority_lock &,
			    flatfile_native_mobile_birth_ordinary_identity_current *,
			    flatfile_scratch_reserve_fn, void *, size_t,
			    size_t *retained_identity_heap = nullptr) noexcept;
};

struct flatfile_native_mobile_birth_ordinary_player_physical_absence
{
	uint64_t identity_catalog_revision = 0;
	size_t identities = 0, retired_identities = 0;
	size_t snapshots = 0, unindexed_snapshots = 0;
	size_t inventory_rows = 0, pet_item_rows = 0;
};

class flatfile_native_mobile_birth_ordinary_player_physical_storage final
{
    private:
	friend class flatfile_native_mobile_birth_ordinary_physical_storage;
	// Full canonical ordinary command/NMR1 supplies ALL born UIDs. Actual player
	// namespace enumeration includes retained/unindexed/retired snapshots and
	// every pet, under identity->authority lock order and original writer freeze.
	// Established namespace required: missing directory/security/read/corruption
	// is never accepted as absence. True missing catalog/file remains distinct.
	// No player lock acquisition, recover, write, world/producer/ACK or selector.
	// Strong counts only. Other domains/history/ledger and memory proof are separate.
	static unsigned int
	verify_locked(const std::string &, const flatfile_identity_lock &,
		      const flatfile_authority_lock &, const critical_native_recovery_envelope &,
		      flatfile_native_mobile_birth_ordinary_player_physical_absence *,
		      std::string *) noexcept;

	// Complete namespace and every inventory/pet UID under genuine request and
	// capacity admission. Linux getdents64 uses real admitted storage; unsupported
	// ABI refuses. No selector/authority follows from these private strong counts.
	// Full emitted/library/native/enclosing-budget qualification remains separate.
	static unsigned int
	verify_locked_bounded(const std::string &, const flatfile_identity_lock &,
			      const flatfile_authority_lock &,
			      const critical_native_recovery_envelope &,
			      flatfile_native_mobile_birth_ordinary_player_physical_absence *,
			      flatfile_scratch_reserve_fn, void *, size_t) noexcept;
};

struct flatfile_native_mobile_birth_ordinary_catalog_physical_absence
{
	bool present = false;
	uint64_t file_revision = 0, catalog_revision = 0;
	size_t listings = 0, item_rows = 0;
};

class flatfile_native_mobile_birth_ordinary_auction_physical_storage final
{
    private:
	friend class flatfile_native_mobile_birth_ordinary_physical_storage;
	// Whole original auction catalog, ALL listing/claim states, same recovered
	// root lock. No open-only or pickup projection substitutes for retained rows.
	static unsigned int
	verify_locked(const std::string &, const flatfile_authority_lock &,
		      const critical_native_recovery_envelope &,
		      flatfile_native_mobile_birth_ordinary_catalog_physical_absence *,
		      std::string *) noexcept;

	// Full bounded catalog provider; complete definition is the auction/collector
	// owner's separate immutable handoff. Aggregate friendship remains sole access.
	static unsigned int
	verify_locked_bounded(const std::string &, const flatfile_authority_lock &,
			      const critical_native_recovery_envelope &,
			      flatfile_native_mobile_birth_ordinary_catalog_physical_absence *,
			      flatfile_scratch_reserve_fn, void *, size_t) noexcept;
};

class flatfile_native_mobile_birth_ordinary_collector_physical_storage final
{
    private:
	friend class flatfile_native_mobile_birth_ordinary_physical_storage;
	// Whole original collector catalog, ALL candidate/cancelled/held/terminal
	// rows even without a physical blob. Source/ledger/runtime joins are separate.
	static unsigned int
	verify_locked(const std::string &, const flatfile_authority_lock &,
		      const critical_native_recovery_envelope &,
		      flatfile_native_mobile_birth_ordinary_catalog_physical_absence *,
		      std::string *) noexcept;

	// Full bounded catalog provider; complete definition is the auction/collector
	// owner's separate immutable handoff. Aggregate friendship remains sole access.
	static unsigned int
	verify_locked_bounded(const std::string &, const flatfile_authority_lock &,
			      const critical_native_recovery_envelope &,
			      flatfile_native_mobile_birth_ordinary_catalog_physical_absence *,
			      flatfile_scratch_reserve_fn, void *, size_t) noexcept;
};

struct flatfile_native_mobile_birth_ordinary_catalog_namespaces_absence
{
	flatfile_native_mobile_birth_ordinary_player_physical_absence players;
	flatfile_native_mobile_birth_ordinary_catalog_physical_absence auction, collector;
	size_t corpses = 0, rooms = 0, saved_world_records = 0, world_item_rows = 0;
	size_t lockers = 0, locker_chests = 0, locker_item_rows = 0;
	size_t shopkeepers = 0, shop_item_rows = 0;
};

class flatfile_native_mobile_birth_ordinary_physical_storage final
{
    private:
	friend class flatfile_accounting_native_mobile_birth_ordinary_transaction;
	// Complete ACTUAL player/pet/world/locker/SHOP/auction/collector catalog cut.
	// Applicable siege/restitution, quarantine/ledger and live-world domains remain
	// separate original owner joins: success here is NOT whole SQL physical proof.
	// Borrow authentic identity THEN recovered authority, no lock/recovery/effects.
	static unsigned int verify_catalog_namespaces_locked(
		const std::string &, const flatfile_identity_lock &,
		const flatfile_authority_lock &, const critical_native_recovery_envelope &,
		flatfile_native_mobile_birth_ordinary_catalog_namespaces_absence *,
		std::string *) noexcept;

	// Full passive bounded PLAYER/PET/world/locker/SHOP/auction/collector cut.
	// Borrow identity THEN recovered authority for the complete interval.
	// Strong counts; no acquisition/recovery/world/publication/ACK authority.
	static unsigned int verify_catalog_namespaces_locked_bounded(
		const std::string &, const flatfile_identity_lock &,
		const flatfile_authority_lock &, const critical_native_recovery_envelope &,
		flatfile_native_mobile_birth_ordinary_catalog_namespaces_absence *,
		flatfile_scratch_reserve_fn, void *, size_t) noexcept;
};

#endif
