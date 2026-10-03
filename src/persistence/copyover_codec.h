#ifndef COPYOVER_CODEC_H
#define COPYOVER_CODEC_H

#include "persistence/copyover.h"
#include "telemetry/telemetry_runtime.h"

#include <cstdio>
#include <string>
#include <vector>

constexpr size_t COPYOVER_WIRE_HEADER_BYTES = 64;
constexpr size_t COPYOVER_MAX_FILE_BYTES = 128 * 1024 * 1024;
constexpr size_t COPYOVER_MAX_RECORD_BYTES = 2 * 1024 * 1024;
constexpr int COPYOVER_MAX_CHILD_RECORDS = 32768;

// These are decoded values, never disk layouts.
struct telemetry_copyover_entry
{
	int fd;
	char player_name[50];
	uint8_t handoff_valid;
	uint8_t reserved[3];
	telemetry_session_handoff handoff;
};

struct copyover_decoded_mob
{
	copyover_mob entry = {};
	std::vector<copyover_affect> affects;
	std::vector<copyover_carried_item> inventory;
	std::string generated;
};

struct copyover_decoded_state
{
	copyover_header header = {};
	int listeners[3] = { -1, -1, -1 };
	std::vector<copyover_desc> descriptors;
	std::vector<telemetry_copyover_entry> telemetry;
	std::vector<copyover_decoded_mob> mobs;
	std::vector<std::vector<char>> objects;
	std::vector<copyover_room> doors;
};

bool copyover_codec_legacy_abi_compatible();
bool copyover_codec_begin(FILE *file, const copyover_header &header, const int listeners[3]);
bool copyover_codec_write(FILE *file, const copyover_desc &entry);
bool copyover_codec_write(FILE *file, const telemetry_copyover_entry &entry);
bool copyover_codec_write(FILE *file, const copyover_mob &entry);
bool copyover_codec_write(FILE *file, const copyover_affect &entry);
bool copyover_codec_write(FILE *file, const copyover_carried_item &entry);
bool copyover_codec_write(FILE *file, const copyover_room &entry);
bool copyover_codec_write_generated(FILE *file, int vnum, const std::string &generated);
bool copyover_codec_write_object(FILE *file, const char *native_data, size_t native_size);
// Finish checks framing/state, seals the whole-file CRC, then flushes and fsyncs.
bool copyover_codec_finish(FILE *file, const char **error);
// Entire file is bounded and validated before output is replaced. No gameplay calls.
bool copyover_codec_read(FILE *file, copyover_decoded_state *output, const char **error);
bool copyover_codec_sync_parent(const char *path);

#endif
