#ifndef DURIS_FLATFILE_SHOPKEEPER_REPOSITORY_H
#define DURIS_FLATFILE_SHOPKEEPER_REPOSITORY_H

#include "flatfile/flatfile_authority_transaction.h"
#include "player/player_snapshot.h"
#include "economy/shop_trade_command.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct flatfile_shopkeeper_affect_record
{
	int32_t type = 0;
	int32_t duration = 0;
	int32_t modifier = 0;
	int32_t location = 0;
	std::array<uint64_t, 5> bitvectors = {};
};

struct flatfile_shopkeeper_record
{
	uint32_t shop_id = 0;
	int32_t mob_vnum = 0;
	int32_t room_vnum = 0;
	int64_t saved_at = 0;
	uint64_t revision = 0;
	// -1 marks a catalog decoded from v1, before keeper cash was retained.
	int64_t cash = 0;
	bool roaming = false;
	std::vector<flatfile_shopkeeper_affect_record> affects;
	std::vector<player_item_snapshot> items;
};

// Allocation-free complete catalog wire scan, including unrelated keepers.
// Checksum authentication and semantic validity remain the original decoder's
// responsibility after allocation admission; these scalars never prove either.
// Counts describe fresh decode payload under the reported storage policy, not
// semantic validation, hash-node/bucket growth, encoded-vector capacity or a
// complete memory reservation. The owning reader must budget those separately.
// No I/O, recovery, source, writer or publication authority; strong output.
struct flatfile_shopkeeper_catalog_allocation_profile
{
	size_t record_count = 0, item_count = 0;
	size_t decoded_catalog_payload_bytes = 0;
	size_t largest_item_blob_bytes = 0, largest_item_decode_payload_bytes = 0;
	size_t largest_item_relationship_scratch_bytes = 0;
	size_t largest_item_canonical_bytes = 0, canonical_catalog_bytes = 0;
	size_t largest_item_roundtrip_scratch_bytes = 0;
	size_t largest_item_decode_scratch_bytes = 0;
	bool fresh_decode_storage_policy_supported = false;
};
bool flatfile_shopkeeper_catalog_preflight(
	const uint8_t *encoded, size_t encoded_size,
	flatfile_shopkeeper_catalog_allocation_profile *output) noexcept;

// Pure passive one-record INITIAL checkpoint, using the existing DURSHOPv2 bytes.
// Catalog revision1 is canonical framing only: it is neither the actual catalog
// clock nor proof of a file AFTER/write. Contained record revision must equal1
// and cash must be observed (not historical -1). Encode applies only the original
// affect ordering; decode requires exact canonical bytes. Strong outputs; no I/O,
// source/admission/publication/ACK capability follows from these values.
bool flatfile_shopkeeper_initial_checkpoint_encode(const flatfile_shopkeeper_record &,
						   std::vector<uint8_t> *output) noexcept;
bool flatfile_shopkeeper_initial_checkpoint_decode(const std::vector<uint8_t> &,
						   flatfile_shopkeeper_record *output) noexcept;

enum class flatfile_shopkeeper_result
{
	ok,
	not_found,
	already_exists,
	stale,
	invalid,
	io_error
};

// Prospective-bounded cold INITIAL checkpoint decoder. The complete original
// DURSHOPv2 one-record bytes, revision1, observed cash, AF and item semantics
// are authenticated and canonicalized AFTER admission. Distinct scratch uses
// the original item codec, UID sort array and fixed equipment bitmap; no hash
// node/tree or affect-clone allocation is introduced. Original APIs unchanged.
// Caller includes retained input capacity, prior output state and all already-
// live birth/transaction storage in outer_live_scratch. Callback admits absolute
// simultaneous explicit C++ storage and retains its maximum until temporary
// destruction/output transfer. Pinned libstdc++13 C++11 ABI only; no independent
// budget, I/O, lock, source, recovery, commit, publication or ACK authority.
// OpenSSL/system internals, malloc metadata and scalar/lambda frames excluded.
// Strong output; errno distinguishes ENOBUFS/ENOTSUP from malformed bytes.
flatfile_shopkeeper_result flatfile_shopkeeper_initial_checkpoint_decode_bounded(
	const std::vector<uint8_t> &original_bytes, flatfile_shopkeeper_record *output,
	flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
	size_t outer_live_scratch) noexcept;

struct flatfile_shopkeeper_trade_mutation
{
	uint64_t shop_revision = 0;
	int64_t keeper_cash = 0;
	flatfile_authority_after_image after_image;
};

flatfile_shopkeeper_result
flatfile_shopkeeper_establish(const std::string &root,
			      const std::vector<flatfile_shopkeeper_record> &records,
			      std::string *error);
flatfile_shopkeeper_result
flatfile_shopkeeper_list(const std::string &root, std::vector<flatfile_shopkeeper_record> *records,
			 std::string *error);
// Pure CURRENT catalog read under the caller's actual selected-root lock.
// The owning transaction resolves journal recovery before this read; this helper
// neither reacquires the lock nor runs recovery/publication. Failure preserves output.
flatfile_shopkeeper_result
flatfile_shopkeeper_list_locked(const std::string &root, const flatfile_authority_lock &lock,
				std::vector<flatfile_shopkeeper_record> *records,
				std::string *error);
