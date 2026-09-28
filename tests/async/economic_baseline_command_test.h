#include "economy/economic_baseline_command.h"
#include "economy/economic_command_admission.h"
#include "flatfile/flatfile_accounting_store.h"
#include "persistence/critical_command_coordinator.h"

unsigned baseline_execution_calls = 0;
critical_apply_result baseline_must_not_execute(const critical_command &, void *)
{
	++baseline_execution_calls;
	assert(false && "baseline preparation must never become executable");
	return { critical_apply_outcome::terminal_failure, 0, EINVAL };
}
struct baseline_journal_observation
{
	const critical_command *expected;
	unsigned calls = 0;
};
bool baseline_observe_retained(critical_command command, void *context)
{
	auto &observation = *static_cast<baseline_journal_observation *>(context);
	assert(critical_command_equal(command, *observation.expected));
	++observation.calls;
	return true;
}
void baseline_coordinator_refusal(const critical_command &command, const std::string &path)
{
	// Neither the default coordinator nor its explicit bank-only extension may
	// admit baseline preparation, even though its wire envelope is valid.
	for (critical_extension_validator_fn validator :
	     { static_cast<critical_extension_validator_fn>(nullptr),
	       economic_command_admission_supported })
	{
		assert(critical_command_coordinator_init(path.c_str(), baseline_must_not_execute,
							 nullptr, 1, nullptr, nullptr, validator));
		assert(critical_command_coordinator_submit(command) ==
		       critical_submit_result::invalid);
		assert(critical_command_coordinator_submit_for_publication(command) ==
		       critical_submit_result::invalid);
		assert(critical_command_journal_health_copy().records == 0 &&
		       critical_command_journal_health_copy().checkpoints == 0 &&
		       baseline_execution_calls == 0);
		critical_command_coordinator_shutdown();
	}
	// A durable but unsupported envelope must survive refused startup exactly,
	// without forwarding or checkpointing it as completed work.
	assert(critical_command_journal_init(path.c_str()));
	assert(critical_command_journal_append(command) == critical_command_journal_result::ok);
	critical_command_journal_shutdown();
	assert(!critical_command_coordinator_init(path.c_str(), baseline_must_not_execute, nullptr,
						  1, nullptr, nullptr,
						  economic_command_admission_supported));
	critical_command_coordinator_shutdown();
	assert(critical_command_journal_init(path.c_str()));
	assert(critical_command_journal_health_copy().checkpoints == 0 &&
	       baseline_execution_calls == 0);
	baseline_journal_observation observation{ &command };
	assert(critical_command_journal_replay(baseline_observe_retained, &observation) ==
		       critical_command_journal_result::ok &&
	       observation.calls == 1);
	critical_command_journal_shutdown();
	std::cout
		<< "baseline coordinator: fresh/publication admission and durable replay refused without execution or checkpoint\n";
}
void command_tests(const std::string &journal_path)
{
	static_assert(static_cast<uint16_t>(critical_command_type::player_death_restitution) == 19);
	static_assert(static_cast<uint16_t>(critical_command_type::economic_baseline) == 20);
	auto prepared = prepare(fixture());
	critical_command command;
	assert(economic_baseline_command_build(*prepared, 123456, &command) == error::ok);
	assert(command.payload == REFERENCE_COMMAND_PAYLOAD);
	baseline_coordinator_refusal(command, journal_path);
	assert(command.accounting_intent.size() == ECONOMIC_INTENT_HEADER_BYTES);
	assert(critical_command_envelope_valid(command));
	assert(!critical_command_valid(command) && !economic_command_admission_supported(command));
	auto legacy = command;
	legacy.schema_version = 1;
	legacy.accounting_intent.clear();
	assert(critical_command_envelope_valid(legacy));
	assert(!critical_command_legacy_execution_supported(legacy) &&
	       !critical_command_valid(legacy));
	assert(!critical_command_normalize(&legacy));
	for (uint16_t type = 1; type < 20; ++type)
	{
		auto previous = legacy;
		previous.type = static_cast<critical_command_type>(type);
		assert(critical_command_valid(previous));
	}
	std::vector<uint8_t> wire;
	assert(critical_command_encode(command, &wire) == critical_command_codec_result::ok);
	critical_command restored;
	assert(critical_command_decode(wire.data(), wire.size(), &restored) ==
	       critical_command_codec_result::ok);
	assert(critical_command_equal(command, restored));
	economic_accounting_plan plan;
	assert(economic_baseline_command_plan(restored, *prepared, &plan) == error::ok);
	std::vector<uint8_t> encoded;
	assert(economic_plan_encode(plan, &encoded) == error::ok);
	economic_frozen_intent intent;
	assert(economic_intent_decode(command.accounting_intent, &intent) == error::ok);
	assert(economic_intent_verify_binding(command, intent) == error::ok);
	assert(intent.admission.facts.empty());
	auto original = plan;
	original.metadata = prepared->plan().metadata;
	std::vector<uint8_t> pure;
	assert(economic_plan_encode(original, &pure) == error::ok &&
	       pure == prepared->encoded_plan());
	assert(plan.metadata.intent_digest != original.metadata.intent_digest);
	assert(plan.metadata.domain_digest != original.metadata.domain_digest);
	flatfile_accounting_record record;
	record.command = command;
	record.plan = encoded;
	std::vector<uint8_t> stored;
	assert(flatfile_accounting_record_encode(record, &stored) ==
	       flatfile_accounting_status::ok);
	flatfile_accounting_record recovered;
	assert(flatfile_accounting_record_decode(stored, &recovered) ==
	       flatfile_accounting_status::ok);
	assert(recovered.plan == encoded && critical_command_equal(command, recovered.command));
	record.plan = prepared->encoded_plan();
	assert(flatfile_accounting_record_encode(record, &stored) ==
	       flatfile_accounting_status::invalid);
	auto reject =
		[&](const critical_command &changed, const economic_prepared_baseline &witness)
	{
		assert(economic_baseline_command_plan(changed, witness, &plan) != error::ok);
		std::vector<uint8_t> after;
		assert(economic_plan_encode(plan, &after) == error::ok && after == encoded);
	};
	for (size_t i = 0; i < command.payload.size(); ++i)
	{
		auto changed = command;
		changed.payload[i] ^= 1;
		reject(changed, *prepared);
	}
	for (size_t i = 0; i < command.accounting_intent.size(); ++i)
	{
		auto changed = command;
		changed.accounting_intent[i] ^= 1;
		reject(changed, *prepared);
	}
	for (int change = 0; change < 10; ++change)
	{
		auto changed = command;
		switch (change)
		{
		case 0:
			changed.operation_id.bytes[0] ^= 1;
			break;
		case 1:
			changed.type = critical_command_type::wallet;
			break;
		case 2:
			changed.payload_version = 2;
			break;
		case 3:
			changed.source_site = critical_source_site::command;
			break;
		case 4:
			changed.deadline_class = critical_deadline_class::background;
			break;
		case 5:
			changed.keys[0].id++;
			break;
		case 6:
			changed.keys[0].type = critical_entity_type::player;
			break;
		case 7:
			changed.expected_revisions.push_back({ changed.keys[0], 1 });
			break;
		case 8:
			changed.accepted_at_usec = 0;
			break;
		case 9:
			changed.schema_version = 1;
			break;
		}
		reject(changed, *prepared);
	}
	for (int change = 0; change < 9; ++change)
	{
		auto input = fixture();
		switch (change)
		{
		case 0:
			input.holdings[0].balance[0]++;
			break;
		case 1:
			input.holdings[0].native_revision++;
			break;
		case 2:
			input.holdings[0].source_digest[0]++;
			break;
		case 3:
			input.items[0].source_digest[0]++;
			break;
		case 4:
			input.items[0].snapshot.position.revision++;
			break;
		case 5:
			input.boundary_digest[0]++;
			break;
		case 6:
			input.coverage_digest[0]++;
			break;
		case 7:
			input.actor_id++;
			break;
		case 8:
			input.batch_index++;
			break;
		}
		reject(command, *prepare(input));
	}
	auto later = command;
	later.accepted_at_usec++;
	assert(economic_baseline_command_plan(later, *prepared, &plan) == error::ok);
	assert(!critical_command_equal(command, later)); // Receipt retains exact time.
	assert(economic_plan_encode(plan, &pure) == error::ok && pure == encoded);
	assert(economic_baseline_command_build(*prepared, 0, &restored) == error::invalid_identity);
	assert(critical_command_equal(restored, command));
	size_t build_failures = 0, plan_failures = 0;
	for (size_t target = 1; target < 1024; ++target)
	{
		allocation_seen = 0;
		allocation_target = target;
		auto status = economic_baseline_command_build(*prepared, 123456, &restored);
		allocation_target = 0;
		assert(critical_command_equal(restored, command));
		if (status == error::ok)
			break;
		assert(status == error::capacity);
		++build_failures;
	}
	for (size_t target = 1; target < 1024; ++target)
	{
		allocation_seen = 0;
		allocation_target = target;
		auto status = economic_baseline_command_plan(command, *prepared, &plan);
		allocation_target = 0;
		assert(economic_plan_encode(plan, &pure) == error::ok && pure == encoded);
		if (status == error::ok)
			break;
		assert(status == error::capacity);
		++plan_failures;
	}
	assert(build_failures > 0 && build_failures < 1023 && plan_failures > 0 &&
	       plan_failures < 1023);
	std::cout
		<< "baseline command: reference payload, exact witness conflicts, store roundtrip, legacy/admission refusal; "
		<< build_failures << " build and " << plan_failures
		<< " plan allocation failures passed\n";
}
