#include "core/config.h"
#include "core/structs.h"
#include "sql/sql.h"
#include "sql/zone_story_quest_state_repository.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

// DB and debugcount are defined by the complete real sql.c/utility.c providers.
extern unsigned int debugcount;

namespace
{
using result = sql_zone_story_quest_state_result;
namespace fs = std::filesystem;
MYSQL connection = {};
fs::path evidence;
unsigned escapes = 0;
unsigned traces = 0;
unsigned expansion = 1;
bool escape_refused = false;
bool trace_accepts = true;
size_t input_prefix_bytes = 0;
size_t escaped_bytes = 0;
std::string traced;

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

std::string log_bytes()
{
	if (!fs::exists(LOG_DEBUG))
		return {};
	std::ifstream input(LOG_DEBUG, std::ios::binary);
	require(input.good(), "cannot read genuine debug log");
	return std::string((std::istreambuf_iterator<char>(input)), {});
}

// Independent complete-query oracle: string concatenation, not qry/vsnprintf.
std::string expected_query(uint32_t revision, const std::string &escaped)
{
	return std::string("INSERT INTO zone_story_quest_state ") +
	       "(state_id,state_version,catalog_revision,state_blob,updated_at) " + "VALUES (1,1," +
	       std::to_string(revision) + ",'" + escaped +
	       "',UTC_TIMESTAMP(6)) ON DUPLICATE KEY UPDATE " +
	       "state_version=VALUES(state_version), catalog_revision=VALUES(catalog_revision)," +
	       "state_blob=VALUES(state_blob),updated_at=VALUES(updated_at)";
}

void reset()
{
	DB = &connection; // Non-null opaque token; never passed to a MySQL library.
	escapes = traces = 0;
	expansion = 1;
	escape_refused = false;
	trace_accepts = true;
	input_prefix_bytes = escaped_bytes = 0;
	traced.clear();
}

void save_case(const std::string &name, uint32_t revision, const std::string &state,
	       const std::string &expected_escaped, result expected_result,
	       const std::string &expected_error, unsigned expected_escapes,
	       unsigned expected_traces, const std::string &expected_log = {})
{
	const auto query = expected_query(revision, expected_escaped);
	const auto before_log = log_bytes();
	std::string error;
	const auto actual = sql_zone_story_quest_state_save(revision, state, &error);
	require(actual == expected_result && error == expected_error,
		name + ": repository result/error mismatch: " + error);
	require(escapes == expected_escapes && traces == expected_traces,
		name + ": wrong escape/trace call counts");
	require(traced == (expected_traces ? query : std::string{}),
		name + ": actual complete trace query differs from independent oracle");
	const auto after_log = log_bytes();
	require(after_log.starts_with(before_log), name + ": logger rewrote prior observations");
	const auto added_log = after_log.substr(before_log.size());
	if (expected_log.empty())
		require(added_log.empty(), name + ": unexpected formatter log");
	else
	{
		require(added_log.ends_with("::" + expected_log + "\n") &&
				std::count(added_log.begin(), added_log.end(), '\n') == 1,
			name + ": genuine formatter log mismatch");
	}
	if (query.size() <= 2U * MAX_STRING_LENGTH)
		write(name + ".expected.sql", query);
	write(name + ".trace.sql", traced);
	write(name + ".log.txt", added_log);
	std::ostringstream observation;
	observation << "state_bytes=" << state.size()
		    << "\nescaped_input_prefix_bytes=" << input_prefix_bytes
		    << "\nescaped_output_bytes=" << escaped_bytes
		    << "\nexpected_complete_query_bytes=" << query.size()
		    << "\nactual_trace_query_bytes=" << traced.size()
		    << "\nescape_calls=" << escapes << "\ntrace_calls=" << traces
		    << "\nrepository_result=" << static_cast<int>(actual) << "\nerror=" << error
		    << '\n';
	write(name + ".observation.txt", observation.str());
	std::cout << "PASS " << name << " complete_query_bytes=" << query.size()
		  << " traces=" << traces << '\n';
}
} // namespace

// Explicit SQL doubles. This is neither MySQL escaping nor SQL execution.
char *sql_escape_string(const char *value)
{
	++escapes;
	if (!DB || !value || escape_refused)
		return nullptr;
	input_prefix_bytes = std::strlen(value);
	std::string output;
	output.reserve(input_prefix_bytes * expansion);
	for (size_t index = 0; index < input_prefix_bytes; ++index)
	{
		if (value[index] < 'A' || value[index] > 'Z')
			require(false,
				"SQL double accepts only safe uppercase ASCII component values");
		output.append(expansion, value[index]);
	}
	escaped_bytes = output.size();
	char *allocated = static_cast<char *>(std::malloc(output.size() + 1));
	require(allocated != nullptr, "escape double allocation failed");
	std::memcpy(allocated, output.c_str(), output.size() + 1);
	return allocated; // The actual repository owns and frees this allocation.
}

