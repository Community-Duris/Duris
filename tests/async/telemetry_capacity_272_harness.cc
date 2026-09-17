#include "telemetry/telemetry_transport_private.h"
#include "telemetry/telemetry_queue_private.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

namespace
{

constexpr std::size_t MAX_SAMPLES = 10'000U;
constexpr std::uint32_t MAX_RECORDS = 1'000U;
constexpr std::uint32_t MAX_REPETITIONS = 8U;

enum class mode : std::uint8_t
{
	off,
	capture_only,
	capture_write,
};

struct benchmark_repository
{
	std::uint64_t calls = 0U;
	std::uint64_t records = 0U;
};

struct benchmark_clock
{
	telemetry_monotonic_usec now = 100U;
};

struct benchmark_result
{
	std::uint64_t sample_count = 0U;
	std::uint64_t wall_ns = 0U;
	std::uint64_t cpu_ns = 0U;
	std::uint64_t p50_ns = 0U;
	std::uint64_t p95_ns = 0U;
	std::uint64_t p99_ns = 0U;
	std::uint64_t p999_ns = 0U;
	std::uint64_t worker_p95_ns = 0U;
	std::uint64_t worker_p99_ns = 0U;
	std::uint64_t worker_p999_ns = 0U;
	std::uint64_t admitted = 0U;
	std::uint64_t dropped = 0U;
	std::uint64_t repository_calls = 0U;
	std::uint64_t repository_records = 0U;
	std::uint64_t queue_peak = 0U;
	std::uint64_t queue_reserved_bytes = 0U;
	std::uint64_t queue_capacity = TELEMETRY_QUEUE_CAPACITY_PROPOSAL;
	std::uint64_t control_reserve = TELEMETRY_CONTROL_RESERVE_PROPOSAL;
	std::uint64_t checksum = 0U;
};

[[noreturn]] void fail(const char *message)
{
	std::fprintf(stderr, "telemetry capacity 272 harness: %s\n", message);
	std::exit(2);
}

std::uint32_t parse_u32(const char *value, std::uint32_t minimum, std::uint32_t maximum)
{
	if (value == nullptr || *value == '\0')
		fail("numeric argument is empty");
	char *end = nullptr;
	const unsigned long parsed = std::strtoul(value, &end, 10);
	if (end == value || *end != '\0' || parsed < minimum || parsed > maximum)
		fail("numeric argument is outside the supported bound");
	return static_cast<std::uint32_t>(parsed);
}

mode parse_mode(const char *value)
{
	if (std::strcmp(value, "off") == 0)
		return mode::off;
	if (std::strcmp(value, "capture_only") == 0)
		return mode::capture_only;
	if (std::strcmp(value, "capture_write") == 0)
		return mode::capture_write;
	fail("unknown mode");
}

const char *mode_name(mode value)
{
	switch (value)
	{
	case mode::off:
		return "off";
	case mode::capture_only:
		return "capture_only";
	case mode::capture_write:
		return "capture_write";
	}
	return "unknown";
}

telemetry_repository_outcome repository_init(void *context,
					     telemetry_repository_config config) noexcept
{
	auto &repository = *static_cast<benchmark_repository *>(context);
	if (!telemetry_repository_config_is_bounded(config))
		return telemetry_repository_outcome::invalid_config;
	++repository.calls;
	return telemetry_repository_outcome::ready;
}

telemetry_apply_batch_result repository_apply(void *context, const telemetry_record *records,
					      std::size_t count) noexcept
{
	auto &repository = *static_cast<benchmark_repository *>(context);
	if (records == nullptr || count == 0U || count > TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL)
		return {};
	++repository.calls;
	repository.records += count;
	telemetry_apply_batch_result result{};
	result.outcome = telemetry_batch_outcome::committed;
	result.input_count = static_cast<std::uint16_t>(count);
	result.result_count = static_cast<std::uint16_t>(count);
	result.applied_count = static_cast<std::uint16_t>(count);
	result.first_record_seq = records[0].header.key.record_seq;
	result.last_record_seq = records[count - 1U].header.key.record_seq;
	for (std::size_t index = 0U; index < count; ++index)
	{
		result.results[index].key = records[index].header.key;
		result.results[index].outcome = telemetry_apply_outcome::applied;
	}
	return result;
}

telemetry_repository_outcome repository_request_stop(void *) noexcept
{
	return telemetry_repository_outcome::stopping;
}

void repository_shutdown(void *) noexcept {}

bool clock_now(void *context, telemetry_monotonic_usec *value) noexcept
{
	if (context == nullptr || value == nullptr)
		return false;
	*value = static_cast<benchmark_clock *>(context)->now;
	return true;
}

telemetry_transport_config transport_config()
{
	return { telemetry_storage_backend::sql,
		 0U,
		 TELEMETRY_SCHEMA_VERSION,
		 static_cast<std::uint32_t>(TELEMETRY_QUEUE_CAPACITY_PROPOSAL),
		 static_cast<std::uint32_t>(TELEMETRY_CONTROL_RESERVE_PROPOSAL),
		 static_cast<std::uint16_t>(TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL),
		 0U,
		 static_cast<std::uint32_t>(TELEMETRY_BATCH_MAX_BYTES_PROPOSAL),
		 1U };
}

telemetry_record make_record(std::uint64_t sequence)
{
	telemetry_record record{};
	record.header = { TELEMETRY_SCHEMA_VERSION,
			  telemetry_record_kind::interval,
			  0U,
			  { { 11U, 22U }, sequence },
			  1'000'000 + static_cast<telemetry_utc_usec>(sequence) };
	auto &interval = record.payload.interval;
	interval.session = { { { 11U, 22U }, sequence }, 44U, 55U, 66U, 77U };
	interval.connection = { { 11U, 22U }, sequence };
	interval.window = { sequence * 10U, sequence * 10U + 1U,
			    1'000'000 + static_cast<std::int64_t>(sequence),
			    1'000'001 + static_cast<std::int64_t>(sequence) };
	interval.duration_usec = 1U;
	interval.category = telemetry_interval_category::connected_active;
	interval.context = telemetry_activity_context::combat;
	interval.context_quality = telemetry_context_quality::observed;
	interval.dimensions = { 1U, 2U, 3U, 4U, 5, 1U };
	interval.config_id = 1U;
	interval.classifier_version = 1U;
	interval.policy_version = 1U;
	interval.quality_flags = TELEMETRY_QUALITY_NONE;
	if (!telemetry_record_is_valid(record))
		fail("benchmark record is invalid");
	return record;
}

template <typename Clock> std::uint64_t elapsed_ns(const Clock &started, const Clock &finished)
{
	return static_cast<std::uint64_t>(
		std::chrono::duration_cast<std::chrono::nanoseconds>(finished - started).count());
}

std::uint64_t percentile(std::array<std::uint64_t, MAX_SAMPLES> &samples, std::size_t count,
			 std::uint32_t percentile_value)
{
	if (count == 0U)
		return 0U;
	std::sort(samples.begin(), samples.begin() + static_cast<std::ptrdiff_t>(count));
	const std::size_t rank = (count * percentile_value + 99U) / 100U;
	const std::size_t index = rank == 0U ? 0U : rank - 1U;
	return samples[index < count ? index : count - 1U];
}

benchmark_result run(mode selected, std::uint32_t records, std::uint32_t repetitions)
{
	benchmark_result result{};
	std::array<std::uint64_t, MAX_SAMPLES> samples{};
	std::array<std::uint64_t, MAX_SAMPLES> worker_samples{};
	std::size_t sample_count = 0U;
	std::size_t worker_count = 0U;
	benchmark_repository repository{};
	benchmark_clock clock{};
	if (selected == mode::capture_write)
	{
		const telemetry_transport_repository_binding binding = {
			repository_init, repository_apply, repository_request_stop,
			repository_shutdown, &repository
		};
		const telemetry_transport_clock_binding clock_binding = { clock_now, &clock };
		if (telemetry_transport_bind_for_tests(&binding, &clock_binding) !=
		    telemetry_transport_outcome::started)
			fail("could not bind benchmark repository");
		if (telemetry_transport_init(transport_config()) !=
		    telemetry_transport_outcome::started)
			fail("could not initialize benchmark transport");
	}

	const auto wall_started = std::chrono::steady_clock::now();
	const std::clock_t cpu_started = std::clock();
	for (std::uint32_t repetition = 0U; repetition < repetitions; ++repetition)
	{
		for (std::uint32_t index = 0U; index < records; ++index)
		{
			const auto sample_started = std::chrono::steady_clock::now();
			const std::uint64_t sequence =
				static_cast<std::uint64_t>(repetition) * records + index + 1U;
			const telemetry_record record = make_record(sequence);
			if (selected == mode::off)
			{
				result.checksum += record.header.key.record_seq +
						   record.payload.interval.duration_usec;
			}
			else if (selected == mode::capture_only)
			{
				if (!telemetry_record_is_valid(record))
					fail("capture-only validation rejected a benchmark record");
				result.checksum += record.header.key.record_seq;
			}
			else
			{
				const auto admission = telemetry_transport_enqueue(record);
				if (admission.admission !=
				    telemetry_queue_admission::accepted_detail)
					fail("capture-write benchmark dropped a record");
				++result.admitted;
				result.checksum += admission.key.record_seq;
			}
			const auto sample_finished = std::chrono::steady_clock::now();
			if (sample_count >= samples.size())
				fail("latency sample bound exceeded");
			samples[sample_count++] = elapsed_ns(sample_started, sample_finished);

			if (selected == mode::capture_write && (index + 1U) % 128U == 0U)
			{
				clock.now += 2U;
				const auto worker_started = std::chrono::steady_clock::now();
				(void)telemetry_transport_pulse(clock.now);
				const auto worker_finished = std::chrono::steady_clock::now();
				if (worker_count >= worker_samples.size())
					fail("worker latency sample bound exceeded");
				worker_samples[worker_count++] =
					elapsed_ns(worker_started, worker_finished);
			}
		}
		if (selected == mode::capture_write)
		{
			clock.now += 2U;
			const auto worker_started = std::chrono::steady_clock::now();
			const auto drained =
				telemetry_transport_drain_until(clock.now + 1'000'000U);
			const auto worker_finished = std::chrono::steady_clock::now();
			if (drained.pending != 0U)
				fail("capture-write benchmark left a queue tail");
			if (worker_count >= worker_samples.size())
				fail("worker latency sample bound exceeded");
			worker_samples[worker_count++] =
				elapsed_ns(worker_started, worker_finished);
		}
	}
	const auto wall_finished = std::chrono::steady_clock::now();
	const std::clock_t cpu_finished = std::clock();

	if (selected == mode::capture_write)
	{
		const auto health = telemetry_transport_health_copy();
		result.dropped = health.dropped_detail + health.dropped_control;
		result.queue_peak = health.queue_high_water;
		result.queue_reserved_bytes = sizeof(telemetry_queue_private::queue);
		result.repository_calls = repository.calls;
		result.repository_records = repository.records;
		(void)telemetry_transport_request_stop();
		telemetry_transport_shutdown();
		telemetry_transport_unbind_for_tests();
	}

	result.sample_count = sample_count;
	result.wall_ns = elapsed_ns(wall_started, wall_finished);
	if (cpu_started != static_cast<std::clock_t>(-1) && cpu_finished >= cpu_started &&
	    CLOCKS_PER_SEC > 0)
		result.cpu_ns = static_cast<std::uint64_t>(
			(static_cast<long double>(cpu_finished - cpu_started) * 1'000'000'000.0L) /
			static_cast<long double>(CLOCKS_PER_SEC));
	result.p50_ns = percentile(samples, sample_count, 50U);
	result.p95_ns = percentile(samples, sample_count, 95U);
	result.p99_ns = percentile(samples, sample_count, 99U);
	result.p999_ns = percentile(samples, sample_count, 999U);
	result.worker_p95_ns = percentile(worker_samples, worker_count, 95U);
	result.worker_p99_ns = percentile(worker_samples, worker_count, 99U);
	result.worker_p999_ns = percentile(worker_samples, worker_count, 999U);
	return result;
}

void print_json(mode selected, std::uint32_t records, std::uint32_t repetitions,
		benchmark_result result)
{
	std::printf("{\"schema_version\":1,\"mode\":\"%s\",\"records\":%u,"
		    "\"repetitions\":%u,\"sample_count\":%llu,\"wall_ns\":%llu,"
		    "\"cpu_ns\":%llu,\"p50_ns\":%llu,\"p95_ns\":%llu,"
		    "\"p99_ns\":%llu,\"p999_ns\":%llu,\"worker_p95_ns\":%llu,"
		    "\"worker_p99_ns\":%llu,\"worker_p999_ns\":%llu,"
		    "\"admitted\":%llu,\"dropped\":%llu,\"repository_calls\":%llu,"
		    "\"repository_records\":%llu,\"queue_peak\":%llu,"
		    "\"queue_reserved_bytes\":%llu,\"queue_capacity\":%llu,"
		    "\"control_reserve\":%llu,\"checksum\":%llu}\n",
		    mode_name(selected), records, repetitions,
		    static_cast<unsigned long long>(result.sample_count),
		    static_cast<unsigned long long>(result.wall_ns),
		    static_cast<unsigned long long>(result.cpu_ns),
		    static_cast<unsigned long long>(result.p50_ns),
		    static_cast<unsigned long long>(result.p95_ns),
		    static_cast<unsigned long long>(result.p99_ns),
		    static_cast<unsigned long long>(result.p999_ns),
		    static_cast<unsigned long long>(result.worker_p95_ns),
		    static_cast<unsigned long long>(result.worker_p99_ns),
		    static_cast<unsigned long long>(result.worker_p999_ns),
		    static_cast<unsigned long long>(result.admitted),
		    static_cast<unsigned long long>(result.dropped),
		    static_cast<unsigned long long>(result.repository_calls),
		    static_cast<unsigned long long>(result.repository_records),
		    static_cast<unsigned long long>(result.queue_peak),
		    static_cast<unsigned long long>(result.queue_reserved_bytes),
		    static_cast<unsigned long long>(result.queue_capacity),
		    static_cast<unsigned long long>(result.control_reserve),
		    static_cast<unsigned long long>(result.checksum));
}

} // namespace

/* The benchmark always injects its repository binding.  These definitions
 * keep the transport's production fallback linkable without opening SQL. */
telemetry_repository_outcome telemetry_repository_init(telemetry_repository_config)
{
	return telemetry_repository_outcome::unavailable;
}

telemetry_apply_batch_result telemetry_repository_apply(const telemetry_record *, std::size_t)
{
	return {};
}

telemetry_repository_outcome telemetry_repository_request_stop(void)
{
	return telemetry_repository_outcome::stopping;
}

void telemetry_repository_shutdown(void) {}

int main(int argc, char **argv)
{
	if (argc != 4)
		fail("usage: <off|capture_only|capture_write> <records> <repetitions>");
	const mode selected = parse_mode(argv[1]);
	const std::uint32_t records = parse_u32(argv[2], 1U, MAX_RECORDS);
	const std::uint32_t repetitions = parse_u32(argv[3], 1U, MAX_REPETITIONS);
	if (static_cast<std::uint64_t>(records) * repetitions > MAX_SAMPLES)
		fail("records * repetitions exceeds the sample bound");
	print_json(selected, records, repetitions, run(selected, records, repetitions));
	return 0;
}
