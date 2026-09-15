// Include the real runtime in this test translation unit to exhaust its private
// sequence allocator without adding a production test API or changing its ABI.
#include <openssl/rand.h>
static int entropy_mode = 0;
static unsigned entropy_calls = 0;
static int runtime_test_random(unsigned char *bytes, int count)
{
	++entropy_calls;
	if (entropy_mode == 1)
		return 0;
	if (entropy_mode == 2)
	{
		for (int index = 0; index < count; ++index)
			bytes[index] = 0;
		return 1;
	}
	return RAND_bytes(bytes, count);
}
#define RAND_bytes runtime_test_random
#include "telemetry/telemetry_runtime.c"
#undef RAND_bytes
#define main ordinary_runtime_test_main
#include "telemetry_runtime_integration.cc"
#undef main

namespace
{
void check_copyover_caller_ack_race()
{
	R.flush_completed.store(0U);
	R.flush_durable.store(false);
	static unsigned reads = 0U;
	reads = 0U;
	const auto delayed_clock = [](telemetry_monotonic_usec *now) noexcept
	{
		++reads;
		// On the first clock query, the caller sampled time before deadline,
		// then was suspended while a late durable acknowledgement appeared.
		// Clock-before-ack code incorrectly accepts using the stale 99 value.
		*now = reads == 1U ? 99U : 101U;
		R.flush_durable.store(true);
		R.flush_completed.store(1U, std::memory_order_release);
		return true;
	};
	assert(wait_for_copyover_flush(1U, 100U, delayed_clock) ==
	       telemetry_runtime_outcome::queue_full);
	assert(reads == 2U);
	std::puts("copyover caller late-publication race: rejected");
}

struct generation_barrier_repository
{
	std::mutex mutex;
	std::condition_variable condition;
	bool first_entered = false;
	bool second_entered = false;
	bool release_first = false;
	bool release_second = false;
	std::uint32_t init_calls = 0U;
	std::uint32_t apply_calls = 0U;
	std::uint32_t shutdown_calls = 0U;
	telemetry_record_sequence first_record_seq = 0U;
	telemetry_record_sequence first_last_record_seq = 0U;
	telemetry_record_sequence second_record_seq = 0U;
	telemetry_record_sequence second_last_record_seq = 0U;
	telemetry_batch_outcome first_outcome = telemetry_batch_outcome::committed;
	telemetry_batch_outcome second_outcome = telemetry_batch_outcome::committed;

	bool wait_first_entered(std::chrono::milliseconds timeout)
	{
		std::unique_lock<std::mutex> lock(mutex);
		return condition.wait_for(lock, timeout, [this] { return first_entered; });
	}

	bool wait_second_entered(std::chrono::milliseconds timeout)
	{
		std::unique_lock<std::mutex> lock(mutex);
		return condition.wait_for(lock, timeout, [this] { return second_entered; });
	}

	void release_first_callback()
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			release_first = true;
		}
		condition.notify_all();
	}

	void release_second_callback()
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			release_second = true;
		}
		condition.notify_all();
	}
};

telemetry_repository_outcome generation_barrier_init(void *context,
						     telemetry_repository_config) noexcept
{
	auto *repository = static_cast<generation_barrier_repository *>(context);
	++repository->init_calls;
	return telemetry_repository_outcome::ready;
}

telemetry_apply_batch_result
generation_barrier_apply(void *context, const telemetry_record *records, std::size_t count) noexcept
{
	auto *repository = static_cast<generation_barrier_repository *>(context);
	assert(records != nullptr && count != 0U);
	std::unique_lock<std::mutex> lock(repository->mutex);
	++repository->apply_calls;
	const std::uint32_t call = repository->apply_calls;
	assert(call <= 2U);
	if (call == 1U)
	{
		repository->first_record_seq = records[0].header.key.record_seq;
		repository->first_last_record_seq = records[count - 1U].header.key.record_seq;
		repository->first_outcome = telemetry_batch_outcome::commit_ambiguous;
		repository->first_entered = true;
		repository->condition.notify_all();
		repository->condition.wait(lock,
					   [repository] { return repository->release_first; });
	}
	else
	{
		repository->second_record_seq = records[0].header.key.record_seq;
		repository->second_last_record_seq = records[count - 1U].header.key.record_seq;
		repository->second_outcome = telemetry_batch_outcome::committed;
		repository->second_entered = true;
		repository->condition.notify_all();
		repository->condition.wait(lock,
					   [repository] { return repository->release_second; });
	}
	lock.unlock();

	telemetry_apply_batch_result result{};
	result.input_count = static_cast<std::uint16_t>(count);
	result.result_count = static_cast<std::uint16_t>(count);
	result.first_record_seq = records[0].header.key.record_seq;
	result.last_record_seq = records[count - 1U].header.key.record_seq;
	if (call == 1U)
	{
		result.outcome = telemetry_batch_outcome::commit_ambiguous;
		for (std::size_t index = 0U; index < count; ++index)
		{
			result.results[index].key = records[index].header.key;
			result.results[index].outcome = telemetry_apply_outcome::commit_ambiguous;
		}
	}
	else
	{
		result.outcome = telemetry_batch_outcome::committed;
		result.duplicate_count = static_cast<std::uint16_t>(count);
		for (std::size_t index = 0U; index < count; ++index)
		{
			result.results[index].key = records[index].header.key;
			result.results[index].outcome =
				telemetry_apply_outcome::duplicate_identical;
		}
	}
	return result;
}