bool sql_trace_exec_at(persistence_query_site site, const char *label, const char *sql,
		       size_t length, bool drain_before, bool drain_after)
{
	++traces;
	require(DB == &connection && sql && label && std::strcmp(label, "qry/direct") == 0 &&
			length == std::strlen(sql) && drain_before && !drain_after,
		"actual qry_at trace arguments differ");
	require(site.file && site.function &&
			std::string(site.file) == "src/sql/zone_story_quest_state_repository.c" &&
			std::string(site.function) == "sql_zone_story_quest_state_save" &&
			site.line == 123,
		"repository query-site propagation differs");
	traced.assign(sql, length);
	return trace_accepts;
}

int main(int argc, char **argv)
{
	require(argc == 2, "usage: component EVIDENCE_DIRECTORY");
	evidence = argv[1];
	require(MAX_STRING_LENGTH == 65536, "pinned actual format buffer changed");
	debugcount = 1; // Actual logit must append rather than rewind its debug file.
	const size_t overhead = expected_query(7, "").size();
	const size_t fitting = MAX_STRING_LENGTH - 1 - overhead;
	require(expected_query(7, std::string(fitting, 'A')).size() == 65535 &&
			expected_query(7, std::string(fitting + 1, 'A')).size() == 65536,
		"complete-query boundary oracle is wrong");
	write("capacity.txt",
	      "buffer_bytes=65536\nrevision7_overhead_bytes=" + std::to_string(overhead) +
		      "\nrevision7_ascii_fitting_bytes=" + std::to_string(fitting) + "\n");
	const std::string write_error = "zone-story SQL state write failed";
	const std::string overflow_log = "MySQL error: Query too long or formatting error";

	reset();
	save_case("ascii-largest-fit", 7, std::string(fitting, 'A'), std::string(fitting, 'A'),
		  result::ok, "", 1, 1);
	reset();
	save_case("ascii-first-overflow", 7, std::string(fitting + 1, 'A'),
		  std::string(fitting + 1, 'A'), result::io_error, write_error, 1, 0, overflow_log);
	reset();
	trace_accepts = false;
	save_case("trace-refusal", 7, std::string(fitting, 'A'), std::string(fitting, 'A'),
		  result::io_error, write_error, 1, 1);

	// Same state length, larger revision spelling: complete query must overflow.
	reset();
	save_case("revision-width-overflow", std::numeric_limits<uint32_t>::max(),
		  std::string(fitting, 'A'), std::string(fitting, 'A'), result::io_error,
		  write_error, 1, 0, overflow_log);

	const size_t growth_fit = fitting / 2;
	reset();
	expansion = 2;
	save_case("controlled-growth-fit", 7, std::string(growth_fit, 'A'),
		  std::string(growth_fit * 2, 'A'), result::ok, "", 1, 1);
	reset();
	expansion = 2;
	save_case("controlled-growth-overflow", 7, std::string(growth_fit + 1, 'A'),
		  std::string((growth_fit + 1) * 2, 'A'), result::io_error, write_error, 1, 0,
		  overflow_log);

	reset();
	DB = nullptr;
	save_case("repository-no-DB", 7, "ASCII", "", result::io_error,
		  "zone-story SQL state could not be escaped", 1, 0);
	const auto before_log = log_bytes();
	require(!qry("SELECT 1") && traces == 0 && escapes == 1,
		"actual formatter no-DB guard failed");
	const auto added_log = log_bytes().substr(before_log.size());
	require(added_log.ends_with("::MySQL error: MySQL not initialized!\n") &&
			std::count(added_log.begin(), added_log.end(), '\n') == 1,
		"actual formatter no-DB diagnostic missing");
	write("formatter-no-DB.log.txt", added_log);
	std::cout << "PASS formatter-no-DB traces=0\n";
	reset();
	escape_refused = true;
	save_case("escape-refusal", 7, "ASCII", "", result::io_error,
		  "zone-story SQL state could not be escaped", 1, 0);

	reset();
	const std::string at_guard(16U * 1024U * 1024U, 'A');
	save_case("at-16MiB-preguard", 7, at_guard, at_guard, result::io_error, write_error, 1, 0,
		  overflow_log);
	reset();
	save_case("over-16MiB-preguard", 7, std::string(at_guard.size() + 1, 'A'), "",
		  result::invalid, "zone-story SQL state is oversized", 0, 0);

	// Characterize only this explicit C-string double, not binary-safe MySQL I/O.
	reset();
	save_case("controlled-embedded-NUL", 7, std::string("A\0B", 3), "A", result::ok, "", 1, 1);
	require(input_prefix_bytes == 1 && escaped_bytes == 1,
		"controlled C-string observation did not retain its explicit scope");
	std::cout << "PASS 12 SQL-capacity component observations; no database contacted\n";
}
