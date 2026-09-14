// Link real repository + transport in the disabled build, without SQL stubs.
#include "telemetry/telemetry_repository.h"
#include "telemetry/telemetry_transport.h"
#include <cassert>
#include <cstdio>

int main()
{
	telemetry_transport_config transport{};
	transport.backend = telemetry_storage_backend::flatfile_disabled;
	transport.schema_version = TELEMETRY_SCHEMA_VERSION;
	transport.queue_capacity = 16U;
	transport.control_reserve = 4U;
	transport.max_batch_records = 4U;
	transport.max_batch_bytes = 4U * sizeof(telemetry_record);
	transport.flush_oldest_after_usec = 1000U;
	telemetry_repository_config repository{};
	repository.backend = telemetry_storage_backend::flatfile_disabled;
	repository.schema_version = TELEMETRY_SCHEMA_VERSION;
	repository.max_batch_records = transport.max_batch_records;
	repository.max_batch_bytes = transport.max_batch_bytes;
	for (unsigned lifecycle = 0U; lifecycle != 2U; ++lifecycle)
	{
		assert(telemetry_transport_init(transport) ==
		       telemetry_transport_outcome::flatfile_disabled);
		assert(telemetry_repository_init(repository) ==
		       telemetry_repository_outcome::flatfile_disabled);
		assert(telemetry_transport_health_copy().state == telemetry_health_state::disabled);
		assert(telemetry_repository_health_copy().state ==
		       telemetry_health_state::disabled);
		const auto admission = telemetry_transport_enqueue({}).admission;
		assert(admission != telemetry_queue_admission::accepted_detail);
		assert(admission != telemetry_queue_admission::accepted_control_reserve);
		assert(telemetry_repository_apply(nullptr, 0U).outcome ==
		       telemetry_batch_outcome::disabled);
		assert(telemetry_transport_pulse(1U).pending == 0U);
		assert(telemetry_transport_drain_until(1U).pending == 0U);
		(void)telemetry_transport_request_stop();
		(void)telemetry_repository_request_stop();
		// No worker was started: shutdown cannot be hiding a join or connector operation.
		telemetry_transport_shutdown();
		telemetry_repository_shutdown();
	}
	std::puts("ISSUE262_REAL_DISABLED_REPOSITORY_JOURNEY_OK");
}
