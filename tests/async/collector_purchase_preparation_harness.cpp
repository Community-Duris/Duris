#include "economy/collector_purchase_preparation.h"

#include "player/player_snapshot_codec.h"

#include <cassert>
#include <cstring>
#include <vector>

namespace
{
collector::record available_record()
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

player_item_snapshot exact_item()
{
	player_item_snapshot item = {};
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.equipment_slot = 0;
	item.object_uid = 101;
	item.vnum = 501;
	item.name = "blade collector";
	item.short_description = "an old blade";
	item.description = "An old blade rests here.";
	item.type = 5;
	item.weight = 12;
	item.cost = 75;
	item.condition = 83;
	return item;
}

collector_listing_detail valid_detail(const collector::record &entry)
{
	collector_listing_detail detail;
	detail.entry = entry;
	assert(player_item_snapshot_list_encode({ exact_item() }, &detail.item_blob) ==
	       player_snapshot_codec_result::ok);
	return detail;
}

collector_purchase_actor_snapshot valid_actor(const collector::record &entry)
{
	collector_purchase_actor_snapshot actor;
	actor.pid = 42;
	actor.racewar = 1;
	strcpy(actor.account_name.data(), "CollectorTester");
	actor.wallet_revision = 10;
	actor.bank_revision = 11;
	actor.item_owner_revision = 20;
	actor.carried_currency_value = entry.price_value;
	actor.carried_weight = 50;
	actor.carry_weight_limit = 100;
	actor.carried_items = 2;
	actor.carry_item_limit = 6;
	actor.observed_at = entry.available_at;
	return actor;
}

item_ownership_runtime_entry valid_held(const collector::record &entry)
{
	return { entry.uid,
		 entry.uid,
		 0,
		 { item_owner_type::collector, item_collector_owner_id(entry.listing), 0 },
		 entry.item_revision,
		 1,
		 501,
		 item_custody_state::active };
}
}

int main()
{
	auto entry = available_record();
	auto detail = valid_detail(entry);
	auto actor = valid_actor(entry);
	auto held = valid_held(entry);
	std::unique_ptr<collector_command_payload> payload;
	player_item_snapshot item;
	item.object_uid = 999;
	assert(collector_purchase_prepare(entry, detail, held, actor, &payload, &item) ==
	       collector_purchase_prepare_outcome::prepared);
	assert(payload && payload->action == collector_action::purchase &&
	       payload->listing == entry.listing &&
	       payload->expected_listing_revision == entry.revision &&
	       payload->observed_at == actor.observed_at && payload->actor_pid == actor.pid &&
	       payload->expected_wallet_revision == actor.wallet_revision &&
	       payload->expected_bank_revision == actor.bank_revision &&
	       payload->expected_from_owner_revision == held.owner_revision &&
	       payload->expected_to_owner_revision == actor.item_owner_revision &&
	       payload->selected_item_uid == entry.uid && payload->item_count == 1 &&
	       payload->items[0].item_uid == entry.uid &&
	       payload->items[0].expected_item_revision == entry.item_revision &&
	       payload->item_blob_size == detail.item_blob.size() && item.object_uid == entry.uid &&
	       item.short_description == "an old blade");

	const collector_command_payload *const unchanged_payload = payload.get();
	const uint64_t unchanged_listing = payload->listing;
	const uint32_t unchanged_blob_size = payload->item_blob_size;
	const auto unchanged_item = item;
	auto rejected = [&](collector_purchase_prepare_outcome expected,
			    const collector::record &runtime, const collector_listing_detail &read,
			    const item_ownership_runtime_entry &ownership,
			    const collector_purchase_actor_snapshot &buyer)
	{
		assert(collector_purchase_prepare(runtime, read, ownership, buyer, &payload,
						  &item) == expected);
		assert(payload.get() == unchanged_payload &&
		       payload->listing == unchanged_listing &&
		       payload->item_blob_size == unchanged_blob_size &&
		       item.object_uid == unchanged_item.object_uid &&
		       item.short_description == unchanged_item.short_description);
	};

	auto stale_detail = detail;
	++stale_detail.entry.revision;
	rejected(collector_purchase_prepare_outcome::stale_listing, entry, stale_detail, held,
		 actor);
	auto unavailable = entry;
	unavailable.holding_paused = true;
	unavailable.paused_at = actor.observed_at;
	stale_detail = detail;
	stale_detail.entry = unavailable;
	rejected(collector_purchase_prepare_outcome::unavailable, unavailable, stale_detail, held,
		 actor);
	auto stranger = actor;
	stranger.pid = 43;
	rejected(collector_purchase_prepare_outcome::forbidden, entry, detail, held, stranger);
	auto malformed = detail;
	malformed.item_blob.pop_back();
	rejected(collector_purchase_prepare_outcome::invalid_item, entry, malformed, held, actor);
	auto poor = actor;
	poor.carried_currency_value = entry.price_value - 1;
	rejected(collector_purchase_prepare_outcome::insufficient_funds, entry, detail, held, poor);
	auto stale_held = held;
	++stale_held.item_revision;
	rejected(collector_purchase_prepare_outcome::stale_custody, entry, detail, stale_held,
		 actor);
	auto burdened = actor;
	burdened.carried_weight = 89;
	rejected(collector_purchase_prepare_outcome::capacity, entry, detail, held, burdened);
	burdened = actor;
	burdened.carried_items = burdened.carry_item_limit;
	rejected(collector_purchase_prepare_outcome::capacity, entry, detail, held, burdened);
	auto expired = actor;
	expired.observed_at = entry.expires_at;
	rejected(collector_purchase_prepare_outcome::unavailable, entry, detail, held, expired);
	auto invalid_actor = actor;
	invalid_actor.account_name.fill('x');
	rejected(collector_purchase_prepare_outcome::invalid_actor, entry, detail, held,
		 invalid_actor);
}
