#include "core/config.h"
#include "core/structs.h"
#include "persistence/persistence_mode.h"
#include "sql/sql.h"
#include "world/zone_story_quest_production.h"
#include "world/zone_story_quest_runtime.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

P_index mob_index;
int number_of_quests = 0;
quest_data quest_index[1];
zone_data *zone_table = nullptr;
int top_of_zone_table = -1;
extern unsigned int debugcount; // Actual utility.c logger state.

namespace
{
namespace fs = std::filesystem;
using event = zone_story_quest_feature::completion_event;
fs::path evidence;
MYSQL connection = {};
MYSQL_RES *live_result = nullptr;
unsigned reads = 0, rows = 0, length_reads = 0, frees = 0, traces = 0, escapes = 0;
std::string saved_image, attempted, escaped_attempt, last_trace, definition_id;
std::string version = "1", revision = "1";
char *columns[3] = {};
unsigned long lengths[3] = {};

void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << "FAIL: " << message << '\n';
		std::exit(1);
	}
}

void write(const std::string &name, const std::string &bytes)
{
	std::ofstream output(evidence / name, std::ios::binary);
	output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
	output.close();
	require(!output.fail(), "evidence write failed: " + name);
}

std::string hex(const std::string &bytes)
{
	constexpr char digits[] = "0123456789abcdef";
	std::string result;
	for (unsigned char byte : bytes)
	{
		result += digits[byte >> 4];
		result += digits[byte & 15];
	}
	return result;
}

// Explicit ASCII SQL escape MODEL, not MySQL/charset execution. Real service
// output contains LF; quotes/backslashes are also defined rather than ignored.
std::string escape_model(const std::string &raw)
{
	std::string result;
	for (unsigned char byte : raw)
	{
		switch (byte)
		{
		case '\n':
			result += "\\n";
			break;
		case '\'':
			result += "\\'";
			break;
		case '\\':
			result += "\\\\";
			break;
		default:
			if (byte < 32 || byte > 126)
				require(false,
					"generated history exceeds declared ASCII escape model");
			result += static_cast<char>(byte);
		}
	}
	return result;
}

std::string complete_query(const std::string &escaped)
{
	return std::string("INSERT INTO zone_story_quest_state ") +
	       "(state_id,state_version,catalog_revision,state_blob,updated_at) " +
	       "VALUES (1,1,1,'" + escaped + "',UTC_TIMESTAMP(6)) ON DUPLICATE KEY UPDATE " +
	       "state_version=VALUES(state_version), catalog_revision=VALUES(catalog_revision)," +
	       "state_blob=VALUES(state_blob),updated_at=VALUES(updated_at)";
}

event completion(unsigned index)
{
	event result;
	result.transaction.schema_version = 1;
	result.transaction.transaction_id = "history-" + std::to_string(index);
	result.transaction.quest_definition_id = definition_id;
	result.transaction.zone_number = 1;
	result.transaction.direct_completer_pid = 701 + index * 2;
	result.transaction.credited_pids = { 701 + index * 2, 702 + index * 2 };
	result.transaction.room_vnum = 101;
	result.transaction.completed_at = 1791400000 + index;
	result.transaction.season_id = 101;
	result.transaction.content_revision = 1;
	result.character_name = "Component" + std::to_string(index);
	result.level = 20;
	result.racewar = 1;
	result.party_context_known = true;
	result.party_size = 2;
	result.strongest_party_level = 25;
	return result;
}

std::string memory()
{
	require(zone_story_quest_runtime::ready(), "actual runtime not ready");
	std::string error;
	const auto state = zone_story_quest_runtime::service()->serialize_state(&error);
	require(!state.empty() && error.empty(), "actual runtime serialization failed: " + error);
	return state;
}

void validate(const std::string &state)
{
	zone_story_quest_feature::service check(zone_story_quest_production::runtime_catalog());
	std::string error;
	require(check.deserialize_state(state, &error) && check.serialize_state(&error) == state,
		"real parser/serializer did not round-trip generated history: " + error);
}

