// Matching component hashes test consumer semantics, not SQL provenance.
// Original shared framing helper is included once by the original room driver.
#include "economy/collector_repository.h"
#include "collector_physical_allocation_cases.inc"
namespace collector_physical_test
{
collector::record seed_record(uint64_t listing, uint64_t uid,
			      const char *operation = "123456789abcdef0123456789abcdef0",
			      uint64_t time = 1000)
{
	collector::record result;
	collector::rules rules;
	rules.enabled = true;
	assert(collector::enroll(listing, operation, 42, uid, 5, time, rules, &result) ==
	       collector::outcome::applied);
	assert(collector::collect(&result, result.revision, result.item_revision,
				  result.item_revision, true, 75,
				  result.collect_at) == collector::outcome::applied);
	return result;
}
player_item_snapshot seed_item(uint64_t uid)
{
	player_item_snapshot result{};
	result.object_uid = uid;
	result.vnum = 1200;
	result.parent_index = -1;
	// The collector's native codec permits prototype-reference NULL strings.
	result.string_mask = 0;
	result.type = 1;
	return result;
}
std::string death_bytes(const char *operation = "123456789abcdef0123456789abcdef0")
{
	critical_operation_id value{};
	assert(critical_operation_id_from_hex(operation, &value));
	return std::string(reinterpret_cast<const char *>(value.bytes.data()), value.bytes.size());
}
void append_record(economic_sql_physical_source_snapshot &value, const collector::record &entry,
		   const std::optional<std::string> &literal)
{
	std::array<uint8_t, collector::encoded_record_bytes> record{};
	assert(collector::record_encode(entry, &record) == collector::codec_result::ok);
	uint64_t due = entry.status == collector::state::candidate ? entry.collect_at :
		       entry.status == collector::state::collected ? entry.sale_at :
		       entry.status == collector::state::available && !entry.holding_paused ?
								     entry.expires_at :
								     0;
	component_row(value.source2.tables[10],
		      { std::to_string(entry.listing), death_bytes(entry.death_operation.data()),
			std::to_string(entry.beneficiary), std::to_string(entry.uid),
			std::to_string(static_cast<unsigned>(entry.status)),
			entry.holding_paused ? "1" : "0",
			due ? std::optional<std::string>(std::to_string(due)) : std::nullopt,
			std::to_string(entry.revision), std::to_string(entry.item_revision),
			std::to_string(entry.price_value),
			std::string(reinterpret_cast<const char *>(record.data()), record.size()),
			literal });
}
economic_sql_physical_source_snapshot seed_base()
{
	economic_sql_physical_source_snapshot value;
#include "component_registry.inc"
	const auto record = seed_record(1, 501);
	std::vector<uint8_t> blob;
	assert(player_item_snapshot_list_encode({ seed_item(501) }, &blob) ==
	       player_snapshot_codec_result::ok);
	component_row(value.source2.tables[8], { "1", "0", "2" });
	component_row(value.source2.tables[9],
		      { death_bytes(), "42", "1000", std::to_string(record.policy.collection_delay),
			std::to_string(record.policy.sale_delay),
			std::to_string(record.policy.holding_duration),
			std::to_string(record.policy.price_percent),
			std::to_string(record.policy.minimum_value), "0", "0" });
	append_record(value, record,
		      std::string(reinterpret_cast<const char *>(blob.data()), blob.size()));
	component_row(value.source2.tables[11],
		      { "501", "501", std::nullopt, "10", "1", "0",
			std::to_string(record.item_revision), "1200", "1", std::nullopt });
	component_row(value.source2.item_equipment_sources[0], { "501", "0" });
	component_row(value.source2.tables[12], { "10", "1", "0", "1" });
	component_row(value.source2.tables[13], { "1", "1000" });
	reframe_physical_component(value);
	assert(!economic_sql_validate_physical_sources(value));
	return value;
}
collector_physical_source_report inspect(economic_sql_physical_source_snapshot value,
					 size_t diagnostics = 512)
{
	reframe_physical_component(value);
	collector_physical_source_report output;
	assert(!collector_repository_inspect_physical_sources(value, &output, {}, diagnostics));
	assert(output.physical_digest == value.digest && output.rows == value.rows &&
	       output.cells == value.cells && output.cell_bytes == value.cell_bytes);
	return output;
}
void cases()
{
	const auto base = seed_base();
	const auto good = inspect(base);
	assert(good.correspondence_valid && good.catalog_valid && good.listings.size() == 1 &&
	       good.custody.size() == 1);
	assert(good.listings[0].held && good.listings[0].literal &&
	       good.listings[0].literal->string_mask == 0 &&
	       !good.listings[0].retained_command_proof_known);
	// Establish the original source-normalizer missing requirement: changing only
	// the held literal creates no old correspondence finding. The new inspector
	// must report it while custody and scalar metadata remain identical.
	{
		auto damaged = base;
		damaged.source2.tables[10].rows[0].cells[11] = std::string(4, '\0');
		reframe_physical_component(damaged);
		economic_sql_normalized_sources before, after;
		assert(economic_sql_normalize_sources(base.source2, 512, &before) ==
		       economic_accounting_error::ok);
		assert(economic_sql_normalize_sources(damaged.source2, 512, &after) ==
		       economic_accounting_error::ok);
		assert(before.issue_counts == after.issue_counts);
		const auto result = inspect(damaged);
		assert(!result.correspondence_valid &&
		       (result.listings[0].findings & COLLECTOR_SOURCE_MALFORMED_LITERAL));
	}
	for (int damage = 0; damage < 13; ++damage)
	{
		auto value = base;
		if (damage == 0)
			value.source2.tables[10].rows[0].cells[11] = std::nullopt;
		if (damage == 1)
			value.source2.tables[10].rows[0].cells[10] = std::string(154, '\0');
		if (damage == 2)
			value.source2.tables[10].rows[0].cells[9] = "999";
		if (damage == 3)
			value.source2.tables[9].rows[0].cells[2] = "1001";
		if (damage == 4)
			value.source2.tables[8].rows[0].cells[2] = "1";
		if (damage == 5)
		{
			value.source2.tables[11].rows.clear();
			value.source2.item_equipment_sources[0].rows.clear();
		}
		if (damage == 6)
			value.source2.tables[11].rows[0].cells[4] = "2";
		if (damage == 7)
			value.source2.tables[11].rows[0].cells[2] = "0";
		if (damage == 8)
			value.source2.item_equipment_sources[0].rows[0].cells[1] = "7";
		if (damage == 9)
			value.source2.tables[12].rows[0].cells[3] = "0";
		if (damage == 10)
			value.source2.tables[9].rows.clear();
		if (damage == 11)
			value.source2.tables[9].rows.push_back(value.source2.tables[9].rows[0]);
		if (damage == 12)
			value.source2.tables[10].rows.push_back(value.source2.tables[10].rows[0]);
		const auto result = inspect(value);
		assert(!result.correspondence_valid && !result.diagnostics.empty());
	}
	// Different old/new listings may legitimately retain the same UID.
	{
		auto value = base;
		auto old = seed_record(1, 501);
		assert(collector::activate(&old, old.revision, old.sale_at) ==
		       collector::outcome::applied);
		assert(collector::purchase(&old, old.revision, 43, UINT64_MAX, true,
					   old.available_at) == collector::outcome::applied);
		const auto literal = value.source2.tables[10].rows[0].cells[11];
		value.source2.tables[10].rows.clear();
		append_record(value, old, literal);
		const auto next = seed_record(2, 501, "223456789abcdef0123456789abcdef0", 2000);
		append_record(value, next, literal);
		value.source2.tables[8].rows[0].cells[2] = "3";
		auto death = value.source2.tables[9].rows[0];
		death.cells[0] = death_bytes(next.death_operation.data());
		death.cells[2] = "2000";
		value.source2.tables[9].rows.push_back(death);
		value.source2.tables[11].rows[0].cells[4] = "2";
		value.source2.tables[12].rows[0].cells[1] = "2";
		const auto result = inspect(value);
		assert(result.correspondence_valid && !result.listings[0].held &&
		       result.listings[1].held && result.custody.size() == 1);
		value.source2.tables[11].rows[0].cells[4] = "1";
		assert(!inspect(value).correspondence_valid); // old listing still holds item
	}
	{
		auto value = base;
		collector::record pending;
		collector::rules rules;
		rules.enabled = true;
		assert(collector::enroll(1, "123456789abcdef0123456789abcdef0", 42, 501, 5, 1000,
					 rules, &pending) == collector::outcome::applied);
		value.source2.tables[10].rows.clear();
		append_record(value, pending, std::nullopt);
		value.source2.tables[11].rows[0].cells[3] = "4";
		assert(inspect(value).correspondence_valid);
	}
	for (const char *uid : { "502", "malformed" })
	{
		auto value = base;
		auto extra = value.source2.tables[11].rows[0];
		extra.cells[0] = uid;
		extra.cells[1] = "501";
		extra.cells[2] = "501";
		value.source2.tables[11].rows.push_back(extra);
		component_row(value.source2.item_equipment_sources[0], { uid, "0" });
		const auto result = inspect(value);
		assert(!result.correspondence_valid && result.custody.size() == 2 &&
		       result.custody.back().custody_row == 1);
	}
	// Native singleton closure includes foreign/malformed descendants, even
	// though those rows are not additional collector holding.
	for (const char *uid : { "502", "malformed" })
	{
		auto value = base;
		auto extra = value.source2.tables[11].rows[0];
		extra.cells[0] = uid;
		extra.cells[1] = "501";
		extra.cells[2] = "501";
		extra.cells[3] = "1";
		value.source2.tables[11].rows.push_back(extra);
		component_row(value.source2.item_equipment_sources[0], { uid, "0" });
		const auto result = inspect(value);
		assert(!result.correspondence_valid && result.custody.size() == 2 &&
		       !result.custody.back().collector_owned &&
		       result.custody.back().custody_row == 1);
	}
	{
		auto value = base;
		value.source2.tables[11].rows[0].cells[1] = "999";
		auto extra = value.source2.tables[11].rows[0];
		extra.cells[0] = "502";
		extra.cells[1] = "501";
		extra.cells[2] = "501";
		extra.cells[3] = "malformed";
		value.source2.tables[11].rows.push_back(extra);
		component_row(value.source2.item_equipment_sources[0], { "502", "0" });
		const auto result = inspect(value);
		assert(!result.correspondence_valid && result.custody.size() == 2 &&
		       !result.custody.back().collector_owned &&
		       result.custody.back().custody_row == 1);
	}
	{
		auto value = base;
		append_record(value, seed_record(2, 501),
			      value.source2.tables[10].rows[0].cells[11]);
		value.source2.tables[8].rows[0].cells[2] = "3";
		assert(!inspect(value).correspondence_valid); // duplicate death+UID, distinct listing
	}
	{
		auto value = base;
		value.source2.tables[10].rows[0].cells[11] = std::string(131073, 'x');
		assert(inspect(value).listings[0].findings & COLLECTOR_SOURCE_MALFORMED_LITERAL);
	}
#ifdef DURIS_COLLECTOR_PHYSICAL_ALLOCATION_TEST
	{
		collector_physical_source_report result;
		collector_allocations = 0;
		collector_allocation_instrument = true;
		const auto code = collector_repository_inspect_physical_sources(base, &result);
		collector_allocation_instrument = false;
		assert(!code);
		const auto allocations = collector_allocations;
		assert(allocations);
		for (size_t fault = 1; fault <= allocations;
		     fault += std::max(size_t{ 1 }, allocations / 100))
		{
			collector_physical_source_report output;
			output.next_listing = 987;
			collector_allocations = 0;
			collector_allocation_fault = fault;
			collector_allocation_instrument = true;
			const auto failure =
				collector_repository_inspect_physical_sources(base, &output);
			collector_allocation_instrument = false;
			collector_allocation_fault = 0;
			assert(failure == ENOMEM && output.next_listing == 987 &&
			       output.listings.empty() && output.custody.empty());
		}
	}
#endif
	collector_physical_source_report sentinel;
	sentinel.next_listing = 987;
	auto invalid = base;
	invalid.digest[0] ^= 1;
	assert(collector_repository_inspect_physical_sources(invalid, &sentinel) &&
	       sentinel.next_listing == 987 && sentinel.listings.empty());
	for (size_t bound : { size_t{ 0 }, size_t{ 513 } })
		assert(collector_repository_inspect_physical_sources(base, &sentinel, {}, bound) ==
			       EINVAL &&
		       sentinel.next_listing == 987);
	{
		auto value = base;
		value.source2.tables[10].rows[0].cells[11] = std::nullopt;
		value.source2.tables[9].rows.clear();
		const auto report = inspect(value, 1);
		assert(report.diagnostics.size() == 1 && report.diagnostics_truncated);
	}
}
}
static void collector_physical_component_cases()
{
	collector_physical_test::cases();
}
