#ifndef DURIS_NATIVE_QUEST_CONSUMPTION_CAPTURE_H
#define DURIS_NATIVE_QUEST_CONSUMPTION_CAPTURE_H

#include "world/quest_mobile_native_reference.h"
#include "item/item_transfer_command.h"

class quest_native_completion_owner;

// Original ordered quest decision, issued only by the addressed quest owner.
// Read-only access permits retention/projection, never another issuance.
// The original v11 command binds every selected row/literal/continuation before
// the acknowledged checkpoint adds its explicit v12 recovery extension.
class quest_native_consumption_capture
{
    public:
	quest_native_consumption_capture(const quest_native_consumption_capture &) = delete;
	quest_native_consumption_capture &
	operator=(const quest_native_consumption_capture &) = delete;
	const critical_command &original_command() const noexcept { return original_; }
	const quest_mobile_native_reference &reference() const noexcept { return reference_; }
	uint64_t runtime_generation() const noexcept { return runtime_generation_; }
	uint32_t final_giver_pid() const noexcept { return final_giver_pid_; }
	const economic_source_event &source_event() const noexcept { return source_; }
	const std::vector<uint64_t> &consumed_root_order() const noexcept
	{
		return consumed_root_order_;
	}
	const item_native_quest_publication_terms &publication_terms() const noexcept
	{
		return publication_terms_;
	}

    private:
	friend class quest_native_completion_owner;
	quest_native_consumption_capture(critical_command original,
					 const quest_mobile_native_reference &reference,
					 uint64_t runtime_generation, uint32_t final_giver_pid,
					 uint32_t completion_slot,
					 std::vector<uint64_t> ordered_consumed_roots,
					 item_native_quest_publication_terms publication_terms,
					 economic_source_kind action);
	const critical_command original_;
	const quest_mobile_native_reference reference_;
	const uint64_t runtime_generation_;
	const uint32_t final_giver_pid_;
	const std::vector<uint64_t> consumed_root_order_;
	const item_native_quest_publication_terms publication_terms_;
	const economic_source_event source_;
};

#endif
