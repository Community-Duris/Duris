#ifndef AUCTION_REPOSITORY_H
#define AUCTION_REPOSITORY_H

#include "economy/auction_command.h"
#include "player/player_snapshot_codec.h"
#include <span>

#include <mysql/mysql.h>
#include "economy/economic_accounting_types.h"

bool auction_repository_execute(MYSQL *connection, const critical_command &command,
				auction_command_result *result, unsigned int *result_code,
				bool *mutation_applied);

// Only schema-2 auction components may call this entry point, inside their
// already-locked inbox transaction. The caller must record the matching EAP1
// root before writing the receipt and committing. Other actions refuse.
bool auction_repository_execute_accounted(MYSQL *connection, const critical_command &command,
					  auction_command_result *result, unsigned int *result_code,
					  bool *mutation_applied);

// Borrowed original schema2/v2 participant only. Caller owns original inbox,
// global custody authority and frozen all-node before validation. No transaction
// lifecycle or publication authority. False requires caller rollback; outputs
// stay unchanged. Native events cover complete ordered trees; receipts roots only.
bool auction_repository_execute_accounted_native(
	MYSQL *, const critical_command &, std::span<const player_item_snapshot> original_selected,
	auction_command_result *, unsigned int *, bool *);

// Borrowed accepted native non-item pre-effect cut. Authenticates original source,
// current whole-player level/save/order/digests and all frozen custody fences.
// Never changes the command or executes native effects; caller retains transaction.
unsigned int auction_repository_validate_accounted_native_cut(MYSQL *,
							      const critical_command &) noexcept;

// Pure ACT2 values decoder, not listing/inbox/source entitlement authentication.
// Complete one-root literal tree and immutable listing-after revision vector.
// Both outputs stay unchanged on refusal. No native materialization or SQL.
bool auction_repository_decode_native_tree_blob(std::span<const uint8_t>,
						std::vector<player_item_snapshot> *,
						std::vector<uint64_t> *) noexcept;

// Borrowed historical ACT2 selected source, authenticated against original EAI,
// EAP/native listing receipts and genuine locked current claim entitlement.
// Fresh schema1/v1 request may obtain source before bind; accepted schema2/v2
// additionally compares exact frozen entitlement and selected digest/fences. No header
// reconstruction, materialization or transaction lifecycle. Strong output.
unsigned int auction_repository_read_original_native_selected(
	MYSQL *, const critical_command &original_claim,
	const critical_operation_id &original_listing_operation,
	std::vector<player_item_snapshot> *selected) noexcept;

// Fresh bid/settlement producer capture under the existing trusted native SQL
// caller's transaction with reconnect disabled. No mutation, admission, inbox,
// identity reservation, or transaction management. Caller supplies its qualified
// lineage/epoch; mappings and all absence facts come from native SQL. On failure
// roll back; command stays unchanged. Schema-2 replay cannot enter this function.
unsigned int auction_repository_prepare_accounting(MYSQL *, const critical_operation_id &lineage,
						   const critical_operation_id &epoch,
						   critical_command *);

// Canonical original typed command only. No current lineage/epoch/mapping read.
// Canonical typed classification for bid/settlement/listing/item pickup/money pickup.
// Classification grants no actor, publication, checkpoint or execution authority.
bool auction_repository_frozen_accounting_valid(const critical_command &) noexcept;

struct auction_accounting_endpoint_readback
{
	economic_account_key bidder_claim{}, previous_claim{}, seller_claim{};
	bool unused_bidder = false;
};
// Read original committed AEC1 creator proof using the complete retained command.
// Resolved keys are observations; the original command/tags/zero IDs stay exact.
// Authenticated unused bidder ENODATA is retained as unused_bidder with an empty
// key, including after a later genuine endpoint. No tagged role returns ENODATA
// without claiming mapped-only receipt proof. Output stays unchanged on failure.
unsigned int auction_repository_readback_endpoints(MYSQL *, const critical_command &,
						   auction_accounting_endpoint_readback *);

// Persisted same-cut values only. Native receipt classification and current
// field correspondence never authenticate ANF2/EAI/EAP original provenance.
#include "persistence/sql_room_item_payload.h"
#include <optional>

