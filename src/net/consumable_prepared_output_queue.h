#ifndef DURIS_CONSUMABLE_PREPARED_OUTPUT_QUEUE_H
#define DURIS_CONSUMABLE_PREPARED_OUTPUT_QUEUE_H

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct txt_q;
class consumable_prepared_output_owner;
struct consumable_proposed_output_queue_state
{
	size_t bytes = 0, entries = 0;
	bool overflowed = false;
};

// A value/resource proposal for one ACTUAL original queue. Preparing or reading
// it cannot publish text, grant a native effect or infer a recipient lifetime.
class consumable_prepared_output_queue final
{
    public:
	consumable_prepared_output_queue(const consumable_prepared_output_queue &) = delete;
	consumable_prepared_output_queue &
	operator=(const consumable_prepared_output_queue &) = delete;
	~consumable_prepared_output_queue() noexcept;
	static std::unique_ptr<consumable_prepared_output_queue>
	prepare(const txt_q &, std::span<const std::string_view>) noexcept;
	bool copy_proposed_bytes(std::vector<std::string> *) const noexcept;
	bool proposed_state(consumable_proposed_output_queue_state *) const noexcept;
	bool current_storage_bytes(size_t *) const noexcept;

    private:
	friend class consumable_prepared_output_owner;
	struct state;
	explicit consumable_prepared_output_queue(std::unique_ptr<state>) noexcept;
	// Only the real recipient/native owner may call this after its own current
	// descriptor, held source and once checks. No allocating queue operation.
	bool transfer(txt_q &) noexcept;
	bool retained_exact(const txt_q &) const noexcept;
	bool owned_allocation_request_bytes(size_t *) const noexcept;
	std::unique_ptr<state> state_;
};

#endif
