#include "economy/collector_expiry_preparation.h"

#include "economy/collector_purchase_preparation.h"

#include <algorithm>
#include <limits>
#include <new>
#include <utility>

collector_expiry_prepare_outcome collector_expiry_prepare(
	const collector::record &runtime_entry, const collector_listing_detail &detail,
	const item_ownership_runtime_entry &held_item, uint64_t destruction_owner_revision,
	uint64_t observed_at, std::unique_ptr<collector_command_payload> *payload)
{
	if (!payload || !observed_at ||
	    destruction_owner_revision == std::numeric_limits<uint64_t>::max())
		return collector_expiry_prepare_outcome::invalid_request;
	if (runtime_entry.status != collector::state::available || runtime_entry.holding_paused ||
	    observed_at < runtime_entry.expires_at)
		return collector_expiry_prepare_outcome::not_due;

	player_item_snapshot item;
	switch (collector_listing_decode_item(runtime_entry, detail, &item))
	{
	case collector_listing_decode_outcome::decoded:
		break;
	case collector_listing_decode_outcome::stale_listing:
		return collector_expiry_prepare_outcome::stale_listing;
	case collector_listing_decode_outcome::invalid_item:
		return collector_expiry_prepare_outcome::invalid_item;
	case collector_listing_decode_outcome::allocation_failure:
		return collector_expiry_prepare_outcome::allocation_failure;
	}

	const item_owner_identity collector_owner = {
		item_owner_type::collector, item_collector_owner_id(runtime_entry.listing), 0
	};
	if (held_item.item_uid != runtime_entry.uid ||
	    held_item.root_item_uid != runtime_entry.uid || held_item.parent_item_uid ||
	    !item_owner_identity_equal(held_item.owner, collector_owner) ||
	    held_item.item_revision != runtime_entry.item_revision ||
	    held_item.item_revision == std::numeric_limits<uint64_t>::max() ||
	    held_item.owner_revision == std::numeric_limits<uint64_t>::max() ||
	    held_item.vnum != item.vnum || held_item.state != item_custody_state::active)
		return collector_expiry_prepare_outcome::stale_custody;

	std::unique_ptr<collector_command_payload> candidate;
	try
	{
		candidate = std::make_unique<collector_command_payload>();
	}
	catch (const std::bad_alloc &)
	{
		return collector_expiry_prepare_outcome::allocation_failure;
	}
	candidate->action = collector_action::expire;
	candidate->target_state = item_custody_state::destroyed;
	candidate->listing = runtime_entry.listing;
	candidate->expected_listing_revision = runtime_entry.revision;
	candidate->observed_at = observed_at;
	candidate->from_owner = collector_owner;
	candidate->to_owner = { item_owner_type::destruction, 0, 0 };
	candidate->expected_from_owner_revision = held_item.owner_revision;
	candidate->expected_to_owner_revision = destruction_owner_revision;
	candidate->selected_item_uid = runtime_entry.uid;
	candidate->item_count = 1;
	candidate->items[0] = {
		held_item.item_uid,	 held_item.root_item_uid, held_item.parent_item_uid,
		held_item.item_revision, held_item.vnum,	  held_item.state
	};
	candidate->item_blob_size = static_cast<uint32_t>(detail.item_blob.size());
	std::copy(detail.item_blob.begin(), detail.item_blob.end(), candidate->item_blob.begin());
	*payload = std::move(candidate);
	return collector_expiry_prepare_outcome::prepared;
}