void document(const std::string &label, const std::string &state, unsigned events)
{
	validate(state);
	std::map<char, unsigned> row_counts;
	std::istringstream input(state);
	std::string line;
	while (std::getline(input, line))
		if (!line.empty())
			++row_counts[line[0]];
	require(row_counts['T'] == events && row_counts['N'] == events &&
			row_counts['C'] == events * 2 && row_counts['E'] == events &&
			row_counts['Z'] == 1 && row_counts.size() == 5,
		"generated document has unexpected fact/metadata/credit/telemetry rows");
	const auto escaped = escape_model(state);
	const auto query = complete_query(escaped);
	write(label + ".state.txt", state);
	write(label + ".escaped.txt", escaped);
	write(label + ".expected.sql", query);
	std::ostringstream info;
	info << "events=" << events << "\nraw_bytes=" << state.size()
	     << "\nLFs=" << std::count(state.begin(), state.end(), '\n')
	     << "\nquotes=" << std::count(state.begin(), state.end(), '\'')
	     << "\nbackslashes=" << std::count(state.begin(), state.end(), '\\')
	     << "\nescaped_bytes=" << escaped.size() << "\ncomplete_query_bytes=" << query.size()
	     << "\nT=" << row_counts['T'] << "\nN=" << row_counts['N'] << "\nC=" << row_counts['C']
	     << "\nE=" << row_counts['E'] << '\n';
	std::map<unsigned char, unsigned> alphabet;
	for (unsigned char byte : state)
		++alphabet[byte];
	for (const auto &[byte, count] : alphabet)
		info << "byte_" << static_cast<unsigned>(byte) << '=' << count << '\n';
	write(label + ".metrics.txt", info.str());
}

void no_ghost(const event &failed, const std::string &before)
{
	const auto state = memory();
	require(state == before, "rollback did not restore exact whole prior service document");
	const auto &transaction = failed.transaction;
	require(state.find("T|" + hex(transaction.transaction_id) + "|") == std::string::npos &&
			state.find("E|" + hex("completion:" + transaction.transaction_id) + "|") ==
				std::string::npos,
		"failed transaction or telemetry leaked into restored memory");
	for (uint32_t pid : transaction.credited_pids)
	{
		const auto prefix = "|101|" + std::to_string(pid) + "|";
		require(state.find("N" + prefix) == std::string::npos &&
				state.find("C" + prefix) == std::string::npos,
			"failed recipient metadata/credit leaked into restored memory");
		const auto summary =
			zone_story_quest_runtime::service()->summary_for(101, pid, "Absent");
		require(summary.completed == 0 && summary.character_name == "Absent",
			"failed recipient appears in actual summary");
	}
}

bool runtime_record(const event &value, std::string *error)
{
	const auto &tx = value.transaction;
	return zone_story_quest_runtime::record_authoritative_completion(
		tx.quest_definition_id, tx.zone_number, tx.direct_completer_pid, tx.credited_pids,
		tx.room_vnum, tx.completed_at, value.character_name, value.level, value.racewar,
		value.party_context_known, value.party_size, value.strongest_party_level, error,
		tx.transaction_id);
}
} // namespace

// SQL-only controlled boundary. The actual repository load parses these row
// fields/lengths and the actual runtime deserializes the returned valid history.
MYSQL_RES *db_query_at(persistence_query_site site, const char *format, ...)
{
	const std::string expected =
		"SELECT state_version,catalog_revision,state_blob FROM zone_story_quest_state WHERE state_id=1 LIMIT 1";
	require(DB == &connection && !live_result && format == expected && site.file &&
			std::string(site.file) == "src/sql/zone_story_quest_state_repository.c" &&
			site.function && std::string(site.function) == "load_state" &&
			site.line == 36,
		"actual repository load did not reach the declared query boundary");
	++reads;
	write("load.query.sql", format);
	live_result = static_cast<MYSQL_RES *>(std::malloc(sizeof(MYSQL_RES)));
	require(live_result != nullptr, "controlled result allocation failed");
	return live_result;
}

extern "C" MYSQL_ROW mysql_fetch_row(MYSQL_RES *result)
{
	require(result && result == live_result, "unexpected SQL row handle");
	++rows;
	columns[0] = version.data();
	columns[1] = revision.data();
	columns[2] = saved_image.data();
	return columns;
}