// Bounded CURRENT read using the same borrowed selected-root lock and secure
// same-FD file reader. Caller resolves original recovery first and includes all
// already-live storage and prior output capacities in outer_live_scratch.
// Callback admits absolute simultaneous explicit C++ storage before allocation;
// its peak must stay retained through the call and successful output transfer.
// Distinct scratch representations preserve every original catalog/item check.
// OpenSSL/system internals remain outside this explicit storage measurement.
// No recovery, lock acquisition, source/publication/ACK authority; strong output.
flatfile_shopkeeper_result
flatfile_shopkeeper_list_locked_bounded(const std::string &root,
					const flatfile_authority_lock &lock,
					std::vector<flatfile_shopkeeper_record> *records,
					flatfile_scratch_reserve_fn reserve_scratch_peak,
					void *context, size_t outer_live_scratch) noexcept;
// Decode the complete proposed keeper catalog using its original bounded codec.
// This pure value reader grants no journal, storage, lock or publication authority.
// Failure preserves the selected record; caller independently proves the trade.
flatfile_shopkeeper_result
flatfile_shopkeeper_read_trade_after_image(const flatfile_authority_after_image &image,
					   uint32_t shop_id, flatfile_shopkeeper_record *record,
					   std::string *error);
flatfile_shopkeeper_result flatfile_shopkeeper_replace(const std::string &root,
						       const flatfile_shopkeeper_record &record,
						       uint64_t expected_revision,
						       std::string *error);
flatfile_shopkeeper_result
flatfile_shopkeeper_prepare_trade(const std::string &root, const flatfile_authority_lock &lock,
				  const shop_trade_payload &payload,
				  flatfile_shopkeeper_trade_mutation *mutation,
				  unsigned int *result_code, std::string *error);

// Only the genuine native source checkpoint owner may stage this original
// catalog participant. Its captured body/identity are not public write permits.
// Caller proves the original runtime keeper, configuration and save hold;
// this stage proves CURRENT catalog/custody under the same borrowed root lock.
#include <span>
struct flatfile_item_ownership_record;
class shop_trade_native_checkpoint_owner;
class flatfile_shopkeeper_source_checkpoint_storage final
{
    private:
	friend class shop_trade_native_checkpoint_owner;
	static flatfile_shopkeeper_result
	prepare_locked(const std::string &root, const flatfile_authority_lock &lock,
		       uint32_t shop_id, uint64_t expected_shop_revision,
		       int32_t original_keeper_vnum, int64_t original_cash, bool original_roaming,
		       std::span<const player_item_snapshot> original_keeper_literal,
		       const std::vector<flatfile_item_ownership_record> &actual_keeper_custody,
		       uint64_t actual_keeper_owner_revision,
		       flatfile_shopkeeper_record *original_before,
		       flatfile_shopkeeper_record *actual_after,
		       flatfile_authority_after_image *complete_catalog_after, std::string *error);
	// Exact original CURRENT catalog bytes, including its authentic header clock
	// and every unrelated keeper. Never re-encode a historical BEFORE as v2.
	// The genuine owner retains this with the proposed whole AFTER before writing.
	static flatfile_shopkeeper_result
	read_current_catalog_locked(const std::string &root, const flatfile_authority_lock &lock,
				    flatfile_authority_after_image *original_catalog,
				    std::string *error);
};

// Passive proposal from the private initial birth participant. The actual
// whole-catalog clock is distinct from the checkpoint's framing revision1.
// A missing whole catalog has before_present=false/before_revision=0 and uses
// the existing establish creation policy after_revision=1. Existing clocks
// advance once. These values alone grant no commit or publication authority.
struct flatfile_shopkeeper_initial_catalog_stage
{
	bool catalog_before_present = false;
	uint64_t catalog_before_revision = 0, catalog_after_revision = 0;
	flatfile_authority_operation operation;
};

class flatfile_accounting_native_mobile_birth_shared_shop_transaction;
class flatfile_shopkeeper_initial_catalog_storage final
{
    private:
	friend class flatfile_accounting_native_mobile_birth_shared_shop_transaction;
	// Caller authenticates the SAME original carrier/source/constructor and
	// custody, resolves original journals, and retains this borrowed exclusive
	// selected-root lock through the ONE authority-bundle commit/recovery.
	// Canonical checkpoint bytes are passive observations, never write permits.
	// Selected SHOP must be absent; an existing record is not adopted as proof
	// of this birth/retry. Preserve every unrelated value/order and unknown v1
	// cash=-1 using the existing whole-catalog v2 codec policy. No recovery,
	// lock acquisition, commit, world effect, receipt, ACK or origin retention.
	// Every refusal leaves the complete output unchanged.
	static flatfile_shopkeeper_result
	prepare_locked(const std::string &root, const flatfile_authority_lock &lock,
		       const std::vector<uint8_t> &original_initial_checkpoint,
		       flatfile_shopkeeper_initial_catalog_stage *output,
		       std::string *error) noexcept;
};

#endif
