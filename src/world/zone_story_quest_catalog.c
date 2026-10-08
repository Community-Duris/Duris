#include "world/zone_story_quest_catalog.h"

#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include "core/defines.h"

namespace zone_story_quest_catalog
{
namespace
{
void add_diagnostic(std::vector<diagnostic> *diagnostics, int64_t index, const char *code,
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
		add_diagnostic(diagnostics, -1, "unsupported_schema_version",
			       "catalog schema_version is not supported");
		valid = false;
	}
	if (catalog.content_revision == 0)
	{
		add_diagnostic(diagnostics, -1, "invalid_content_revision",
			       "catalog content_revision must be positive");
		valid = false;
	}

	std::unordered_set<int32_t> zone_numbers;
	for (const auto &zone : catalog.zones)
	{
		if (!zone_numbers.insert(zone.zone_number).second || zone.name.empty() ||
		    zone.first_vnum < 0 || zone.last_vnum < zone.first_vnum ||
		    (zone.discoverable && zone.zone_number <= 0))
		{
			add_diagnostic(diagnostics, -1, "invalid_zone_registry",
				       "area registry has duplicate or invalid ownership metadata");
			valid = false;
		}
	}
	std::unordered_set<std::string> definition_ids;
	std::unordered_map<std::string, const zone_story_quest_tracking::quest_definition *>
		definitions;
	for (std::size_t index = 0; index < catalog.definitions.size(); ++index)
	{
		const auto &definition = catalog.definitions[index];
		std::string error;
		if (!zone_story_quest_tracking::validate_definition(definition, &error))
		{
			add_diagnostic(diagnostics, static_cast<int64_t>(index),
				       "invalid_definition", error);
			valid = false;
		}
		if (!definition_ids.insert(definition.definition_id).second)
		{
			add_diagnostic(diagnostics, static_cast<int64_t>(index),
				       "duplicate_definition_id", definition.definition_id);
			valid = false;
		}
		if (definition.content_revision != catalog.content_revision)
		{
			add_diagnostic(diagnostics, static_cast<int64_t>(index),
				       "revision_mismatch",
				       "definition revision does not match catalog revision");
			valid = false;
		}
		definitions.emplace(definition.definition_id, &definition);
	}
	std::unordered_set<std::string> mapped_areas, story_ids, bound_contracts;
	for (const auto &mapping : catalog.story_mappings)
	{
		int32_t zone_number = -1;
		for (const auto &zone : catalog.zones)
			if (zone.source_area == mapping.source_area && zone.discoverable)
				zone_number = zone.zone_number;
		auto bad = [&](const std::string &message)
		{
			add_diagnostic(diagnostics, -1, "invalid_story_mapping", message);
			valid = false;
		};
		if (!mapping.revision || zone_number <= 0 ||
		    !mapped_areas.insert(mapping.source_area).second)
			bad("story mapping has an invalid/duplicate area or revision");
		std::unordered_set<int32_t> contacts;
		for (const auto &contact : mapping.contacts)
			if (contact.mob_vnum <= 0 || !contacts.insert(contact.mob_vnum).second ||
			    contact.name.empty() || contact.keyword.empty() ||
			    contact.description.empty())
				bad("story mapping has an invalid or duplicate contact");
		auto bind = [&](const std::string &id)
		{
			const auto found = definitions.find(id);
			if (found == definitions.end() || !found->second->active ||
			    found->second->zone_number != zone_number ||
			    !bound_contracts.insert(id).second)
				bad("unknown, wrong-area, or multiply bound native contract: " +
				    id);
		};
		for (const auto &story : mapping.stories)
		{
			if (story.id.empty() || !story_ids.insert(story.id).second ||
			    story.zone_number != zone_number || story.title.empty() ||
			    story.summary.empty() || story.contracts.empty() ||
			    story.steps.empty() ||
			    (story.category != "story" && story.category != "request" &&
			     story.category != "service"))
				bad("story has invalid identity, presentation, category, or terminal contracts");
			for (const auto &id : story.contracts)
				bind(id);
			std::unordered_set<std::string> steps;
			for (const auto &step : story.steps)
			{
				if (step.id.empty() || !steps.insert(step.id).second ||
				    step.text.empty())
					bad("story has an invalid or duplicate step");
				if (step.kind == "completion")
				{
					if (step.contracts.empty())
						bad("completion step requires native contracts");
					for (const auto &id : step.contracts)
						if (!definitions.contains(id))
							bad("completion step refers to an unknown native contract");
				}
				else if (step.kind == "carried_item" ||
					 step.kind == "equipped_item")
				{
					if (!step.count || step.count > 3000 ||
					    step.item_vnums.empty() || step.slot < -1 ||
					    step.slot >= MAX_WEAR ||
					    std::any_of(step.item_vnums.begin(),
							step.item_vnums.end(),
							[](int32_t vnum) { return vnum <= 0; }))
						bad("item step has invalid items, count, or equipment slot");
				}
				else
					bad("story uses an unsupported step kind");
			}
		}
		for (const auto &[id, reason] : mapping.exclusions)
		{
			bind(id);
			if (reason.empty())
				bad("excluded contract requires a reason");
		}
		if (mapping.complete)
			for (const auto &definition : catalog.definitions)
				if (definition.active &&
				    definition.source_area == mapping.source_area &&
				    !bound_contracts.contains(definition.definition_id))
					bad("complete story mapping leaves a native contract unclassified");
	}
	return valid;
}

bool excluded_contract(const catalog &catalog, std::string_view id)
{
	for (const auto &mapping : catalog.story_mappings)
		if (mapping.exclusions.contains(std::string(id)))
			return true;
	return false;
}

const story_definition *story_for_contract(const catalog &catalog, std::string_view id)
{
	for (const auto &mapping : catalog.story_mappings)
		for (const auto &story : mapping.stories)
			if (std::find(story.contracts.begin(), story.contracts.end(), id) !=
			    story.contracts.end())
				return &story;
	return nullptr;
}

std::vector<quest_unit> quest_units(const catalog &catalog)
{
	std::vector<quest_unit> result;
	std::unordered_set<std::string> bound;
	std::unordered_map<std::string, const zone_story_quest_tracking::quest_definition *>
		definitions;
	for (const auto &definition : catalog.definitions)
		definitions.emplace(definition.definition_id, &definition);
	for (const auto &mapping : catalog.story_mappings)
	{
		for (const auto &[id, reason] : mapping.exclusions)
		{
			(void)reason;
			bound.insert(id);
		}
		for (const auto &story : mapping.stories)
		{
			quest_unit unit{ .id = story.id, .zone_number = story.zone_number };
			for (const auto &id : story.contracts)
			{
				bound.insert(id);
				const auto found = definitions.find(id);
				if (found == definitions.end() || !found->second->active)
					continue;
				unit.contracts.push_back(id);
				unit.achievement = unit.achievement ||
						   found->second->eligible_for_zone_completion;
				unit.daily_candidate = unit.daily_candidate ||
						       found->second->daily_eligible;
			}
			if (story.category == "service")
				unit.achievement = unit.daily_candidate = false;
			if (!unit.contracts.empty())
				result.push_back(std::move(unit));
		}
	}
	for (const auto &definition : catalog.definitions)
		if (definition.active && !bound.contains(definition.definition_id))
			result.push_back({ .id = definition.definition_id,
					   .zone_number = definition.zone_number,
					   .achievement = definition.eligible_for_zone_completion,
					   .daily_candidate = definition.daily_eligible,
					   .contracts = { definition.definition_id } });
	return result;
}

std::size_t eligible_definition_count(const catalog &catalog, int32_t zone_number,
				      uint32_t content_revision)
{
	std::size_t count = 0;
	for (const auto &unit : quest_units(catalog))
	{
		if (unit.achievement && unit.zone_number == zone_number &&
		    catalog.content_revision == content_revision)
			++count;
	}
	return count;
}
} // namespace zone_story_quest_catalog