enum class auction_physical_issue : uint8_t
{
	invalid_identity,
	duplicate_row_key,
	unbound_listing,
	invalid_slots,
	opaque_literal,
	literal_decode_refused,
	original_provenance_unknown,
	listing_literal_mismatch,
	duplicate_current_uid,
	custody_mismatch,
	extra_custody_descendant,
	unmatched_auction_custody,
	generic_proof_unknown,
	generic_proof_conflict,
	missing_literal,
	history_metadata,
	native_bound_refused,
	missing_identity,
	count
};
enum class auction_physical_family : uint8_t
{
	unresolved,
	legacy_or_v1,
	observed_native_v2
};
enum class auction_physical_generic_proof : uint8_t
{
	unknown,
	observed_match,
	conflict
};
struct auction_physical_reference
{
	// false addresses base.source2.tables; true addresses supplement.tables.
	bool supplemental = false;
	size_t table = 0, row = 0;
	economic_sql_source_digest digest = {};
	// true addresses base.source2.item_equipment_sources (supplemental=false).
	bool equipment = false;
};
struct auction_physical_listing
{
	auction_physical_reference source;
	std::optional<uint32_t> auction_id;
	std::optional<int64_t> quantity;
	std::optional<auction_physical_reference> inbox, outbox, operation;
	auction_physical_family family = {};
	std::optional<auction_command_result> listing_receipt;
	// Canonical original schema1/payload1 receipt, never a decoded literal.
	std::optional<auction_command_result> legacy_listing_receipt;
	uint32_t issue_mask = 0;
};
struct auction_physical_retained_listing
{
	auction_physical_reference inbox;
	auction_command_result receipt;
	std::optional<auction_physical_reference> parent, outbox, operation;
};
struct auction_physical_slot
{
	auction_physical_reference source;
	std::optional<uint32_t> auction_id, claim_pid;
	std::optional<uint16_t> slot;
	std::optional<uint64_t> item_uid, row_revision;
	std::optional<int32_t> vnum;
	std::optional<bool> claimed;
	std::optional<size_t> listing_index, retained_receipt_index;
	size_t node_begin = 0, node_count = 0;
	std::optional<size_t> legacy_identity_index;
	uint32_t issue_mask = 0;
};
struct auction_physical_node
{
	// Complete original decoded literal, including every string/property.
	// parent_index addresses this slot's node range, not the whole report.
	player_item_snapshot literal;
	uint64_t listing_after_revision = 0;
	size_t slot_index = 0;
	std::optional<auction_physical_reference> custody, owner_revision, ledger, item_reference,
		equipment;
	auction_physical_generic_proof generic_proof = {};
	bool current_field_correspondence = false;
	// Always unknown: the original locked listing forest is outside this packet.
	bool original_provenance_unknown = true;
};
// Original v1 singleton identity evidence. The opaque common blob remains in
// the immutable raw slot/parent rows. No player snapshot is synthesized.
struct auction_physical_legacy_identity
{
	size_t slot_index = 0;
	std::optional<uint64_t> item_uid, listing_after_revision;
	std::optional<int32_t> vnum;
	auction_physical_reference parent;
	std::optional<auction_physical_reference> inbox, custody, owner_revision, equipment, ledger;
	auction_physical_generic_proof ownership_ledger_proof = {};
	bool current_field_correspondence = false;
	bool decoded_content_unknown = true, original_provenance_unknown = true;
};
struct auction_physical_pickup
{
	auction_physical_reference source;
	std::optional<uint64_t> row_id;
	std::optional<uint32_t> pid;
	std::optional<int64_t> retrieved, quantity;
};
struct auction_physical_diagnostic
{
	auction_physical_issue issue = {};
	auction_physical_reference source;
};
struct auction_physical_correspondence
{
	economic_sql_source_digest physical_digest = {};
	std::array<economic_sql_source_digest, 5> validated_supplement_table_digests = {};
	// Retained caller value only; the public inspector does not verify this outer hash.
	economic_sql_source_digest caller_supplied_supplement_digest = {};
	std::vector<auction_physical_listing> listings;
	// Every canonical successful listed receipt, including missing parent metadata.
	std::vector<auction_physical_retained_listing> retained_listings;
	std::vector<auction_physical_slot> slots;
	std::vector<auction_physical_node> nodes;
	std::vector<auction_physical_legacy_identity> legacy_identities;
	std::vector<auction_physical_pickup> pickups;
	// Retain ALL auction-owned native rows, including malformed/extra descendants.
	std::vector<auction_physical_reference> auction_custody;
	// Complete references bounded by input rows, independent of detail truncation.
	std::vector<auction_physical_reference> related_custody, extra_custody, malformed_rows;
	std::vector<auction_physical_reference> unmatched_auction_custody;
	std::vector<auction_physical_reference> missing_literal_events;
	std::array<uint64_t, static_cast<size_t>(auction_physical_issue::count)> issue_counts = {};
	uint64_t diagnostic_count = 0;
	bool diagnostics_truncated = false;
	std::vector<auction_physical_diagnostic> diagnostics;
};
// Pure borrowed consumer, both profiles. Caller owns the immutable packets and
// same original RR cut guarantee. Reuses public physical/room framing validation
// and original ACT2 decoder; no SQL, hydration, mutation or activation authority.
// All raw auctions/custody/pickups remain addressable. Claimed rows are history;
// staged claim_pid alone does not remove an unclaimed row from live escrow.
// Decoder false is an ambiguous refusal (malformed/allocation reason erased by
// its original bool API), never a definite corruption or allocation diagnosis.
// Malformed scalar observations remain referenced semantic findings.
// Own structural framing/allocation failures preserve output. Stored details=1..512;
// complete issue counts do not truncate. Original raw and decoder bounds remain.
unsigned int auction_repository_inspect_physical_sources(
	const economic_sql_physical_source_snapshot &, const sql_room_item_source_snapshot &,
	const economic_sql_source_limits &, size_t maximum_diagnostics,
	auction_physical_correspondence *) noexcept;

#endif
