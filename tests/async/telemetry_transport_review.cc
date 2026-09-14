#define main original_transport_harness_main
#include "telemetry_transport_harness.cc"
#undef main

int main(int argc, char **argv)
{
	CHECK(argc == 2);
	case_name = argv[1];
	if (std::strcmp(argv[1], "stop-init-race") == 0)
	{
		for (unsigned iteration = 0U; iteration < 1000U; ++iteration)
		{
			telemetry_transport_shutdown();
			auto binding = repository_binding();
			auto clock = clock_binding();
			CHECK(telemetry_transport_bind_for_tests(&binding, &clock) ==
			      telemetry_transport_outcome::started);
			auto settings = config(4U, 1U, 2U);
			if ((iteration & 1U) != 0U)
				settings.backend = telemetry_storage_backend::flatfile_disabled;
			std::atomic<bool> go{ false };
			std::thread initializer(
				[&]
				{
					while (!go.load(std::memory_order_acquire))
					{
					}
					(void)telemetry_transport_init(settings);
				});
			go.store(true, std::memory_order_release);
			(void)telemetry_transport_request_stop();
			initializer.join();
			CHECK(telemetry_transport_init(settings) ==
			      telemetry_transport_outcome::stopping);
			CHECK(telemetry_transport_enqueue(detail_record(1U)).admission ==
			      telemetry_queue_admission::rejected_stopping);
			CHECK(telemetry_transport_health_copy().state ==
			      telemetry_health_state::stopping);
			CHECK(repository_state.init_calls == 0U);
		}
		telemetry_transport_shutdown();
		std::puts("Transport parent stop/init race: PASS");
		return 0;
	}
	bind_and_init(config(4U, 1U, 2U));
	if (std::strcmp(argv[1], "worker-init") == 0)
	{
		CHECK(repository_state.init_calls == 0U);
		std::thread worker([] { (void)telemetry_transport_pulse(1000U); });
		worker.join();
		CHECK(repository_state.init_calls == 1U);
	}
	else if (std::strcmp(argv[1], "control-reserve") == 0)
	{
		for (unsigned seq = 1; seq <= 3; ++seq)
			CHECK(telemetry_transport_enqueue(detail_record(seq)).admission ==
			      telemetry_queue_admission::accepted_detail);
		CHECK(telemetry_transport_enqueue(detail_record(4U)).admission ==
		      telemetry_queue_admission::rejected_detail_full);
		CHECK(telemetry_transport_enqueue(control_record(5U)).admission ==
		      telemetry_queue_admission::accepted_control_reserve);
	}
	else if (std::strcmp(argv[1], "immutable-rejected-key") == 0)
	{
		for (unsigned seq = 1; seq <= 4; ++seq)
			CHECK(telemetry_transport_enqueue(control_record(seq)).admission ==
			      telemetry_queue_admission::accepted_control_reserve);
		CHECK(telemetry_transport_enqueue(detail_record(5U)).admission ==
		      telemetry_queue_admission::rejected_detail_full);
		(void)telemetry_transport_drain_until(1000U);
		for (unsigned call = 0; call < repository_state.calls; ++call)
			for (unsigned row = 0; row < repository_state.observed[call].count; ++row)
				CHECK(repository_state.observed[call]
					      .records[row]
					      .header.key.record_seq != 5U);
		CHECK(telemetry_transport_enqueue(detail_record(5U)).admission ==
		      telemetry_queue_admission::accepted_detail);
	}
	else if (std::strcmp(argv[1], "producer-order") == 0)
	{
		CHECK(telemetry_transport_enqueue(control_record(2U)).admission ==
		      telemetry_queue_admission::accepted_control_reserve);
		CHECK(telemetry_transport_enqueue(control_record(1U)).admission ==
		      telemetry_queue_admission::rejected_invalid);
		auto changed = control_record(2U);
		changed.payload.gap.dropped_records = 1U;
		CHECK(telemetry_transport_enqueue(changed).admission ==
		      telemetry_queue_admission::rejected_invalid);
		auto foreign = control_record(3U);
		foreign.header.key.producer.boot_id = 99U;
		CHECK(telemetry_transport_enqueue(foreign).admission ==
		      telemetry_queue_admission::rejected_invalid);
		CHECK(telemetry_transport_enqueue(control_record(3U)).admission ==
		      telemetry_queue_admission::accepted_control_reserve);
	}
	else if (std::strcmp(argv[1], "ambiguous-invalid-barrier") == 0 ||
		 std::strcmp(argv[1], "mixed-retry-barrier") == 0)
	{
		if (std::strcmp(argv[1], "mixed-retry-barrier") == 0)
		{
			telemetry_transport_shutdown();
			auto binding = repository_binding();
			auto clock = clock_binding();
			binding.apply = [](void *context, const telemetry_record *records,
					   std::size_t count) noexcept
			{
				auto result = fake_apply(context, records, count);
				if (result.outcome == telemetry_batch_outcome::commit_ambiguous)
					result.outcome = telemetry_batch_outcome::retryable_failure;
				return result;
			};
			CHECK(telemetry_transport_bind_for_tests(&binding, &clock) ==
			      telemetry_transport_outcome::started);
			CHECK(telemetry_transport_init(config(4U, 1U, 2U)) ==
			      telemetry_transport_outcome::started);
		}
		CHECK(telemetry_transport_enqueue(detail_record(1U)).admission ==
		      telemetry_queue_admission::accepted_detail);
		CHECK(telemetry_transport_enqueue(detail_record(2U)).admission ==
		      telemetry_queue_admission::accepted_detail);
		repository_state.mode = apply_mode::ambiguous;
		repository_state.failures_left = 1U;
		(void)telemetry_transport_pulse(1000U);
		repository_state.mode = apply_mode::invalid_batch;
		repository_state.failures_left = 1U;
		(void)telemetry_transport_pulse(3000U);
		repository_state.mode = apply_mode::normal;
		(void)telemetry_transport_pulse(6000U);
		CHECK(repository_state.calls == 3U);
		for (unsigned call = 0; call != 3; ++call)
			CHECK(repository_state.observed[call].count == 2U);
	}
	else if (std::strcmp(argv[1], "missing-clock-flush") == 0)
	{
		clock_state.available.store(false);
		CHECK(telemetry_transport_enqueue(detail_record(1U)).admission ==
		      telemetry_queue_admission::accepted_detail);
		clock_state.available.store(true);
		(void)telemetry_transport_pulse(1000U);
		CHECK(repository_state.calls == 1U);
	}
	else if (std::strcmp(argv[1], "bounded-loss") == 0)
	{
		for (unsigned seq = 1; seq <= 3; ++seq)
			CHECK(telemetry_transport_enqueue(detail_record(seq)).admission ==
			      telemetry_queue_admission::accepted_detail);
		CHECK(telemetry_transport_enqueue(detail_record(4U)).admission ==
		      telemetry_queue_admission::rejected_detail_full);
		auto loss = telemetry_transport_loss_copy_for_producer();
		CHECK(loss.rejected_detail == 1U && loss.first_rejected_record_seq == 4U &&
		      loss.last_rejected_record_seq == 4U);
		CHECK(telemetry_transport_enqueue(control_record(5U)).admission ==
		      telemetry_queue_admission::accepted_control_reserve);
		CHECK(telemetry_transport_enqueue(detail_record(6U)).admission ==
		      telemetry_queue_admission::rejected_detail_full);
		loss = telemetry_transport_loss_copy_for_producer();
		CHECK(loss.rejected_detail == 2U && loss.first_rejected_record_seq == 0U &&
		      loss.last_rejected_record_seq == 0U);
		CHECK(telemetry_transport_enqueue(control_record(7U)).admission ==
		      telemetry_queue_admission::rejected_control_full);
		CHECK(telemetry_transport_loss_copy_for_producer().rejected_control == 1U);
	}
	else if (std::strcmp(argv[1], "duplicate-results") == 0)
	{
		telemetry_transport_shutdown();
		auto binding = repository_binding();
		auto clock = clock_binding();
		binding.apply = [](void *context, const telemetry_record *records,
				   std::size_t count) noexcept
		{
			auto result = fake_apply(context, records, count);
			result.applied_count = 0U;
			result.duplicate_count = static_cast<std::uint16_t>(count);
			for (std::size_t i = 0; i < count; ++i)
				result.results[i].outcome =
					telemetry_apply_outcome::duplicate_identical;
			return result;
		};
		CHECK(telemetry_transport_bind_for_tests(&binding, &clock) ==
		      telemetry_transport_outcome::started);
		CHECK(telemetry_transport_init(config(4U, 1U, 2U)) ==
		      telemetry_transport_outcome::started);
		CHECK(telemetry_transport_enqueue(detail_record(1U)).admission ==
		      telemetry_queue_admission::accepted_detail);
		CHECK(telemetry_transport_enqueue(detail_record(2U)).admission ==
		      telemetry_queue_admission::accepted_detail);
		const auto result = telemetry_transport_pulse(1000U);
		CHECK(result.duplicate_or_stale == 2U && result.pending == 0U);
		CHECK(telemetry_transport_health_copy().applied_records == 0U);
		CHECK(telemetry_transport_health_copy().duplicate_records == 2U);
	}
	else if (std::strcmp(argv[1], "stop-worker-init") == 0)
	{
		telemetry_transport_shutdown();
		auto binding = repository_binding();
		auto clock = clock_binding();
		binding.init = [](void *, telemetry_repository_config) noexcept
		{
			io_control.apply_entered.store(true, std::memory_order_release);
			while (!io_control.release_apply.load(std::memory_order_acquire))
				std::this_thread::yield();
			return telemetry_repository_outcome::flatfile_disabled;
		};
		CHECK(telemetry_transport_bind_for_tests(&binding, &clock) ==
		      telemetry_transport_outcome::started);
		CHECK(telemetry_transport_init(config(4U, 1U, 2U)) ==
		      telemetry_transport_outcome::started);
		std::thread worker([] { (void)telemetry_transport_pulse(1000U); });
		while (!io_control.apply_entered.load(std::memory_order_acquire))
			std::this_thread::yield();
		(void)telemetry_transport_request_stop();
		io_control.release_apply.store(true, std::memory_order_release);
		worker.join();
		CHECK(telemetry_transport_init(config(4U, 1U, 2U)) ==
		      telemetry_transport_outcome::stopping);
	}
	else if (std::strcmp(argv[1], "stop-invalid-init") == 0)
	{
		(void)telemetry_transport_request_stop();
		auto bad = config(4U, 1U, 2U);
		bad.queue_capacity = 0U;
		CHECK(telemetry_transport_init(bad) == telemetry_transport_outcome::stopping);
	}
	else if (std::strcmp(argv[1], "cancelled-empty-start") == 0)
	{
		(void)telemetry_transport_request_stop();
		CHECK(telemetry_transport_pulse(1000U).pending == 0U);
		CHECK(repository_state.init_calls == 0U);
		CHECK(telemetry_transport_health_copy().state == telemetry_health_state::stopped);
	}
	else if (std::strcmp(argv[1], "disabled-init-loss") == 0)
	{
		telemetry_transport_shutdown();
		auto binding = repository_binding();
		auto clock = clock_binding();
		binding.init = [](void *, telemetry_repository_config) noexcept
		{ return telemetry_repository_outcome::flatfile_disabled; };
		CHECK(telemetry_transport_bind_for_tests(&binding, &clock) ==
		      telemetry_transport_outcome::started);
		CHECK(telemetry_transport_init(config(4U, 1U, 2U)) ==
		      telemetry_transport_outcome::started);
		CHECK(telemetry_transport_enqueue(detail_record(1U)).admission ==
		      telemetry_queue_admission::accepted_detail);
		CHECK(telemetry_transport_enqueue(control_record(2U)).admission ==
		      telemetry_queue_admission::accepted_control_reserve);
		CHECK(telemetry_transport_pulse(1000U).outcome ==
		      telemetry_transport_outcome::flatfile_disabled);
		CHECK(telemetry_transport_health_copy().queue_depth == 2U);
		CHECK(repository_state.calls == 0U);
		telemetry_transport_shutdown();
		const auto health = telemetry_transport_health_copy();
		CHECK(health.queue_depth == 0U && health.dropped_detail == 1U &&
		      health.dropped_control == 1U);
		CHECK(health.unclosed_tail_count == 1U);
	}
	else
		CHECK(false);
	finish();
	std::printf("Transport parent regression %s: PASS\n", argv[1]);
}
