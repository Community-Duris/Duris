#ifndef DURIS_ARTIFACT_MANA_RUNTIME_H
#define DURIS_ARTIFACT_MANA_RUNTIME_H

#include "item/artifact_mana_store.h"

#include <cstddef>
#include <memory>

class artifact_mana_backend
{
    public:
	virtual ~artifact_mana_backend() = default;
	virtual artifact_mana_read read(uint64_t uid, artifact_mana_record &) = 0;
	virtual bool write(uint64_t expected, const artifact_mana_record &) = 0;
};

struct artifact_mana_health
{
	size_t cached = 0;
	size_t dirty = 0;
	size_t outstanding = 0;
	uint64_t write_failures = 0;
};

// Caller is the game thread; the backend runs only on the owned worker. Neither
// the queue nor the cache contains world pointers. Time arguments are UTC seconds
// for regeneration and monotonic milliseconds for persistence admission limits.
class artifact_mana_runtime
{
    public:
	explicit artifact_mana_runtime(std::unique_ptr<artifact_mana_backend>);
	~artifact_mana_runtime();
	artifact_mana_runtime(const artifact_mana_runtime &) = delete;
	artifact_mana_runtime &operator=(const artifact_mana_runtime &) = delete;
	bool inspect(uint64_t uid, const artifact_mana_profile &, uint64_t wall, uint64_t mono,
		     artifact_mana_record &);
	// Action tokens must be globally increasing in this process. Retries of any
	// accepted token are rejected, including after other powers use the same pool.
	bool debit(uint64_t uid, const artifact_mana_profile &, uint64_t wall, uint64_t mono,
		   uint64_t cost, bool passive, uint64_t action_token);
	void pulse(uint64_t mono);
	bool spending_ready(uint64_t uid, uint64_t mono) const;
	artifact_mana_health health() const;
	// Caller pumps pulse until clean before shutdown. Destruction joins at most
	// the current backend operation and discards queued work; dirty work is never
	// reported as durable. Ordinary shutdown uses the bounded drain in the bridge.
	void stop();

    private:
	struct implementation;
	std::unique_ptr<implementation> impl;
};

std::unique_ptr<artifact_mana_backend> artifact_mana_persistent_backend(bool sql,
									const std::string &root);

#endif
