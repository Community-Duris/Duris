#include "economy/boon_ram.h"
#include "economy/boon.h"
#include <cassert>
#include <cerrno>

static critical_command seal(critical_command request, const critical_apply_result &result)
{
	request.schema_version = CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION;
	request.outcome_revision = result.durable_revision;
	request.outcome_payload.assign(result.result_payload.begin(),
				       result.result_payload.begin() + result.result_size);
	return request;
}

int main()
{
	boon_ram_state initial;
	flatfile_boon_definition definition;
	definition.id = 1;
	definition.active = true;
	definition.type = BTYPE_STATS;
	definition.option = BOPT_MOB;
	definition.criteria = 2;
	definition.criteria2 = 100;
	definition.bonus = 3;
	definition.target_pid = 42;
	initial.definitions.emplace(1, definition);
	initial.stat_revisions.emplace(42, 9);
	boon_ram_seed(initial);
	critical_operation_id id{};
	id.bytes[0] = 1;
	critical_command request;
	boon_reward_payload payload{ 42, 1, 10, 2, BOPT_MOB, 0, 100, 0, 1 };
	assert(boon_reward_command_build(&request, id, payload));
	const auto prepared = boon_ram_prepare(request);
	assert(prepared.outcome == critical_apply_outcome::applied);
	double progress = 0;
	assert(!boon_ram_progress(1, 42, &progress)); // A failed append cannot award anything.
	auto committed = seal(request, prepared);
	auto corrupt = committed;
	corrupt.outcome_payload.back() ^= 1;
	assert(!boon_ram_publish(corrupt));
	assert(boon_ram_publish(committed));
	assert(boon_ram_progress(1, 42, &progress) && progress == 1);
	id.bytes[0] = 2;
	assert(boon_reward_command_build(&request, id, payload));
	auto second = boon_ram_prepare(request);
	flatfile_boon_player_projection shop;
	assert(boon_ram_player(42, &shop) && shop.stats == 0);
	assert(boon_ram_publish(seal(request, second)));
	assert(boon_ram_player(42, &shop) && shop.stats == 3);
	assert(boon_ram_definition(1, &definition) && !definition.active);
	assert(boon_ram_progress(1, 42, &progress) && progress == -1);
	// Retries use the durable receipt, not preparation. Even an independent later
	// event cannot award this one-shot boon again.
	id.bytes[0] = 3;
	assert(boon_reward_command_build(&request, id, payload));
	auto later = boon_ram_prepare(request);
	boon_reward_result result;
	assert(boon_reward_command_decode_result(later.result_payload.data(), later.result_size,
						 &result));
	assert(result.entry_count == 0);
	assert(boon_ram_publish(seal(request, later)));
	assert(boon_ram_player(42, &shop) && shop.stats == 3);

	id.bytes[0] = 4;
	assert(boon_shop_command_build(&request, id, { 42, 0 }));
	assert(boon_ram_prepare(request, 100).error_code == EALREADY);
	auto purchase = boon_ram_prepare(request, 98);
	assert(purchase.outcome == critical_apply_outcome::applied);
	assert(boon_ram_player(42, &shop) && shop.stats == 3);
	assert(boon_ram_publish(seal(request, purchase)));
	boon_shop_result bought;
	assert(boon_shop_command_decode_result(purchase.result_payload.data(), purchase.result_size,
					       &bought));
	assert(bought.stat_value == 99 && bought.remaining_stat_points == 2 &&
	       bought.stat_revision == 10);
	assert(boon_ram_player(42, &shop) && shop.stats == 2);
	assert(!boon_ram_publish(seal(request, purchase))); // Publication cannot debit twice.

	initial.definitions.at(1).criteria = 1;
	initial.shops[42].stats = INT64_MAX;
	boon_ram_seed(initial);
	id.bytes[0] = 5;
	assert(boon_reward_command_build(&request, id, payload));
	assert(boon_ram_prepare(request).error_code == ERANGE);
	assert(boon_ram_player(42, &shop) && shop.stats == INT64_MAX);
	assert(!boon_ram_progress(1, 42, &progress));
	return 0;
}
