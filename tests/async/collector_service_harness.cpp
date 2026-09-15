#include "economy/collector_service.h"

#include "cmd/interp.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "economy/auction_room_registry.h"
#include "economy/collector_listing_pipeline.h"
#include "economy/collector_transaction.h"
#include "persistence/persistence_mode.h"
#include "player/player_load_items.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

namespace
{
char_data character = {};
pc_only_data pc = {};
room_data room = {};
collector::record runtime_entry;
collector_listing_detail detail;
item_ownership_runtime_entry held = {};
collector_listing_request queued_request = {};
collector_listing_result queued_result = {};
collector_command_payload submitted_payload = {};
critical_operation_id submitted_operation = {};
collector_completion_fn submitted_completion = nullptr;
uint64_t request_cursor = 100;
uint8_t operation_cursor = 1;
size_t transaction_submissions = 0;
size_t materializations = 0;
size_t alerts = 0;
bool cache_ready = true;
bool collector_present = true;
bool player_online = true;
bool pipeline_accepts = true;
bool transaction_accepts = true;
bool materialization_succeeds = true;
bool item_movement_busy = false;
bool bulk_get_busy = false;
bool result_ready = false;
std::vector<std::string> output_messages;

player_item_snapshot item_snapshot()
{
	player_item_snapshot item = {};
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.equipment_slot = 0;
	item.object_uid = runtime_entry.uid;
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

void seed_available()
{
	const uint64_t now = static_cast<uint64_t>(time(nullptr));
	collector::rules rules;
	rules.enabled = true;
	assert(collector::enroll(77, "123456789abcdef0123456789abcdef0", 42, 101, 5,
				 now - 25 * 60 * 60, rules,
				 &runtime_entry) == collector::outcome::applied);
	assert(collector::collect(&runtime_entry, runtime_entry.revision, 5, 5, true, 75,
				  runtime_entry.collect_at) == collector::outcome::applied);
	assert(collector::activate(&runtime_entry, runtime_entry.revision, runtime_entry.sale_at) ==
	       collector::outcome::applied);
	detail.entry = runtime_entry;
	assert(player_item_snapshot_list_encode({ item_snapshot() }, &detail.item_blob) ==
	       player_snapshot_codec_result::ok);
	held = { runtime_entry.uid,
		 runtime_entry.uid,
		 0,
		 { item_owner_type::collector, item_collector_owner_id(runtime_entry.listing), 0 },
		 runtime_entry.item_revision,
		 1,
		 501,
		 item_custody_state::active };
}

bool saw(const char *fragment)
{
	return std::any_of(output_messages.begin(), output_messages.end(),
			   [&](const std::string &message)
			   { return message.find(fragment) != std::string::npos; });
}

void clear_messages()
{
	output_messages.clear();
}

void complete_detail(collector_listing_outcome outcome = collector_listing_outcome::found)
{
	queued_result = { queued_request.request_id, queued_request.listing, outcome, 0, detail };
	result_ready = true;
	collector_service_pulse();
}
}

P_obj object_list = nullptr;
P_room world = &room;
extern const int top_of_world = 0;
str_app_type str_app[52] = {};

int STAT_INDEX(int)
{
	return 0;
}

void send_to_char(const char *message, P_char target)
{
	assert(target == &character);
	output_messages.emplace_back(message ? message : "");
}

void send_to_char_f(P_char target, const char *format, ...)
{
	assert(target == &character);
	char output[4096] = {};
	va_list arguments;
	va_start(arguments, format);
	vsnprintf(output, sizeof(output), format, arguments);
	va_end(arguments);
	output_messages.emplace_back(output);
}

void persistence_alert(int, const char *, const char *, const char *, const char *, const char *,
		       const char *, ...)
{
	++alerts;
}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}

P_char find_player_by_pid(int pid)
{
	return player_online && pid == 42 ? &character : nullptr;
}

int total_carried_weight(P_char target)
{
	assert(target == &character);
	return target->specials.carry_weight;
}

const char *get_account_name_safe(P_char target)
{
	assert(target == &character);
	return "CollectorTester";
}

bool collector_catalog_cache_ready()
{
	return cache_ready;
}

persistence_mode persistence_mode_get()
{
	return PERSISTENCE_MODE_MARIADB_PRIMARY;
}

bool collector_presence_room_active(int room_rnum)
{
	return collector_present && room_rnum == 0 &&
	       auction_house_is_registered_room_vnum(ROOM_VNUM(room_rnum));
}

bool collector_runtime_find(uint64_t listing, collector::record *entry)
{
	if (!entry || listing != runtime_entry.listing)
		return false;
	*entry = runtime_entry;
	return true;
}

bool collector_runtime_available_for(uint32_t beneficiary, size_t limit,
				     std::vector<collector::record> *entries)
{
	if (!entries || !limit)
		return false;
	entries->clear();
	if (runtime_entry.status == collector::state::available &&
	    runtime_entry.beneficiary == beneficiary)
		entries->push_back(runtime_entry);
	return true;
}