extern "C" unsigned long *mysql_fetch_lengths(MYSQL_RES *result)
{
	require(result && result == live_result, "unexpected SQL lengths handle");
	++length_reads;
	lengths[0] = version.size();
	lengths[1] = revision.size();
	lengths[2] = saved_image.size();
	return lengths;
}

extern "C" void mysql_free_result(MYSQL_RES *result)
{
	require(result && result == live_result, "unexpected SQL result free");
	++frees;
	std::free(result);
	live_result = nullptr;
}

char *sql_escape_string(const char *raw)
{
	require(DB == &connection && raw, "invalid controlled escape context");
	++escapes;
	attempted = raw;
	escaped_attempt = escape_model(attempted);
	char *output = static_cast<char *>(std::malloc(escaped_attempt.size() + 1));
	require(output != nullptr, "controlled escape allocation failed");
	std::memcpy(output, escaped_attempt.c_str(), escaped_attempt.size() + 1);
	return output; // Actual repository unique_ptr frees this allocation.
}

bool sql_trace_exec_at(persistence_query_site site, const char *label, const char *sql,
		       size_t length, bool drain_before, bool drain_after)
{
	require(DB == &connection && site.file && site.function &&
			std::string(site.file) == "src/sql/zone_story_quest_state_repository.c" &&
			std::string(site.function) == "sql_zone_story_quest_state_save" &&
			site.line == 123 && label && std::strcmp(label, "qry/direct") == 0 && sql &&
			length == std::strlen(sql) && drain_before && !drain_after,
		"actual formatter trace arguments differ");
	last_trace.assign(sql, length);
	require(last_trace == complete_query(escaped_attempt) && length < MAX_STRING_LENGTH,
		"controlled trace did not receive the exact fitting complete query");
	++traces;
	saved_image = attempted; // Explicit controlled image; not a DB/commit receipt.
	write("trace-" + std::to_string(traces) + ".sql", last_trace);
	return true;
}

