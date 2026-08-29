#ifndef DURIS_FLATFILE_ARTIFACT_REPOSITORY_H
#define DURIS_FLATFILE_ARTIFACT_REPOSITORY_H

#include "flatfile_authority_transaction.h"
#include "item_transfer_command.h"
#include "player_snapshot.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

constexpr int32_t FLATFILE_ARTIFACT_NOT_IN_GAME = 1;
constexpr int32_t FLATFILE_ARTIFACT_ON_NPC = 2;
constexpr int32_t FLATFILE_ARTIFACT_ON_PLAYER = 3;
constexpr int32_t FLATFILE_ARTIFACT_ON_GROUND = 4;
constexpr int32_t FLATFILE_ARTIFACT_ON_CORPSE = 5;

struct flatfile_artifact_record
{
	int32_t vnum = 0;
	bool owned = false;
	int32_t location_type = FLATFILE_ARTIFACT_NOT_IN_GAME;
	int32_t location = 0;
	int64_t timer = 0;
	int32_t type = 0;
	int64_t last_update = 0;
	int32_t bind_owner_pid = -1;
	int64_t bind_timer = 0;
	uint64_t revision = 0;
	bool operator==(const flatfile_artifact_record &) const = default;
};

struct flatfile_artifact_transfer_mutation
{
	flatfile_authority_after_image after_image;
};

struct flatfile_artifact_war_owner
{
	int32_t pid = 0;
	int32_t total = 0;
	int32_t major = 0;
	int32_t unique = 0;
	int32_t ioun = 0;
};

struct flatfile_artifact_player_item
{
	int32_t vnum = 0;
	int32_t pid = 0;
};

struct flatfile_artifact_reconcile_result
{
	size_t cleared = 0;
	size_t updated = 0;
};

enum class flatfile_artifact_result
{
	ok,
	not_found,
	already_exists,
	unchanged,
	conflict,
	invalid,
	io_error
};

flatfile_artifact_result
flatfile_artifact_establish(const std::string &root,
			    const std::vector<flatfile_artifact_record> &records,
			    std::string *error);
flatfile_artifact_result flatfile_artifact_ensure(const std::string &root, std::string *error);
flatfile_artifact_result flatfile_artifact_list(const std::string &root,
						std::vector<flatfile_artifact_record> *records,
						std::string *error);
flatfile_artifact_result flatfile_artifact_get(const std::string &root, int32_t vnum,
					       flatfile_artifact_record *record,
					       std::string *error);
flatfile_artifact_result flatfile_artifact_erase(const std::string &root, int32_t vnum,
						 std::string *error);
flatfile_artifact_result flatfile_artifact_gameplay_update(const std::string &root, int32_t vnum,
							   bool owned, int32_t location_type,
							   int32_t location, int64_t timer,
							   int32_t type, int64_t last_update,
							   std::string *error);
flatfile_artifact_result flatfile_artifact_remove_owned(const std::string &root, int32_t vnum,
							int32_t corpse_pid, int32_t type,
							int64_t last_update, std::string *error);
flatfile_artifact_result flatfile_artifact_extend_timer(const std::string &root, int32_t vnum,
							int64_t minimum_timer, int64_t last_update,
							std::string *error);
flatfile_artifact_result flatfile_artifact_find_next_expired(const std::string &root,
							     int32_t after_vnum, int64_t now,
							     flatfile_artifact_record *record,
							     std::string *error);
flatfile_artifact_result flatfile_artifact_expire(const std::string &root, int32_t vnum,
						  int64_t now, std::string *error);
flatfile_artifact_result
flatfile_artifact_war_owners(const std::string &root, int32_t after_pid, size_t maximum,
			     std::vector<flatfile_artifact_war_owner> *owners, std::string *error);
flatfile_artifact_result flatfile_artifact_apply_war_burn(const std::string &root, int32_t pid,
							  int64_t now, double retained,
							  int64_t last_update, std::string *error);
flatfile_artifact_result flatfile_artifact_bind_get(const std::string &root, int32_t vnum,
						    int32_t *owner_pid, int64_t *timer,
						    std::string *error);
flatfile_artifact_result flatfile_artifact_bind_update(const std::string &root, int32_t vnum,
						       int32_t owner_pid, int64_t timer,
						       std::string *error);
flatfile_artifact_result flatfile_artifact_bind_reset_all(const std::string &root,
							  std::string *error);
flatfile_artifact_result
flatfile_artifact_repair_player_binding(const std::string &root, int32_t vnum,
					int64_t artifact_timer, int64_t bind_timer,
					int64_t last_update, std::string *error);
flatfile_artifact_result flatfile_artifact_prepare_player_release(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	flatfile_authority_operation *operation, std::string *error);
flatfile_artifact_result flatfile_artifact_release_player(const std::string &root, uint32_t pid,
							  std::string *error);
flatfile_artifact_result flatfile_artifact_reconcile_players(
	const std::string &root, const std::vector<flatfile_artifact_player_item> &items,
	int64_t reconciled_at, flatfile_artifact_reconcile_result *result, std::string *error);
flatfile_artifact_result flatfile_artifact_prepare_corpse_transfer(
	const std::string &root, const flatfile_authority_lock &lock,
	const item_transfer_payload &payload, uint64_t accepted_at_usec,
	flatfile_artifact_transfer_mutation *mutation, std::string *error);
flatfile_artifact_result flatfile_artifact_prepare_room_transfer(
	const std::string &root, const flatfile_authority_lock &lock,
	const item_transfer_payload &payload, uint64_t accepted_at_usec,
	flatfile_artifact_transfer_mutation *mutation, std::string *error);
flatfile_artifact_result flatfile_artifact_prepare_corpse_release(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t corpse_pid,
	int32_t room_vnum, uint64_t accepted_at_usec,
	const std::vector<player_item_snapshot> &items,
	flatfile_artifact_transfer_mutation *mutation, std::string *error);
flatfile_artifact_result flatfile_artifact_prepare_corpse_destruction(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t corpse_pid,
	uint64_t accepted_at_usec, const std::vector<player_item_snapshot> &items,
	flatfile_artifact_transfer_mutation *mutation, std::string *error);

#endif
