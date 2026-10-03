#include "telemetry/telemetry_battle.h"
#include "telemetry/telemetry_battle_contract.h"
#include "telemetry/telemetry_battle_contribution.h"

#include <cassert>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <string_view>
#include <vector>

bool forbid_contribution_allocation = false;
void *operator new(std::size_t size)
{
	if (forbid_contribution_allocation)
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
using context = telemetry_battle_contribution_context;
using payload = telemetry_battle_contribution_payload;
using outcome = telemetry_battle_contribution_outcome;
using end = telemetry_battle_contribution_end;
constexpr telemetry_producer_id producer{ 101U, 202U };
constexpr telemetry_encounter_source scope{ 11U, 22U, 33U, 4U, 5U, -1, 0U };
constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
constexpr telemetry_utc_usec epoch = 1'700'000'000'000'000LL;
unsigned fixture_sequence = 0U;
std::FILE *contract_export = nullptr;

template <class Callable> auto checked(Callable call)
{
	struct guard
	{
		guard() { forbid_contribution_allocation = true; }
		~guard() { forbid_contribution_allocation = false; }
	} no_allocation;
	return call();
}

context player(int pid, telemetry_sequence battle = 1U)
{
	context c{};
	c.battle = { producer, battle };
	c.scope = scope;
	c.actor.actor = { static_cast<telemetry_id>(pid),      pid, static_cast<telemetry_id>(pid),
			  telemetry_combat_actor_kind::player, {},  20U };
	c.actor.encounter = { producer, static_cast<telemetry_sequence>(pid) };
	c.actor.session = { producer, static_cast<telemetry_sequence>(pid) };
	c.actor.dimensions = { 5U, 1U, 2U, 3U, 700, 1U };
	c.actor.context_version = TELEMETRY_BATTLE_ACTOR_CONTEXT_VERSION;
	c.association_revision = 1U;
	c.association_fact_sequence = 5U;
	c.available_metrics = TELEMETRY_BC_METRICS;
	c.side_status = telemetry_battle_side_status::qualified_observed_graph;
	c.mode = telemetry_encounter_mode::pvp;
	c.side = 1U;
	assert(telemetry_battle_contribution_context_is_valid(c));
	return c;
}
context npc(telemetry_sequence generation, telemetry_subject_id owner = 0U)
{
	auto c = player(1);
	c.actor.actor.actor_id = TELEMETRY_BATTLE_NPC_GENERATION_TAG | generation;
	c.actor.actor.actor_pid = TELEMETRY_UNKNOWN_PID;
	c.actor.actor.owner_subject_id = owner;
	c.actor.actor.kind = owner ? telemetry_combat_actor_kind::pet :
				     telemetry_combat_actor_kind::npc;
	c.actor.encounter = {};
	c.actor.session = {};
	assert(telemetry_battle_contribution_context_is_valid(c));
	return c;
}
telemetry_battle_actor_key key(const context &c)
{
	return { c.actor.actor.actor_id, c.actor.actor.kind };
}
void advance(context &c)
{
	++c.association_revision;
	c.association_fact_sequence += 10U;
}

struct fixture
{
	std::unique_ptr<telemetry_battle_contribution_state> state =
		std::make_unique<telemetry_battle_contribution_state>();
	std::unique_ptr<telemetry_battle_state> graph = std::make_unique<telemetry_battle_state>();
	std::vector<payload> attempted;
	std::vector<payload> rows;
	std::vector<telemetry_battle_fact> facts;
	unsigned reject_remaining = 0U;
	unsigned fixture_id = ++fixture_sequence;
	fixture()
	{
		attempted.reserve(20'000U);
		rows.reserve(20'000U);
		facts.reserve(10'000U);
		assert(telemetry_battle_contribution_state_init(state.get(), producer, 11U, 22U));
		assert(telemetry_battle_state_init(graph.get(), producer, scope, 100'000U));
	}
	static bool sink(void *opaque, const payload &value) noexcept
	{
		auto &f = *static_cast<fixture *>(opaque);
		assert(telemetry_battle_contribution_payload_is_valid(value));
		std::array<std::uint8_t, TELEMETRY_BATTLE_CONTRIBUTION_WIRE_BYTES> wire{};
		telemetry_battle_contribution_payload decoded{};
		assert(telemetry_battle_contribution_encode(value, wire.data(), wire.size()));
		assert(telemetry_battle_contribution_decode(wire.data(), wire.size(), &decoded));
		assert(telemetry_battle_contribution_same_key(value, decoded) &&
		       telemetry_battle_contribution_equal(value, decoded));
		if (contract_export)
		{
			std::fprintf(contract_export, "{\"case\":%u,\"wire\":\"", f.fixture_id);
			for (auto byte : wire)
				std::fprintf(contract_export, "%02x", byte);
			std::fputs("\"}\n", contract_export);
		}
		f.attempted.push_back(value);
		if (f.reject_remaining)
		{
			--f.reject_remaining;
			return false;
		}
		f.rows.push_back(value);
		return true;
	}
	static bool graph_sink(void *opaque, const telemetry_battle_fact &fact) noexcept
	{
		assert(telemetry_battle_fact_is_valid(fact));
		static_cast<fixture *>(opaque)->facts.push_back(fact);
		return true;
	}
	void observe(const context &a, const context &b, telemetry_battle_relation relation,
		     telemetry_monotonic_usec at)
	{
		const auto before = facts.size();
		const auto r = checked(
			[&]
			{
				return telemetry_battle_observe(
					graph.get(), relation, a.actor, b.actor, at,
					epoch + static_cast<telemetry_utc_usec>(at), graph_sink,
					this);
			});
		assert(r.outcome == telemetry_battle_outcome::accepted ||
		       r.outcome == telemetry_battle_outcome::idempotent);
		if (before != facts.size())
			assert(telemetry_battle_packet_is_valid(facts.data() + before,
								facts.size() - before));
	}
	context association(telemetry_id actor)
	{
		for (const auto &s : graph->slots)
			if (s.occupied)
				for (const auto &a : s.actors)
					if (a.context.actor.actor_id == actor)
					{
						assert(a.active && !facts.empty());
						context c{};
						c.battle = s.id;
						c.scope = graph->scope;
						c.actor = a.context;
						c.association_revision = s.revision;
						c.association_fact_sequence = s.fact_sequence;
						c.available_metrics = TELEMETRY_BC_METRICS;
						c.side_status = s.side_status;
						c.mode = s.mode;
						c.side = a.side;
						c.quality_flags = s.quality_flags |
								  a.context.quality_flags;
						assert(telemetry_battle_contribution_context_is_valid(
							c));
						return c;
					}
		std::abort();
	}
	auto damage(const context &a, const context &b, std::uint64_t amount,
		    telemetry_monotonic_usec at, std::uint32_t modifiers = 0U)
	{
		return checked(
			[&]
			{
				return telemetry_battle_contribution_damage(
					state.get(), a, b, amount, at,
					epoch + static_cast<telemetry_utc_usec>(at), modifiers,
					sink, this);
			});
	}
	auto healing(const context &a, const context &b, std::uint64_t amount,
		     std::uint64_t effective, telemetry_monotonic_usec at)
	{
		return checked(
			[&]
			{
				return telemetry_battle_contribution_healing(
					state.get(), a, b, amount, effective, at,
					epoch + static_cast<telemetry_utc_usec>(at), 0U, sink,
					this);
			});
	}
	auto control(const context &a, const context &b, std::uint16_t amount,
		     telemetry_monotonic_usec at)
	{
		return checked(
			[&]
			{
				return telemetry_battle_contribution_control(
					state.get(), a, b, amount, at,
					epoch + static_cast<telemetry_utc_usec>(at), 0U, sink,
					this);
			});
	}
	auto cast(const context &c, telemetry_monotonic_usec at)
	{
		return checked(
			[&]
			{
				return telemetry_battle_contribution_cast_attempt(
					state.get(), c, at,
					epoch + static_cast<telemetry_utc_usec>(at), sink, this);
			});
	}
	auto finish(const context &c, bool completed, telemetry_monotonic_usec at)
	{
		return checked(
			[&]
			{
				return telemetry_battle_contribution_cast_finish(
					state.get(), c, completed, at,
					epoch + static_cast<telemetry_utc_usec>(at), sink, this);
			});
	}
	auto engage(const context &c, const telemetry_battle_actor_key *target,
		    telemetry_monotonic_usec at)
	{
		return checked(
			[&]
			{
				return telemetry_battle_contribution_engagement(
					state.get(), c, target, at,
					epoch + static_cast<telemetry_utc_usec>(at), sink, this);
			});
	}
	auto changed(const context &c, telemetry_monotonic_usec at)
	{
		return checked(
			[&]
			{
				return telemetry_battle_contribution_context_changed(
					state.get(), c, at,
					epoch + static_cast<telemetry_utc_usec>(at), sink, this);
			});
	}
	auto leave(const context &c, telemetry_monotonic_usec at, end reason = end::actor_left)
	{
		return checked(
			[&]
			{
				return telemetry_battle_contribution_leave(
					state.get(), c.battle, key(c),
					{ at, epoch + static_cast<telemetry_utc_usec>(at), at,
					  epoch + static_cast<telemetry_utc_usec>(at) },
					reason, sink, this);
			});
	}
	auto close(telemetry_sequence battle, telemetry_monotonic_usec observed,
		   telemetry_monotonic_usec decision = 0U, end reason = end::battle_ended)
	{
		if (!decision)
			decision = observed;
		return checked(
			[&]
			{
				return telemetry_battle_contribution_close(
					state.get(), { producer, battle },
					{ observed,
					  epoch + static_cast<telemetry_utc_usec>(observed),
					  decision,
					  epoch + static_cast<telemetry_utc_usec>(decision) },
					reason, sink, this);
			});
	}
	std::size_t active_battles() const
	{
		std::size_t count = 0U;
		for (const auto &s : state->slots)
			count += s.occupied;
		return count;
	}
	const payload &row(telemetry_id actor, std::size_t ordinal = 0U) const
	{
		for (const auto &r : rows)
			if (r.context.actor.actor.actor_id == actor)
			{
				if (!ordinal)
					return r;
				--ordinal;
			}
		std::abort();
	}
};

void shared_association_and_conservation()
{
	fixture f;
	auto a = player(11), b = player(12), healer = player(13);
	f.observe(a, b, telemetry_battle_relation::hostile, 100U);
	a = f.association(11U);
	b = f.association(12U);
	assert(a.battle.sequence == b.battle.sequence && a.side != b.side);
	assert(f.damage(a, b, 700U, 101U).outcome == outcome::accepted);
	assert(f.damage(a, b, 112U, 102U).outcome == outcome::accepted);
	assert(f.damage(b, a, 51U, 103U).outcome == outcome::accepted);
	for (unsigned i = 0U; i < 1000U; ++i)
		assert(f.damage(a, b, 1U, 104U + i).rows_attempted == 0U);
	f.observe(healer, a, telemetry_battle_relation::support, 1200U);
	a = f.association(11U);
	b = f.association(12U);
	healer = f.association(13U);
	assert(f.healing(healer, a, 100U, 61U, 1201U).outcome == outcome::accepted);
	assert(f.healing(a, a, 10U, 4U, 1202U).outcome == outcome::accepted);
	assert(f.control(b, a, 3U, 1203U).outcome == outcome::accepted);
	assert(f.rows.empty());
	const auto r = f.close(a.battle.sequence, 1300U);
	assert(r.outcome == outcome::accepted && r.rows_attempted == 3U && r.rows_accepted == 3U);
	const auto &x = f.row(11U).counters, &y = f.row(12U).counters, &z = f.row(13U).counters;
	assert(x.damage_dealt == 1812U && y.damage_taken == 1812U);
	assert(x.damage_taken == 51U && y.damage_dealt == 51U);
	assert(x.healing_attempted == 10U && x.effective_healing == 4U && x.overhealing == 6U);
	assert(z.healing_attempted == 100U && z.effective_healing == 61U && z.overhealing == 39U);
	assert(x.healing_received == 65U && x.control_received == 3U &&
	       y.control_applications == 3U);
	assert((f.row(12U).modifier_flags & TELEMETRY_COMBAT_MODIFIER_CONTROL) != 0U);
	assert(f.row(11U).context.association_revision == 1U);
	assert(f.row(11U).last_association_revision == a.association_revision);
	assert(f.row(11U).last_association_fact_sequence == a.association_fact_sequence);
	std::uint64_t dealt = 0U, taken = 0U, effective = 0U, received = 0U;
	for (const auto &v : f.rows)
	{
		dealt += v.counters.damage_dealt;
		taken += v.counters.damage_taken;
		effective += v.counters.effective_healing;
		received += v.counters.healing_received;
	}
	assert(dealt == taken && effective == received);
	assert(f.close(a.battle.sequence, 1301U).outcome == outcome::not_found &&
	       f.rows.size() == 3U);
}

void disjoint_contexts_and_live_identity()
{
	fixture f;
	auto a = player(21), pet = npc(501U, 21U);
	assert(f.damage(a, pet, 7U, 10U).outcome == outcome::accepted);
	advance(a);
	advance(pet);
	a.actor.group_key = TELEMETRY_GROUP_GENERATION_TAG | 9U;
	a.actor.group_revision = 1U;
	a.actor.dimensions.group_size = 3U;
	pet.actor.actor.owner_subject_id = 22U;
	assert(f.damage(pet, a, 11U, 20U).rows_attempted == 2U);
	assert(f.row(21U).counters.damage_dealt == 7U);
	assert(f.row(pet.actor.actor.actor_id).counters.damage_taken == 7U);
	const auto former_pet = pet;
	advance(a);
	advance(pet);
	a.actor.group_revision = 2U;
	pet.actor.actor.kind = telemetry_combat_actor_kind::npc;
	pet.actor.actor.owner_subject_id = 0U;
	assert(f.damage(pet, a, 13U, 30U).rows_attempted == 2U);
	assert(f.leave(former_pet, 31U).outcome == outcome::not_found);
	assert(f.row(pet.actor.actor.actor_id, 1U).counters.damage_dealt == 11U);
	assert(f.row(pet.actor.actor.actor_id, 1U).context.actor.actor.owner_subject_id == 22U);
	auto clone = npc(502U);
	assert(f.damage(clone, a, 17U, 40U).outcome == outcome::accepted);
	a.battle.sequence = pet.battle.sequence = 2U;
	a.association_revision = pet.association_revision = 1U;
	a.association_fact_sequence = pet.association_fact_sequence = 5U;
	assert(f.damage(pet, a, 19U, 50U).rows_attempted == 2U);
	assert(f.row(pet.actor.actor.actor_id, 2U).counters.damage_dealt == 13U);
	assert(f.row(pet.actor.actor.actor_id, 2U).context.actor.actor.kind ==
	       telemetry_combat_actor_kind::npc);
	assert(f.close(1U, 60U).rows_accepted == 1U);
	assert(f.close(2U, 61U).rows_accepted == 2U);
	assert(f.row(clone.actor.actor.actor_id).counters.damage_dealt == 17U);
	assert(f.row(pet.actor.actor.actor_id, 3U).counters.damage_dealt == 19U);
	std::uint64_t dealt = 0U, taken = 0U;
	for (std::size_t i = 0U; i < f.rows.size(); ++i)
	{
		dealt += f.rows[i].counters.damage_dealt;
		taken += f.rows[i].counters.damage_taken;
		for (std::size_t j = 0U; j < i; ++j)
			assert(f.rows[i].sequence != f.rows[j].sequence);
	}
	assert(dealt == 67U && taken == dealt && f.active_battles() == 0U);
}
void actual_alias_mode_and_configuration_boundaries()
{
	fixture f;
	auto a = player(81), b = player(82), c = player(83), d = player(84), n = npc(900U);
	f.observe(a, b, telemetry_battle_relation::hostile, 10U);
	a = f.association(81U);
	b = f.association(82U);
	assert(f.damage(a, b, 3U, 11U).outcome == outcome::accepted);
	f.observe(c, d, telemetry_battle_relation::hostile, 20U);
	c = f.association(83U);
	d = f.association(84U);
	const auto retired = c.battle;
	assert(retired.sequence != a.battle.sequence);
	assert(f.damage(c, d, 5U, 21U).outcome == outcome::accepted);
	f.observe(b, c, telemetry_battle_relation::hostile, 30U);
	unsigned aliases = 0U;
	for (const auto &fact : f.facts)
		if (fact.kind == telemetry_battle_fact_kind::merge_alias)
		{
			++aliases;
			assert(fact.battle.sequence == a.battle.sequence &&
			       fact.related_battle.sequence == retired.sequence);
		}
	assert(aliases == 1U);
	for (auto *actor : { &a, &b, &c, &d })
	{
		*actor = f.association(actor->actor.actor.actor_id);
		assert(actor->battle.sequence == a.battle.sequence);
		f.changed(*actor, 31U);
	}
	assert(f.row(83U).context.battle.sequence == retired.sequence &&
	       f.row(83U).counters.damage_dealt == 5U);
	assert(f.damage(b, c, 7U, 32U).outcome == outcome::accepted);
	f.observe(b, n, telemetry_battle_relation::hostile, 40U);
	for (auto *actor : { &a, &b, &c, &d, &n })
	{
		*actor = f.association(actor->actor.actor.actor_id);
		assert(actor->mode == telemetry_encounter_mode::mixed);
		f.changed(*actor, 40U);
	}
	assert(f.row(83U, 1U).context.battle.sequence == a.battle.sequence);
	assert(f.row(83U, 1U).counters.damage_dealt == 0U &&
	       f.row(83U, 1U).counters.damage_taken == 7U);
	assert(f.damage(b, n, 11U, 41U).outcome == outcome::accepted);
	auto config = scope;
	++config.config_id;
	const auto first = f.facts.size();
	const auto reconfigured = checked(
		[&]
		{
			return telemetry_battle_reconfigure(f.graph.get(), config, 45U, epoch + 45,
							    fixture::graph_sink, &f);
		});
	assert(reconfigured.outcome == telemetry_battle_outcome::accepted);
	for (auto at = first; at < f.facts.size();)
	{
		const auto count = f.facts[at].fact_count;
		assert(at + count <= f.facts.size() &&
		       telemetry_battle_packet_is_valid(f.facts.data() + at, count));
		at += count;
	}
	for (auto *actor : { &a, &b, &c, &d, &n })
	{
		*actor = f.association(actor->actor.actor.actor_id);
		assert(actor->scope.config_id == config.config_id);
		f.changed(*actor, 45U);
	}
	assert(f.damage(b, n, 2U, 46U).outcome == outcome::accepted);
	assert(f.close(a.battle.sequence, 47U).rows_accepted == 2U);
	std::uint64_t dealt = 0U, taken = 0U;
	for (const auto &row : f.rows)
	{
		dealt += row.counters.damage_dealt;
		taken += row.counters.damage_taken;
	}
	assert(dealt == 28U && taken == dealt);
	assert(f.row(n.actor.actor.actor_id).context.scope.config_id == scope.config_id);
	assert(f.row(n.actor.actor.actor_id, 1U).context.scope.config_id == config.config_id);
	assert(f.row(n.actor.actor.actor_id, 1U).counters.damage_taken == 2U);
}

void casting_and_observed_engagement()
{
	fixture f;
	auto a = player(31), b = player(32), c = player(33);
	const auto bk = key(b), ck = key(c);
	assert(f.finish(a, true, 1U).outcome == outcome::not_found);
	assert(f.engage(a, nullptr, 2U).outcome == outcome::not_found);
	assert(f.changed(a, 3U).outcome == outcome::not_found && f.state->next_sequence == 1U);
	assert(f.cast(a, 10U).outcome == outcome::accepted);
	assert(f.cast(a, 11U).outcome == outcome::idempotent);
	assert(f.finish(a, true, 20U).outcome == outcome::accepted);
	assert(f.finish(a, true, 21U).outcome == outcome::not_found);
	assert(f.cast(a, 30U).outcome == outcome::accepted);
	assert(f.finish(a, false, 40U).outcome == outcome::accepted);
	assert(f.cast(a, 50U).outcome == outcome::accepted);
	assert(f.engage(a, &bk, 55U).outcome == outcome::accepted);
	assert(f.engage(a, &bk, 56U).outcome == outcome::idempotent);
	assert(f.engage(a, &ck, 60U).outcome == outcome::accepted);
	assert(f.engage(a, nullptr, 65U).outcome == outcome::accepted);
	assert(f.engage(a, &bk, 70U).outcome == outcome::accepted);
	advance(a);
	a.actor.dimensions.zone_vnum = 701;
	assert(f.changed(a, 80U).rows_accepted == 1U);
	assert(f.changed(a, 81U).outcome == outcome::not_found);
	const auto &x = f.rows[0].counters;
	assert(x.casting_attempts == 3U && x.casting_completions == 1U && x.casting_aborts == 1U);
	assert(x.casting_unresolved == 1U && x.casting_elapsed_usec == 50U &&
	       x.engaged_target_usec == 20U);
	assert((f.rows[0].quality_flags & TELEMETRY_QUALITY_UNCLOSED_TAIL) != 0U);
	assert(f.cast(a, 90U).outcome == outcome::accepted);
	assert(f.engage(a, &bk, 95U).outcome == outcome::accepted);
	assert(f.close(1U, 94U, 200U).outcome == outcome::invalid && f.rows.size() == 1U);
	assert(f.close(1U, 100U, 200U).rows_accepted == 1U);
	assert(f.rows[1].counters.casting_unresolved == 1U &&
	       f.rows[1].counters.casting_elapsed_usec == 10U);
	assert(f.rows[1].counters.engaged_target_usec == 5U && f.rows[1].cut.decision_usec == 200U);
	fixture changed_finish;
	a = player(31);
	assert(changed_finish.cast(a, 10U).outcome == outcome::accepted);
	advance(a);
	a.scope.config_id = 34U;
	const auto terminal = changed_finish.finish(a, true, 20U);
	assert(terminal.outcome == outcome::not_found && terminal.rows_accepted == 1U);
	assert(changed_finish.rows[0].counters.casting_unresolved == 1U);
	assert(changed_finish.rows[0].counters.casting_completions == 0U &&
	       changed_finish.active_battles() == 0U);
}

void refusal_and_reference_order()
{
	fixture f;
	auto a = player(41), b = player(42);
	assert(f.damage(a, b, 10U, 10U).outcome == outcome::accepted);
	const auto next = f.state->next_sequence;
	auto changed = a;
	changed.actor.dimensions.class_id = 3U;
	assert(f.damage(changed, b, 5U, 11U).outcome == outcome::invalid);
	++changed.association_revision;
	assert(f.damage(changed, b, 5U, 11U).outcome == outcome::invalid);
	changed.association_fact_sequence = 4U;
	assert(f.damage(changed, b, 5U, 11U).outcome == outcome::invalid);
	changed = a;
	changed.available_metrics = TELEMETRY_BC_HEALING;
	assert(f.damage(changed, b, 5U, 11U).outcome == outcome::invalid);
	changed = b;
	changed.battle.sequence = 2U;
	assert(f.damage(a, changed, 5U, 11U).outcome == outcome::invalid);
	changed = b;
	changed.scope.environment_id = 99U;
	assert(f.damage(a, changed, 5U, 11U).outcome == outcome::invalid);
	assert(f.healing(a, b, 5U, 6U, 11U).outcome == outcome::invalid);
	assert(f.damage(a, b, 5U, 11U, 0x80000000U).outcome == outcome::invalid);
	assert(f.damage(a, a, 0U, 11U).outcome == outcome::idempotent);
	assert(f.rows.empty() && f.state->next_sequence == next && f.state->latest_usec == 10U);
	assert(f.damage(a, b, 5U, 9U).outcome == outcome::invalid);
	assert(f.close(1U, 20U).rows_accepted == 2U);
	assert(f.row(41U).counters.damage_dealt == 10U && f.row(42U).counters.damage_taken == 10U);
	for (const auto &v : f.rows)
		assert((v.quality_flags & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) != 0U);
	fixture same_revision;
	assert(same_revision.damage(a, b, 3U, 10U).outcome == outcome::accepted);
	changed = a;
	++changed.association_fact_sequence;
	changed.actor.dimensions.class_id = 3U;
	assert(same_revision.damage(changed, b, 4U, 20U).rows_accepted == 1U);
	assert(same_revision.close(1U, 30U).rows_accepted == 2U);
	assert(same_revision.row(41U).counters.damage_dealt == 3U);
	assert(same_revision.row(41U, 1U).counters.damage_dealt == 4U);
}

void actor_capacity_is_atomic()
{
	fixture f;
	for (int i = 1; i <= 63; ++i)
		assert(f.damage(player(i), player(i), 1U, i).outcome == outcome::accepted);
	const auto next = f.state->next_sequence;
	const auto refused = f.damage(player(64), player(65), 9U, 100U);
	assert(refused.outcome == outcome::capacity_full &&
	       (refused.quality_flags & TELEMETRY_QUALITY_CARDINALITY_OVERFLOW) != 0U);
	assert(f.state->next_sequence == next && f.rows.empty());
	assert(f.damage(player(1), player(64), 7U, 101U).outcome == outcome::accepted);
	assert(f.damage(player(1), player(65), 11U, 102U).outcome == outcome::capacity_full);
	assert(f.close(1U, 200U).rows_accepted == 64U);
	assert(f.row(1U).counters.damage_dealt == 8U && f.row(64U).counters.damage_taken == 7U);
	for (const auto &v : f.rows)
		assert((v.quality_flags & TELEMETRY_QUALITY_CARDINALITY_OVERFLOW) != 0U);
}

void full_battle_capacity_transition()
{
	fixture f;
	auto a = player(101, 1U), resident = player(102, 1U), b = player(201, 2U);
	assert(f.damage(a, resident, 3U, 1U).outcome == outcome::accepted);
	assert(f.damage(b, b, 5U, 2U).outcome == outcome::accepted);
	for (unsigned i = 3U; i <= 128U; ++i)
	{
		const auto c = player(1000 + i, i);
		assert(f.damage(c, c, 1U, i).outcome == outcome::accepted);
	}
	assert(f.active_battles() == 128U);
	const auto next = f.state->next_sequence;
	assert(f.damage(player(9001, 129U), player(9002, 129U), 11U, 200U).outcome ==
	       outcome::capacity_full);
	assert(f.rows.empty() && f.state->next_sequence == next);
	// Only the target's old battle slot can be freed; the source has a resident.
	a.battle.sequence = b.battle.sequence = 129U;
	const auto r = f.damage(a, b, 7U, 201U);
	assert(r.outcome == outcome::accepted && r.rows_attempted == 2U && r.rows_accepted == 2U);
	assert(f.active_battles() == 128U);
	assert(f.row(101U).counters.damage_dealt == 3U && f.row(201U).counters.damage_dealt == 5U);
	assert(f.close(129U, 300U).rows_accepted == 2U);
	assert(f.row(101U, 1U).counters.damage_dealt == 7U &&
	       f.row(201U, 1U).counters.damage_taken == 7U);
	assert(f.row(201U, 1U).counters.damage_dealt == 0U);
	assert(f.close(1U, 301U).rows_accepted == 1U && f.row(102U).counters.damage_taken == 3U);
}

void sequence_overflow_and_saturation()
{
	fixture f;
	auto a = player(51), b = player(52);
	f.state->next_sequence = maximum;
	assert(f.damage(a, b, 1U, 10U).outcome == outcome::capacity_full);
	assert(f.active_battles() == 0U && f.state->next_sequence == maximum);
	assert(f.damage(a, a, maximum, 11U).outcome == outcome::accepted);
	assert(f.state->next_sequence == 0U);
	assert(f.damage(a, a, 1U, 12U).outcome == outcome::accepted);
	assert(f.healing(a, a, maximum, 0U, 13U).outcome == outcome::accepted);
	assert(f.healing(a, a, 1U, 1U, 14U).outcome == outcome::accepted);
	assert(f.damage(b, b, 1U, 15U).outcome == outcome::capacity_full);
	assert(f.close(1U, 20U).rows_accepted == 1U);
	assert(f.rows[0].sequence == maximum && f.rows[0].counters.damage_dealt == maximum);
	assert(f.rows[0].counters.damage_taken == maximum &&
	       f.rows[0].counters.healing_attempted == maximum);
	assert(f.rows[0].counters.effective_healing == 1U &&
	       f.rows[0].counters.overhealing == maximum);
	assert((f.rows[0].quality_flags &
		(TELEMETRY_QUALITY_CONTEXT_OVERFLOW | TELEMETRY_QUALITY_CARDINALITY_OVERFLOW)) ==
	       (TELEMETRY_QUALITY_CONTEXT_OVERFLOW | TELEMETRY_QUALITY_CARDINALITY_OVERFLOW));
}

void sink_loss_is_explicit()
{
	fixture f;
	auto a = player(61), b = player(62);
	assert(f.damage(a, b, 7U, 10U).outcome == outcome::accepted);
	advance(a);
	advance(b);
	a.scope.config_id = b.scope.config_id = 34U;
	f.reject_remaining = 1U;
	const auto r = f.damage(a, b, 2U, 20U);
	assert(r.outcome == outcome::sink_rejected && r.rows_attempted == 2U &&
	       r.rows_accepted == 1U);
	assert((r.quality_flags & TELEMETRY_QUALITY_QUEUE_DROP) != 0U);
	assert(f.close(1U, 30U).rows_accepted == 2U);
	assert(f.row(61U).counters.damage_dealt == 2U &&
	       f.row(62U, 1U).counters.damage_taken == 2U);
	assert(f.attempted[0].counters.damage_dealt == 7U &&
	       f.attempted[0].sequence != f.row(61U).sequence);
	assert((f.row(61U).quality_flags & TELEMETRY_QUALITY_QUEUE_DROP) != 0U);
	assert((f.row(62U, 1U).quality_flags & TELEMETRY_QUALITY_QUEUE_DROP) != 0U);
	fixture gap;
	assert(gap.cast(a, 40U).outcome == outcome::accepted);
	assert(gap.leave(a, 50U, end::source_gap).rows_accepted == 1U);
	assert((gap.rows[0].quality_flags &
		(TELEMETRY_QUALITY_CONTEXT_UNKNOWN | TELEMETRY_QUALITY_QUEUE_DROP)) ==
	       (TELEMETRY_QUALITY_CONTEXT_UNKNOWN | TELEMETRY_QUALITY_QUEUE_DROP));
	assert(gap.rows[0].counters.casting_unresolved == 1U);
}

unsigned contract_mutation = 0U;
template <class Change> void rejects(payload value, Change change)
{
	++contract_mutation;
	change(value);
	if (telemetry_battle_contribution_payload_is_valid(value))
		std::fprintf(stderr, "Unexpected valid contract mutation %u\n", contract_mutation);
	assert(!telemetry_battle_contribution_payload_is_valid(value));
}
void intrinsic_contract()
{
	fixture f;
	auto a = player(71);
	assert(f.damage(a, a, 9U, 10U).outcome == outcome::accepted);
	assert(f.healing(a, a, 5U, 2U, 11U).outcome == outcome::accepted);
	assert(f.cast(a, 12U).outcome == outcome::accepted);
	assert(f.close(1U, 20U).rows_accepted == 1U);
	const auto v = f.rows[0];
	rejects(v, [](auto &r) { r.sequence = 0U; });
	rejects(v, [](auto &r) { ++r.definition_version; });
	rejects(v, [](auto &r) { r.reserved = 1U; });
	rejects(v, [](auto &r) { r.context.reserved = 1U; });
	rejects(v, [](auto &r) { r.context.association_revision = 0U; });
	rejects(v, [](auto &r) { r.last_association_fact_sequence = 4U; });
	rejects(v, [](auto &r) { ++r.last_association_revision; });
	rejects(v, [](auto &r) { r.context.available_metrics = 0U; });
	rejects(v, [](auto &r) { r.context.available_metrics |= 0x80U; });
	rejects(v, [](auto &r) { r.context.side = 0U; });
	rejects(v, [](auto &r) { r.context.side_status = telemetry_battle_side_status::partial; });
	rejects(v, [](auto &r) { r.context.scope.zone_vnum = 700; });
	rejects(v, [](auto &r) { r.context.actor.actor.reserved[1] = 1U; });
	rejects(v, [](auto &r) { r.cut.decision_usec = 19U; });
	rejects(v, [](auto &r) { r.cut.observed_usec = 9U; });
	rejects(v, [](auto &r) { r.end_reason = static_cast<end>(0U); });
	rejects(v, [](auto &r) { ++r.counters.overhealing; });
	rejects(v, [](auto &r) { r.counters.effective_healing = 6U; });
	rejects(v, [](auto &r) { r.counters.casting_unresolved = 0U; });
	rejects(v, [](auto &r) { r.counters.casting_completions = 2U; });
	rejects(v, [](auto &r) { r.counters.casting_elapsed_usec = 11U; });
	rejects(v,
		[](auto &r)
		{
			r.counters.casting_attempts = 2U;
			r.counters.casting_unresolved = 2U;
		});
	rejects(v,
		[](auto &r) { r.counters.casting_attempts = r.counters.casting_unresolved = 0U; });
	rejects(v, [](auto &r) { r.quality_flags = 0U; });
	auto partial = v;
	partial.context.side_status = telemetry_battle_side_status::partial;
	partial.context.side = 0U;
	partial.context.quality_flags = TELEMETRY_QUALITY_QUEUE_DROP;
	partial.quality_flags |= partial.context.quality_flags;
	assert(telemetry_battle_contribution_payload_is_valid(partial));
	auto unknown = v;
	unknown.start_utc_usec = unknown.cut.observed_utc_usec = unknown.cut.decision_utc_usec =
		TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_battle_contribution_payload_is_valid(unknown));
	auto negative = v;
	negative.start_utc_usec = -20;
	negative.cut.observed_utc_usec = -10;
	negative.cut.decision_utc_usec = -5;
	assert(telemetry_battle_contribution_payload_is_valid(negative));
	rejects(v, [](auto &r) { r.cut.observed_utc_usec = epoch + 5; });
	auto reversed = v;
	reversed.cut.observed_utc_usec = epoch + 5;
	reversed.quality_flags |= TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
	assert(telemetry_battle_contribution_payload_is_valid(reversed));
	fixture utc_reversal;
	assert(utc_reversal.damage(a, a, 1U, 10U).outcome == outcome::accepted);
	const auto sealed = checked(
		[&]
		{
			return telemetry_battle_contribution_leave(
				utc_reversal.state.get(), a.battle, key(a),
				{ 20U, epoch + 5, 30U, epoch + 4 }, end::actor_left, fixture::sink,
				&utc_reversal);
		});
	assert(sealed.rows_accepted == 1U);
	assert((utc_reversal.rows[0].quality_flags & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) != 0U);
	fixture recovered_clock;
	assert(recovered_clock.damage(a, a, 1U, 10U).outcome == outcome::accepted);
	assert(checked(
		       [&]
		       {
			       return telemetry_battle_contribution_damage(
				       recovered_clock.state.get(), a, a, 1U, 11U, epoch + 5, 0U,
				       fixture::sink, &recovered_clock);
		       })
		       .outcome == outcome::accepted);
	assert(recovered_clock.close(1U, 20U).rows_accepted == 1U);
	assert(recovered_clock.rows[0].cut.observed_utc_usec >
	       recovered_clock.rows[0].start_utc_usec);
	assert((recovered_clock.rows[0].quality_flags & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) !=
	       0U);
	fixture recovered_unknown;
	assert(recovered_unknown.damage(a, a, 1U, 10U).outcome == outcome::accepted);
	assert(checked(
		       [&]
		       {
			       return telemetry_battle_contribution_damage(
				       recovered_unknown.state.get(), a, a, 1U, 11U,
				       TELEMETRY_UTC_UNKNOWN, 0U, fixture::sink,
				       &recovered_unknown);
		       })
		       .outcome == outcome::accepted);
	assert(recovered_unknown.close(1U, 20U).rows_accepted == 1U);
	assert(recovered_unknown.rows[0].cut.observed_utc_usec != TELEMETRY_UTC_UNKNOWN);
	assert((recovered_unknown.rows[0].quality_flags & TELEMETRY_QUALITY_CONTEXT_UNKNOWN) != 0U);
	std::array<std::uint8_t, TELEMETRY_BATTLE_CONTRIBUTION_WIRE_BYTES> wire{};
	assert(checked(
		[&] { return telemetry_battle_contribution_encode(v, wire.data(), wire.size()); }));
	auto decoded = v;
	assert(!checked(
		[&] {
			return telemetry_battle_contribution_decode(wire.data(), wire.size() - 1U,
								    &decoded);
		}));
#define TELEMETRY_BC_FIELD(name, member, width, signed_value) \
	assert(static_cast<std::uint64_t>(decoded.member) == 0U);
#include "telemetry/telemetry_battle_contribution_fields.inc"
#undef TELEMETRY_BC_FIELD
	assert(!checked(
		[&]
		{ return telemetry_battle_contribution_decode(nullptr, wire.size(), &decoded); }));
	assert(!checked(
		[&] {
			return telemetry_battle_contribution_decode(wire.data(), wire.size(),
								    nullptr);
		}));
	assert(!checked([&]
			{ return telemetry_battle_contribution_encode(v, nullptr, wire.size()); }));
	assert(!checked(
		[&] {
			return telemetry_battle_contribution_encode(v, wire.data(),
								    wire.size() - 1U);
		}));
	auto changed = v;
	++changed.context.battle.sequence;
	assert(telemetry_battle_contribution_same_key(v, changed) &&
	       !telemetry_battle_contribution_equal(v, changed));
	changed = v;
	++changed.context.actor.actor.power_band;
	assert(telemetry_battle_contribution_same_key(v, changed) &&
	       !telemetry_battle_contribution_equal(v, changed));
	++changed.sequence;
	assert(!telemetry_battle_contribution_same_key(v, changed));
}

int verify_file(const char *path)
{
	std::FILE *input = std::fopen(path, "rb");
	if (!input)
		return 2;
	std::array<std::uint8_t, TELEMETRY_BATTLE_CONTRIBUTION_WIRE_BYTES + 1U> bytes{}, encoded{};
	std::array<std::uint8_t, 4U> header{};
	for (;;)
	{
		const auto read = std::fread(header.data(), 1U, header.size(), input);
		if (!read && std::feof(input))
			break;
		if (read != header.size())
		{
			std::fclose(input);
			return 2;
		}
		const auto length = (std::uint32_t{ header[0] } << 24U) |
				    (std::uint32_t{ header[1] } << 16U) |
				    (std::uint32_t{ header[2] } << 8U) | header[3];
		if (length > bytes.size() || std::fread(bytes.data(), 1U, length, input) != length)
		{
			std::fclose(input);
			return 2;
		}
		payload value{};
		value.sequence = 999U;
		const auto valid = checked(
			[&] {
				return telemetry_battle_contribution_decode(bytes.data(), length,
									    &value);
			});
		if (valid)
		{
			assert(checked(
				[&] {
					return telemetry_battle_contribution_encode(
						value, encoded.data(), length);
				}));
			for (std::size_t i = 0U; i < length; ++i)
				assert(bytes[i] == encoded[i]);
		}
		else
		{
#define TELEMETRY_BC_FIELD(name, member, width, signed_value) \
	assert(static_cast<std::uint64_t>(value.member) == 0U);
#include "telemetry/telemetry_battle_contribution_fields.inc"
#undef TELEMETRY_BC_FIELD
		}
		std::printf("%u\n", static_cast<unsigned>(valid));
	}
	return std::fclose(input) == 0 ? 0 : 2;
}
} // namespace

int main(int argc, char **argv)
{
	if (argc == 3 && std::string_view(argv[1]) == "--verify")
		return verify_file(argv[2]);
	if (argc == 3 && std::string_view(argv[1]) == "--export")
	{
		contract_export = std::fopen(argv[2], "wb");
		if (!contract_export)
			return 2;
	}
	else if (argc != 1)
		return 2;
	shared_association_and_conservation();
	disjoint_contexts_and_live_identity();
	actual_alias_mode_and_configuration_boundaries();
	casting_and_observed_engagement();
	refusal_and_reference_order();
	actor_capacity_is_atomic();
	full_battle_capacity_transition();
	sequence_overflow_and_saturation();
	sink_loss_is_explicit();
	intrinsic_contract();
	std::printf(
		"Battle contributions passed: 10 journeys; payload=%zu, wire=%zu, state=%zu bytes; no event-time allocation.\n",
		sizeof(payload), TELEMETRY_BATTLE_CONTRIBUTION_WIRE_BYTES,
		sizeof(telemetry_battle_contribution_state));
	if (contract_export && std::fclose(contract_export) != 0)
		return 2;
}