uint64_t collector_listing_pipeline_next_request_id()
{
	return ++request_cursor;
}

collector_listing_submit_outcome
collector_listing_pipeline_submit(const collector_listing_request &request)
{
	if (!pipeline_accepts)
		return collector_listing_submit_outcome::capacity_exceeded;
	queued_request = request;
	return collector_listing_submit_outcome::accepted;
}

size_t collector_listing_pipeline_pulse_for(collector_listing_consumer consumer,
					    collector_listing_result *results, size_t capacity)
{
	assert(consumer == collector_listing_consumer::player);
	if (!result_ready || !results || !capacity)
		return 0;
	results[0] = std::move(queued_result);
	result_ready = false;
	return 1;
}

bool collector_listing_pipeline_cancel(uint64_t)
{
	return true;
}

bool critical_operation_id_generate(critical_operation_id *operation_id)
{
	if (!operation_id)
		return false;
	*operation_id = {};
	operation_id->bytes[0] = operation_cursor++;
	return true;
}

bool item_owner_identity_equal(const item_owner_identity &left, const item_owner_identity &right)
{
	return left.type == right.type && left.id == right.id &&
	       left.context_id == right.context_id;
}

uint64_t item_collector_owner_id(uint64_t listing_id)
{
	return listing_id;
}

bool currency_transaction_player_busy(P_char)
{
	return false;
}

bool collector_transaction_player_busy(P_char)
{
	return false;
}

bool item_movement_transaction_player_busy(P_char target)
{
	return target == &character && item_movement_busy;
}

bool bulk_get_player_busy(P_char target)
{
	return target == &character && bulk_get_busy;
}

bool currency_transaction_can_submit_nonrebasable(P_char target)
{
	return target == &character;
}

bool item_ownership_runtime_lookup(uint64_t uid, item_ownership_runtime_entry *entry)
{
	if (!entry || uid != held.item_uid)
		return false;
	*entry = held;
	return true;
}

bool item_ownership_runtime_owner_revision(const item_owner_identity &owner, uint64_t *revision)
{
	if (!revision || owner.type != item_owner_type::player || owner.id != 42)
		return false;
	*revision = 20;
	return true;
}

bool collector_transaction_submit_identified(P_char target,
					     const critical_operation_id &operation_id,
					     const collector_command_payload &payload,
					     collector_completion_fn completion,
					     critical_deadline_class)
{
	if (!transaction_accepts || target != &character)
		return false;
	submitted_operation = operation_id;
	submitted_payload = payload;
	submitted_completion = completion;
	++transaction_submissions;
	return true;
}

bool player_load_item_graph_materialize_for_owner(
	P_char target, const std::vector<player_item_snapshot> &items,
	const std::vector<player_load_item_identity> &identities, const item_owner_identity &owner,
	uint64_t owner_revision, bool hydrate_ownership, bool complete_snapshot_state,
	player_load_item_materialize_metrics *metrics)
{
	assert(target == &character && items.size() == 1 && identities.size() == 1 &&
	       items[0].object_uid == runtime_entry.uid &&
	       identities[0].item_uid == runtime_entry.uid && identities[0].database_id == 123 &&
	       owner.type == item_owner_type::player && owner.id == 42 && owner_revision == 21 &&
	       !hydrate_ownership && complete_snapshot_state && metrics);
	++materializations;
	return materialization_succeeds;
}

bool isname(const char *name, const char *list)
{
	if (!name || !list)
		return false;
	std::string sought(name);
	const char *cursor = list;
	while (*cursor)
	{
		while (*cursor == ' ')
			++cursor;
		const char *end = cursor;
		while (*end && *end != ' ')
			++end;
		if (sought == std::string(cursor, end))
			return true;
		cursor = end;
	}
	return false;
}

void half_chop(char *input, char *first, char *rest)
{
	while (input && *input == ' ')
		++input;
	char *out = first;
	while (input && *input && *input != ' ')
		*out++ = *input++;
	*out = 0;
	while (input && *input == ' ')
		++input;
	strcpy(rest, input ? input : "");
}

