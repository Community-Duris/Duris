#include "telemetry/telemetry_battle.h"
#include "telemetry/telemetry_battle_contract.h"

#include <cassert>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <string_view>
#include <vector>

bool forbid_battle_allocation = false;
std::FILE *battle_contract_export = nullptr;
unsigned battle_fixture_sequence = 0U;
void *operator new(std::size_t size)
{
	if (forbid_battle_allocation)
		std::abort();
	if (void *value = std::malloc(size))
		return value;
	throw std::bad_alloc();
}
void operator delete(void *value) noexcept
{
	std::free(value);
}
void operator delete(void *value, std::size_t) noexcept
{
	std::free(value);
}

namespace
{
struct allocation_guard
{
	allocation_guard() { forbid_battle_allocation = true; }
	~allocation_guard() { forbid_battle_allocation = false; }
};

struct fixture
{
	std::unique_ptr<telemetry_battle_state> state = std::make_unique<telemetry_battle_state>();
	std::vector<telemetry_battle_fact> facts;
	int reject_after = -1;
	unsigned fixture_id = ++battle_fixture_sequence;
	fixture()
	{
		facts.reserve(5000U);
		assert(telemetry_battle_state_init(state.get(), { 101U, 202U },
						   { 11U, 22U, 33U, 4U, 5U, -1, 0U }, 100'000U));
	}
};

bool sink(void *context, const telemetry_battle_fact &fact) noexcept
{
	auto &test = *static_cast<fixture *>(context);
	assert(telemetry_battle_fact_is_valid(fact));
	std::array<std::uint8_t, TELEMETRY_BATTLE_WIRE_BYTES> wire{};
	assert(telemetry_battle_fact_encode(fact, wire.data(), wire.size()));
	telemetry_battle_fact decoded{};
	assert(telemetry_battle_fact_decode(wire.data(), wire.size(), &decoded));
	assert(telemetry_battle_fact_same_key(fact, decoded) &&
	       telemetry_battle_fact_equal(fact, decoded));
	assert(fact.definition_version == TELEMETRY_BATTLE_DEFINITION_VERSION);
	assert(fact.battle.producer.boot_id == 101U && fact.battle.sequence != 0U);
	assert(fact.fact_count != 0U && fact.fact_index < fact.fact_count);
	assert(fact.fact_sequence != 0U && fact.revision != 0U);
	assert(fact.actor_count <= TELEMETRY_BATTLE_MAX_ACTORS);
	assert(fact.active_actor_count <= fact.actor_count &&
	       fact.observed_owner_count <= fact.active_actor_count);
	assert(telemetry_quality_mask_is_valid(fact.quality_flags));
	if (test.reject_after == 0)
		return false;
	if (test.reject_after > 0)
		--test.reject_after;
	test.facts.push_back(fact);
	if (battle_contract_export)
	{
		std::fprintf(battle_contract_export, "{\"case\":%u,\"wire\":\"", test.fixture_id);
		for (auto byte : wire)
			std::fprintf(battle_contract_export, "%02x", byte);
		std::fputs("\"}\n", battle_contract_export);
	}
	return true;
}

telemetry_battle_actor_context player(int pid, telemetry_id group = 0U)
{
	telemetry_battle_actor_context context{};
	context.actor = { static_cast<telemetry_id>(pid),      pid, static_cast<telemetry_id>(pid),
			  telemetry_combat_actor_kind::player, {},  20U };
	context.encounter = { { 101U, 202U }, static_cast<telemetry_sequence>(pid) };
	context.session = { { 101U, 202U }, static_cast<telemetry_sequence>(pid) };
	context.dimensions = { 5U, 1U, 2U, 3U, 700, 1U };
	context.group_key = group;
	context.group_revision = group ? 1U : 0U;
	context.context_version = TELEMETRY_BATTLE_ACTOR_CONTEXT_VERSION;
	return context;
}

telemetry_battle_actor_context npc(std::uint64_t generation, int owner = 0)
{
	auto context = player(1);
	context.actor.actor_id = TELEMETRY_BATTLE_NPC_GENERATION_TAG | generation;
	context.actor.actor_pid = TELEMETRY_UNKNOWN_PID;
	context.actor.kind = owner ? telemetry_combat_actor_kind::pet :
				     telemetry_combat_actor_kind::npc;
	context.actor.owner_subject_id = owner;
	context.encounter = {};
	context.session = {};
	return context;
}

telemetry_battle_update observe(fixture &test, telemetry_battle_relation relation,
				const telemetry_battle_actor_context &source,
				const telemetry_battle_actor_context &target,
				telemetry_monotonic_usec at)
{
	allocation_guard no_allocation;
	return telemetry_battle_observe(
		test.state.get(), relation, source, target, at,
		1'700'000'000'000'000LL + static_cast<telemetry_utc_usec>(at), sink, &test);
}

telemetry_battle_actor_key actor_key(const telemetry_battle_actor_context &context)
{
	return { context.actor.actor_id, context.actor.kind };
}

const telemetry_battle_fact &summary(const fixture &test, telemetry_id id)
{
	for (auto row = test.facts.rbegin(); row != test.facts.rend(); ++row)
		if (row->kind == telemetry_battle_fact_kind::actor_summary &&
		    row->actor.actor.actor_id == id)
			return *row;
	std::abort();
}

void close(fixture &test, telemetry_battle_id id, telemetry_monotonic_usec at)
{
	allocation_guard no_allocation;
	assert(telemetry_battle_close(test.state.get(), id, telemetry_battle_close_reason::shutdown,
				      at, TELEMETRY_UTC_UNKNOWN, sink, &test)
		       .outcome == telemetry_battle_outcome::accepted);
}

void complete_packets(const fixture &test)
{
	for (std::size_t start = 0U; start < test.facts.size();)
	{
		const auto &first = test.facts[start];
		assert(start + first.fact_count <= test.facts.size());
		assert(telemetry_battle_packet_is_valid(&first, first.fact_count));
		telemetry_battle_packet_state packet{};
		{
			allocation_guard no_allocation;
			for (std::size_t index = first.fact_count; index != 0U; --index)
			{
				const auto received = telemetry_battle_packet_receive(
					&packet, test.facts[start + index - 1U]);
				assert(received ==
				       (index == 1U ? telemetry_battle_packet_result::complete :
						      telemetry_battle_packet_result::pending));
				assert(telemetry_battle_packet_receive(
					       &packet, test.facts[start + index - 1U]) ==
				       telemetry_battle_packet_result::duplicate_identical);
			}
		}
		assert(packet.complete && packet.received == first.fact_count);
		for (std::uint16_t index = 0U; index < first.fact_count; ++index)
		{
			const auto &row = test.facts[start + index];
			assert(row.battle.sequence == first.battle.sequence &&
			       row.revision == first.revision);
			assert(row.fact_index == index && row.fact_count == first.fact_count);
			assert(row.fact_sequence == first.fact_sequence + index);
			assert(row.inactivity_grace_usec == 100'000U);
			if (row.kind == telemetry_battle_fact_kind::actor_context ||
			    row.kind == telemetry_battle_fact_kind::actor_summary)
				assert(telemetry_battle_actor_context_is_valid(row.actor));
			if (row.kind == telemetry_battle_fact_kind::actor_summary)
				assert(row.effort.present_usec ==
				       row.effort.pve_usec + row.effort.pvp_usec +
					       row.effort.mixed_usec +
					       row.effort.unknown_mode_usec);
		}
		const auto terminal_kind = test.facts[start + first.fact_count - 1U].kind;
		assert(terminal_kind == telemetry_battle_fact_kind::cut ||
		       terminal_kind == telemetry_battle_fact_kind::close);
		start += first.fact_count;
	}
}

void duel_reinforcement_support_and_departure()
{
	fixture test;
	const auto a = player(1), b = player(2), reinforcement = player(3), healer = player(4);
	assert(observe(test, telemetry_battle_relation::support, healer, a, 0U).outcome ==
	       telemetry_battle_outcome::not_found);
	assert(test.facts.empty());
	const auto started = observe(test, telemetry_battle_relation::hostile, a, b, 0U);
	assert(started.outcome == telemetry_battle_outcome::accepted);
	const auto rows = test.facts.size();
	assert(observe(test, telemetry_battle_relation::hostile, a, b, 0U).outcome ==
	       telemetry_battle_outcome::idempotent);
	assert(test.facts.size() == rows);
	assert(observe(test, telemetry_battle_relation::hostile, reinforcement, a, 1000U)
		       .battle.sequence == started.battle.sequence);
	assert(observe(test, telemetry_battle_relation::support, healer, a, 3000U).outcome ==
	       telemetry_battle_outcome::accepted);
	assert(observe(test, telemetry_battle_relation::support, a, player(99), 3000U).outcome ==
	       telemetry_battle_outcome::
		       not_found); // A bystander is not attached because its healer participates.
	assert(telemetry_battle_leave(test.state.get(), actor_key(healer), 4000U, 44, sink, &test)
		       .outcome == telemetry_battle_outcome::accepted);
	close(test, started.battle, 6000U);
	assert(summary(test, 1).effort.present_usec == 6000U);
	assert(summary(test, 1).effort.outnumbered_owner_usec == 4000U);
	assert(summary(test, 1).effort.pvp_usec == 6000U);
	assert(summary(test, 3).effort.present_usec == 5000U);
	assert(summary(test, 4).effort.present_usec == 1000U);
	assert(summary(test, 4).effort.contributor_usec == 1000U);
	assert((summary(test, 4).roles & TELEMETRY_BATTLE_ROLE_SUPPORT) != 0U);
	assert(test.facts.back().kind == telemetry_battle_fact_kind::close &&
	       test.facts.back().end_censored);
	assert(telemetry_battle_close(test.state.get(), started.battle,
				      telemetry_battle_close_reason::shutdown, 6000U, 0, sink,
				      &test)
		       .outcome == telemetry_battle_outcome::idempotent);
	assert(telemetry_battle_close(test.state.get(), started.battle,
				      telemetry_battle_close_reason::copyover, 6000U, 0, sink,
				      &test)
		       .outcome == telemetry_battle_outcome::duplicate_conflict);
	complete_packets(test);
}

void group_presence_context_flee_rejoin_and_chase()
{
	fixture test;
	const auto group = TELEMETRY_GROUP_GENERATION_TAG | 11U;
	const auto a = player(1, group), b = player(2), member = player(3, group);
	const auto started = observe(test, telemetry_battle_relation::hostile, a, b, 0U);
	assert(observe(test, telemetry_battle_relation::group_presence, a, player(8), 1000U)
		       .outcome == telemetry_battle_outcome::invalid);
	assert(observe(test, telemetry_battle_relation::group_presence, a, member, 1000U).outcome ==
	       telemetry_battle_outcome::accepted);
	assert(telemetry_battle_leave(test.state.get(), actor_key(a), 2000U, 22, sink, &test)
		       .outcome == telemetry_battle_outcome::accepted);
	auto chased = a;
	chased.dimensions.zone_vnum = 701;
	assert(observe(test, telemetry_battle_relation::hostile, chased, b, 3000U).battle.sequence ==
	       started.battle.sequence);
	auto departed = member;
	departed.group_key = 0U;
	departed.group_revision = 0U;
	assert(telemetry_battle_context(test.state.get(), departed, 4000U, 44, sink, &test)
		       .outcome == telemetry_battle_outcome::accepted);
	close(test, started.battle, 6000U);
	assert(summary(test, 1).effort.present_usec == 5000U);
	assert(summary(test, 1).actor.dimensions.zone_vnum == 701);
	assert(summary(test, 3).effort.present_usec == 3000U);
	assert(summary(test, 3).effort.contributor_usec == 0U);
	assert(summary(test, 3).active == 0U);
	complete_packets(test);
}

void pve_mixed_pvp_and_pet_lifetimes()
{
	fixture test;
	const auto a = player(1), b = player(2), mob = npc(1), pet = npc(2, 2);
	const auto started = observe(test, telemetry_battle_relation::hostile, a, mob, 0U);
	assert(observe(test, telemetry_battle_relation::hostile, b, a, 1000U).outcome ==
	       telemetry_battle_outcome::accepted);
	assert(telemetry_battle_leave(test.state.get(), actor_key(mob), 3000U, 33, sink, &test)
		       .outcome == telemetry_battle_outcome::accepted);
	assert(observe(test, telemetry_battle_relation::hostile, pet, a, 4000U).outcome ==
	       telemetry_battle_outcome::accepted);
	assert(test.facts.back().observed_owner_count == 2U &&
	       test.facts.back().active_actor_count == 3U);
	assert(test.facts.back().side_status ==
	       telemetry_battle_side_status::qualified_observed_graph);
	auto unowned = pet;
	unowned.actor.kind = telemetry_combat_actor_kind::npc;
	unowned.actor.owner_subject_id = 0U;
	assert(telemetry_battle_context(test.state.get(), unowned, 5000U, 55, sink, &test).outcome ==
	       telemetry_battle_outcome::accepted);
	assert(test.facts.back().actor_count == 4U &&
	       test.facts.back().mode == telemetry_encounter_mode::mixed);
	close(test, started.battle, 6000U);
	const auto &effort = summary(test, 1).effort;
	assert(effort.present_usec == 6000U && effort.pve_usec == 1000U &&
	       effort.mixed_usec == 3000U && effort.pvp_usec == 2000U &&
	       effort.outnumbered_owner_usec == 0U);
	assert(summary(test, pet.actor.actor_id).actor.actor.kind ==
	       telemetry_combat_actor_kind::npc);
	assert(summary(test, pet.actor.actor_id).effort.present_usec == 2000U);
	auto prototype = npc(3);
	prototype.actor.actor_id = 123U;
	assert(!telemetry_battle_actor_context_is_valid(prototype));
	complete_packets(test);
}

void independent_clones_and_proven_merge()
{
	fixture test;
	const auto a = player(1), b = player(2);
	const auto first = observe(test, telemetry_battle_relation::hostile, a, npc(1), 0U);
	const auto second = observe(test, telemetry_battle_relation::hostile, b, npc(2), 1000U);
	assert(first.battle.sequence != second.battle.sequence);
	const auto linked = observe(test, telemetry_battle_relation::hostile, b, a, 2000U);
	assert(linked.battle.sequence == first.battle.sequence &&
	       linked.related_battle.sequence == second.battle.sequence);
	assert(linked.quality_flags & TELEMETRY_QUALITY_CONTEXT_UNKNOWN);
	unsigned aliases = 0U;
	for (const auto &row : test.facts)
		aliases += row.kind == telemetry_battle_fact_kind::merge_alias;
	assert(aliases == 1U);
	assert(observe(test, telemetry_battle_relation::hostile, b, a, 2000U).outcome ==
	       telemetry_battle_outcome::idempotent);
	assert(telemetry_battle_close(test.state.get(), second.battle,
				      telemetry_battle_close_reason::shutdown, 3000U, 33, sink,
				      &test)
		       .outcome == telemetry_battle_outcome::not_found);
	close(test, first.battle, 4000U);
	assert(summary(test, 1).effort.present_usec == 4000U &&
	       summary(test, 2).effort.present_usec == 3000U);
	complete_packets(test);
}

void ambiguous_coalitions_and_timeout_bounds()
{
	fixture test;
	const auto a = player(1), b = player(2), c = player(3);
	const auto started = observe(test, telemetry_battle_relation::hostile, a, b, 0U);
	assert(observe(test, telemetry_battle_relation::hostile, b, c, 1000U).outcome ==
	       telemetry_battle_outcome::accepted);
	assert(observe(test, telemetry_battle_relation::hostile, c, a, 2000U).outcome ==
	       telemetry_battle_outcome::accepted);
	assert(test.facts.back().side_status == telemetry_battle_side_status::ambiguous);
	assert(telemetry_battle_close(test.state.get(), started.battle,
				      telemetry_battle_close_reason::inactivity, 3000U, 33, sink,
				      &test)
		       .outcome == telemetry_battle_outcome::invalid);
	assert(telemetry_battle_expire(test.state.get(), 101'999U, 100, sink, &test)
		       .facts_attempted == 0U);
	const auto expired = telemetry_battle_expire(test.state.get(), 102'000U,
						     TELEMETRY_UTC_UNKNOWN, sink, &test);
	assert(expired.outcome == telemetry_battle_outcome::accepted);
	assert(test.facts.back().close_reason == telemetry_battle_close_reason::inactivity);
	assert(test.facts.back().at_monotonic_usec == 102'000U &&
	       test.facts.back().observed_through_monotonic_usec == 2000U);
	assert(summary(test, 1).effort.present_usec == 2000U);
	assert(observe(test, telemetry_battle_relation::hostile, a, b, 103'000U).battle.sequence !=
	       started.battle.sequence);
	complete_packets(test);
}

void source_scopes_roster_uncertainty_and_inline_expiry()
{
	fixture test;
	auto foreign = player(1);
	foreign.encounter.producer.boot_id = 999U;
	assert(observe(test, telemetry_battle_relation::hostile, foreign, player(2), 0U).outcome ==
	       telemetry_battle_outcome::invalid);
	assert(test.facts.empty());
	auto legacy_session = player(1);
	legacy_session.session.producer.boot_id =
		999U; // Copyover may retain an older logical session.
	const auto first =
		observe(test, telemetry_battle_relation::hostile, legacy_session, player(2), 0U);
	const auto renewed = observe(test, telemetry_battle_relation::hostile, legacy_session,
				     player(2), 100'000U);
	assert(renewed.battle.sequence != first.battle.sequence && renewed.facts_attempted == 8U &&
	       renewed.facts_accepted == 8U); // Three closure facts plus five new-start facts.
	complete_packets(test);
	fixture roster;
	auto a = player(1, TELEMETRY_GROUP_GENERATION_TAG | 12U);
	auto b = player(2, TELEMETRY_GROUP_GENERATION_TAG | 12U);
	b.group_revision = 2U;
	const auto started = observe(roster, telemetry_battle_relation::hostile, a, b, 0U);
	assert(roster.facts.back().side_status == telemetry_battle_side_status::partial);
	b.group_revision = 1U;
	assert(telemetry_battle_context(roster.state.get(), b, 1000U, 33, sink, &roster).outcome ==
	       telemetry_battle_outcome::accepted);
	assert(roster.facts.back().side_status == telemetry_battle_side_status::ambiguous);
	close(roster, started.battle, 2000U);
	assert(summary(roster, 1).effort.unknown_side_usec == 2000U);
	complete_packets(roster);
}

telemetry_battle_id graph(fixture &test, int first_pid, int player_count,
			  std::uint64_t npc_generation)
{
	const auto mob = npc(npc_generation);
	const auto started =
		observe(test, telemetry_battle_relation::hostile, player(first_pid), mob, 0U);
	for (int offset = 1; offset < player_count; ++offset)
		assert(observe(test, telemetry_battle_relation::hostile, player(first_pid + offset),
			       mob, 0U)
			       .outcome == telemetry_battle_outcome::accepted);
	return started.battle;
}

void merge_at_capacity_and_sequence_exhaustion()
{
	fixture merged;
	const auto first = graph(merged, 1, 31, 1U);
	const auto second = graph(merged, 101, 31, 2U);
	const auto joined =
		observe(merged, telemetry_battle_relation::hostile, player(101), player(1), 1000U);
	assert(joined.battle.sequence == first.sequence &&
	       joined.related_battle.sequence == second.sequence);
	assert(merged.facts.back().actor_count == 64U &&
	       merged.facts.back().observed_owner_count == 62U);
	close(merged, first, 2000U);
	std::uint64_t total = 0U;
	for (const auto &row : merged.facts)
		if (row.kind == telemetry_battle_fact_kind::actor_summary)
			total += row.effort.present_usec;
	assert(total == 128'000U); // Each retained actor contributes two milliseconds exactly once.
	complete_packets(merged);
	fixture refused;
	graph(refused, 1, 32, 1U);
	graph(refused, 101, 32, 2U);
	const auto rejected =
		observe(refused, telemetry_battle_relation::hostile, player(101), player(1), 1000U);
	assert(rejected.outcome == telemetry_battle_outcome::capacity_full &&
	       rejected.related_battle.sequence != 0U);
	assert(telemetry_battle_close_all(refused.state.get(),
					  telemetry_battle_close_reason::shutdown, 2000U, 33, sink,
					  &refused)
		       .facts_attempted == 68U);
	assert(refused.facts.back().quality_flags & TELEMETRY_QUALITY_CARDINALITY_OVERFLOW);
	complete_packets(refused);
	fixture sequence;
	sequence.state->next_sequence = std::numeric_limits<telemetry_sequence>::max();
	const auto last =
		observe(sequence, telemetry_battle_relation::hostile, player(1), player(2), 0U);
	assert(last.battle.sequence == std::numeric_limits<telemetry_sequence>::max());
	assert(observe(sequence, telemetry_battle_relation::hostile, player(3), player(4), 0U)
		       .outcome == telemetry_battle_outcome::capacity_full);
	close(sequence, last.battle, 1000U);
	complete_packets(sequence);
}

void loss_clock_and_cap_refusals()
{
	fixture lost;
	lost.reject_after = 2;
	const auto result =
		observe(lost, telemetry_battle_relation::hostile, player(1), player(2), 0U);
	assert(result.outcome == telemetry_battle_outcome::sink_rejected &&
	       result.facts_attempted == 5U);
	assert(result.quality_flags & TELEMETRY_QUALITY_QUEUE_DROP);
	lost.reject_after = -1;
	close(lost, result.battle, 1000U);
	assert(lost.facts.back().quality_flags & TELEMETRY_QUALITY_QUEUE_DROP);
	assert(lost.facts.back().side_status == telemetry_battle_side_status::partial);
	fixture clock;
	const auto initial =
		observe(clock, telemetry_battle_relation::hostile, player(1), player(2), 1000U);
	assert(observe(clock, telemetry_battle_relation::hostile, player(1), player(2), 999U)
		       .outcome == telemetry_battle_outcome::invalid);
	assert(observe(clock, telemetry_battle_relation::hostile, player(3), player(4), 999U)
		       .outcome ==
	       telemetry_battle_outcome::
		       invalid); // No fresh slot can conceal a producer clock regression.
	close(clock, initial.battle, 2000U);
	assert(clock.facts.back().quality_flags & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY);
	fixture capped;
	const auto started =
		observe(capped, telemetry_battle_relation::hostile, player(1), player(2), 0U);
	for (int pid = 3; pid <= 64; ++pid)
		assert(observe(capped, telemetry_battle_relation::hostile, player(pid), player(1),
			       pid)
			       .outcome == telemetry_battle_outcome::accepted);
	const auto refused =
		observe(capped, telemetry_battle_relation::hostile, player(65), player(1), 1000U);
	assert(refused.outcome == telemetry_battle_outcome::capacity_full &&
	       (refused.quality_flags & TELEMETRY_QUALITY_CARDINALITY_OVERFLOW));
	close(capped, started.battle, 2000U);
	assert(capped.facts.back().actor_count == 64U &&
	       capped.facts.back().dropped_actor_count == 1U);
	fixture rows;
	const auto limited =
		observe(rows, telemetry_battle_relation::hostile, player(1), player(2), 0U);
	auto actor = player(1);
	unsigned refused_count = 0U;
	for (unsigned index = 1U; index < 2300U; ++index)
	{
		actor.dimensions.level_band = index % 2U + 5U;
		const auto updated =
			telemetry_battle_context(rows.state.get(), actor, index, 33, sink, &rows);
		refused_count += updated.outcome == telemetry_battle_outcome::capacity_full;
	}
	assert(refused_count != 0U &&
	       rows.facts.size() <= TELEMETRY_BATTLE_MAX_MUTATION_FACTS + 1U);
	close(rows, limited.battle, 3000U);
	assert(rows.facts.back().quality_flags & TELEMETRY_QUALITY_CONTEXT_OVERFLOW);
	fixture slots;
	for (int index = 0; index < 128; ++index)
		assert(observe(slots, telemetry_battle_relation::hostile, player(index * 2 + 1),
			       player(index * 2 + 2), 0U)
			       .outcome == telemetry_battle_outcome::accepted);
	assert(observe(slots, telemetry_battle_relation::hostile, player(1001), player(1002), 0U)
		       .outcome == telemetry_battle_outcome::capacity_full);
	assert(telemetry_battle_close_all(slots.state.get(),
					  telemetry_battle_close_reason::copyover, 1000U, 33, sink,
					  &slots)
		       .facts_attempted == 384U);
	assert(telemetry_battle_close_all(slots.state.get(), telemetry_battle_close_reason::unknown,
					  1000U, 33, sink, &slots)
		       .outcome == telemetry_battle_outcome::invalid);
}

void packet_loss_replay_and_malformed_values()
{
	fixture test;
	const auto started =
		observe(test, telemetry_battle_relation::hostile, player(1), player(2), 10U);
	assert(test.facts.size() == 5U);
	const auto original = test.facts;
	telemetry_battle_packet_state packet{};
	for (std::size_t index = 0U; index < 4U; ++index)
		assert(telemetry_battle_packet_receive(&packet, original[index]) ==
		       telemetry_battle_packet_result::pending);
	assert(!packet.complete && packet.received == 4U);
	assert(!telemetry_battle_packet_is_valid(original.data(), 4U));
	assert(telemetry_battle_packet_receive(&packet, original[4]) ==
	       telemetry_battle_packet_result::complete);
	assert(telemetry_battle_packet_receive(&packet, original[4]) ==
	       telemetry_battle_packet_result::duplicate_identical);
	auto changed = original[1];
	++changed.actor.actor.power_band;
	assert(telemetry_battle_fact_is_valid(changed));
	assert(telemetry_battle_fact_same_key(changed, original[1]) &&
	       !telemetry_battle_fact_equal(changed, original[1]));
	assert(telemetry_battle_packet_receive(&packet, changed) ==
	       telemetry_battle_packet_result::duplicate_conflict);
	assert(!packet.complete && packet.conflicted);
	assert(telemetry_battle_packet_receive(&packet, original[1]) ==
	       telemetry_battle_packet_result::duplicate_conflict);
	telemetry_battle_packet_reset(&packet);
	assert(packet.count == 0U && packet.received == 0U && !packet.conflicted);
	assert(telemetry_battle_packet_receive(&packet, original[1]) ==
	       telemetry_battle_packet_result::pending);
	auto foreign = original[0];
	++foreign.battle.sequence;
	assert(telemetry_battle_packet_receive(&packet, foreign) ==
	       telemetry_battle_packet_result::other_packet);
	assert(packet.received == 1U && !packet.complete);
	changed = original[1];
	++changed.fact_sequence;
	assert(telemetry_battle_packet_receive(&packet, changed) ==
	       telemetry_battle_packet_result::duplicate_conflict);
	assert(!telemetry_battle_packet_is_valid(nullptr, 5U));
	assert(!telemetry_battle_packet_is_valid(original.data(), 0U));
	assert(!telemetry_battle_packet_is_valid(original.data(),
						 TELEMETRY_BATTLE_PACKET_MAX_FACTS + 1U));
	auto malformed = original;
	malformed[2].actor = malformed[1].actor;
	assert(!telemetry_battle_packet_is_valid(malformed.data(), malformed.size()));
	malformed = original;
	malformed[3].actor.actor.power_band++;
	assert(!telemetry_battle_packet_is_valid(malformed.data(), malformed.size()));
	malformed = original;
	for (auto &row : malformed)
		row.observed_owner_count = 1U;
	assert(!telemetry_battle_packet_is_valid(malformed.data(), malformed.size()));
	malformed = original;
	malformed.back().scope.config_id++;
	assert(!telemetry_battle_packet_is_valid(malformed.data(), malformed.size()));
	malformed = original;
	for (std::size_t index = 2U; index < malformed.size(); ++index)
	{
		malformed[index].quality_flags |= TELEMETRY_QUALITY_QUEUE_DROP;
		malformed[index].side_status = telemetry_battle_side_status::partial;
		malformed[index].side = 0U;
	}
	assert(telemetry_battle_packet_is_valid(malformed.data(), malformed.size()));
	malformed.back().quality_flags &= ~TELEMETRY_QUALITY_QUEUE_DROP;
	assert(!telemetry_battle_packet_is_valid(malformed.data(), malformed.size()));
	std::array<std::uint8_t, TELEMETRY_BATTLE_WIRE_BYTES> bytes{};
	assert(telemetry_battle_fact_encode(original[1], bytes.data(), bytes.size()));
	assert(bytes[7] == 101U && bytes[15] == 202U && bytes[23] == 1U);
	telemetry_battle_fact decoded = original[1];
	assert(!telemetry_battle_fact_decode(bytes.data(), bytes.size() - 1U, &decoded));
	assert(telemetry_battle_id_is_zero(decoded.battle));
	assert(!telemetry_battle_fact_encode(original[1], bytes.data(), bytes.size() + 1U));
	assert(!telemetry_battle_fact_decode(nullptr, bytes.size(), &decoded));
	assert(!telemetry_battle_fact_decode(bytes.data(), bytes.size(), nullptr));
	auto invalid = original[1];
	invalid.actor.actor.reserved[0] = 1U;
	assert(!telemetry_battle_fact_encode(invalid, bytes.data(), bytes.size()));
	invalid = original[1];
	invalid.effort.pvp_usec = 1U;
	assert(!telemetry_battle_fact_is_valid(invalid));
	invalid = original[1];
	invalid.actor.session.session_seq = 0U;
	assert(!telemetry_battle_fact_is_valid(invalid));
	invalid = original[1];
	invalid.quality_flags |= TELEMETRY_QUALITY_QUEUE_DROP;
	assert(!telemetry_battle_fact_is_valid(invalid));
	close(test, started.battle, 20U);
	malformed.assign(test.facts.end() - 3U, test.facts.end());
	malformed[1].actor = malformed[0].actor;
	assert(!telemetry_battle_packet_is_valid(malformed.data(), malformed.size()));
	invalid = test.facts.back();
	invalid.end_censored = 0U;
	assert(!telemetry_battle_fact_is_valid(invalid));
	complete_packets(test);
}
} // namespace

int main(int argc, char **argv)
{
	if (argc == 3)
	{
		assert(std::string_view(argv[1]) == "--export");
		battle_contract_export = std::fopen(argv[2], "wb");
		assert(battle_contract_export);
	}
	else
		assert(argc == 1);
	duel_reinforcement_support_and_departure();
	group_presence_context_flee_rejoin_and_chase();
	pve_mixed_pvp_and_pet_lifetimes();
	independent_clones_and_proven_merge();
	ambiguous_coalitions_and_timeout_bounds();
	source_scopes_roster_uncertainty_and_inline_expiry();
	merge_at_capacity_and_sequence_exhaustion();
	loss_clock_and_cap_refusals();
	packet_loss_replay_and_malformed_values();
	if (battle_contract_export)
		assert(std::fclose(battle_contract_export) == 0);
	std::printf("bounded battle association journeys passed: state=%zu fact=%zu bytes\n",
		    sizeof(telemetry_battle_state), sizeof(telemetry_battle_fact));
}
