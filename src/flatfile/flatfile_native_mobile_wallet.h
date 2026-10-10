#ifndef DURIS_FLATFILE_NATIVE_MOBILE_WALLET_H
#define DURIS_FLATFILE_NATIVE_MOBILE_WALLET_H

#include "flatfile/flatfile_accounting_authority.h"
#include "economy/native_mobile_birth_accounting.h"
#include "world/quest_mobile_native.h"

// The existing SQL namespace uses locator 7 and the SAME native-mobile context.
// This identifies values only, never mapping creation or original birth proof.
constexpr uint16_t FLATFILE_NATIVE_MOBILE_WALLET_LOCATOR = 7;
constexpr size_t FLATFILE_NATIVE_MOBILE_WALLET_KEY_BYTES = 20;

bool flatfile_native_mobile_wallet_locator_valid(economic_account_kind, uint64_t context,
						 const flatfile_economic_locator &) noexcept;
// Exact existing native-index bytes: kind2/context8/locator2/native-ID8, LE.
// These pure companions allocate nothing and preserve outputs on every refusal.
bool flatfile_native_mobile_wallet_key_encode(
	economic_account_kind, uint64_t context, const flatfile_economic_locator &,
	std::array<uint8_t, FLATFILE_NATIVE_MOBILE_WALLET_KEY_BYTES> *) noexcept;
struct flatfile_native_mobile_wallet_key
{
	economic_account_kind kind = economic_account_kind::wallet;
	uint64_t context = ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT;
	uint16_t locator_kind = FLATFILE_NATIVE_MOBILE_WALLET_LOCATOR;
	uint64_t native_id = 0;
};
bool flatfile_native_mobile_wallet_key_decode(std::span<const uint8_t>,
					      flatfile_native_mobile_wallet_key *) noexcept;

// Unsealed CURRENT values, distinct from an authenticated historical lifetime.
// active_epoch is today's persisted admission epoch, NEVER an inferred birth_epoch.
// The full native image keeps its actual last transition, cash and complete stock.
struct flatfile_native_mobile_wallet_current
{
	critical_operation_id active_epoch{};
	uint64_t lineage_revision = 0;
	flatfile_economic_mapping mapping;
	quest_mobile_native_image native;
};

class flatfile_native_mobile_wallet_storage
{
    public:
	// Caller first recovers the original shared bundle and keeps this exact root
	// lock and actual lifecycle/writer exclusion throughout the observation/use.
	// Requires the exact caller reference and LIVE known current cash. Proves
	// selected lineage/epoch, nonretired mapping and active native index under
	// the SAME borrowed lock, then correlates the creator and current birth ID.
	// Strong output. No recovery, lock acquisition, writer, native mutation,
	// source/receipt/history/season/physical-custody proof, publication or ACK.
	// Uses real allocating original readers; no retained-memory bound is claimed.
	static unsigned int
	observe_current_locked(const std::string &, const flatfile_authority_lock &,
			       const critical_operation_id &active_epoch,
			       const economic_account_key &original_wallet,
			       const quest_mobile_native_reference &exact_current_reference,
			       flatfile_native_mobile_wallet_current *) noexcept;
};

#endif
