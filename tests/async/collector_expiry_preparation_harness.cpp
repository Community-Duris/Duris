#include "economy/collector_expiry_preparation.h"

#include "player/player_snapshot_codec.h"

#include <cassert>
#include <limits>

namespace
{
collector::record expired_record()
{
	collector::rules rules;
	rules.enabled = true;
	collector::record entry;
	assert(collector::enroll(77, "123456789abcdef0123456789abcdef0", 42, 101, 5, 1000, rules,
				 &entry) == collector::outcome::applied);
	assert(collector::collect(&entry, entry.revision, 5, 5, true, 75, entry.collect_at) ==
	       collector::outcome::applied);
	assert(collector::activate(&entry, entry.revision, entry.sale_at) ==
	       collector::outcome::applied);
	return entry;
}

collector_listing_detail valid_detail(const collector::record &entry)
{
	player_item_snapshot item = {};
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.object_uid = entry.uid;
	item.vnum = 501;
	item.name = "blade collector";
	item.short_description = "an old blade";
	item.description = "An old blade rests here.";
	item.type = 5;
	item.weight = 12;
	item.cost = 75;
	item.condition = 83;
	collector_listing_detail detail;
	detail.entry = entry;
	assert(player_item_snapshot_list_encode({ item }, &detail.item_blob) ==
	       player_snapshot_codec_result::ok);
	return detail;
}

item_ownership_runtime_entry valid_held(const collector::record &entry)
{
	return { entry.uid,
		 entry.uid,
		 0,
		 { item_owner_type::collector, item_collector_owner_id(entry.listing), 0 },
		 entry.item_revision,
		 8,
		 501,
		 item_custody_state::active };
}
}

int main()
{
	const collector::record entry = expired_record();
	const collector_listing_detail detail = valid_detail(entry);
	const item_ownership_runtime_entry held = valid_held(entry);
	std::unique_ptr<collector_command_payload> payload;
	assert(collector_expiry_prepare(entry, detail, held, 12, entry.expires_at, &payload) ==
	       collector_expiry_prepare_outcome::prepared);
	assert(payload && payload->action == collector_action::expire &&
	       payload->listing == entry.listing &&
	       payload->expected_listing_revision == entry.revision &&
	       payload->observed_at == entry.expires_at && !payload->actor_pid &&
	       payload->from_owner.type == item_owner_type::collector &&
	       payload->to_owner.type == item_owner_type::destruction &&
	       payload->expected_from_owner_revision == held.owner_revision &&
	       payload->expected_to_owner_revision == 12 &&
	       payload->selected_item_uid == entry.uid && payload->item_count == 1 &&
	       payload->items[0].expected_item_revision == entry.item_revision &&
	       payload->target_state == item_custody_state::destroyed &&
	       payload->item_blob_size == detail.item_blob.size());

	const collector_command_payload *const unchanged = payload.get();
	const uint64_t unchanged_listing = payload->listing;
	const uint32_t unchanged_blob_size = payload->item_blob_size;
	auto rejected = [&](collector_expiry_prepare_outcome expected,
			    const collector::record &runtime, const collector_listing_detail &read,
			    const item_ownership_runtime_entry &ownership,
			    uint64_t destruction_revision, uint64_t observed_at)
	{
		assert(collector_expiry_prepare(runtime, read, ownership, destruction_revision,
						observed_at, &payload) == expected);
		assert(payload.get() == unchanged && payload->listing == unchanged_listing &&
		       payload->item_blob_size == unchanged_blob_size);
	};

	collector_listing_detail stale = detail;
	++stale.entry.revision;
	rejected(collector_expiry_prepare_outcome::stale_listing, entry, stale, held, 12,
		 entry.expires_at);
	collector::record paused = entry;
	paused.holding_paused = true;
	paused.paused_at = entry.available_at;
	collector_listing_detail paused_detail = detail;
	paused_detail.entry = paused;
	rejected(collector_expiry_prepare_outcome::not_due, paused, paused_detail, held, 12,
		 paused.expires_at);
	rejected(collector_expiry_prepare_outcome::not_due, entry, detail, held, 12,
		 entry.expires_at - 1);
	collector_listing_detail malformed = detail;
	malformed.item_blob.pop_back();
	rejected(collector_expiry_prepare_outcome::invalid_item, entry, malformed, held, 12,
		 entry.expires_at);
	item_ownership_runtime_entry stale_held = held;
	++stale_held.item_revision;
	rejected(collector_expiry_prepare_outcome::stale_custody, entry, detail, stale_held, 12,
		 entry.expires_at);
	rejected(collector_expiry_prepare_outcome::invalid_request, entry, detail, held,
		 std::numeric_limits<uint64_t>::max(), entry.expires_at);
	rejected(collector_expiry_prepare_outcome::invalid_request, entry, detail, held, 12, 0);
}
