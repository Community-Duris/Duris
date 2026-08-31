#!/usr/bin/env python3
"""ASan/UBSan coverage for the dynamically sized event-name registry."""

from _paths import SRC
import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "world/event_names.h"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

static void require(bool condition, int code)
{
	if (!condition)
		std::exit(code);
}

static void count_entry(const void *, const char *, void *context)
{
	(*static_cast<size_t *>(context))++;
}

static std::filesystem::path write_symbols(const std::filesystem::path &directory,
					   size_t count)
{
	const std::filesystem::path path = directory / ("symbols-" + std::to_string(count));
	std::ofstream output(path);
	for (size_t index = 0; index < count; index++)
	{
		output << std::hex << (0x1000U + index) << " T callback_" << std::dec << index
		       << '\n';
	}
	return path;
}

int main(int argc, char **argv)
{
	require(argc == 2, 1);
	const std::filesystem::path directory(argv[1]);
	constexpr uintptr_t base = 0x100000U;
	int test = 0;

	for (size_t count : { 0U, 5999U, 6000U, 6001U, 6220U })
	{
		const std::filesystem::path path = write_symbols(directory, count);
		const event_name_load_result result = event_name_registry_load(path.c_str(), base);
		require(result.opened && !result.read_error, 10 + test);
		require(result.symbols_loaded == count && result.duplicate_addresses == 0 &&
			result.malformed_lines == 0, 20 + test);
		require(event_name_registry_size() == count, 30 + test);
		if (count > 0)
		{
			const void *first = reinterpret_cast<void *>(base + 0x1000U);
			const void *last = reinterpret_cast<void *>(base + 0x1000U + count - 1U);
			require(std::string(event_name_registry_lookup(first)) == "callback_0", 40 + test);
			require(std::string(event_name_registry_lookup(last)) ==
					"callback_" + std::to_string(count - 1U),
				50 + test);
		}
		require(event_name_registry_lookup(reinterpret_cast<void *>(base + 0xffffU)) ==
				nullptr,
			60 + test);
		test++;
	}

	const std::filesystem::path validation_path = directory / "validation-symbols";
	{
		std::ofstream output(validation_path);
		output << "1000 T first_name\n";
		output << "1000 T ignored_alias\n";
		output << "not-hex T bad_address\n";
		output << "2000 D wrong_type\n";
		output << "3000 T too many fields\n";
		output << "4000 T " << std::string(4097, 'f') << '\n';
		output << std::hex << std::numeric_limits<uintptr_t>::max() << " T overflow\n";
	}
	const event_name_load_result validation =
		event_name_registry_load(validation_path.c_str(), base);
	require(validation.opened && !validation.read_error, 70);
	require(validation.symbols_loaded == 1 && validation.duplicate_addresses == 1 &&
			validation.malformed_lines == 5,
		71);
	require(event_name_registry_size() == 1, 72);
	size_t visited = 0;
	event_name_registry_visit(count_entry, &visited);
	require(visited == 1, 76);
	require(std::string(event_name_registry_lookup(reinterpret_cast<void *>(base + 0x1000U))) ==
			"first_name",
		73);

	const event_name_load_result missing =
		event_name_registry_load((directory / "missing").c_str(), base);
	require(!missing.opened && event_name_registry_size() == 1, 74);

	event_name_registry_clear();
	require(event_name_registry_size() == 0, 75);
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-event-names-") as directory:
    temp = Path(directory)
    harness = temp / "harness.cpp"
    binary = temp / "harness"
    harness.write_text(HARNESS, encoding="ascii")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            f"-I{SRC}",
            str(harness),
            str(SRC / "event_names.c"),
            "-o",
            str(binary),
        ],
        check=True,
    )
    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    subprocess.run([str(binary), str(temp)], check=True, env=environment)

print("event-name registry boundary tests passed under ASan/UBSan")