int main()
{
	character.only.pc = &pc;
	pc.pid = 42;
	pc.wallet_revision = 10;
	pc.bank_revision = 11;
	character.player.level = 1;
	character.player.racewar = 1;
	character.specials.position = STAT_NORMAL | POS_STANDING;
	character.in_room = 0;
	character.points.cash[2] = 2;
	character.specials.carry_weight = 50;
	character.specials.carry_items = 2;
	str_app[0].carry_w = 100;
	room.number = 16885;
	seed_available();
	collector_service_reset_for_tests();

	char list[] = "list";
	collector_service_command(&character, list, CMD_COLLECTOR);
	assert(saw("Your antiquities") && saw("item UID 101"));

	clear_messages();
	char inspect[] = "inspect 77";
	collector_service_command(&character, inspect, CMD_COLLECTOR);
	assert(!collector_service_player_busy(&character) && saw("consults"));
	complete_detail();
	assert(saw("an old blade") && saw("condition: 83"));

	clear_messages();
	char buy[] = "buy 77";
	const uint64_t request_before_busy = request_cursor;
	item_movement_busy = true;
	collector_service_command(&character, buy, CMD_COLLECTOR);
	assert(request_cursor == request_before_busy &&
	       !collector_service_player_busy(&character) && saw("Another transaction"));
	item_movement_busy = false;
	clear_messages();
	bulk_get_busy = true;
	collector_service_command(&character, buy, CMD_COLLECTOR);
	assert(request_cursor == request_before_busy &&
	       !collector_service_player_busy(&character) && saw("Another transaction"));
	bulk_get_busy = false;

	// A movement can start while the detail worker is reading. The completion gate
	// must reject that race before it snapshots currency, capacity, or ownership.
	clear_messages();
	collector_service_command(&character, buy, CMD_COLLECTOR);
	assert(collector_service_player_busy(&character));
	item_movement_busy = true;
	complete_detail();
	assert(transaction_submissions == 0 && saw("Another transaction"));
	item_movement_busy = false;

	clear_messages();
	collector_service_command(&character, buy, CMD_COLLECTOR);
	assert(collector_service_player_busy(&character) && saw("begins verifying"));
	const uint8_t expected_operation = operation_cursor - 1;
	complete_detail();
	assert(!collector_service_player_busy(&character) && transaction_submissions == 1 &&
	       submitted_operation.bytes[0] == expected_operation &&
	       submitted_payload.listing == 77 && submitted_payload.actor_pid == 42 &&
	       submitted_payload.capacity_admitted &&
	       submitted_payload.expected_wallet_revision == 10 &&
	       submitted_payload.expected_bank_revision == 11 &&
	       submitted_payload.expected_to_owner_revision == 20 &&
	       submitted_payload.item_blob_size == detail.item_blob.size() && saw("committing"));
	collector_command_result committed;
	committed.action = collector_action::purchase;
	committed.record_present = true;
	committed.to_owner_revision = 21;
	committed.materialized_item_id = 123;
	committed.entry = runtime_entry;
	assert(collector::purchase(
		       &committed.entry, committed.entry.revision, 42, committed.entry.price_value,
		       true, static_cast<uint64_t>(time(nullptr))) == collector::outcome::applied);
	submitted_completion(&character, true, committed, 0, submitted_payload);
	assert(materializations == 1 && saw("You buy back"));

	clear_messages();
	character.specials.carry_weight = 95;
	collector_service_command(&character, buy, CMD_COLLECTOR);
	complete_detail();
	assert(transaction_submissions == 1 && saw("cannot carry"));
	character.specials.carry_weight = 50;

	clear_messages();
	character.points.cash[2] = 1;
	collector_service_command(&character, buy, CMD_COLLECTOR);
	complete_detail();
	assert(transaction_submissions == 1 && saw("enough carried currency"));
	character.points.cash[2] = 2;

	clear_messages();
	player_online = false;
	collector_service_command(&character, inspect, CMD_COLLECTOR);
	complete_detail();
	player_online = true;
	assert(collector_service_health_copy().abandoned_offline == 1);

	clear_messages();
	room.number = 999999;
	collector_service_command(&character, inspect, CMD_COLLECTOR);
	assert(saw("not available here"));
	room.number = 16885;
	collector_present = false;
	clear_messages();
	collector_service_command(&character, inspect, CMD_COLLECTOR);
	assert(saw("not available here"));
	collector_present = true;

	clear_messages();
	cache_ready = false;
	collector_service_command(&character, inspect, CMD_COLLECTOR);
	assert(saw("records are unavailable"));
	cache_ready = true;

	clear_messages();
	const collector_service_health before_unknown = collector_service_health_copy();
	char unknown[] = "inspect 999999";
	collector_service_command(&character, unknown, CMD_COLLECTOR);
	const collector_service_health after_unknown = collector_service_health_copy();
	assert(saw("not available to you") &&
	       after_unknown.submitted_details == before_unknown.submitted_details &&
	       after_unknown.rejected_details == before_unknown.rejected_details + 1);

	clear_messages();
	pipeline_accepts = false;
	collector_service_command(&character, inspect, CMD_COLLECTOR);
	assert(saw("records are busy"));
	pipeline_accepts = true;

	clear_messages();
	collector_service_command(&character, inspect, CMD_COLLECTOR);
	complete_detail(collector_listing_outcome::invalid_data);
	assert(alerts == 1 && saw("invalid record"));

	materialization_succeeds = false;
	clear_messages();
	submitted_completion(&character, true, committed, 0, submitted_payload);
	assert(materializations == 2 && alerts == 2 && saw("safely recorded"));

	const collector_service_health snapshot = collector_service_health_copy();
	assert(snapshot.pending_details == 0 && snapshot.submitted_details == 7 &&
	       snapshot.completed_details == 7 && snapshot.rejected_details == 3 &&
	       snapshot.submitted_purchases == 1 && snapshot.committed_purchases == 2 &&
	       snapshot.materialization_failures == 1);
	collector_service_reset_for_tests();
}