int main(int argc, char **argv)
{
	require(argc == 2, "usage: component EVIDENCE_DIRECTORY");
	evidence = argv[1];
	DB = &connection;
	debugcount = 1;
	require(setenv("PERSISTENCE_MODE", "mariadb-primary", 1) == 0 &&
			setenv("ZONE_STORY_SEASON_ID", "101", 1) == 0 &&
			unsetenv("ZONE_STORY_DAILY_ENABLED") == 0,
		"component environment setup failed");
	char mode_error[512] = {};
	require(persistence_mode_configure(mode_error, sizeof(mode_error)) &&
			persistence_mode_get() == PERSISTENCE_MODE_MARIADB_PRIMARY &&
			persistence_mode_flatfile_root() == nullptr && sql_season_epoch() == 0 &&
			zone_story_quest_runtime::current_season_id() == 101 &&
			MAX_STRING_LENGTH == 65536,
		"real mode/epoch/config providers differ from pinned component assumptions");
	index_data mobs[1] = {};
	mob_index = mobs;
	mobs[0].virtual_number = 17;
	char mob_name[] = "the archivist";
	mobs[0].desc2 = mob_name;
	zone_data zones[1] = {};
	char zone_name[] = "The First Heavens";
	zones[0].number = 1;
	zones[0].name = zone_name;
	zone_table = zones;
	top_of_zone_table = 0;
	goal_data give{ .goal_type = QUEST_GOAL_ITEM, .number = 24402, .next = nullptr };
	goal_data receive{ .goal_type = QUEST_GOAL_ITEM, .number = 24403, .next = nullptr };
	quest_complete_data q{ .message = nullptr,
			       .receive = &receive,
			       .give = &give,
			       .disappear = false,
			       .disappear_message = nullptr,
			       .echoAll = false,
			       .next = nullptr };
	quest_index[0] = { .quester = 0, .quest_message = nullptr, .quest_complete = &q };
	number_of_quests = 1;
	std::string error;
	require(zone_story_quest_production::bootstrap(1, &error), "production bootstrap failed");
	const auto *bound = zone_story_quest_production::definition_id_for(&q);
	require(bound && zone_story_quest_production::runtime_catalog().definitions.size() == 1,
		"actual production fixture Q was not bound");
	definition_id = *bound;
	zone_story_quest_feature::service builder(zone_story_quest_production::runtime_catalog());
	std::string before, oversized;
	event failed;
	unsigned fitting_events = 0;
	std::ostringstream event_manifest;
	event_manifest
		<< "index\tkey\tdirect_pid\tordered_pids\troom\ttime\tseason\trevision\tname\n";
	for (unsigned index = 0; index < 256; ++index)
	{
		const auto value = completion(index);
		auto next = builder;
		require(next.record_completion(value, &error) ==
				zone_story_quest_feature::result::applied,
			"real typed event generation failed: " + error);
		const auto candidate = next.serialize_state(&error);
		require(!candidate.empty(), "real service generated empty history");
		const auto &tx = value.transaction;
		event_manifest << index << '\t' << tx.transaction_id << '\t'
			       << tx.direct_completer_pid << '\t' << tx.credited_pids[0] << ','
			       << tx.credited_pids[1] << '\t' << tx.room_vnum << '\t'
			       << tx.completed_at << '\t' << tx.season_id << '\t'
			       << tx.content_revision << '\t' << value.character_name << '\n';
		if (complete_query(escape_model(candidate)).size() >= MAX_STRING_LENGTH)
		{
			before = builder.serialize_state(&error);
			oversized = candidate;
			failed = value;
			fitting_events = index;
			break;
		}
		builder = std::move(next);
	}
	require(fitting_events > 0 && !oversized.empty() &&
			complete_query(escape_model(before)).size() < MAX_STRING_LENGTH,
		"bounded genuine event set did not produce a nearby valid crossing");
	write("events.tsv", event_manifest.str());
	write("definition.txt", definition_id);
	document("fitting", before, fitting_events);
	document("oversized", oversized, fitting_events + 1);
	saved_image = before;
	require(zone_story_quest_runtime::bootstrap(&error) && memory() == before && reads == 1 &&
			rows == 1 && length_reads == 1 && frees == 1 && !live_result,
		"actual repository/runtime load did not reconstruct the exact valid history");
	require(zone_story_quest_runtime::persist(&error) && traces == 1 && escapes == 1 &&
			saved_image == before && last_trace == complete_query(escape_model(before)),
		"healthy fitting runtime persist did not reach exact trace");
	write("healthy.saved.txt", saved_image);
	for (unsigned attempt = 1; attempt <= 2; ++attempt)
	{
		error.clear();
		require(!runtime_record(failed, &error) &&
				error == "zone-story SQL state write failed",
			"oversized actual runtime completion did not report repository write failure");
		require(traces == 1 && escapes == attempt + 1 && attempted == oversized &&
				escaped_attempt == escape_model(oversized) && saved_image == before,
			"failed runtime save reached trace or changed controlled image/attempted history");
		no_ghost(failed, before);
		const auto label = "failed-" + std::to_string(attempt);
		write(label + ".attempted.txt", attempted);
		write(label + ".restored.txt", memory());
		write(label + ".saved.txt", saved_image);
		write(label + ".error.txt", error);
	}
	error.clear();
	require(zone_story_quest_runtime::persist(&error) && traces == 2 && escapes == 4 &&
			memory() == before && saved_image == before &&
			last_trace == complete_query(escape_model(before)),
		"fitting persist after repeated refusal did not remain healthy/exact");
	write("final.memory.txt", memory());
	write("final.saved.txt", saved_image);
	std::ostringstream counters;
	counters << "fitting_events=" << fitting_events << "\nreads=" << reads << "\nrows=" << rows
		 << "\nlength_reads=" << length_reads << "\nfrees=" << frees
		 << "\nescapes=" << escapes << "\ntraces=" << traces << '\n';
	write("counters.txt", counters.str());
	std::cout << "PASS valid-history runtime SQL rollback; fitting_events=" << fitting_events
		  << " before_query=" << complete_query(escape_model(before)).size()
		  << " after_query=" << complete_query(escape_model(oversized)).size()
		  << "; identical repeated refusal; no ghost T/N/C/E; fitting trace controls=2\n";
}
