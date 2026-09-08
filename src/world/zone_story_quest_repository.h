#ifndef ZONE_STORY_QUEST_REPOSITORY_H
#define ZONE_STORY_QUEST_REPOSITORY_H

#include "world/zone_story_quest_tracking.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace zone_story_quest_repository
{
enum class record_result : uint8_t
{
	applied,
	already_applied,
	invalid,
	conflict,
};

class completion_repository
{
    public:
	virtual ~completion_repository() = default;

	virtual record_result
	record(const zone_story_quest_tracking::completion_transaction &transaction,
	       std::string *error = nullptr) = 0;
	virtual bool get_transaction(std::string_view transaction_id,
				     zone_story_quest_tracking::completion_transaction *transaction,
				     std::string *error = nullptr) const = 0;
	virtual std::vector<zone_story_quest_tracking::completion_transaction>
	list_for_pid(uint32_t pid, uint32_t season_id, int32_t zone_number) const = 0;
};

class in_memory_completion_repository final : public completion_repository
{
    public:
	record_result record(const zone_story_quest_tracking::completion_transaction &transaction,
			     std::string *error = nullptr) override;
	bool get_transaction(std::string_view transaction_id,
			     zone_story_quest_tracking::completion_transaction *transaction,
			     std::string *error = nullptr) const override;
	std::vector<zone_story_quest_tracking::completion_transaction>
	list_for_pid(uint32_t pid, uint32_t season_id, int32_t zone_number) const override;
	std::size_t size() const;

    private:
	std::map<std::string, zone_story_quest_tracking::completion_transaction> transactions_;
};
} // namespace zone_story_quest_repository

#endif
