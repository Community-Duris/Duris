// Reuse the focused runtime's synthetic game/repository fixtures.
#define main ordinary_runtime_test_main
#include "telemetry_runtime_integration.cc"
#undef main

#include <filesystem>
#include <fstream>
#include <iterator>
#include <sys/stat.h>
#include <vector>

namespace
{
std::atomic<bool> outage_fail_write{ false };
std::atomic<bool> outage_io_on_producer{ false };
std::thread::id outage_producer_thread;
}

extern "C" ssize_t __real_write(int, const void *, size_t);
extern "C" int __real_fsync(int);
extern "C" ssize_t __wrap_write(int fd, const void *data, size_t count)
{
	if (std::this_thread::get_id() == outage_producer_thread)
		outage_io_on_producer = true;
	if (outage_fail_write.load())
	{
		errno = ENOSPC;
		return -1;
	}
	return __real_write(fd, data, count);
}
extern "C" int __wrap_fsync(int fd)
{
	if (std::this_thread::get_id() == outage_producer_thread)
		outage_io_on_producer = true;
	return __real_fsync(fd);
}

namespace
{
namespace fs = std::filesystem;
using outage_result = telemetry_outage_result;
using outage_phase = telemetry_outage_phase;

struct outage_fixture
{
	fs::path directory;
	fake_repository fake{};
	std::atomic<bool> lose_once{ false };
	std::atomic<bool> ambiguous{ false };
	telemetry_producer_id expected_producer{};

	outage_fixture()
	{
		char path[] = "/tmp/duris-runtime-outage-XXXXXX";
		assert(mkdtemp(path));
		directory = path;
		assert(setenv("TELEMETRY_OUTAGE_LEDGER_DIR", directory.c_str(), 1) == 0);
	}
	~outage_fixture()
	{
		assert(unsetenv("TELEMETRY_OUTAGE_LEDGER_DIR") == 0);
		telemetry_transport_unbind_for_tests();
		fs::remove_all(directory);
	}
};

std::uint64_t outage_word(const std::vector<unsigned char> &bytes, std::size_t offset)
{
	std::uint64_t value = 0U;
	for (unsigned int index = 0U; index < 8U; ++index)
		value = (value << 8U) | bytes[offset + index];
	return value;
}

telemetry_repository_outcome outage_init(void *context, telemetry_repository_config config) noexcept
{
	auto &fixture = *static_cast<outage_fixture *>(context);
	assert(std::this_thread::get_id() != outage_producer_thread);
	if (fixture.fake.init_calls == 0U)
	{
		std::ifstream ledger(fixture.directory / "outages.ledger", std::ios::binary);
		const std::vector<unsigned char> bytes{ std::istreambuf_iterator<char>(ledger),
							std::istreambuf_iterator<char>() };
		assert(bytes.size() >= 384U && outage_word(bytes, 16U) != 0U);
		const auto last = 32U + (outage_word(bytes, 16U) - 1U) * 320U;
		assert(outage_word(bytes, last) == fixture.expected_producer.boot_id);
		assert(outage_word(bytes, last + 8U) == fixture.expected_producer.process_id);
		assert(outage_word(bytes, last + 8U * 10U) == 0U);
		assert(telemetry_transport_health_copy().last_admitted_record_seq == 0U);
	}
	return fake_init(&fixture.fake, config);
}

telemetry_apply_batch_result outage_apply(void *context, const telemetry_record *records,
					  std::size_t count) noexcept
{
	auto &fixture = *static_cast<outage_fixture *>(context);
	assert(std::this_thread::get_id() != outage_producer_thread);
	if (fixture.lose_once.exchange(false) || fixture.ambiguous.load())
	{
		telemetry_apply_batch_result failed{};
		failed.outcome = fixture.ambiguous.load() ?
					 telemetry_batch_outcome::commit_ambiguous :
					 telemetry_batch_outcome::unavailable;
		failed.failure_class = fixture.ambiguous.load() ?
					       telemetry_failure_class::commit_ambiguous :
					       telemetry_failure_class::transient_connection;
		if (fixture.ambiguous.load())
		{
			failed.input_count = failed.result_count =
				static_cast<std::uint16_t>(count);
			failed.first_record_seq = records[0].header.key.record_seq;
			failed.last_record_seq = records[count - 1U].header.key.record_seq;
			for (std::size_t index = 0U; index < count; ++index)
			{
				failed.results[index].key = records[index].header.key;
				failed.results[index].outcome =
					telemetry_apply_outcome::commit_ambiguous;
				failed.results[index].failure_class =
					telemetry_failure_class::commit_ambiguous;
			}
		}
		return failed;
	}
	return fake_apply(&fixture.fake, records, count);
}

telemetry_repository_outcome outage_stop(void *context) noexcept
{
	return fake_request_stop(&static_cast<outage_fixture *>(context)->fake);
}
void outage_shutdown(void *context) noexcept
{
	fake_shutdown(&static_cast<outage_fixture *>(context)->fake);
}

void bind_outage(outage_fixture &fixture, const telemetry_runtime_options &options)
{
	fixture.expected_producer = options.producer;
	const telemetry_transport_repository_binding repository = { outage_init, outage_apply,
								    outage_stop, outage_shutdown,
								    &fixture };
	const telemetry_transport_clock_binding clock = { fake_clock, nullptr };
	assert(telemetry_transport_bind_for_tests(&repository, &clock) ==
	       telemetry_transport_outcome::started);
}

void stop_outage_runtime(bool flush = true)
{
	telemetry_monotonic_usec now = 0U;
	telemetry_utc_usec utc = TELEMETRY_UTC_UNKNOWN;
	assert(telemetry_runtime_now(&now, &utc));
	const auto result = telemetry_runtime_shutdown(
		{ now + 1'000'000U, static_cast<std::uint8_t>(flush), {} });
	assert(result == telemetry_runtime_outcome::accepted ||
	       result == telemetry_runtime_outcome::stopping);
	assert(telemetry_runtime_final_reap() == telemetry_runtime_outcome::accepted);
}

void wait_for_storage_refusal()
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
	while (telemetry_runtime_health_copy().state != telemetry_health_state::circuit_open &&
	       std::chrono::steady_clock::now() < deadline)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	const auto health = telemetry_runtime_health_copy();
	assert(health.state == telemetry_health_state::circuit_open);
	assert(health.last_failure_class == telemetry_failure_class::permanent_repository);
}