telemetry_repository_outcome generation_barrier_request_stop(void *) noexcept
{
	return telemetry_repository_outcome::stopping;
}

void generation_barrier_shutdown(void *context) noexcept
{
	auto *repository = static_cast<generation_barrier_repository *>(context);
	++repository->shutdown_calls;
}

void check_copyover_generation_barrier_recovery()
{
	generation_barrier_repository barrier{};
	const telemetry_transport_repository_binding repository = {
		generation_barrier_init, generation_barrier_apply, generation_barrier_request_stop,
		generation_barrier_shutdown, &barrier
	};
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	const auto options = make_enabled_options();
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);
	const auto enter = make_enter(options.producer, options.config);
	assert(telemetry_runtime_session_enter(enter).outcome ==
	       telemetry_runtime_outcome::accepted);

	// Start generation 1 explicitly so the first repository callback belongs to
	// its drain, then leave that callback unresolved past its deadline.
	telemetry_monotonic_usec now = 0U;
	assert(production_monotonic_now(&now));
	const auto generation1_deadline = now + 2'000'000U;
	R.flush_deadline.store(generation1_deadline, std::memory_order_release);
	R.flush_requested.store(1U, std::memory_order_release);
	wake_worker();
	assert(barrier.wait_first_entered(std::chrono::seconds(2)));
	const auto first = wait_for_copyover_flush(1U, generation1_deadline);
	assert(first == telemetry_runtime_outcome::queue_full);
	assert(R.flush_requested.load(std::memory_order_acquire) == 1U);
	assert(R.flush_completed.load(std::memory_order_acquire) != 1U);
	const bool after_deadline = production_monotonic_now(&now) && now >= generation1_deadline;
	std::printf(
		"generation1 wait: deadline=%llu now=%llu completed=%llu durable=%u after=%u\n",
		static_cast<unsigned long long>(generation1_deadline),
		static_cast<unsigned long long>(now),
		static_cast<unsigned long long>(R.flush_completed.load(std::memory_order_acquire)),
		R.flush_durable.load(std::memory_order_acquire), after_deadline);
	std::fflush(stdout);
	assert(after_deadline);

	// Request generation 2 before releasing the old callback. Its duplicate
	// response is the explicit reconciliation of generation 1's ambiguity.
	telemetry_runtime_outcome second = telemetry_runtime_outcome::invalid;
	std::thread next_flush(
		[&]
		{
			telemetry_monotonic_usec second_now = 0U;
			assert(production_monotonic_now(&second_now));
			second = telemetry_runtime_flush_for_copyover(second_now + 250'000U);
		});
	const auto request_wait_until = std::chrono::steady_clock::now() + std::chrono::seconds(2);
	while (R.flush_requested.load(std::memory_order_acquire) != 2U &&
	       std::chrono::steady_clock::now() < request_wait_until)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	assert(R.flush_requested.load(std::memory_order_acquire) == 2U);
	assert(R.flush_completed.load(std::memory_order_acquire) != 1U);
	assert(!R.flush_durable.load(std::memory_order_acquire));

	barrier.release_first_callback();
	assert(barrier.wait_second_entered(std::chrono::seconds(2)));
	barrier.release_second_callback();
	next_flush.join();

	const auto health = telemetry_runtime_health_copy();
	assert(second == telemetry_runtime_outcome::accepted);
	assert(barrier.first_outcome == telemetry_batch_outcome::commit_ambiguous);
	assert(barrier.second_outcome == telemetry_batch_outcome::committed);
	assert(barrier.first_record_seq == barrier.second_record_seq);
	assert(barrier.first_last_record_seq == barrier.second_last_record_seq);
	assert(health.ambiguous_commits > 0U);
	assert(health.duplicate_records > 0U);
	assert(health.queue_depth == 0U);
	std::printf("copyover generations: gen1=queue_full gen2=accepted ambiguous=%llu "
		    "duplicates=%llu reused=%llu/%llu\n",
		    static_cast<unsigned long long>(health.ambiguous_commits),
		    static_cast<unsigned long long>(health.duplicate_records),
		    static_cast<unsigned long long>(barrier.first_record_seq),
		    static_cast<unsigned long long>(barrier.second_record_seq));
	std::fflush(stdout);
	assert(production_monotonic_now(&now));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 0U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	assert(barrier.init_calls == 1U);
	assert(barrier.apply_calls == 2U);
	assert(barrier.shutdown_calls == 1U);
	telemetry_transport_unbind_for_tests();
}

