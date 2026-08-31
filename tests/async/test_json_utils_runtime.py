#!/usr/bin/env python3
"""Runtime regressions for JSON command parsing and untrusted text escaping."""

from _paths import SRC
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "json_utils.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static void require(bool condition, int code)
{
	if (!condition)
		std::exit(code);
}

static void require_text(char *actual, const char *expected, int code)
{
	require(actual != nullptr, code);
	require(std::strcmp(actual, expected) == 0, code);
	json_free_string(actual);
}

int main()
{
	int tests = 0;

	// 1. Null input has a stable allocated empty-string result.
	require_text(json_escape_string(nullptr), "", 1);
	tests++;

	// 2. JSON metacharacters and every ASCII control form are escaped.
	{
		const char input[] = { '"', '\\', '\n', '\r', '\t', 1, 0 };
		require_text(json_escape_string(input), "\\\"\\\\\\n\\r\\t\\u0001", 2);
	}
	tests++;

	// 3. Valid two-, three-, and four-byte UTF-8 passes through unchanged.
	require_text(json_escape_string("\xc2\xa2\xe2\x82\xac\xf0\x9f\x99\x82"),
		     "\xc2\xa2\xe2\x82\xac\xf0\x9f\x99\x82", 3);
	tests++;

	// 4. Stray, truncated, and overlong encodings are removed safely.
	{
		const char input[] = { 'A', char(0x80), char(0xc0), char(0xaf), char(0xe2),
				       char(0x82), 'B', 0 };
		require_text(json_escape_string(input), "AB", 4);
	}
	tests++;

	// 5. UTF-16 surrogate encodings are never copied into JSON.
	{
		const char input[] = { 'A', char(0xed), char(0xa0), char(0x80), 'B', 0 };
		require_text(json_escape_string(input), "AB", 5);
	}
	tests++;

	// 6. Encodings above U+10FFFF are never copied into JSON.
	{
		const char input[] = { 'A', char(0xf4), char(0x90), char(0x80), char(0x80),
				       char(0xf5), char(0x80), char(0x80), char(0x80), 'B', 0 };
		require_text(json_escape_string(input), "AB", 6);
	}
	tests++;

	// 7. Complete, recognized Duris color sequences are stripped before escaping.
	require_text(json_escape_ansi_string("&+rRed &-bBack &=gWBoth&n!"),
		     "Red Back Both!", 7);
	tests++;

	// 8. Truncated and unknown ampersand markup remains literal and bounded.
	for (const char *input : { "&", "&+", "&-", "&=", "&=r", "&+?", "&x" })
		require_text(json_escape_ansi_string(input), input, 8);
	tests++;

	// 9. Command parsing requires exactly one top-level JSON object.
	{
		cJSON *valid = json_parse_command("  {\"type\":\"cmd\"}  ");
		require(valid != nullptr, 9);
		cJSON_Delete(valid);
		require(json_parse_command(nullptr) == nullptr, 10);
		require(json_parse_command("") == nullptr, 11);
		require(json_parse_command("[]") == nullptr, 12);
		require(json_parse_command("{} trailing") == nullptr, 13);
	}
	tests++;

	// 10. Typed getters reject absent/wrongly typed values and null keys.
	{
		cJSON *json = json_parse_command(
			"{\"type\":\"cmd\",\"cmd\":\"game\",\"data\":{\"name\":\"Ada\",\"n\":7,\"bad\":\"7\"}}");
		require(json != nullptr, 14);
		cJSON *data = json_get_data(json);
		require(std::strcmp(json_get_cmd_type(json), "cmd") == 0, 15);
		require(std::strcmp(json_get_cmd_name(json), "game") == 0, 16);
		require(std::strcmp(json_get_string(data, "name"), "Ada") == 0, 17);
		require(json_get_string(data, "n") == nullptr, 18);
		require(json_get_string(data, nullptr) == nullptr, 19);
		require(json_get_int(data, "n", -1) == 7, 20);
		require(json_get_int(data, "bad", -1) == -1, 21);
		require(json_get_int(data, nullptr, -1) == -1, 22);
		cJSON_Delete(json);
	}
	tests++;

	// 11. A string data node can be retrieved directly without a key.
	{
		cJSON *value = cJSON_CreateString("game");
		require(std::strcmp(json_get_string(value, nullptr), "game") == 0, 23);
		cJSON_Delete(value);
	}
	tests++;

	// 12. Text builders apply defaults and produce parseable JSON.
	{
		char *encoded = json_build_text(nullptr, nullptr);
		cJSON *json = cJSON_Parse(encoded);
		require(json != nullptr, 24);
		require(std::strcmp(cJSON_GetObjectItem(json, "type")->valuestring, "text") == 0, 25);
		require(std::strcmp(cJSON_GetObjectItem(json, "category")->valuestring, "info") == 0,
			26);
		require(std::strcmp(cJSON_GetObjectItem(json, "data")->valuestring, "") == 0, 27);
		cJSON_Delete(json);
		json_free_string(encoded);
	}
	tests++;

	// 13. GMCP wrappers reject trailing JSON and safely accept null package/data.
	{
		char *encoded = json_build_gmcp_message(nullptr, "{\"ok\":true} trailing");
		cJSON *json = cJSON_Parse(encoded);
		require(json != nullptr, 28);
		require(std::strcmp(cJSON_GetObjectItem(json, "package")->valuestring, "") == 0, 29);
		require(cJSON_IsNull(cJSON_GetObjectItem(json, "data")), 30);
		cJSON_Delete(json);
		json_free_string(encoded);

		encoded = json_build_gmcp_message("Core.Hello", nullptr);
		json = cJSON_Parse(encoded);
		require(json != nullptr && cJSON_IsNull(cJSON_GetObjectItem(json, "data")), 31);
		cJSON_Delete(json);
		json_free_string(encoded);
	}
	tests++;

	std::printf("%d JSON utility runtime cases passed\n", tests);
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-json-utils-runtime-") as directory:
    temp = Path(directory)
    harness = temp / "harness.cpp"
    binary = temp / "harness"
    harness.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-ffunction-sections",
            "-fdata-sections",
            f"-I{SRC}",
            str(harness),
            str(SRC / "json_utils.c"),
            "-Wl,--gc-sections",
            "-lcjson",
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("JSON utility runtime regressions passed")
