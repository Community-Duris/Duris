#include "economy/collector_catalog_source.h"
#include "economy/collector_listing_pipeline.h"

#include <atomic>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <vector>

namespace
{
struct executor_state
{
	std::mutex mutex;
	std::condition_variable changed;
	uint64_t blocked_listing = 0;
	bool entered = false;
	bool released = false;
};

collector::record candidate(uint64_t listing)
{
	collector::rules rules;
	rules.enabled = true;
	collector::record entry;
	assert(collector::enroll(listing, "11111111111111111111111111111111", 42, 900000 + listing,
				 1, 1700000000, rules, &entry) == collector::outcome::applied);
	return entry;
}

collector_listing_result execute(const collector_listing_request &request, void *raw)
{
	auto &state = *static_cast<executor_state *>(raw);
	if (request.listing == state.blocked_listing)
	{
		std::unique_lock<std::mutex> lock(state.mutex);
		state.entered = true;
		state.changed.notify_all();
		state.changed.wait(lock, [&] { return state.released; });
	}
	if (request.listing == 2)
		return { request.request_id,
			 request.listing,
			 collector_listing_outcome::not_found,
			 ENOENT,
			 { candidate(request.listing), { 1, 2, 3 } } };
	if (request.listing == 3)
		return { request.request_id + 1,
			 request.listing,
			 collector_listing_outcome::found,
			 0,
			 { candidate(request.listing), {} } };
	if (request.listing == 4)
	{
		collector::record held = candidate(request.listing);
		assert(collector::collect(&held, held.revision, 1, 1, true, 50, held.collect_at) ==
		       collector::outcome::applied);
		return { request.request_id,
			 request.listing,
			 collector_listing_outcome::found,
			 0,
			 { held, {} } };
	}
	if (request.listing == 5)
		throw std::bad_alloc();
	if (request.listing == 6)
		return { request.request_id,
			 request.listing,
			 static_cast<collector_listing_outcome>(255),
			 0,
			 { candidate(request.listing), {} } };
	return { request.request_id,
		 request.listing,
		 collector_listing_outcome::found,
		 0,
		 { candidate(request.listing), {} } };
}

bool wait_entered(executor_state &state)
{
	std::unique_lock<std::mutex> lock(state.mutex);
	return state.changed.wait_for(lock, std::chrono::seconds(5), [&] { return state.entered; });
}

std::vector<collector_listing_result> wait_results(size_t wanted)
{
	std::vector<collector_listing_result> results(wanted);
	size_t count = 0;
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (count < wanted && std::chrono::steady_clock::now() < deadline)
	{
		count += collector_listing_pipeline_pulse(results.data() + count, wanted - count);
		if (count < wanted)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	assert(count == wanted);
	return results;
}
} // namespace

bool collector_listing_source_load(uint64_t, collector_listing_detail &, bool &, unsigned int &,
				   std::string &)
{
	return false;
}

int main()
{
	collector_listing_pipeline_reset_for_tests();
	executor_state state;
	assert(collector_listing_pipeline_init(execute, &state));
	assert(!collector_listing_pipeline_init(execute, &state));
	assert(collector_listing_pipeline_submit({}) == collector_listing_submit_outcome::invalid);

	for (uint64_t listing = 1; listing <= 6; ++listing)
		assert(collector_listing_pipeline_submit(
			       { collector_listing_pipeline_next_request_id(), listing }) ==
		       collector_listing_submit_outcome::accepted);
	auto basics = wait_results(6);
	assert(basics[0].outcome == collector_listing_outcome::found &&
	       basics[0].detail.entry.listing == 1);
	assert(basics[1].outcome == collector_listing_outcome::not_found &&
	       basics[1].detail.item_blob.empty());
	assert(basics[2].outcome == collector_listing_outcome::invalid_data &&
	       basics[2].error_code == EBADMSG);
	assert(basics[3].outcome == collector_listing_outcome::invalid_data &&
	       basics[3].error_code == EBADMSG);
	assert(basics[4].outcome == collector_listing_outcome::retryable_failure &&
	       basics[4].error_code == ENOMEM);
	assert(basics[5].outcome == collector_listing_outcome::invalid_data &&
	       basics[5].error_code == EBADMSG);

	state.blocked_listing = 1000;
	state.entered = false;
	state.released = false;
	const uint64_t first_id = collector_listing_pipeline_next_request_id();
	assert(collector_listing_pipeline_submit({ first_id, 1000 }) ==
	       collector_listing_submit_outcome::accepted);
	assert(wait_entered(state));
	assert(collector_listing_pipeline_submit({ first_id, 9999 }) ==
	       collector_listing_submit_outcome::duplicate);
	for (uint64_t offset = 1; offset < COLLECTOR_LISTING_MAX_PENDING; ++offset)
		assert(collector_listing_pipeline_submit(
			       { collector_listing_pipeline_next_request_id(), 1000 + offset }) ==
		       collector_listing_submit_outcome::accepted);
	assert(collector_listing_pipeline_submit(
		       { collector_listing_pipeline_next_request_id(), 9999 }) ==
	       collector_listing_submit_outcome::capacity_exceeded);
	{
		std::lock_guard<std::mutex> lock(state.mutex);
		state.released = true;
		state.changed.notify_all();
	}
	auto batch = wait_results(COLLECTOR_LISTING_MAX_PENDING);
	for (const auto &result : batch)
		assert(result.outcome == collector_listing_outcome::found);

	state.blocked_listing = 2000;
	state.entered = false;
	state.released = false;
	const uint64_t cancelled_id = collector_listing_pipeline_next_request_id();
	assert(collector_listing_pipeline_submit({ cancelled_id, 2000 }) ==
	       collector_listing_submit_outcome::accepted);
	assert(wait_entered(state));
	assert(collector_listing_pipeline_cancel(cancelled_id));
	assert(!collector_listing_pipeline_cancel(999999));
	{
		std::lock_guard<std::mutex> lock(state.mutex);
		state.released = true;
		state.changed.notify_all();
	}
	auto cancelled = wait_results(1);
	assert(cancelled[0].outcome == collector_listing_outcome::cancelled &&
	       cancelled[0].detail.item_blob.empty());

	const collector_listing_pipeline_health health = collector_listing_pipeline_health_copy();
	assert(health.running && !health.stop_pending && health.queued == 0 &&
	       health.inflight == 0 && health.completions == 0 &&
	       health.high_water == COLLECTOR_LISTING_MAX_PENDING && health.submitted == 71 &&
	       health.delivered == 71 && health.cancelled == 1 && health.not_found == 1 &&
	       health.retryable_failures == 1 && health.invalid_data == 3);
	collector_listing_pipeline_shutdown();
	assert(!collector_listing_pipeline_health_copy().running);
	collector_listing_pipeline_reset_for_tests();
}
