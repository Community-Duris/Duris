#include "world/zone_story_quest_catalog.h"

#include <unordered_set>

namespace zone_story_quest_catalog
{
namespace
{
void add_diagnostic(std::vector<diagnostic> *diagnostics, std::size_t index, const char *code,
		    const std::string &message)
{
	if (diagnostics)
		diagnostics->push_back({ .index = index, .code = code, .message = message });
}
} // namespace

bool validate(const catalog &catalog, std::vector<diagnostic> *diagnostics)
{
	if (diagnostics)
		diagnostics->clear();
	bool valid = true;
	if (catalog.schema_version != ZONE_STORY_QUEST_CATALOG_SCHEMA_VERSION)
	{
		add_diagnostic(diagnostics, 0, "unsupported_schema_version",
			       "catalog schema_version is not supported");
		valid = false;
	}
	if (catalog.content_revision == 0)
	{
		add_diagnostic(diagnostics, 0, "invalid_content_revision",
			       "catalog content_revision must be positive");
		valid = false;
	}

	std::unordered_set<std::string> definition_ids;
	for (std::size_t index = 0; index < catalog.definitions.size(); ++index)
	{
		const auto &definition = catalog.definitions[index];
		std::string error;
		if (!zone_story_quest_tracking::validate_definition(definition, &error))
		{
			add_diagnostic(diagnostics, index, "invalid_definition", error);
			valid = false;
		}
		if (!definition_ids.insert(definition.definition_id).second)
		{
			add_diagnostic(diagnostics, index, "duplicate_definition_id",
				       definition.definition_id);
			valid = false;
		}
		if (definition.content_revision != catalog.content_revision)
		{
			add_diagnostic(diagnostics, index, "revision_mismatch",
				       "definition revision does not match catalog revision");
			valid = false;
		}
	}
	return valid;
}

std::size_t eligible_definition_count(const catalog &catalog, int32_t zone_number,
				      uint32_t content_revision)
{
	std::size_t count = 0;
	for (const auto &definition : catalog.definitions)
	{
		if (definition.active && definition.eligible_for_zone_completion &&
		    definition.repeatable && definition.zone_number == zone_number &&
		    definition.content_revision == content_revision)
			++count;
	}
	return count;
}
} // namespace zone_story_quest_catalog