void required_directory()
{
	assert(unsetenv("TELEMETRY_OUTAGE_LEDGER_DIR") == 0);
	telemetry_transport_unbind_for_tests();
	assert(telemetry_runtime_init(make_enabled_options()) ==
	       telemetry_runtime_outcome::accepted);
	wait_for_storage_refusal();
	assert(telemetry_runtime_health_copy().last_admitted_record_seq == 0U);
	stop_outage_runtime(false);
	std::puts(
		"runtime production binding requires durable registration before SQL admission passed");
}

void clean_and_recovery()
{
	outage_fixture fixture;
	for (unsigned int lifetime = 0U; lifetime < 2U; ++lifetime)
	{
		const auto options = make_enabled_options();
		bind_outage(fixture, options);
		telemetry_test_start_runtime(options);
		assert(telemetry_runtime_session_enter(make_enter(options.producer, options.config))
			       .outcome == telemetry_runtime_outcome::accepted);
		if (lifetime == 0U)
			fixture.lose_once = true;
		stop_outage_runtime();
		telemetry_outage_journal evidence{};
		assert(telemetry_outage_read(&evidence, fixture.directory.c_str()) ==
		       outage_result::ready);
		assert(evidence.count == lifetime + 1U);
		const auto &last = evidence.observations[lifetime];
		assert(last.phase == outage_phase::clean_drained);
		assert(last.health.admitted_control > 0U && last.health.queue_depth == 0U);
		assert(last.health.admitted_detail + last.health.admitted_control ==
		       last.health.applied_records + last.health.duplicate_records +
			       last.health.stale_checkpoint_records);
		if (lifetime == 0U)
			assert(last.health.retryable_failures > 0U);
		telemetry_outage_close(&evidence);
		telemetry_transport_unbind_for_tests();
		fixture.fake.init_calls = 0U;
	}
	std::puts(
		"runtime worker registration, transient SQL recovery and clean restart evidence passed");
}

void ambiguous_shutdown()
{
	outage_fixture fixture;
	const auto options = make_enabled_options();
	bind_outage(fixture, options);
	telemetry_test_start_runtime(options);
	fixture.ambiguous = true;
	assert(telemetry_runtime_session_enter(make_enter(options.producer, options.config))
		       .outcome == telemetry_runtime_outcome::accepted);
	stop_outage_runtime();
	telemetry_outage_journal evidence{};
	assert(telemetry_outage_read(&evidence, fixture.directory.c_str()) == outage_result::ready);
	const auto &last = evidence.observations[0];
	assert(last.phase == outage_phase::unknown_tail && last.inflight_records != 0U);
	assert(last.health.ambiguous_commits > 0U && last.health.queue_depth > 0U);
	telemetry_outage_close(&evidence);
	std::puts("runtime unresolved SQL commit remains an unknown tail after shutdown passed");
}

void failed_storage(bool startup)
{
	outage_fixture fixture;
	const auto options = make_enabled_options();
	bind_outage(fixture, options);
	if (!startup)
		telemetry_test_start_runtime(options);
	outage_fail_write = true;
	if (startup)
		assert(telemetry_runtime_init(options) == telemetry_runtime_outcome::accepted);
	wait_for_storage_refusal();
	const auto before = telemetry_runtime_health_copy();
	assert(before.last_error_code == ENOSPC);
	assert(telemetry_runtime_session_enter(make_enter(options.producer, options.config))
		       .outcome != telemetry_runtime_outcome::accepted);
	assert(telemetry_runtime_health_copy().admitted_control == before.admitted_control);
	outage_fail_write = false;
	stop_outage_runtime(false);
	if (startup)
		assert(fixture.fake.init_calls == 0U &&
		       !fs::exists(fixture.directory / "outages.ledger"));
	telemetry_outage_journal evidence{};
	assert(telemetry_outage_read(&evidence, fixture.directory.c_str()) ==
	       outage_result::corrupt);
	assert(fs::exists(fixture.directory / "outages.pending"));
	std::puts(
		startup ?
			"runtime disk-full startup refuses all SQL admission passed" :
			"runtime disk-full checkpoint stops capture and preserves unresolved evidence passed");
}
} // namespace

int main()
{
	outage_producer_thread = std::this_thread::get_id();
	required_directory();
	clean_and_recovery();
	ambiguous_shutdown();
	failed_storage(true);
	failed_storage(false);
	assert(!outage_io_on_producer.load());
	std::puts("telemetry runtime durable outage journey passed");
}
