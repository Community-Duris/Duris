#ifndef DURIS_TELEMETRY_TRANSPORT_PRIVATE_H
#define DURIS_TELEMETRY_TRANSPORT_PRIVATE_H

#include "telemetry/telemetry_repository.h"
#include "telemetry/telemetry_transport.h"

#include <cstddef>
#include <cstdint>

/*
 * Test/coordinator seam for #262.  Binding is copied before init and is never
 * changed while a transport lifecycle is active.  The callback context is
 * borrowed and must remain valid through shutdown.  Repository callbacks are
 * synchronous and must be invoked by the designated worker/lifetime owner;
 * the clock callback is also used by the bounded producer admission path.
 */
struct telemetry_transport_repository_binding
{
	using init_function =
		telemetry_repository_outcome (*)(void *, telemetry_repository_config) noexcept;
	using apply_function = telemetry_apply_batch_result (*)(void *, const telemetry_record *,
								std::size_t) noexcept;
	using request_stop_function = telemetry_repository_outcome (*)(void *) noexcept;
	using shutdown_function = void (*)(void *) noexcept;

	init_function init;
	apply_function apply;
	request_stop_function request_stop;
	shutdown_function shutdown;
	void *context;
};

struct telemetry_transport_clock_binding
{
	using now_function = bool (*)(void *, telemetry_monotonic_usec *) noexcept;

	now_function now;
	void *context;
};

/* These limits are implementation bounds, not public configuration knobs. */
inline constexpr std::uint32_t TELEMETRY_TRANSPORT_MAX_RETRY_ATTEMPTS = 8U;
inline constexpr telemetry_duration_usec TELEMETRY_TRANSPORT_RETRY_BACKOFF_INITIAL_USEC = 1'000U;
inline constexpr telemetry_duration_usec TELEMETRY_TRANSPORT_RETRY_BACKOFF_MAX_USEC = 1'000'000U;
inline constexpr std::uint32_t TELEMETRY_TRANSPORT_MAX_DRAIN_BATCHES = 64U;

/* Only the external lifetime owner may bind/unbind or use these lifecycle seams. */
telemetry_transport_outcome
telemetry_transport_bind_for_tests(const telemetry_transport_repository_binding *repository,
				   const telemetry_transport_clock_binding *clock);
void telemetry_transport_unbind_for_tests(void);
telemetry_transport_outcome telemetry_transport_quiesce_for_tests(void);
telemetry_transport_outcome telemetry_transport_resume_for_tests(void);

/* Producer-only cumulative rejection metadata. These are failed admissions,
 * not proof of durable loss: an unadmitted value may still be retried. Zero
 * bounds mean noncontiguous/unknown; never infer a missing inclusive span.
 * The runtime owns fresh keys, gap publication and reported-counter baselines.
 */
struct telemetry_transport_loss_snapshot
{
	std::uint64_t rejected_detail = 0U;
	std::uint64_t rejected_control = 0U;
	std::uint64_t first_rejected_record_seq = 0U;
	std::uint64_t last_rejected_record_seq = 0U;
};
telemetry_transport_loss_snapshot telemetry_transport_loss_copy_for_producer(void);

#endif
