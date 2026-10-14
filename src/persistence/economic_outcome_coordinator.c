#include "persistence/economic_outcome.h"

#include <cerrno>
#include <limits>

extern bool nevent_is_game_thread();
extern bool economic_outcome_recovery_admission_allowed() noexcept;

namespace
{
uint64_t published_sequence = 0;
bool outcome_in_progress = false;
bool outcome_checking_current = false;
bool outcome_fenced = false;
const economic_outcome_record *publication_record = nullptr;
struct ordered_scope
{
	ordered_scope() { outcome_in_progress = true; }
	~ordered_scope()
	{
		publication_record = nullptr;
		outcome_checking_current = false;
		outcome_in_progress = false;
	}
};
}

bool economic_outcome_capture_cut(uint64_t *output) noexcept
{
	if (!output || !nevent_is_game_thread() || outcome_in_progress || outcome_fenced)
		return false;
	const auto health = economic_outcome_journal_health_copy();
	if (!health.initialized || health.uncertain ||
	    published_sequence != health.durable_sequence)
		return false;
	*output = published_sequence;
	return true;
}

bool economic_outcome_current_observation_cut(uint64_t *output) noexcept
{
	if (!output || !nevent_is_game_thread() || !outcome_in_progress ||
	    !outcome_checking_current || outcome_fenced)
		return false;
	const auto health = economic_outcome_journal_health_copy();
	if (!health.initialized || health.uncertain ||
	    published_sequence != health.durable_sequence)
		return false;
	*output = published_sequence;
	return true;
}

bool economic_outcome_clock_matches(const critical_operation_id &lineage,
				    const critical_operation_id &epoch,
				    uint64_t published_cut) noexcept
{
	if (critical_operation_id_is_zero(lineage) || critical_operation_id_is_zero(epoch))
		return false;
	uint64_t observed = 0;
	if (!(economic_outcome_capture_cut(&observed) ||
	      economic_outcome_current_observation_cut(&observed)) ||
	    observed != published_cut)
		return false;
	critical_operation_id actual_lineage{}, actual_epoch{};
	if (!economic_outcome_journal_namespace(&actual_lineage, &actual_epoch) ||
	    !critical_operation_id_equal(actual_lineage, lineage) ||
	    !critical_operation_id_equal(actual_epoch, epoch))
		return false;
	uint64_t confirmed = 0;
	return (economic_outcome_capture_cut(&confirmed) ||
		economic_outcome_current_observation_cut(&confirmed)) &&
	       confirmed == observed;
}

bool economic_outcome_publication_record_matches(const economic_outcome_record *record) noexcept
{
	return record && nevent_is_game_thread() && outcome_in_progress &&
	       !outcome_checking_current && !outcome_fenced && publication_record == record;
}

bool economic_outcome_install_recovered_cut(uint64_t sequence,
					    economic_outcome_base_verify_fn verify,
					    void *context) noexcept
{
	if (!nevent_is_game_thread() || outcome_in_progress || !verify)
		return false;
	const auto before = economic_outcome_journal_health_copy();
	if (!before.initialized || before.uncertain || sequence != before.durable_sequence ||
	    !verify(sequence, context))
		return false;
	const auto after = economic_outcome_journal_health_copy();
	if (!after.initialized || after.uncertain ||
	    after.durable_sequence != before.durable_sequence ||
	    after.compacted_through != before.compacted_through)
		return false;
	published_sequence = sequence;
	outcome_fenced = false;
	return true;
}

bool economic_outcome_entity_cut(const critical_entity_key &key, uint64_t *output) noexcept
{
	if (!output || !key.id || !nevent_is_game_thread() || outcome_fenced ||
	    (outcome_in_progress && !outcome_checking_current))
		return false;
	const auto health = economic_outcome_journal_health_copy();
	if (!health.initialized || health.uncertain ||
	    health.durable_sequence != published_sequence)
		return false;
	const uint64_t published = published_sequence;
	struct observation
	{
		critical_entity_key key;
		uint64_t published;
		uint64_t cut;
	} current{ key, published, health.compacted_through };
	const auto inspect = +[](const economic_outcome_record &record, void *raw) noexcept
	{
		auto &state = *static_cast<observation *>(raw);
		if (record.sequence > state.published)
			return false;
		for (const auto &effect : record.effects)
			if (critical_entity_key_equal(effect.key, state.key))
				state.cut = effect.after_revision;
		return true;
	};
	if (!economic_outcome_journal_replay(health.compacted_through, inspect, &current))
		return false;
	*output = current.cut;
	return true;
}

economic_outcome_result
economic_outcome_commit(economic_outcome_record &record,
			const economic_outcome_publication &publication) noexcept
try
{
	if (!nevent_is_game_thread() || outcome_in_progress || outcome_fenced ||
	    !publication.current || !publication.publish || !publication.fence ||
	    !economic_outcome_recovery_admission_allowed())
		return { economic_outcome_status::unavailable, 0, EPERM };
	const auto before = economic_outcome_journal_health_copy();
	if (!before.initialized || before.uncertain ||
	    before.durable_sequence != published_sequence)
		return { economic_outcome_status::unavailable, 0, EBUSY };
	ordered_scope serial;
	economic_outcome_record existing;
	if (economic_outcome_journal_lookup(record.command.operation_id, &existing))
	{
		const uint64_t old_sequence = record.sequence;
		record.sequence = existing.sequence;
		std::vector<uint8_t> proposed, original;
		const bool exact = economic_outcome_encode(record, &proposed) &&
				   economic_outcome_encode(existing, &original) &&
				   proposed == original;
		if (!exact)
		{
			record.sequence = old_sequence;
			return { economic_outcome_status::conflict, existing.sequence, EEXIST };
		}
		// Recovery/previous ordered publication established this exact cut.
		// A retry must not call current preparation, RNG or publish again.
		if (existing.sequence <= published_sequence)
			return { economic_outcome_status::durable, existing.sequence, 0 };
		return { economic_outcome_status::unavailable, existing.sequence, EBUSY };
	}
	if (before.durable_sequence == std::numeric_limits<uint64_t>::max() ||
	    (record.sequence && record.sequence != before.durable_sequence + 1))
		return { economic_outcome_status::rejected, 0, ESTALE };
	outcome_checking_current = true;
	const bool current = publication.current(publication.context);
	outcome_checking_current = false;
	if (!current)
		return { economic_outcome_status::rejected, 0, ESTALE };
	record.sequence = before.durable_sequence + 1;
	// All participant successors carry this native ordered cut; the domain
	// independently validated its real holder/state and before cut.
	for (auto &effect : record.effects)
		effect.after_revision = record.sequence;
	const auto appended = economic_outcome_journal_append(record);
	if (appended.status != economic_outcome_status::durable)
	{
		if (appended.status == economic_outcome_status::uncertain)
		{
			outcome_fenced = true;
			publication.fence(appended.sequence, publication.context);
		}
		return appended;
	}
	// No world callback or worker native mutation is allowed between the
	// domain's current check, append/fsync and this preallocated publication.
	publication_record = &record;
	const bool published = publication.publish(record, publication.context);
	publication_record = nullptr;
	if (!published)
	{
		outcome_fenced = true;
		publication.fence(appended.sequence, publication.context);
		return { economic_outcome_status::uncertain, appended.sequence, EIO };
	}
	published_sequence = appended.sequence;
	return appended; // ACK-safe durable AND published, never append-only.
}
catch (...)
{
	// Allocations precede durability. Publication/fence callbacks are noexcept.
	return { economic_outcome_status::rejected, 0, ENOMEM };
}