void check_late_copyover_ack()
{
	fake_repository fake{};
	fake.block_apply = true;
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	assert(telemetry_runtime_init(make_enabled_options()) ==
	       telemetry_runtime_outcome::accepted);
	telemetry_monotonic_usec now = 0U;
	assert(production_monotonic_now(&now));
	const auto deadline = now + 500'000U;
	// Drive the same request protocol directly to inspect the worker's late ack,
	// even after the ordinary caller would already have timed out.
	R.flush_deadline.store(deadline);
	R.flush_requested.store(1U, std::memory_order_release);
	wake_worker();
	assert(fake.apply_callback.wait_until_entered(std::chrono::seconds(2)));
	assert(production_monotonic_now(&now) && now < deadline);
	std::this_thread::sleep_for(std::chrono::microseconds(deadline - now + 10'000U));
	fake.apply_callback.unblock();
	const auto wait_until = std::chrono::steady_clock::now() + std::chrono::seconds(2);
	while (R.flush_completed.load(std::memory_order_acquire) != 1U &&
	       std::chrono::steady_clock::now() < wait_until)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	assert(R.flush_completed.load(std::memory_order_acquire) == 1U);
	const bool late_durable = R.flush_durable.load();
	assert(production_monotonic_now(&now));
	const auto fresh = telemetry_runtime_flush_for_copyover(now + 250'000U);
	assert(telemetry_runtime_shutdown({ now + 1'000'000U, 0U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
	std::printf("late copyover ack: durable=%u fresh=%u\n", late_durable,
		    static_cast<unsigned>(fresh));
	std::fflush(stdout);
	assert(!late_durable);
	assert(fresh == telemetry_runtime_outcome::accepted);
}

void check_identity_fail_closed()
{
	for (int mode : { 1, 2 })
	{
		entropy_mode = mode;
		entropy_calls = 0;
		const auto options = telemetry_runtime_default_options();
		assert(telemetry_producer_id_is_zero(options.producer));
		assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::invalid);
		assert(entropy_calls == (mode == 1 ? 1U : 4U));
	}
	entropy_mode = 0;
	const auto options = make_enabled_options();
	assert(telemetry_producer_id_is_valid(options.producer));
	const auto saved_count = used_producer_count;
	used_producer_count = sizeof(used_producers) / sizeof(used_producers[0]);
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::invalid);
	assert(used_producer_count == sizeof(used_producers) / sizeof(used_producers[0]));
	used_producer_count = saved_count;
	std::puts("producer entropy failure, zero-token retry bound, and ledger exhaustion passed");
}

void check_exhausted_lifecycle(bool exiting, bool reattaching = false)
{
	fake_repository fake{};
	const telemetry_transport_repository_binding repository = { fake_init, fake_apply,
								    fake_request_stop,
								    fake_shutdown, &fake };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
	const auto options = make_enabled_options();
	assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);
	auto enter = make_enter(options.producer, options.config);
	assert(telemetry_runtime_now(&enter.at_monotonic_usec, &enter.at_utc_usec));
	assert(telemetry_runtime_session_enter(enter).outcome ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_record_evidence({ enter.session,
						   enter.connection,
						   enter.at_monotonic_usec,
						   enter.at_utc_usec,
						   telemetry_runtime_evidence_kind::player_action,
						   {},
						   TELEMETRY_QUALITY_NONE })
		       .outcome == telemetry_runtime_outcome::accepted);

	// Zero is the real allocator's exhausted sentinel. It must never wrap or
	// acquire a fresh identity just to keep emitting records.
	if (reattaching)
	{
		telemetry_connection_transition prior{};
		prior.session = enter.session;
		prior.connection = enter.connection;
		prior.at_monotonic_usec = enter.at_monotonic_usec + 500U;
		prior.at_utc_usec = enter.at_utc_usec + 500;
		prior.kind = telemetry_connection_transition_kind::detached;
		assert(telemetry_runtime_connection_transition(prior).outcome ==
		       telemetry_runtime_outcome::accepted);
	}
	R.next_record_sequence = 0U;
	telemetry_capture_result changed{};
	if (exiting)
	{
		telemetry_session_exit exit{};
		exit.session = enter.session;
		exit.connection = enter.connection;
		exit.at_monotonic_usec = enter.at_monotonic_usec + 1'000U;
		exit.at_utc_usec = enter.at_utc_usec + 1'000;
		exit.reason = telemetry_session_end_reason::logout;
		changed = telemetry_runtime_session_exit(exit);
	}
	else
	{
		telemetry_connection_transition detach{};
		detach.session = enter.session;
		detach.connection = enter.connection;
		detach.at_monotonic_usec = enter.at_monotonic_usec + 1'000U;
		detach.at_utc_usec = enter.at_utc_usec + 1'000;
		detach.kind = reattaching ? telemetry_connection_transition_kind::attached :
					    telemetry_connection_transition_kind::detached;
		if (reattaching)
			detach.connection.connection_seq = 2U;
		changed = telemetry_runtime_connection_transition(detach);
	}
	const auto pulse = telemetry_runtime_pulse(
		{ enter.at_monotonic_usec + 2'000U, enter.at_utc_usec + 2'000, 0U, 0U });
	telemetry_activity_state_view activity{};
	telemetry_session_state_view session{};
	assert(telemetry_activity_state_copy_view(&R.activity, enter.session, &activity));
	assert(telemetry_session_state_copy_view(&R.session, enter.session, &session));
	std::printf("exhausted %s: outcome=%u pulse=%u connected=%u/%u closed=%u/%u "
		    "resident=%llu/%llu linkdead=%llu/%llu\n",
		    exiting ? "exit" : (reattaching ? "attach" : "detach"),
		    static_cast<unsigned>(changed.outcome), static_cast<unsigned>(pulse.outcome),
		    activity.connected, session.connected, activity.closed, session.closed,
		    static_cast<unsigned long long>(activity.cumulative.resident_usec),
		    static_cast<unsigned long long>(session.cumulative.resident_usec),
		    static_cast<unsigned long long>(activity.cumulative.linkdead_usec),
		    static_cast<unsigned long long>(session.cumulative.linkdead_usec));
	std::fflush(stdout);
	assert(R.next_record_sequence == 0U);
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec utc = 0;
	assert(telemetry_runtime_now(&now, &utc));
	assert(telemetry_runtime_shutdown({ now + 5'000'000U, 1U, {} }) ==
	       telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
	telemetry_transport_unbind_for_tests();
	assert(changed.outcome == telemetry_runtime_outcome::queue_full);
	assert(activity.connected == session.connected);
	assert(activity.closed == session.closed);
	assert(session.closed == (exiting ? 1U : 0U));
	assert(activity.cumulative.resident_usec == session.cumulative.resident_usec);
	assert(activity.cumulative.active_usec == session.cumulative.active_usec);
	assert(activity.cumulative.linkdead_usec == session.cumulative.linkdead_usec);
	assert(session.cumulative.active_usec == (reattaching ? 500U : 1'000U));
	assert(session.cumulative.linkdead_usec == (exiting ? 0U : (reattaching ? 500U : 1'000U)));
	assert(((changed.quality_flags | pulse.quality_flags) & TELEMETRY_QUALITY_SEQUENCE_GAP) !=
	       0U);
}
} // namespace

int main()
{
	check_identity_fail_closed();
	check_copyover_caller_ack_race();
	check_copyover_generation_barrier_recovery();
	check_late_copyover_ack();
	check_exhausted_lifecycle(false);
	check_exhausted_lifecycle(true);
	check_exhausted_lifecycle(false, true);
	std::puts("telemetry sequence-exhausted lifecycle consistency passed");
}
