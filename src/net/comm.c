#include "item/held_retirement_recovery.h"
#include "item/held_retirement_transport.h"
#include "economy/native_mobile_birth_recovery.h"
#include "world/native_quest_recovery_context.h"
#include "persistence/death_recovery_visibility.h"
#include "account/password_async.h"
#include "account/account_async.h"
/*
 **************************************************************************
 *  File: comm.c                                             Part of Duris
 *  Usage: socket handling, main game loop
 *  Copyright 1994 - 2008 - Duris Systems Ltd.
 **************************************************************************
 */

#include "core/prototypes.h"
#include "core/game_loop_watchdog.h"
#include "combat/attack_cadence.h"
#include "world/world_singletons.h"
#include "item/item_actions.h"
#include "item/artifact_mana.h"
#include "item/device_actions.h"
#include "persistence/persistence_log.h"
#include "persistence/quest_reward_obligation_pipeline.h"
#include "world/quest_reward_recovery.h"
#include "world/native_quest_frozen_continuation.h"
#include "core/structs.h"
#include "telemetry/telemetry_runtime.h"
#include "net/comm.h"
#include "net/session_input.h"
#include "net/network_readiness.h"
#include "net/network_wakeup.h"
#include "net/output_style.h"
#include "net/chat_presentation.h"
#include "net/output_profiles.h"
#include "player/output_preferences.h"
#include "net/command_latency.h"
#include "world/db.h"
#include "world/quest_mobile_native_birth.h"
#include "world/zone_reset_item_owner.h"
#include "economy/zone_reset_item_recovery.h"
#include "world/object_template.h"
#include "world/events.h"
#include "world/world_activity.h"
#include "world/economic_initialized_world_owner.h"
#include "cmd/interp.h"
#include "core/utility.h"
#include "core/utils.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <gnutls/gnutls.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <utility>
#include <vector>
#include <zlib.h>
#include "guild/assocs.h"
#include "economy/auction_houses.h"
#include "economy/boon.h"
#include "persistence/copyover.h"
#include "net/transport.h"
#include "combat/ctf.h"
#include "world/epic.h"
#include "world/epic_task_catalog.h"
#include "world/ferry.h"
#include "net/gmcp.h"
#include "world/graph.h"
#include "guild/guildhall.h"
#include "kingdom/kingdom.h"
#include "world/hardcore.h"
#include "core/json_utils.h"
#include "core/lookup_process.h"
#include "world/map.h"
#include "net/mccp.h"
#include "core/mm.h"
#include "economy/nexus_stones.h"
#include "world/outposts.h"
#include "player/player_log.h"
#include "net/poll.h"
#include "core/profile.h"
#include "combat/racewar_stat_mods.h"
#include "redis/redis_lifecycle.h"
#include "redis/redis_presence_runtime.h"
#include "redis/redis_report_cache.h"
#include "redis/redis_world_runtime.h"
#include "ships/ships.h"
#include "magic/spells.h"
#include "magic/spell_item_lifecycle.h"
#include "item/enhance.h"
#include "economy/crafting.h"
#include "player/craft_progression_hooks.h"
#include "account/account_recovery.h"
#include "item/locker_identify.h"
#include "cmd/information_cache.h"
#include "cmd/help_cache.h"
#include "account/password_hash.h"
#include "account/account_reward_config.h"
#include "combat/frag_cap_config.h"
#include "world/hardcore_config.h"
#include "item/random_equipment_config.h"
#include "account/creation_availability_config.h"
#include "item/material_rarity.h"
#include "sql/sql.h"
#include "sql/sql_economic_runtime.h"
#include <chrono>
#include <thread>
#include "sql/sql_player_migration.h"
#include "net/telnet.h"
#include "world/timers.h"
#include "economy/tradeskill.h"
#include "net/ttype.h"
#include "net/unicode.h"
#include "net/websocket.h"
#include "world/world_quest.h"
#include "net/ws_handlers.h"
#include "persistence/latency_trace.h"
#include "persistence/persistence_queue.h"
#include "persistence/persistence_mode.h"
#include "core/env_file.h"
#include "persistence/locker_async.h"
#include "persistence/maintenance_repository.h"
#include "persistence/maintenance_scheduler.h"
#include "persistence/maintenance_snapshot.h"
#include "persistence/critical_command_coordinator.h"
#include "economy/economic_command_admission.h"
#include "economy/economic_gameplay_authority.h"
#include "persistence/critical_command_repository.h"
#include "persistence/critical_outbox.h"
#include "persistence/corpse_lifecycle_transaction.h"
#include "economy/currency_transaction.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "economy/shop_trade_transaction.h"
#include "item/item_uid_allocator.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_accounting_dispatch.h"
#include "flatfile/flatfile_economic_runtime.h"
#include "economy/auction_transaction.h"
#include "economy/auction_native_publication.h"
#include "economy/auction_native_command_context.h"
#include "economy/collector_catalog_cache.h"
#include "economy/collector_listing_pipeline.h"
#include "economy/collector_maintenance.h"
#include "economy/collector_presence.h"
#include "economy/collector_service.h"
#include "economy/collector_transaction.h"
#include "combat/combat_outcome_transaction.h"
#include "guild/artifact_guild_transaction.h"
#include "economy/boon_reward_transaction.h"
#include "economy/boon_shop_transaction.h"
#include "world/zone_touch_transaction.h"
#include "world/epic_transaction.h"
#include "world/vnum.mob.h"
#include "player/player_save_pipeline.h"
#include "player/player_quarantine_recovery.h"
#include "player/player_load_pipeline.h"
#include "player/player_death_restitution_adapter.h"
#if !defined(__NO_TESTS__) || defined(TEST_REAL_PERSISTENCE)
#include "core/test_async.h"
#endif

void account_player_load_complete(P_desc d, player_load_result result);
void nanny_player_load_complete(P_desc d, player_load_result result);

/* external variables */

extern P_index mob_index;
extern P_room world;
extern char debug_mode;
extern const int top_of_world;
extern int top_of_zone_table;
extern struct ban_t *ban_list;
extern struct wizban_t *wizconnect;
extern struct time_info_data time_info;
extern struct zone_data *zone;
extern struct zone_data *zone_table;
extern char *shutdown_message;
extern const int max_ingame_good;
extern const int max_ingame_evil;
extern TimedShutdownData shutdownData;
extern void timedShutdown(P_char ch, P_char, P_obj, void *data);

long sentbytes = 0;
long receivedbytes = 0;
bool game_booted = FALSE;
static std::vector<int32_t> maintenance_catalog_candidate;

static bool hydrate_flatfile_system_item_owner(void)
{
	const char *root = persistence_mode_flatfile_root();
	const item_owner_identity owner = { item_owner_type::system, 0, 0 };
	uint64_t revision = 0;
	std::vector<flatfile_item_ownership_record> items;
	std::string error;
	const auto loaded =
		root ? flatfile_item_repository_load_owner(root, owner, &revision, &items, &error) :
		       flatfile_item_repository_result::invalid;
	if (loaded == flatfile_item_repository_result::not_found)
		return item_ownership_runtime_hydrate_owner(owner, 0);
	return loaded == flatfile_item_repository_result::ok && items.empty() &&
	       item_ownership_runtime_hydrate_owner(owner, revision);
}

static void maintenance_handle_completions(const maintenance_result *results, size_t count)
{
	for (size_t index = 0; index < count; ++index)
	{
		const auto &result = results[index];
		if (result.job_id == maintenance_job_id::cargo_market &&
		    (result.outcome == maintenance_outcome::complete ||
		     result.outcome == maintenance_outcome::permanent_failure))
			cargo_maintenance_complete(result.work_id,
						   result.outcome == maintenance_outcome::complete);
		if (result.job_id == maintenance_job_id::auction_due_scan &&
		    (result.outcome == maintenance_outcome::complete ||
		     result.outcome == maintenance_outcome::more))
		{
			for (size_t value = 0; value < result.value_count; ++value)
				if (!finalize_auction(static_cast<int>(result.values[value]),
						      nullptr))
					logit(LOG_DEBUG,
					      "maintenance job=auction_due_scan outcome=submit_failed actor=redacted");
		}
		if (result.job_id == maintenance_job_id::level_cap &&
		    result.outcome == maintenance_outcome::complete && result.rows > 0)
		{
			redis_invalidate_fraglist();
			if (result.value_count == 3 && result.values[0] > 0 &&
			    result.values[0] <= INT32_MAX)
				boon_notify_snapshot(static_cast<int>(result.values[0]),
						     static_cast<int>(result.values[1]),
						     static_cast<int>(result.values[2]), BN_CREATE);
		}
		if (result.job_id == maintenance_job_id::boon_scan &&
		    (result.outcome == maintenance_outcome::complete ||
		     result.outcome == maintenance_outcome::more) &&
		    result.value_count % 6 == 0)
			for (size_t value = 0; value < result.value_count; value += 6)
			{
				const int id = static_cast<int>(result.values[value]);
				const int racewar = static_cast<int>(result.values[value + 1]);
				const int pid = static_cast<int>(result.values[value + 2]);
				const int reason = static_cast<int>(result.values[value + 3]);
				const int option = static_cast<int>(result.values[value + 4]);
				const int criteria = static_cast<int>(result.values[value + 5]);
				if (option == BOPT_CTFB && reason == BN_VOID)
					ctf_delete_flag(criteria);
				boon_notify_snapshot(id, racewar, pid, reason);
			}
		if (result.job_id != maintenance_job_id::epic_task_catalog)
			continue;
		if (result.outcome != maintenance_outcome::complete &&
		    result.outcome != maintenance_outcome::more)
		{
			maintenance_catalog_candidate.clear();
			continue;
		}
		bool valid = maintenance_catalog_candidate.size() + result.value_count <=
			     EPIC_TASK_CATALOG_MAX;
		for (size_t value = 0; valid && value < result.value_count; ++value)
			if (result.values[value] <= 0 || result.values[value] > INT32_MAX)
				valid = false;
			else
				maintenance_catalog_candidate.push_back(
					static_cast<int32_t>(result.values[value]));
		if (!valid)
		{
			maintenance_catalog_candidate.clear();
			continue;
		}
		if (result.outcome == maintenance_outcome::complete)
		{
			if (!epic_task_catalog_publish(maintenance_catalog_candidate.data(),
						       maintenance_catalog_candidate.size()))
				logit(LOG_DEBUG,
				      "maintenance job=epic_task_catalog outcome=publish_failed actor=redacted");
			maintenance_catalog_candidate.clear();
		}
	}
}

static void critical_gameplay_handle_completions(const critical_completion *completions,
						 size_t count)
{
	epic_transaction_handle_completions(completions, count);
	currency_transaction_handle_completions(completions, count);
	locker_identify_pulse();
	corpse_lifecycle_transaction_handle_completions(completions, count);
	item_movement_transaction_handle_completions(completions, count);
	quest_mobile_native_birth_completions(completions, count);
	zone_reset_room_item_completions(completions, count);
	shop_trade_transaction_handle_completions(completions, count);
	auction_transaction_handle_completions(completions, count);
	collector_transaction_handle_completions(completions, count);
	combat_outcome_transaction_handle_completions(completions, count);
	artifact_guild_transaction_handle_completions(completions, count);
	boon_reward_transaction_handle_completions(completions, count);
	boon_shop_transaction_handle_completions(completions, count);
	zone_touch_transaction_handle_completions(completions, count);
	player_death_restitution_runtime_handle_completions(completions, count);
}

static void critical_gameplay_publish_outbox();
static void critical_gameplay_drain_completions(const critical_completion *completions,
						size_t count)
{
	if (player_save_execution_guard::current_ownership_epoch())
	{
		item_movement_transaction_cancel_drop_preparations();
		player_save_pipeline_pulse();
	}
	critical_gameplay_handle_completions(completions, count);
	quest_mobile_native_birth_pulse(false);
	zone_reset_room_item_pulse(false);
}

// This TU owns the original drained repository ACK results. Only this owner
// can forward them to the retained native continuation; no public boolean is used.
class quest_native_reward_ack_owner final
{
    public:
	static void acknowledged(const quest_reward_ack_completion &completion) noexcept
	{
		quest_native_frozen_continuation_owner::acknowledged(completion);
	}
};

static void quest_reward_ack_pipeline_pulse(void)
{
	quest_reward_ack_completion completions[QUEST_REWARD_ACK_PIPELINE_PULSE_MAX] = {};
	const size_t count = quest_reward_obligation_pipeline_pulse(
		completions, QUEST_REWARD_ACK_PIPELINE_PULSE_MAX);
	for (size_t index = 0; index < count; ++index)
		if (!completions[index].error_code &&
		    (completions[index].result == quest_reward_obligation_result::ok ||
		     completions[index].result ==
			     quest_reward_obligation_result::already_acknowledged))
			quest_native_reward_ack_owner::acknowledged(completions[index]);
	for (size_t index = 0; index < count; ++index)
		if (completions[index].result != quest_reward_obligation_result::ok &&
		    completions[index].result !=
			    quest_reward_obligation_result::already_acknowledged)
			persistence_alert(AVATAR, "quest_reward", "offering", "redacted",
					  "acknowledge", "retained_pending", "error=%u",
					  completions[index].error_code);
}

static bool critical_gameplay_restore_replayed_command(const critical_command &command,
						       void *context)
{
	// Replay runs under the coordinator mutex. No observer may call back
	// into the coordinator; refusal keeps the journal and fails startup closed.
	return player_death_restitution_runtime_restore_replayed_command(command, context) &&
	       currency_transaction_restore_replayed_command(command) &&
	       spell_item_lifecycle_restore_replayed_command(command) &&
	       item_movement_transaction_restore_replayed_command(command) &&
	       quest_mobile_native_birth_restore(command) &&
	       (command.type != critical_command_type::auction ||
		command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		command.payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ||
		!command.publication_required ||
		auction_native_publication_restore_replayed_command(command)) &&
	       (command.type != critical_command_type::shop_trade ||
		command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		!command.publication_required ||
		shop_trade_transaction_restore_replayed_command(command)) &&
	       (command.type != critical_command_type::collector ||
		command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		!command.publication_required ||
		collector_service_restore_replayed_purchase(command));
}

static bool
critical_gameplay_restore_native_envelope(const critical_native_recovery_envelope &envelope, void *)
{
	if (envelope.command.type == critical_command_type::zone_reset_item_birth)
		return zone_reset_room_item_restore(envelope);
	if (envelope.command.type == critical_command_type::native_mobile_birth)
		return quest_mobile_native_birth_restore(envelope);
	if (envelope.command.type == critical_command_type::auction)
		return auction_native_publication_restore(envelope);
	if (held_retirement_transport_command(envelope.command))
		return item_movement_transaction_restore_held_retirement_recovery(envelope);
	if (envelope.command.type == critical_command_type::item_transfer)
		return item_movement_transaction_restore_native_recovery(envelope);
	return false;
}
static bool
critical_gameplay_native_publication_body_valid(const critical_native_recovery_envelope &envelope,
						const critical_completion &completion) noexcept
{
	if (envelope.command.type == critical_command_type::zone_reset_item_birth)
		return zone_reset_item_recovery_publication(envelope, completion);
	if (envelope.command.type == critical_command_type::native_mobile_birth)
		return native_mobile_birth_recovery_publication(envelope, completion);
	if (envelope.command.type == critical_command_type::auction)
		return auction_recovery_publication_context_valid(envelope, completion);
	if (held_retirement_transport_command(envelope.command))
		return held_retirement_recovery_publication_context_valid(envelope, completion);
	return native_quest_recovery_publication_context_valid(envelope, completion);
}

#ifndef __NO_MYSQL__
static critical_outbox_delivery_result
critical_gameplay_outbox_delivery(const critical_outbox_record &record, void *context)
{
	if (record.destination == 6)
		return combat_outcome_transaction_outbox_delivery(record, context);
	if (record.destination == 7)
		return artifact_guild_transaction_outbox_delivery(record, context);
	if (record.destination == 8)
		return boon_reward_transaction_outbox_delivery(record, context);
	if (record.destination == 9)
		return zone_touch_transaction_outbox_delivery(record, context);
	if (record.destination == COLLECTOR_OUTBOX_DESTINATION)
		return collector_transaction_outbox_delivery(record, context);
	if (record.destination == CORPSE_LIFECYCLE_OUTBOX_DESTINATION)
		return corpse_lifecycle_transaction_outbox_delivery(record, context);
	return auction_transaction_outbox_delivery(record, context);
}
#endif

/** Request an immediate game-thread shutdown transition through the existing persistence gates. */
void request_shutdown(int shutdown_type, const char *issuer, const char *reason)
{
	if (!quest_mobile_native_birth_lifecycle_ready() || !zone_reset_room_item_lifecycle_ready())
	{
		logit(LOG_STATUS,
		      "Shutdown request refused: original native birth preparation is unresolved.");
		return;
	}
	// Launcher signals request an immediate transition from the game thread.
	// A wall-clock "now" schedules another world event, which can be starved.
	shutdownData.reboot_time = 0;
	shutdownData.next_warning = -1;
	snprintf(shutdownData.IssuedBy, sizeof(shutdownData.IssuedBy), "%s",
		 issuer ? issuer : "Launcher");
	snprintf(shutdownData.Reason, sizeof(shutdownData.Reason), "%s",
		 reason ? reason : "signal from launcher");
	switch (shutdown_type)
	{
	case 1:
		shutdownData.eShutdownType = TimedShutdownData::OK;
		break;
	case 2:
		shutdownData.eShutdownType = TimedShutdownData::REBOOT;
		break;
	case 3:
		shutdownData.eShutdownType = TimedShutdownData::COPYOVER;
		break;
	default:
		shutdownData.eShutdownType = TimedShutdownData::OK;
		break;
	}
	timedShutdown(NULL, NULL, NULL, NULL);
}

extern void ne_events();
extern void event_wait(P_char, P_char, P_obj, void *);
extern unsigned long long ne_event_tick;

long unsigned int ip2ul(const char *ip);
void load_alliances();
void initialize_transport();
bool newHardcoreBoard(P_char ch, const char *arg, int cmd);
void format_to_snoopers(char *from_string, char *to_string);
extern void update_breath_weapon_properties();
extern void update_regen_properties();
static void greet(P_desc newd);
static void note_player_input_activity(P_desc t, const char *input);
static void process_line(P_desc t, char *in);

/* local globals */

P_desc descriptor_list, next_to_process, next_save = 0;
int mini_mode = 0;
int lawful = 0;
int no_specials = 0;
int override = 1;
int pulse = 0;
bool after_events_call = FALSE;
const char *material_rarity_report_dir = NULL;
bool material_rarity_report_mode = FALSE;
int _reboot = 0;
int _copyover = 0;
int _autoboot = 0;
int _pwipe = 0;
int req_passwd = 1;
int shutdownflag = 0;
// signal-initiated shutdown: 0=none, 1=shutdown, 2=reboot, 3=copyover
volatile sig_atomic_t signal_shutdown_pending = 0;
int slow_death = 0;
volatile sig_atomic_t tics = 0;
long boot_time;
int ipc_id = 0;
pid_t lookup_host_process;
int max_users_playing = 0;
int used_descs = 0, avail_descs = 0, max_descs = 0, max_descs_this_hour = 0;
struct mm_ds *dead_desc_pool = NULL;
int RUNNING_PORT = 0;
int no_random = 0;
int no_ferries = 0;

// copyover support
int copyover_boot = 0;
static int recovered_mother_desc = -1;
static int recovered_mother_desc_ssl = -1;
static int recovered_ws_desc = -1;

// listening sockets - stored here so copyover can access them
static int mother_desc = -1;
static int mother_desc_ssl = -1;
static int ws_desc = -1;
P_char executing_ch;
#define MAX_COMMAND_OUTPUT (15 * MAX_STRING_LENGTH) // upped this to 3x vs original MWD26
#define PAD_COMMAND_OUTPUT (500) // some space for appending a warning
char command_output[MAX_COMMAND_OUTPUT + PAD_COMMAND_OUTPUT + 1];
size_t output_length;
static std::string pager_original;
static bool pager_style_fallback = false;

#define MIN_SOCKET_BUFFER_SIZE 20480

/*
 * ********************************************************************* *
 * main game loop and related stuff                                 *
 * *********************************************************************
 */

int main(int argc, char **argv)
{
	if (!game_loop_watchdog_init())
	{
		fprintf(stderr, "Invalid launcher game-loop watchdog channel.\n");
		return 78;
	}

	int port, sslport;
	int pos = 1;
	const char *dir;
	int migrate_mode = 0;
	bool persistent_transport = false;

	port = DFLT_PORT;
	dir = DFLT_DIR;
	sslport = SSL_PORT;

	randomize(0);

	// check for --migrate-all before regular arg parsing
	for (int i = 1; i < argc; i++)
	{
		if (!strncmp(argv[i], "--material-rarity-report",
			     strlen("--material-rarity-report")))
		{
			const char *value = argv[i] + strlen("--material-rarity-report");
			if (*value == '=')
				value++;
			else if (!*value && i + 1 < argc)
				value = argv[++i];
			material_rarity_report_dir = *value ? value : "material-rarity-report";
			material_rarity_report_mode = TRUE;
			break;
		}
		if (!strcmp(argv[i], "--migrate-all"))
		{
			migrate_mode = 1;
			break;
		}
	}

	while ((pos < argc) && (*(argv[pos]) == '-'))
	{
		if (!strcmp(argv[pos], "--persistent-transport"))
		{
			persistent_transport = true;
			pos++;
			continue;
		}
		if (!strcmp(argv[pos], "--minimal"))
		{
			mini_mode = 1;
			no_random = 1;
			no_ferries = 1;
			logit(LOG_STATUS, "Running in minimal world mode");
			pos++;
			continue;
		}
		if (!strncmp(argv[pos], "--material-rarity-report",
			     strlen("--material-rarity-report")))
		{
			if (argv[pos][strlen("--material-rarity-report")] == '\0' && pos + 1 < argc)
				pos++;
			pos++;
			continue;
		}
		switch (*(argv[pos] + 1))
		{
		case 'f':
			no_ferries = 1;
			logit(LOG_STATUS, "Without ferries.");
			break;
		case 'l':
			no_random = 1;
			// lawful = 1;
			logit(LOG_STATUS, "Without randoms.");
			break;
		case 'd':
			if (*(argv[pos] + 2))
				dir = argv[pos] + 2;
			else if (++pos < argc)
				dir = argv[pos];
			else
			{
				fatal_boot_error("comm", "Directory arg expected after option -d.");
			}
			break;
		case 's':
			no_specials = 1;
			logit(LOG_STATUS, "Suppressing assignment of special routines.");
			break;
		case 'p':
			req_passwd = 0;
			logit(LOG_STATUS, "Allowing changing of password without old one.");
			break;
		case 'm':
			mini_mode = 1;
			no_random = 1;
			no_ferries = 1;
			logit(LOG_STATUS, "Running in mini mode");
			break;
		case 'z':
			mini_mode = 2;
			logit(LOG_STATUS, "Running with area debugger on");
			break;
		case 'C':
			// copyover boot - sockets recovered from copyover.dat
			copyover_boot = 1;
			logit(LOG_STATUS, "Copyover boot mode");
			break;
		default:
			logit(LOG_STATUS, "Unknown option -% in argument string.",
			      *(argv[pos] + 1));
			break;
		}
		pos++;
	}

	if (pos < argc)
	{
		if (!isdigit(*argv[pos]))
		{
			fatal_boot_error("comm",
					 "Usage: %s [-l] [-m|--minimal] [-s] [-p] [-f] "
					 "[-d pathname] [ port # ]",
					 argv[0]);
		}
		else if ((port = atoi(argv[pos])) <= 1024)
		{
			fatal_boot_error("comm", "Illegal port #");
		}
		else
			sslport = port + 1;
	}
	// Global variable so can check if mainmud or not!
	RUNNING_PORT = port;

	/* create an IPC msg queue to deal with hostname lookups.  */
	/*
	  ipc_id = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0600);
	  if (ipc_id < 0) {
	    fatal_boot_error("comm", "Unable to create message queue due to %d!", ipc_id);
	  }
	*/
	/* fork() off a new process to deal with hostname lookups. */
	/* fork will return 0 to the newly created process */

	/*
	  if (!(lookup_host_process = fork()))
	    exit(run_lookup_host_process(ipc_id));
	*/
	if (chdir(dir) < 0)
	{
		fatal_boot_error("comm", "chdir failed: %s", strerror(errno));
	}
	if (material_rarity_report_mode)
	{
		boot_material_rarity_objects(mini_mode);
		write_material_rarity_report(material_rarity_report_dir);
		return 0;
	}
	logit(LOG_STATUS, "Running game on port %d.", port);

	logit(LOG_STATUS, "Using %s as data directory.", dir);

	if (load_env_file() < 0)
		fatal_boot_error("comm", "Unsafe environment configuration file");

	const char *configured_tls_port = getenv("DURIS_TLS_PORT");
	if (configured_tls_port && *configured_tls_port)
	{
		char *end = NULL;
		errno = 0;
		long parsed_tls_port = strtol(configured_tls_port, &end, 10);
		if (errno == ERANGE || end == configured_tls_port || *end || parsed_tls_port < 1 ||
		    parsed_tls_port > 65535 || parsed_tls_port == port)
			fatal_boot_error("comm", "DURIS_TLS_PORT is invalid");
		sslport = static_cast<int>(parsed_tls_port);
	}
	logit(LOG_STATUS, "Using TLS telnet port %d.", sslport);
	if (!transport_world_configure())
		fatal_boot_error("transport", "Invalid inherited transport capability");
	if (persistent_transport && !transport_world_active())
		return transport_frontend_main(argc, argv, port, sslport);

	char persistence_error[2048];
	if (!persistence_mode_configure(persistence_error, sizeof(persistence_error)))
		fatal_boot_error("comm", "%s", persistence_error);
	logit(LOG_STATUS, "Persistence mode: %s.", persistence_mode_name());

	if (!persistence_log_start(LOG_FILE, LOG_WIZ))
		fatal_boot_error("comm", "Could not start persistence log worker");

	if (persistence_mode_requires_mysql() && initialize_mysql() < 0)
	{
		fatal_boot_error("comm", "MySQL initialization failed!");
	}
	// Resolve only an already selected, receipt-bearing native authority before
	// item/world hydration and before choosing save replay ownership. This never
	// installs a baseline or infers activation from configuration.
	if (!persistence_mode_requires_mysql() && !flatfile_economic_runtime_start())
		fatal_boot_error("comm", "Flatfile accounting runtime evidence unavailable");
	if (!persistence_mode_requires_mysql() &&
	    !item_uid_allocator_reserve(nullptr, ITEM_UID_BOOT_RESERVATION))
		fatal_boot_error("comm", "Could not reserve a collision-free flat item UID range");
	if (!persistence_mode_requires_mysql() && !hydrate_flatfile_system_item_owner())
		fatal_boot_error("comm",
				 "Could not hydrate the flat-file system item-owner revision");
	if (persistence_mode_requires_mysql() && !sql_hydrate_item_owner_revisions())
		logit(LOG_STATUS,
		      "Authoritative item owner revisions unavailable; movement fails closed.");

	redis_init();

	// run migration and exit if requested
	if (migrate_mode)
	{
		printf("running pfile migration...\n");
		int count = sql_migrate_all_players();
		printf("migration complete: %d players migrated\n", count);
		return 0;
	}

	// Property hooks now also own main-thread item-action cancellation. Bind
	// before loading them; the event pool is initialized later during world boot.
	nevent_bind_game_thread();
	initialize_properties();
	// Optional configuration is loaded once before gameplay, never during a send.
	if (const char *profile_path = getenv("DURIS_OUTPUT_PROFILES_FILE");
	    profile_path && *profile_path)
	{
		auto profiles = output_profile_registry().reload_file(profile_path);
		if (profiles.ok)
			logit(LOG_STATUS, "Output profiles loaded: revision %u.",
			      profiles.revision);
		else
			logit(LOG_STATUS,
			      "Output profiles unavailable: %s; using Preserve fallback.",
			      profiles.diagnostic.c_str());
	}

	load_event_names();

	init_cmdlog(); /* init cmd.debug file - DCL */
	(void)telemetry_runtime_init(telemetry_runtime_options_from_environment());

	const int game_exit_status = run_the_game(port, sslport);
	telemetry_shutdown_request telemetry_shutdown{};
	telemetry_shutdown.final_flush = 1U;
	telemetry_monotonic_usec telemetry_deadline_now = 0U;
	telemetry_utc_usec telemetry_deadline_utc = TELEMETRY_UTC_UNKNOWN;
	if (telemetry_runtime_now(&telemetry_deadline_now, &telemetry_deadline_utc))
	{
		telemetry_shutdown.deadline_monotonic_usec =
			telemetry_deadline_now > UINT64_MAX - 2'000'000U ?
				UINT64_MAX :
				telemetry_deadline_now + 2'000'000U;
	}
	(void)telemetry_runtime_shutdown(telemetry_shutdown);
	/* Final reap is deliberately mandatory and may block on an in-flight
	 * repository callback. Keep it before global SQL teardown even when the
	 * runtime clock could not provide a bounded request deadline. */
	(void)telemetry_runtime_final_reap();
	artifact_mana_shutdown();
	shutdown_mysql();
	critical_command_coordinator_release_lifecycle_guard();
	close_cmdlog();

	return game_exit_status;
}

static void finalize_styled_command(P_desc descriptor)
{
	if (pager_original.empty() || pager_style_fallback)
		return;
	const bool paged = descriptor && descriptor->character &&
			   IS_SET(descriptor->character->specials.act, PLR_PAGING_ON) &&
			   descriptor->connected != CON_MAIN_MENU;
	char *end = command_output + strlen(command_output);
	for (char *page = command_output; page < end;)
	{
		char *next = paged ? next_page(page, descriptor) : nullptr;
		char *page_end = next ? next : end;
		if (!output_message_fits_serializers(
			    std::string_view(page, (size_t)(page_end - page))))
		{
			// Individually safe sends can combine into an unsafe page. Check
			// before replay starts, while the entire original command is available.
			strcpy(command_output, pager_original.c_str());
			output_length = pager_original.size();
			pager_style_fallback = true;
			return;
		}
		page = page_end;
	}
}

// all text meant to go to executing_ch - a player whos command
// is currently processed is saved into command_output
// buffer instead of being sent over the network
// the intercepting happens in send_to_char
void process_with_paging(P_char ch, char *comm)
{
	executing_ch = ch;
	*command_output = '\0';
	output_length = 0;
	pager_original.clear();
	pager_style_fallback = false;
	command_interpreter(ch, comm);
	executing_ch = NULL;
	if (!ch->desc)
		return;
	finalize_styled_command(ch->desc);
	if (next_page(command_output, ch->desc))
		// page_string_real(ch->desc, command_output, 1);
		page_string_real(ch->desc, command_output);
	else
		SEND_TO_Q(command_output, ch->desc);
}

void game_up_message(int port)
{
	FILE *f;
	char Gbuf1[200];

	f = fopen("foo_tmp", "w");
	snprintf(Gbuf1, 200, "Duris> The mud is up at port %d. Run! Panic! *FLEE*\n", port);
	fputs(Gbuf1, f);
	fclose(f);
	if (system("/usr/local/bin/stealth-wall < foo_tmp") != 0)
		logit(LOG_STATUS, "game_up_message: stealth-wall failed");
	unlink("foo_tmp");
	//  signal(SIGCHLD, (void *) reaper);
}

static void touch(const char *filename)
{
	// no need to check for failure, the next step will do
	close(open(filename, O_WRONLY | O_CREAT, 0666));
}

/* Init sockets, run game, and cleanup sockets */

int run_the_game(int port, int sslport)
{
	long time_before = 0;
	long time_after = 0;

	descriptor_list = NULL;

	time_before = clock();

	logit(LOG_STATUS, "Signal trapping.");
	signal_setup();

	SetSpellCircles(); /* spells circlewise done with pure math */

	// check for redis crash recovery before boot_db (so ne_init_events skips zone resets)
	if (!copyover_boot && redis_runtime_enabled() && redis_world_runtime_enabled() &&
	    redis_has_world_state())
	{
		redis_world_recovery_boot_set(true);
		logit(LOG_STATUS,
		      "%s recovery data found in redis; world state restores after boot",
		      redis_world_clean_restart_boot() ? "Clean restart" : "Crash");
	}
	if (!mini_mode)
	{
		/* Legacy raw event queues are retired. Historical fallback records are
		 * inspected or quarantined by the explicit operator tool only. */
	}

	boot_db(mini_mode);
	// Minimal test worlds normally omit the collector prototype. A deliberately
	// complete fixture may include it so the real service can be exercised without
	// booting the production world; ordinary minimal boots stay silent and inert.
	if ((!mini_mode || real_mobile(VMOB_COLLECTOR_ANTIQUITIES) >= 0) &&
	    !collector_presence_init())
		logit(LOG_STATUS,
		      "Collector presence unavailable; collector commands fail closed.");

	// game_up_message(port);
	init_astral_clock(); // fix the map sight distances

	// cache named report, fraglist, and epic zones in redis
	redis_cache_named_report();
	redis_cache_fraglist();
	redis_cache_epic_zones();

	// clear stale online list from previous boot/crash
	redis_clear_online_players();

	if (no_random == 0)
		create_randoms();
	else
		fprintf(stderr, "Starting without random zones!.\n\r");

	if (!mini_mode)
	{
		fprintf(stderr, "-- Updating zone database.\r\n");
		update_zone_db();
		if (!epic_task_catalog_refresh())
			logit(LOG_STATUS,
			      "Epic task catalog unavailable; zone task selection uses safe fallback.");
	}
	else
	{
		fprintf(stderr, "--  Skipping zone database publication in mini mode.\r\n");
	}

	calculate_map_coordinates();
	fprintf(stderr, "--  Done calculating maps coordinates.\r\n");

	fprintf(stderr, "-- Calculating avg mob level and world-quest catalog.\r\n");
	if (calc_zone_mob_level() < 0)
	{
		logit(LOG_EXIT, "World quest catalog unavailable; bartender quests fail closed.");
		fprintf(stderr, "World quest catalog unavailable; bartender quests fail closed.\n");
	}
	fprintf(stderr, "--  Done calculating mob level and world-quest catalog.\r\n");

	// Recipe commands and receipt recovery also run in mini-mode worlds.
	craft_progression_initialize();

	if (!mini_mode)
		initialize_tradeskills();
	else
		fprintf(stderr, "--  Skipping tradeskills/mines in mini mode.\r\n");
	fprintf(stderr, "--  Done loading tradeskills/mines.\r\n");

	if (!mini_mode)
		load_cmd_attributes();
	else
		fprintf(stderr, "--  Skipping command attributes in mini mode.\r\n");
	fprintf(stderr, "--  Done loading command attributes.\r\n");

	if (!mini_mode)
	{
		if (no_ferries == 0)
			init_ferries();
		else
			fprintf(stderr, "Starting without ferries.\r\n");

		update_breath_weapon_properties();
		update_regen_properties();

		// initialize_buildings();

		Guild::initialize();
		fprintf(stderr, "-- Done loading guilds\r\n");
		if (!artifact_guild_state_hydrate())
			logit(LOG_FILE,
			      "artifact_guild: component=hydration outcome=unavailable state=retained");

		Guildhall::initialize();
		fprintf(stderr, "-- Done loading guildhalls\r\n");

		/* AFTER the guildhalls: a realm's anchor is a hall's outside square,
		 * and the orphan sweep needs them loaded to tell a hall that is really
		 * gone from one that simply has not booted yet. */
		kingdom_initialize();
		fprintf(stderr, "-- Done loading kingdoms\r\n");

		init_auction_houses();

		reset_racewar_stat_mods();
		init_nexus_stones();

		init_outposts();

		fprintf(stderr, "-- Loading alliances\r\n");
		load_alliances();

		fprintf(stderr, "-- Booting enhancement system\r\n");
		boot_enhancement_system();

		fprintf(stderr, "-- Booting crafting system\r\n");
		boot_crafting_system();

		fprintf(stderr, "-- Loading random equipment configuration\r\n");
		boot_random_equipment_config();

		fprintf(stderr, "-- Loading frag-cap configuration\r\n");
		boot_frag_cap_config();

		fprintf(stderr, "-- Loading account reward configuration\r\n");
		boot_account_reward_config();

		fprintf(stderr, "-- Loading Hardcore configuration\r\n");
		boot_hardcore_config();

		fprintf(stderr, "-- Loading creation availability configuration\r\n");
		boot_creation_availability_config();

		fprintf(stderr, "-- Touching hall of fame\r\n");
		touch(halloffamelist_file);
		newHardcoreBoard(NULL, "boot", 0);
		init_ctf();

		loadHints();
		epic_initialization();
	}
	else
	{
		fprintf(stderr, "--  Skipping optional subsystems in mini mode.\r\n");
	}
	// Final SQL recovery binding provenance uses all existing optional startup
	// assignments. Native stock restoration already used the parsed boot values.
	// This pre-worker cut never reparses, prebinds instance procedures or grants
	// accounting/publication authority; failure keeps recovery closed.
	if (persistence_mode_requires_mysql() && !finalize_recovery_object_template_bindings())
		logit(LOG_STATUS, "SQL recovery object-template final binding seal unavailable");
	// The distinct flat owner uses the same complete parsed boot catalog. Seal
	// its actual final procedure bindings at this serialized pre-worker cut;
	// failed provenance keeps its existing preparation consumers closed.
	if (persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY &&
	    !finalize_flatfile_shop_recovery_object_template_bindings())
		logit(LOG_STATUS,
		      "Flatfile recovery object-template final binding seal unavailable");

	ssl_read_cert();

	fprintf(stderr, "Assigning map glyph variations.\r\n");
	init_map_glyphs();

	time_after = clock();
	bfs_reset_marks();
	fprintf(stderr, "Boot completed in: %d milliseconds\n",
		(int)((time_after - time_before) * 1E3 / CLOCKS_PER_SEC));
	logit(LOG_STATUS, "Boot completed in:%d milliseconds\n",
	      (int)((time_after - time_before) * 1E3 / CLOCKS_PER_SEC));

	/* Do not start joinable worker threads until all fatal world-data loading is
	 * complete.  Legacy boot_db() errors exit immediately; starting this pipeline
	 * before boot_db() made a missing generated world invoke std::terminate while
	 * the global worker thread was still joinable, obscuring the real diagnostic
	 * and turning a controlled configuration failure into SIGABRT. */
	if (!player_load_pipeline_init())
		logit(LOG_STATUS,
		      "Player load pipeline unavailable; existing-character login will use synchronous fallback.");
#ifndef __NO_MYSQL__
	if (!account_load_worker_init())
		logit(LOG_STATUS,
		      "Account load worker unavailable; interactive login will report busy.");
#endif
	if (!quest_reward_obligation_pipeline_init())
		logit(LOG_STATUS,
		      "Quest reward acknowledgement worker unavailable; obligations remain pending.");
	/* Same rule for the mail worker: joinable thread only after the fatal loads. */
	if (!account_recovery_init())
		logit(LOG_STATUS,
		      "Account recovery unavailable; password reset by email disabled.");

	information_cache_refresh();
#ifndef __NO_MYSQL__
	help_cache_refresh();
#endif
	game_booted = TRUE;

	fprintf(stderr, "Entering game loop.\n\r");
	logit(LOG_STATUS, "Entering game loop.");
	if (!mini_mode)
		locker_async_init();
	const char *journal_directory = getenv("PLAYER_SAVE_JOURNAL_DIR");
	const bool owned_accounting_boot = economic_gameplay_authority::active();
	const bool player_saves_ready =
#ifdef __NO_MYSQL__
		owned_accounting_boot ?
			player_save_pipeline_prepare(
				journal_directory, player_quarantine_recovery_revalidate_selected) :
			player_save_pipeline_init(journal_directory,
						  player_quarantine_recovery_revalidate_selected);
#else
		player_save_pipeline_prepare(journal_directory,
					     player_quarantine_recovery_revalidate_selected);
#endif
	// Selected accounting policy is already held; full SQL/world recovery completes below.
	// Preparation/revalidation must finish at epoch zero; critical replay then
	// installs its original holds before any ordinary save execution starts.
	if (owned_accounting_boot)
	{
		uint64_t ownership_epoch = 0;
		if (!player_saves_ready ||
		    !player_save_execution_guard::begin_ownership_epoch(&ownership_epoch))
		{
			fprintf(stderr,
				"Active accounting save ownership unavailable; aborting boot.\n");
			_exit(1);
		}
	}
	if (!player_saves_ready)
	{
		logit(LOG_STATUS,
		      "Player save pipeline unavailable; nonterminal saves fail closed.");
		persistence_alert(AVATAR, "player_save", "pipeline", "none", "none", "start_failed",
				  "check PLAYER_SAVE_JOURNAL_DIR");
	}
	const char *critical_journal_directory = getenv("CRITICAL_COMMAND_JOURNAL_DIR");
	critical_apply_fn critical_apply = critical_command_repository_apply_from_pool;
	critical_shared_native_apply_fn shared_native_apply =
		critical_command_repository_apply_shared_native_from_pool;
	critical_zone_reset_item_apply_fn zone_reset_apply = nullptr;
	critical_extension_validator_fn critical_extension_validator =
		economic_command_admission_supported;
	critical_extension_validator_bounded_fn critical_extension_validator_bounded =
		economic_room_command_admission_supported_bounded;
#ifdef __NO_MYSQL__
	critical_apply = flatfile_accounting_apply_selected;
	shared_native_apply = critical_command_repository_apply_shared_native_flat;
	zone_reset_apply = critical_command_repository_apply_zone_reset_item_flat;
	critical_extension_validator = economic_flatfile_command_admission_supported;
	critical_extension_validator_bounded =
		economic_flatfile_room_command_admission_supported_bounded;
#else
	const bool critical_outbox_ready =
		critical_outbox_init(critical_gameplay_outbox_delivery, NULL);
#endif
	const bool critical_commands_ready =
#ifndef __NO_MYSQL__
		critical_outbox_ready &&
#endif
		critical_command_coordinator_init(
			critical_journal_directory, critical_apply, NULL,
			CRITICAL_COORDINATOR_DEFAULT_WORKERS,
			critical_gameplay_restore_replayed_command, NULL,
			critical_extension_validator, critical_gameplay_restore_native_envelope,
			critical_gameplay_native_publication_body_valid,
			{ native_mobile_birth_recovery_valid, native_mobile_birth_recovery_initial,
			  native_mobile_birth_recovery_successor,
			  native_mobile_birth_recovery_publication,
			  native_mobile_birth_recovery_terminal },
			native_quest_recovery_pair_context_valid,
			{ auction_recovery_envelope_valid, auction_recovery_initial_valid,
			  auction_recovery_successor_valid,
			  auction_recovery_publication_context_valid,
			  auction_recovery_terminal_valid },
			{ zone_reset_item_recovery_valid, zone_reset_item_recovery_initial,
			  zone_reset_item_recovery_successor, zone_reset_item_recovery_publication,
			  zone_reset_item_recovery_terminal, zone_reset_item_recovery_valid_bounded,
			  zone_reset_item_recovery_terminal_bounded,
			  zone_reset_item_recovery_successor_bounded,
			  zone_reset_item_recovery_publication_bounded },
			shared_native_apply, zone_reset_apply,
			critical_extension_validator_bounded);
	quest_mobile_native_birth_replay_ready(critical_commands_ready);
	zone_reset_room_item_replay_ready(critical_commands_ready);
	if (!critical_commands_ready)
	{
		if (owned_accounting_boot)
		{
			// Replay may already retain original holds. Never discard them or
			// enter gameplay after partial initialization of active authority.
			fprintf(stderr,
				"Active accounting critical recovery unavailable; aborting boot.\n");
			_exit(1);
		}
		if (critical_command_coordinator_shutdown())
		{
			player_death_restitution_runtime_abort_all();
			critical_outbox_shutdown();
		}
		else
			logit(LOG_STATUS,
			      "Critical command coordinator shutdown refused; retaining dependent pipelines.");
		logit(LOG_STATUS,
		      "Critical command pipeline unavailable; critical gameplay fails closed.");
		persistence_alert(AVATAR, "critical_command", "pipeline", "none", "none",
				  "start_failed", "check critical schema and journal");
	}
	critical_command_coordinator_set_drain_observer(critical_gameplay_drain_completions);
	critical_outbox_set_drain_observer(
		critical_commands_ready ? critical_gameplay_publish_outbox : nullptr);
#ifndef __NO_MYSQL__
	if (player_saves_ready && critical_commands_ready)
	{
		// Actual validators/generation/save holds now exist, and all original
		// world/template/procedure seals are complete. The retained boot owner
		// pulses only accepted recovery, restoring genuine journal births and
		// SQL-only published lifetimes before regular projection/save admission.
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
		for (;;)
		{
			const auto progress = sql_economic_runtime_recover_boot_step(
				critical_gameplay_drain_completions);
			// Progress the original room journal owner too. Completion/drain
			// delivery cannot replace its actual current SQL/world proof or
			// leave retained room recovery waiting behind a birth-only boot loop.
			zone_reset_room_item_pulse(false);
			if (progress == sql_economic_boot_progress::ready &&
			    !zone_reset_room_item_recovery_pending())
				break;
			if (progress == sql_economic_boot_progress::refused ||
			    std::chrono::steady_clock::now() >= deadline)
			{
				fprintf(stderr,
					"Selected SQL/world accounting recovery unavailable; aborting boot.\n");
				_exit(1);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
#endif
	// Critical replay installs original save holds before any save replay/worker
	// can execute. Failed critical initialization keeps preparation closed and
	// retains its original slots for shutdown/restart, rather than running past it.
	if (
#ifdef __NO_MYSQL__
		owned_accounting_boot &&
#endif
		player_saves_ready && critical_commands_ready && !player_save_pipeline_start())
	{
		if (owned_accounting_boot)
		{
			fprintf(stderr,
				"Active accounting save execution unavailable; aborting boot.\n");
			_exit(1);
		}
		logit(LOG_STATUS,
		      "Player save execution unavailable; prepared recovery remains held.");
		persistence_alert(AVATAR, "player_save", "pipeline", "none", "none", "start_failed",
				  "check prepared save recovery");
	}
#ifndef __NO_MYSQL__
	if (player_saves_ready && critical_commands_ready &&
	    !sql_economic_runtime_finish_boot_admission())
	{
		fprintf(stderr, "SQL accounting boot admission unavailable; aborting boot.\n");
		_exit(1);
	}
#endif
	if (!collector_catalog_cache_refresh())
		logit(LOG_STATUS,
		      "Collector catalog refresh unavailable; collector gameplay fails closed.");
	if (!collector_listing_pipeline_init())
		logit(LOG_STATUS,
		      "Collector listing pipeline unavailable; collector commands fail closed.");
	if (!locker_identify_init(critical_journal_directory))
		logit(LOG_STATUS,
		      "Locker identification unavailable: receipt storage could not initialize.");
	if (!mini_mode)
	{
		const uint64_t maintenance_instance =
			(static_cast<uint64_t>(static_cast<uint32_t>(port)) << 32) |
			static_cast<uint32_t>(sslport);
		const char *maintenance_state = getenv("MAINTENANCE_STATE_FILE");
		if (!maintenance_state || !*maintenance_state)
			maintenance_state = "bin/server/maintenance-scheduler.state";
		if (!maintenance_scheduler_set_state_path(maintenance_state) ||
		    !maintenance_scheduler_init(maintenance_instance,
						maintenance_repository_execute, nullptr,
						maintenance_prepare_request))
			logit(LOG_STATUS,
			      "Maintenance scheduler unavailable; recurring external jobs fail closed.");
	}

	/* Boot-time scalar queue flood test: overflows the queue so the
	 * latency_trace instrumentation can capture scalar_enq_ok/drop
	 * and fallback_file_write statistics in the next periodic dump.
	 * Reset the process-global trace before the test so data is clean. */
	latency_trace_init();
	latency_trace_reset();
#ifndef __NO_TESTS__
	test_persistence_run_one("queue_flood_scalar");
	test_persistence_run_one("queue_routes_oversize_scalar_to_large");
	test_persistence_run_one("queue_routes_oversize_item_to_large");
	test_persistence_run_one("worker_scalar_fallback");
	test_persistence_run_one("worker_scalar_fifo_after_retry");
	test_persistence_run_one("worker_item_fifo");
	test_persistence_run_one("worker_large_roundtrip");
#endif
#ifdef TEST_REAL_PERSISTENCE
	test_real_persistence_run_all();
	test_real_persistence_print_summary();
#endif

	game_loop(port, sslport);
	game_loop_watchdog_lifecycle(_reboot || _autoboot || _pwipe ? 'D' : 'S');
	/* Flush dirty realms and reap the placed resource nodes while the
	 * world and the store are still up. Idempotent and self-gating, so
	 * a build with kingdoms disabled pays nothing here. */
	kingdom_shutdown();
	maintenance_scheduler_shutdown();
	redis_cleanup();
	quest_reward_obligation_pipeline_shutdown();
	player_load_pipeline_shutdown();
	for (P_desc descriptor = descriptor_list; descriptor; descriptor = descriptor->next)
		account_async_cancel(descriptor);
	account_load_worker_shutdown();
	collector_maintenance_shutdown();
	collector_listing_pipeline_shutdown();
	collector_presence_shutdown();
	collector_catalog_cache_shutdown();
	information_cache_shutdown();
	help_cache_shutdown();
	account_recovery_shutdown();
	password_login_shutdown();
	player_death_restitution_runtime_shutdown();
	const bool owned_saves_present = player_save_execution_guard::current_ownership_epoch() !=
					 0;
	const bool owned_saves_stopped = !owned_saves_present ||
					 player_save_pipeline_shutdown_owned();
	if (!owned_saves_stopped)
	{
		fprintf(stderr,
			"Owned save shutdown incomplete; retaining journals for cold recovery.\n");
		// Normal return destroys SQL/world dependencies and joinable globals.
		// A refused close is an unclean exit, never successful owner release.
		_exit(1);
	}
	const bool critical_coordinator_stopped = owned_saves_stopped &&
						  critical_command_coordinator_shutdown();
	if (!critical_coordinator_stopped)
	{
		logit(LOG_EXIT,
		      "Critical coordinator refused shutdown after lifecycle guard acquisition.");
		if (owned_saves_present)
			_exit(1);
	}
	locker_identify_shutdown();
	if (critical_coordinator_stopped)
		critical_outbox_shutdown();
	if (critical_coordinator_stopped && !_pwipe)
	{
		locker_async_shutdown();
		if (!owned_saves_present)
			player_save_pipeline_shutdown();
	}

	// Refused/cancelled shutdown and copyover retain their runtime projection.
	// Pwipe skips ordinary asynchronous owner closure and is not this boundary.
	if (critical_coordinator_stopped && owned_saves_stopped && !_pwipe && !_copyover &&
	    !persistence_mode_requires_mysql())
		flatfile_economic_runtime_shutdown();

		/* Don't need this anymore, as dropped artis are handled in real time on the DB.
	// Look for dropped artis and remove them from the next boot.
	dropped_arti_hunt();
	*/

#ifdef MEMCHK
	if (!_copyover)
	{
		free_world();
		dump_mem_log();
	}
#endif

	if (!persistence_log_drain(3000))
		fprintf(stderr,
			"PERSISTENCE: log drain timed out; unwritten diagnostic records may be lost.\n");
	const auto log_metrics = persistence_log_snapshot();
	if (log_metrics.rejected || log_metrics.file_failures || log_metrics.wiz_failures)
		fprintf(stderr,
			"PERSISTENCE: log delivery rejected=%llu file_failures=%llu wiz_failures=%llu\n",
			(unsigned long long)log_metrics.rejected,
			(unsigned long long)log_metrics.file_failures,
			(unsigned long long)log_metrics.wiz_failures);

	if (_reboot)
	{
		logit(LOG_EXIT, "Rebooting.");
		logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
		ws_broadcast_mud_shutdown("reboot");
		return 52; /* what's so great about HHGTTG, anyhow? */
	}
	// A successful copyover replaces this process from inside game_loop(). A
	// failed copyover resumes that loop, so reaching here with the flag set is
	// an invariant failure and must not fall back to a destructive restart.
	if (_copyover)
	{
		logit(LOG_EXIT, "Copyover returned unexpectedly; refusing fallback exit.");
		return 0;
	}
	if (_autoboot)
	{
		logit(LOG_EXIT, "Auto reboot.");
		logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
		ws_broadcast_mud_shutdown("autoreboot");
		return 54;
	}
	if (_pwipe)
	{
		logit(LOG_EXIT, "Pwipe Shutdown.");
		logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
		ws_broadcast_mud_shutdown("pwipe");
		return 55;
	}
	ws_broadcast_mud_shutdown("manual");
	logit(LOG_EXIT, "Normal termination of game.");
	logit(LOG_EXIT, "Max Goods: %d, Max Evils: %d.", max_ingame_good, max_ingame_evil);
	logit(LOG_STATUS, "Normal termination of game.");
	return 0;
}

/* Accept new connects, relay commands, and call 'heartbeat-functs' */

#define MAX_ACCEPTS_PER_TURN 32

static int drain_new_connections(int listener, int conn_type, const char *label)
{
	int accepted_count = 0;

	for (int attempt = 0; attempt < MAX_ACCEPTS_PER_TURN; attempt++)
	{
		if (new_descriptor(listener, conn_type) == 0)
		{
			accepted_count++;
			continue;
		}
		if (errno == EINTR)
		{
			attempt--;
			continue;
		}
		if (errno != EAGAIN
#if EWOULDBLOCK != EAGAIN
		    && errno != EWOULDBLOCK
#endif
		)
			logit(LOG_COMM, "%s accept failed: %s", label, strerror(errno));
		break;
	}
	return accepted_count;
}

static uint64_t loop_monotonic_us(void)
{
	static bool clock_failure_reported = false;
	const uint64_t now_us = latency_trace_monotonic_us();

	if (!now_us && !clock_failure_reported)
	{
		clock_failure_reported = true;
		logit(LOG_STATUS,
		      "LATENCY CLOCK FAILURE: CLOCK_MONOTONIC unavailable; invalid elapsed samples are omitted until recovery");
	}
	return now_us;
}

#define COMMAND_LATENCY_TIMESTAMP_SIZE 32

static command_latency_report_state command_report_state = {};

static void prepare_command_latency_log_buffer(command_latency_log_buffer *report)
{
	if (!report)
		return;
	char continuation_prefix[COMMAND_LATENCY_LOG_PREFIX_SIZE] = "timestamp-unavailable::";
	const time_t now = time(0);
	struct tm local_time = {};
	char timestamp[COMMAND_LATENCY_TIMESTAMP_SIZE] = {};
	if (localtime_r(&now, &local_time) && asctime_r(&local_time, timestamp))
	{
		char *newline = strchr(timestamp, '\n');
		if (newline)
			*newline = '\0';
		snprintf(continuation_prefix, sizeof continuation_prefix, "%s::", timestamp);
	}
	command_latency_log_buffer_reset(report, continuation_prefix);
}

static void collect_command_latency_log(const char *line, void *context)
{
	command_latency_log_buffer *report = (command_latency_log_buffer *)context;
	if (!report->length)
		prepare_command_latency_log_buffer(report);
	command_latency_log_buffer_collect(line, context);
}

static void prepare_descriptor_latency_event(command_latency_event *event,
					     command_latency_kind kind, P_desc descriptor,
					     const char *playing_input)
{
	P_char player =
		descriptor ? (descriptor->original ? descriptor->original : descriptor->character) :
			     NULL;
	const bool identified_player = player && IS_PC(player);
	command_latency_event_prepare(
		event, kind, descriptor ? descriptor->connected : -1,
		identified_player ? (long)GET_ID(player) : -1L,
		identified_player ? GET_NAME(player) : NULL,
		kind == COMMAND_LATENCY_PLAYING ? input_command_label(playing_input) : NULL);
}

class scoped_command_latency
{
    public:
	scoped_command_latency(command_latency_tracker *tracker, const command_latency_event *event)
		: tracker_(tracker)
		, event_(event)
		, started_us_(loop_monotonic_us())
		, active_(true)
	{
	}

	~scoped_command_latency() { finish(); }

	void finish()
	{
		if (!active_)
			return;
		command_latency_record(tracker_, event_,
				       latency_trace_elapsed_us(started_us_, loop_monotonic_us()));
		active_ = false;
	}

	scoped_command_latency(const scoped_command_latency &) = delete;
	scoped_command_latency &operator=(const scoped_command_latency &) = delete;

    private:
	command_latency_tracker *tracker_;
	const command_latency_event *event_;
	uint64_t started_us_;
	bool active_;
};

/** Select normal or transaction-gated dequeue from the live busy state. */
static int get_playing_cmd_from_q(P_char character, struct txt_q *queue, char *dest)
{
	if (!character)
		return get_from_q(queue, dest);
	return get_pending_transaction_cmd_from_q(
		queue, dest,
		item_movement_transaction_player_busy(character) ||
			bulk_get_player_busy(character) ||
			collector_transaction_player_busy(character) ||
			collector_service_player_busy(character),
		currency_transaction_player_busy(character) ||
			collector_transaction_player_busy(character) ||
			collector_service_player_busy(character));
}

/** Select the restricted queue throughout ordinary casting or active item use. */
static bool casting_input_for_descriptor(P_desc descriptor, P_char character)
{
	return character && descriptor &&
	       (IS_AFFECTED2(character, AFF2_CASTING) || item_action_active(character)) &&
	       descriptor->connected == CON_PLAYING && !descriptor->showstr_count &&
	       !descriptor->str;
}

/** Send a dequeued playing-state command through the normal command dispatcher. */
static void dispatch_playing_command(P_char character, char *input)
{
	if (character && character->desc && IS_SET(character->specials.act, PLR_PAGING_ON))
		process_with_paging(character, input);
	else
		command_interpreter(character, input);
}

struct game_loop_pulse_context
{
	int telnet_listener;
	int ssl_listener;
	int websocket_listener;
	char *network_buffer;
	char *command_buffer;
	struct host_answer *host_answer;
	unsigned long *accept_debug_pulse;
	long *last_desc_per_hour_reset;
	bool accept_debug;
	uint64_t loop_time_begin_us;
	uint64_t loop_tick;
	uint64_t loop_start_mono_us;
	command_latency_tracker command_latency = {};
	uint64_t connections_us = 0;
	uint64_t command_sweep_us = 0;
	uint64_t prompts_us = 0;
	uint64_t ne_events_us = 0;
	uint64_t activities_us = 0;
	uint64_t combat_us = 0;
	uint64_t affect_us = 0;
	uint64_t point_us = 0;
	uint64_t loop_us = 0;
};

static bool session_input_authentication_pending(P_desc descriptor)
{
	return account_async_pulse(descriptor) || password_async_pulse(descriptor) ||
	       account_login_password_pulse(descriptor);
}

static void repair_session_command_gate(P_char character)
{
	if (!character || CAN_ACT(character) ||
	    (get_scheduled(character, event_wait) &&
	     ne_event_tick <= character->specials.wait_until_pulse))
		return;
	logit(LOG_DEBUG,
	      "command gate: clearing stuck PLR2_WAIT on %s (event_wait scheduled: %s, pulse %llu, deadline %llu).",
	      J_NAME(character), get_scheduled(character, event_wait) ? "yes" : "no", ne_event_tick,
	      character->specials.wait_until_pulse);
	REMOVE_BIT(character->specials.act2, PLR2_WAIT);
	if (character->in_room != NOWHERE)
		update_pos(character);
}

static session_input_route select_session_input(P_desc descriptor, P_char character, char *input)
{
	if (!descriptor || !input)
		return session_input_route::none;
	const bool casting_input = casting_input_for_descriptor(descriptor, character);
	const bool creation_grant_input = descriptor->connected == CON_PLAYING && character &&
					  item_creation_grant_blocks_commands(character);
	if (character &&
	    (creation_grant_input || (!CAN_ACT(character) && !casting_input) ||
	     (IS_SET(character->specials.affected_by, AFF_CHARM) && !descriptor->original)))
		return session_input_route::none;
	const bool dequeued = casting_input ?
				      get_casting_cmd_from_q(character, &descriptor->input, input) :
			      descriptor->connected == CON_PLAYING && !descriptor->showstr_count &&
					      !descriptor->str ?
				      get_playing_cmd_from_q(character, &descriptor->input, input) :
				      get_from_q(&descriptor->input, input);
	if (!dequeued)
		return session_input_route::none;
	if (descriptor->showstr_count)
		return session_input_route::pager;
	if (descriptor->str)
		return session_input_route::editor;
	return descriptor->connected == CON_PLAYING ? session_input_route::playing :
						      session_input_route::nanny;
}

static void dispatch_session_input(P_desc descriptor, P_char character, char *input,
				   session_input_route route, command_latency_tracker *latency)
{
	if (!session_input_route_dispatches(route))
		return;
	if (descriptor->transport_command)
		descriptor->transport_command_started = true;
	if (character)
		character->specials.timer = 0;
	descriptor->prompt_mode = TRUE;
	command_latency_event event = {};
	const command_latency_kind kind =
		route == session_input_route::pager   ? COMMAND_LATENCY_PAGER :
		route == session_input_route::editor  ? COMMAND_LATENCY_EDITOR :
		route == session_input_route::playing ? COMMAND_LATENCY_PLAYING :
							COMMAND_LATENCY_NANNY;
	prepare_descriptor_latency_event(&event, kind, descriptor,
					 route == session_input_route::playing ? input : NULL);
	const uint64_t started_us = loop_monotonic_us();
	switch (route)
	{
	case session_input_route::pager:
		show_string(descriptor, input);
		break;
	case session_input_route::editor:
		string_add(descriptor, input);
		break;
	case session_input_route::playing:
		dispatch_playing_command(character, input);
		break;
	case session_input_route::nanny:
		descriptor->wait = 0;
		nanny(descriptor, input);
		break;
	case session_input_route::none:
	case session_input_route::authentication_pending:
		return;
	}
	command_latency_record(latency, &event,
			       latency_trace_elapsed_us(started_us, loop_monotonic_us()));
}

/*
 * The pulse phase order is a compatibility boundary.  Each helper owns one
 * stage, but all helpers execute on the game thread in the order documented
 * in docs/network/GAME_LOOP_PHASES.md.  In particular, output drains before
 * ne_events() closes the current tick's scheduling window, and two-pulse
 * durable completions remain after that event phase.
 */

/* Drain retained wire bytes only. Application output/prompt construction remains
 * in run_output_phase() at the simulation boundary. */
static int drain_network_transport(P_desc point)
{
	if (point->write_failed)
		return -1;
	if (point->connected == CON_SSLNEGO)
		return 0;
	if (network_transport_pending(point) &&
	    (point->network_revents & network_write_interest(point)))
	{
		if ((!point->websocket && telnet_flush_output(point) < 0) ||
		    (point->websocket && websocket_flush_output(point) < 0))
		{
			point->write_failed = 1;
			return -1;
		}
	}
	if (point->websocket && point->ws_state == WS_STATE_OPEN && point->ws_ping_queued &&
	    point->ws_control_output_len == 0)
	{
		point->ws_ping_queued = 0;
		point->ws_ping_outstanding = 1;
		point->ws_pong_received = 0;
		point->ws_last_ping = time(0);
	}
	if (point->websocket && point->ws_state == WS_STATE_CLOSING &&
	    !network_transport_pending(point))
		return -1;
	return 0;
}

/* One bounded network turn: direct descriptor entries, one read per client,
 * bounded accept/output/parser work, and no game commands or world callbacks.
 * Every registered client gets a turn, so a busy client cannot take another
 * read ahead of its peers. Never retain borrowed pointers across a turn. */
static bool service_network_turn(game_loop_pulse_context &ctx, int timeout_ms)
{
	std::vector<pollfd> sockets = {
		{ ctx.telnet_listener, POLLIN, 0 },
		{ ctx.ssl_listener, POLLIN, 0 },
		{ ctx.websocket_listener, POLLIN, 0 },
		{ network_wakeup_fd(), POLLIN, 0 },
	};
	std::vector<P_desc> clients;
	const uint64_t registration_us = loop_monotonic_us();
	for (P_desc point = descriptor_list; point; point = point->next)
	{
		point->network_revents = 0;
		const short interests = network_read_interest(point) |
					network_write_interest(point);
		sockets.push_back(
			{ point->network_close_pending || !interests ? -1 : point->descriptor,
			  static_cast<short>(interests | POLLPRI), 0 });
		clients.push_back(point);
		if (!point->network_close_pending && network_buffered_input(point))
			timeout_ms = 0;
		if (!point->network_close_pending && point->connected == CON_SSLNEGO &&
		    point->tls_handshake_deadline_us)
			timeout_ms =
				MIN(timeout_ms, network_timeout_ms(point->tls_handshake_deadline_us,
								   registration_us));
	}
	const int poll_result = poll(sockets.data(), sockets.size(), timeout_ms);
	if (poll_result < 0)
	{
		if (errno != EINTR)
		{
			logit(LOG_COMM, "Network poll failed: %s", strerror(errno));
			shutdownflag = 1;
		}
		return false;
	}
	// Assign results before accepting/closing anything. A reused descriptor
	// number cannot inherit readiness from the previous connection.
	for (size_t index = 0; index < clients.size(); ++index)
		clients[index]->network_revents = sockets[index + 4].revents;
	if (sockets[3].revents & POLLIN)
		network_wakeup_drain();
	for (size_t index = 0; index < 3; ++index)
	{
		if (sockets[index].revents & (POLLERR | POLLHUP | POLLNVAL))
		{
			logit(LOG_COMM, "Listener %d failed: poll events=%d", sockets[index].fd,
			      sockets[index].revents);
			shutdownflag = 1;
			return false;
		}
	}
	if (ctx.accept_debug &&
	    ((++*ctx.accept_debug_pulse % 20) == 0 || (sockets[0].revents & POLLIN)))
		logit(LOG_STATUS,
		      "ACCEPT DEBUG: turn=%lu listener=%d poll_result=%d listener_ready=%d descriptors=%d",
		      *ctx.accept_debug_pulse, ctx.telnet_listener, poll_result,
		      (sockets[0].revents & POLLIN) ? 1 : 0, used_descs);

	/* Nonblocking accept is the authoritative readiness check. */
	// The boundary's zero-time poll also probes listeners, retaining the accept
	// watchdog. Blocking waits drain only listeners reported ready.
	if ((sockets[0].revents & POLLIN) || timeout_ms == 0)
		drain_new_connections(ctx.telnet_listener, 0, "Telnet");
	if ((sockets[1].revents & POLLIN) || timeout_ms == 0)
		drain_new_connections(ctx.ssl_listener, 1, "SSL");
	if (ctx.websocket_listener >= 0 && ((sockets[2].revents & POLLIN) || timeout_ms == 0))
		drain_new_connections(ctx.websocket_listener, 2, "WebSocket");

	for (P_desc point = descriptor_list; point; point = next_to_process)
	{
		next_to_process = point->next;
		if (point->network_close_pending)
			continue;
		if (point->network_revents & (POLLERR | POLLNVAL | POLLPRI))
		{
			point->network_close_pending = 1;
			continue;
		}
		if (point->connected == CON_SSLNEGO)
		{
			if (point->tls_handshake_deadline_us &&
			    loop_monotonic_us() >= point->tls_handshake_deadline_us)
			{
				logit(LOG_COMM, "TLS handshake timed out on descriptor %d",
				      point->descriptor);
				point->network_close_pending = 1;
				continue;
			}
			if (!(point->network_revents & (network_read_interest(point) | POLLHUP)))
				continue;
			command_latency_event ssl_event = {};
			prepare_descriptor_latency_event(&ssl_event, COMMAND_LATENCY_SSL, point,
							 NULL);
			const uint64_t ssl_started_us = loop_monotonic_us();
			const int ssl_result = ssl_negotiate(point->sslses);
			command_latency_record(&ctx.command_latency, &ssl_event,
					       latency_trace_elapsed_us(ssl_started_us,
									loop_monotonic_us()));
			if (ssl_result == 0)
			{
				point->tls_read_interest = 0;
				point->tls_handshake_deadline_us = 0;
				greet(point);
			}
			else if (ssl_result == 1)
				point->tls_read_interest =
					gnutls_record_get_direction(point->sslses) ? POLLOUT :
										     POLLIN;
			else
				point->network_close_pending = 1;
			continue;
		}
		// HUP is reported even without the requested readiness direction. A
		// paused reader/retained TLS send must not spin on a terminal peer.
		if ((point->network_revents & POLLHUP) && !network_read_interest(point))
		{
			point->network_close_pending = network_has_unoffered_input(point) ? 2 : 1;
			continue;
		}
		// Resume a retained TLS send before allowing another TLS operation to
		// overwrite its required retry direction.
		if (drain_network_transport(point) < 0)
		{
			// A normal WebSocket close can follow queued application frames.
			// Offer those at the boundary before retiring the connection, as for EOF.
			point->network_close_pending =
				point->websocket && point->ws_state == WS_STATE_CLOSING &&
						!point->write_failed &&
						point->ws_pending_application ?
					2 :
					1;
			continue;
		}
		if (point->telnet_tls_retry)
			continue;
		if (point->websocket && websocket_input_paused(point))
			continue;
		if (!point->network_input_remaining && !network_buffered_input(point))
			continue;
		if (!(point->network_revents & (network_read_interest(point) | POLLHUP)) &&
		    !network_buffered_input(point))
			continue;
		const int input_result = process_input(point);
		if (input_result == NETWORK_INPUT_EOF)
		{
			// A read performed between pulses has not yet been offered to
			// session input. Retain the old one-boundary opportunity before
			// EOF teardown; input from an earlier boundary gets no extension.
			point->network_close_pending = network_has_unoffered_input(point) ? 2 : 1;
		}
		else if (input_result < 0)
		{
			if (point->websocket && point->ws_state == WS_STATE_OPEN)
			{
				const int close_code = point->ws_error_code ?
							       point->ws_error_code :
							       WS_CLOSE_PROTOCOL_ERROR;
				const char *reason =
					close_code == WS_CLOSE_MESSAGE_TOO_BIG ? "Message too big" :
					close_code == WS_CLOSE_INVALID_DATA    ? "Invalid data" :
					close_code == WS_CLOSE_INTERNAL_ERROR  ? "Internal error" :
										 "Protocol error";
				websocket_close(point, close_code, reason);
			}
			else
				point->network_close_pending = 1;
		}
	}
	return true;
}

static void network_wait_until(game_loop_pulse_context &ctx, uint64_t deadline_us)
{
	for (;;)
	{
		if (shutdownflag)
			return;
		const uint64_t now_us = loop_monotonic_us();
		if (!now_us)
		{
			logit(LOG_COMM, "Network deadline clock unavailable; requesting shutdown.");
			shutdownflag = 1;
			return;
		}
		if (now_us >= deadline_us)
			return;
		if (transport_world_active())
		{
			// Logical session slots are not socket FDs. IPC application dispatch
			// stays at the connection boundary; completion hints may wake the wait.
			pollfd wake{ network_wakeup_fd(), POLLIN, 0 };
			if (poll(&wake, 1, network_timeout_ms(deadline_us, now_us)) > 0)
				network_wakeup_drain();
		}
		else
			service_network_turn(ctx, network_timeout_ms(deadline_us, now_us));
	}
}

static bool run_connection_phase(game_loop_pulse_context &ctx)
{
	if (signal_shutdown_pending)
	{
		const int type = signal_shutdown_pending;
		signal_shutdown_pending = 0;
		request_shutdown(type, "Launcher", "signal from launcher");
	}
	persistence_log_poll();
	if (transport_world_active())
	{
		transport_world_pump();
		ctx.connections_us =
			latency_trace_elapsed_us(ctx.loop_time_begin_us, loop_monotonic_us());
		return true;
	}

	if ((*ctx.last_desc_per_hour_reset + 3600) <= time(0))
	{
		max_descs_this_hour = used_descs;
		*ctx.last_desc_per_hour_reset = time(0);
	}
	char *buf = ctx.network_buffer;
	struct host_answer &host_ans_buf = *ctx.host_answer;
	bzero(&host_ans_buf, sizeof(host_ans_buf));
	for (P_desc point = descriptor_list, next_point; point; point = next_point)
	{
		next_point = point->next;
		if ((point->descriptor == host_ans_buf.desc) &&
		    !strncmp(host_ans_buf.addr, point->host /*+ 3 */, strlen(host_ans_buf.addr)))
		{
			/* we have a match! */
			strlcpy(point->host, host_ans_buf.name, sizeof point->host);

			/* site ban code, skip if address is junk */
			snprintf(buf, MAX_STRING_LENGTH, "%s\r\n", point->host);
			SEND_TO_Q(buf, point);
			if (bannedsite(point->host, 0) || bannedsite(host_ans_buf.addr, 0))
			{
				write_to_descriptor(
					point,
					"Your site has been banned from being able to connect to Duris.\r\n"
					"You were banned because someone at your site has flagrantly violated\r\n"
					"the rules to a point where banning your site was necessary.  If you\r\n"
					"feel this is in error, please e-mail multiplay@newduris.com\r\n");
				banlog(56, "Reject Connect from %s, banned site.", point->host);
				logit(LOG_STATUS, "Rejected Connect from %s, banned site.",
				      point->host);
				close_socket(point);
				continue;
			}
			else
			{
				/* good connection, send them on their way :) */
				SEND_TO_Q(
					"Please enter your term type (<CR> for ANSI, '1' for Generic, '3' for MSP markup, '9' for Quick, '?' for help): ",
					point);
				point->connected = CON_GET_TERM;
				point->wait = 1;
			}
		}
	}
	PROFILE_START(connections);
	const uint64_t connections_begin_us = loop_monotonic_us();
	const bool ready = service_network_turn(ctx, 0);
	// Application WebSocket actions and link-loss teardown retain the original
	// connection phase boundary. close_socket() advances next_to_process if an
	// application handler removes a later descriptor during reconnect/logout.
	// An interrupted poll leaves this work staged for a successful boundary.
	for (P_desc point = ready ? descriptor_list : NULL; point; point = next_to_process)
	{
		next_to_process = point->next;
		if (point->network_close_pending == 1)
			close_socket(point);
		else
		{
			if (point->network_close_pending == 2)
				point->network_close_pending = 1;
			point->network_input_remaining = point->websocket ? WS_INPUT_BUFFER_SIZE :
									    MAX_QUEUE_LENGTH - 1;
			if (point->websocket)
				websocket_dispatch_pending_input(point);
		}
	}
	ctx.connections_us = latency_trace_elapsed_us(connections_begin_us, loop_monotonic_us());
	latency_trace_record("connections", ctx.connections_us, ctx.loop_tick);
	PROFILE_END(connections);
	return ready;
}

static void run_session_input_phase(game_loop_pulse_context &ctx)
{
	const uint64_t loop_tick = ctx.loop_tick;
	const uint64_t loop_start_mono_us = ctx.loop_start_mono_us;
	char *comm = ctx.command_buffer;
	P_desc point;
	P_char t_ch;
	int player_count;
	command_latency_tracker &command_latency = ctx.command_latency;

	/* process_commands */
	PROFILE_START(commands);
	const uint64_t command_sweep_started_us = loop_monotonic_us();
	for (point = descriptor_list, player_count = 0; point; point = next_to_process)
	{
		next_to_process = point->next;
		t_ch = point->character;
		const bool authenticated_service = websocket_is_authenticated_service(point);

		if (point->connected == CON_SSLNEGO)
			continue;

		command_latency_event descriptor_event = {};
		prepare_descriptor_latency_event(&descriptor_event, COMMAND_LATENCY_DESCRIPTOR,
						 point, NULL);
		scoped_command_latency descriptor_latency(&command_latency, &descriptor_event);

		/* update max_users_playing for "who" information */
		if ((point->connected) == CON_PLAYING)
		{
			player_count++;
			if (player_count > max_users_playing)
				max_users_playing = player_count;
		}

		/* WebSocket handshake timeout is independent of connected state. */
		if (point->websocket && !point->ws_handshake_done &&
		    point->ws_handshake_started > 0)
		{
			time_t now = time(0);
			if (now - point->ws_handshake_started >= WS_HANDSHAKE_TIMEOUT)
			{
				statuslog(56, "WebSocket: Closing incomplete handshake from %s",
					  point->host);
				close_socket(point);
				continue;
			}
		}

		/* WebSocket ping/pong dead connection detection */
		if (point->websocket && !point->transport_session &&
		    point->ws_state == WS_STATE_OPEN)
		{
			time_t now = time(0);

			/* Check for ping timeout (no pong received) */
			if (point->ws_last_ping > 0 && point->ws_ping_outstanding &&
			    !point->ws_pong_received &&
			    (now - point->ws_last_ping) > WS_PING_TIMEOUT)
			{
				statuslog(
					56,
					"WebSocket: Closing dead connection from %s (ping timeout)",
					point->host);
				websocket_close(point, WS_CLOSE_GOING_AWAY, "Ping timeout");
				if (point->ws_state == WS_STATE_CLOSING)
					continue;
				close_socket(point);
				continue;
			}

			/* Send one periodic ping at a time.  A queued ping becomes outstanding only after control output drains. */
			if (!point->ws_ping_queued && !point->ws_ping_outstanding &&
			    (point->ws_last_ping == 0 ||
			     (now - point->ws_last_ping) >= WS_PING_INTERVAL))
			{
				if (websocket_send_ping(point) == 0)
				{
					if (point->ws_control_output_len == 0)
					{
						point->ws_last_ping = now;
						point->ws_pong_received = 0;
						point->ws_ping_outstanding = 1;
					}
					else
					{
						point->ws_ping_queued = 1;
					}
				}
			}
		}

		/* new timeout for non-playing sockets */

		if (point->connected && !authenticated_service)
		{
			point->wait++;

			switch (point->connected)
			{
				/* short protocol/login transitions retain a 60 second timeout */
			case CON_FLUSH:
			case CON_GET_TERM:
				if (point->wait > 240)
				{
					write_to_descriptor(point, "Idle Timeout\n");
					close_socket(point);
					continue;
				}
				break;

				/* slightly more involved, 10 minute timeout */
			case CON_ALIGN:
			case CON_BONUS1:
			case CON_BONUS2:
			case CON_BONUS3:
			case CON_HOMETOWN:
			case CON_NAME:
			case CON_PWD_CONF:
			case CON_PWD_D_CONF:
			case CON_PWD_GET:
			case CON_PWD_NO_CONF:
			case CON_PWD_NEW:
			case CON_PWD_GET_NEW:
			case CON_PWD_NORM:
			case CON_GET_CLASS:
			case CON_GET_RACE:
			case CON_REROLL:
			case CON_APPROPRIATE_NAME:
			case CON_NAME_CONF:
			case CON_GET_SEX:
				if (point->wait > 2400)
				{
					write_to_descriptor(point, "Idle Timeout\n");
					close_socket(point);
					continue;
				}
				break;
				/*
					 * for remaining states, 15 minutes, same as idle
					 * timeout in game
					 */
			default:
				if (point->wait > 3600)
				{
					write_to_descriptor(point, "Idle Timeout\n");
					close_socket(point);
					continue;
				}
				break;
			}
		}
		else if (!authenticated_service && t_ch && IS_AFFECTED2(t_ch, AFF2_SLOW) &&
			 (pulse % 2) && !GET_CLASS(t_ch, CLASS_MONK))
			continue;
		else if (!authenticated_service && t_ch && affected_by_spell(t_ch, TAG_CTF) &&
			 (pulse % (int)get_property("ctf.slowness", 3)))
			continue;

		/* Keep type-ahead queued until the worker's result has been applied
		 * on this thread. Completion never retains a descriptor pointer. */
		if (session_input_authentication_pending(point))
			continue;
		descriptor_latency.finish();
		repair_session_command_gate(t_ch);
		const session_input_route route = select_session_input(point, t_ch, comm);
		dispatch_session_input(point, t_ch, comm, route, &command_latency);
	}
	const uint64_t command_sweep_us =
		latency_trace_elapsed_us(command_sweep_started_us, loop_monotonic_us());
	PROFILE_END(commands);
	latency_trace_record("commands", command_sweep_us, loop_tick);
	command_latency_log_buffer command_report;
	command_report.length = 0;
	command_latency_report_throttled(&command_report_state, &command_latency, command_sweep_us,
					 latency_trace_boot_id(), loop_tick, loop_start_mono_us,
					 collect_command_latency_log, &command_report);
	if (command_report.length)
		logit(LOG_STATUS, "%s", command_report.text);

	ctx.command_sweep_us = command_sweep_us;
}

static void run_output_phase(game_loop_pulse_context &ctx)
{
	const uint64_t loop_tick = ctx.loop_tick;
	P_desc point, next_point;

	PROFILE_START(prompts);
	const uint64_t prompts_begin_us = loop_monotonic_us();
	for (point = descriptor_list; point; point = next_point)
	{
		next_point = point->next;

		/* Close at a safe loop boundary even when the socket is not writable
		 * or its transport queue is stalled. Enqueue never frees a descriptor. */
		if (point->output.overflowed || point->oob_input_overflowed)
		{
			logit(LOG_COMM, "Closing descriptor %d: application queue limit",
			      point->descriptor);
			if (point->websocket && point->ws_state == WS_STATE_OPEN)
				websocket_send_close(point, WS_CLOSE_POLICY_VIOLATION,
						     "Session queue limit");
			point->write_failed = 1;
			close_socket(point);
			continue;
		}
		// Application output stays at this boundary. Retained bytes must
		// drain first, following the transport/TLS readiness direction.
		if (point->connected == CON_SSLNEGO)
			continue;
		if (drain_network_transport(point) < 0)
		{
			close_socket(point);
			continue;
		}
		if (network_transport_pending(point))
			continue;

		if (process_output(point) < 0)
		{
			close_socket(point);
			continue;
		}
		if (point->websocket && !point->transport_session &&
		    websocket_flush_output(point) < 0)
		{
			close_socket(point);
			continue;
		}
		/* Logout must finish without requiring another command from the client. */
		if (point->connected == CON_FLUSH && !point->output.head &&
		    point->telnet_output_len == 0 && point->ws_output_len == 0 &&
		    point->ws_control_output_len == 0)
		{
			close_socket(point);
			continue;
		}
		if (point->websocket && point->ws_state == WS_STATE_OPEN && point->ws_ping_queued &&
		    point->ws_control_output_len == 0)
		{
			point->ws_ping_queued = 0;
			point->ws_ping_outstanding = 1;
			point->ws_pong_received = 0;
			point->ws_last_ping = time(0);
		}
		if (point->websocket && point->ws_state == WS_STATE_CLOSING &&
		    point->ws_output_len == 0 && point->ws_control_output_len == 0)
		{
			close_socket(point);
			continue;
		}
	}

	PROFILE_END(prompts);
	const uint64_t prompts_us = latency_trace_elapsed_us(prompts_begin_us, loop_monotonic_us());
	latency_trace_record("prompts", prompts_us, loop_tick);

	ctx.prompts_us = prompts_us;
	transport_world_finish_pulse();
}

static void log_telemetry_health_event(const telemetry_health_event &event)
{
	if (event.kind == telemetry_health_event_kind::none)
		return;
	char record_kinds[160]{};
	char reason_flags[160]{};
	const std::uint32_t failure_reasons = TELEMETRY_HEALTH_REASON_WRITER_DEGRADED |
					      TELEMETRY_HEALTH_REASON_CIRCUIT_OPEN |
					      TELEMETRY_HEALTH_REASON_PERMANENT_FAILURE |
					      TELEMETRY_HEALTH_REASON_RECOVERY_PENDING;
	const std::uint64_t kind_mask = event.health.inflight_active != 0U ?
						event.health.inflight_record_kind_mask :
					(event.reason_mask & failure_reasons) != 0U ?
						event.health.last_failure_record_kind_mask :
						0U;
	(void)telemetry_health_record_kind_mask_format(kind_mask, record_kinds,
						       sizeof(record_kinds));
	(void)telemetry_health_reason_mask_format(event.reason_mask, reason_flags,
						  sizeof(reason_flags));
	logit(LOG_STATUS,
	      "telemetry_health event=%s severity=%s reasons=%u reason_flags=%s state=%s "
	      "previous_state=%s "
	      "backend=%s schema=%u producer=%llu:%llu last_admitted_seq=%llu "
	      "last_committed_seq=%llu last_commit_monotonic_us=%llu last_commit_age_known=%u "
	      "last_commit_age_us=%llu failure_class=%s error=%u "
	      "last_failure_monotonic_us=%llu last_failure_age_known=%u last_failure_age_us=%llu "
	      "admitted=%llu/%llu applied=%llu duplicate=%llu stale=%llu invalid=%llu conflict=%llu "
	      "dropped=%llu/%llu queue=%llu/%u high_water=%llu inflight=%u "
	      "inflight_seq=%llu-%llu record_kinds=%s retries=%u/%u backoff_us=%llu "
	      "advisory_lock=%s gaps=%llu unclosed_tails=%llu quarantined=%llu "
	      "affected_producer=%llu:%llu affected_seq=%llu-%llu duration_us=%llu",
	      telemetry_health_event_kind_name(event.kind),
	      telemetry_health_alert_severity_name(event.severity), event.reason_mask, reason_flags,
	      telemetry_health_state_name(event.current_state),
	      telemetry_health_state_name(event.previous_state),
	      telemetry_health_backend_name(event.health.backend), event.health.schema_version,
	      (unsigned long long)event.health.producer.boot_id,
	      (unsigned long long)event.health.producer.process_id,
	      (unsigned long long)event.health.last_admitted_record_seq,
	      (unsigned long long)event.health.last_committed_record_seq,
	      (unsigned long long)event.health.last_success_monotonic_usec,
	      event.last_commit_age_available, (unsigned long long)event.last_commit_age_usec,
	      telemetry_health_failure_class_name(event.health.last_failure_class),
	      event.health.last_error_code,
	      (unsigned long long)event.health.last_failure_monotonic_usec,
	      event.last_failure_age_available, (unsigned long long)event.last_failure_age_usec,
	      (unsigned long long)event.health.admitted_detail,
	      (unsigned long long)event.health.admitted_control,
	      (unsigned long long)event.health.applied_records,
	      (unsigned long long)event.health.duplicate_records,
	      (unsigned long long)event.health.stale_checkpoint_records,
	      (unsigned long long)event.health.invalid_records,
	      (unsigned long long)event.health.conflict_records,
	      (unsigned long long)event.health.dropped_detail,
	      (unsigned long long)event.health.dropped_control,
	      (unsigned long long)event.health.queue_depth, event.health.queue_capacity,
	      (unsigned long long)event.health.queue_high_water, event.health.inflight_active,
	      (unsigned long long)event.health.inflight_first_record_seq,
	      (unsigned long long)event.health.inflight_last_record_seq, record_kinds,
	      event.health.inflight_retry_attempts, event.health.repository_retry_attempts,
	      (unsigned long long)event.health.retry_backoff_remaining_usec,
	      telemetry_health_advisory_lock_name(event.health.advisory_lock_state),
	      (unsigned long long)event.health.sequence_gap_count,
	      (unsigned long long)event.health.unclosed_tail_count,
	      (unsigned long long)event.health.quarantined_records,
	      (unsigned long long)event.affected_producer.boot_id,
	      (unsigned long long)event.affected_producer.process_id,
	      (unsigned long long)event.affected_first_record_seq,
	      (unsigned long long)event.affected_last_record_seq,
	      (unsigned long long)event.alert_duration_usec);
}

static void run_event_phase(game_loop_pulse_context &ctx)
{
	const uint64_t loop_tick = ctx.loop_tick;

	/* handle heartbeat stuff */
	/* ne_events() closes the current tick's pre-event scheduling phase. */
	const uint64_t ne_events_begin_us = loop_monotonic_us();
	ne_events();
	const uint64_t ne_events_us =
		latency_trace_elapsed_us(ne_events_begin_us, loop_monotonic_us());
	latency_trace_record("ne_events", ne_events_us, loop_tick);
	telemetry_monotonic_usec telemetry_pulse_now = 0U;
	telemetry_utc_usec telemetry_pulse_utc = TELEMETRY_UTC_UNKNOWN;
	if (telemetry_runtime_now(&telemetry_pulse_now, &telemetry_pulse_utc))
	{
		const std::uint16_t telemetry_slots = telemetry_runtime_pulse_slot_count();
		telemetry_pulse_request telemetry_request{};
		telemetry_request.now_monotonic_usec = telemetry_pulse_now;
		telemetry_request.occurrence_utc_usec = telemetry_pulse_utc;
		telemetry_request.slot = static_cast<std::uint16_t>(
			static_cast<unsigned int>(pulse) % telemetry_slots);
		(void)telemetry_runtime_pulse(telemetry_request);
		log_telemetry_health_event(telemetry_runtime_health_observe(telemetry_pulse_now));
	}

	item_creation_grant_prepare_pulse();
	item_movement_transaction_drop_prepare_pulse();
	artifact_mana_pulse();
	device_actions_pulse();

	ctx.ne_events_us = ne_events_us;
}

// Existing bounded native publishers, shared by the regular pulse and
// lifecycle drain. No session input, new command or general game pulse runs.
static void critical_gameplay_publish_outbox()
{
	if (!nevent_is_game_thread())
		return;
	auction_transaction_publish_outbox();
	corpse_lifecycle_transaction_publish_outbox();
	collector_transaction_publish_outbox();
	combat_outcome_transaction_publish_outbox();
	artifact_guild_transaction_publish_outbox();
}

static void run_recurring_persistence_phase(game_loop_pulse_context &ctx)
{
	const uint64_t loop_tick = ctx.loop_tick;

	/* Flush dirty room GMCP updates every 2 pulses (~500ms) */
	if (!(pulse % 2))
	{
		const uint64_t gmcp_begin_us = loop_monotonic_us();
		gmcp_flush_dirty_rooms();
		gmcp_flush_dirty_ship_contacts();
		gmcp_flush_dirty_ship_info();
		flush_pending_ship_saves();
		locker_async_pulse();
		corpse_lifecycle_transaction_pulse();
		shop_trade_preparation_owner::pulse();
		auction_native_publication_pulse();
		shop_trade_transaction_restore_pulse();
		quest_mobile_native_birth_pulse(true);
		zone_reset_room_item_pulse(true);
		critical_completion critical_completions[64] = {};
		const size_t critical_completion_count =
			critical_command_coordinator_pulse(critical_completions, 64);
		critical_gameplay_handle_completions(critical_completions,
						     critical_completion_count);
		quest_reward_ack_pipeline_pulse();
		critical_gameplay_publish_outbox();
		for (size_t index = 0; index < critical_completion_count; ++index)
			if (critical_completions[index].outcome ==
				    critical_apply_outcome::terminal_failure ||
			    ((critical_completions[index].outcome ==
				      critical_apply_outcome::retryable_failure ||
			      critical_completions[index].outcome ==
				      critical_apply_outcome::ambiguous_commit) &&
			     critical_completions[index].attempt >
				     CRITICAL_COORDINATOR_MAX_RETRIES))
			{
				char summary[256] = {};
				snprintf(summary, sizeof(summary),
					 "correlation=%s error=%u refusal=%s attempts=%u",
					 critical_completions[index].recovery_correlation[0] ?
						 critical_completions[index]
							 .recovery_correlation.data() :
						 "none",
					 critical_completions[index].error_code,
					 death_recovery_refusal_name(
						 critical_completions[index].error_code),
					 critical_completions[index].attempt);
				persistence_alert(
					AVATAR, "critical_command", "completion", "none",
					critical_failure_stage_name(
						critical_completions[index].failure_stage),
					"integrity_failure",
					death_recovery_literal_detail(summary));
			}
		player_save_pipeline_pulse();
		quest_reward_recovery_pulse();
		persistence_pulse_character_saves();
		death_extract_retry_pulse();
		player_load_result load_completions[32] = {};
		const size_t load_completion_count =
			player_load_pipeline_pulse(load_completions, 32);
		for (size_t index = 0; index < load_completion_count; ++index)
		{
			bool delivered = false;
			for (P_desc descriptor = descriptor_list; descriptor;
			     descriptor = descriptor->next)
				if (descriptor->player_load_request_id ==
				    load_completions[index].request_id)
				{
					if (descriptor->player_load_mode == PLAYER_LOAD_MODE_LEGACY)
						nanny_player_load_complete(
							descriptor,
							std::move(load_completions[index]));
					else
						account_player_load_complete(
							descriptor,
							std::move(load_completions[index]));
					delivered = true;
					break;
				}
			if (!delivered)
				player_load_pipeline_note_stale();
		}
		information_cache_pulse();
		help_cache_pulse();
		collector_catalog_cache_pulse();
		collector_maintenance_pulse();
		collector_presence_pulse();
		collector_service_pulse();
		account_recovery_pulse();
		redis_world_recovery_pulse();
		latency_trace_record("gmcp_flush",
				     latency_trace_elapsed_us(gmcp_begin_us, loop_monotonic_us()),
				     loop_tick);
	}
	maintenance_result maintenance_results[MAINTENANCE_COMPLETION_MAX] = {};
	const size_t maintenance_count = maintenance_scheduler_pulse(
		ne_event_tick, maintenance_results, MAINTENANCE_COMPLETION_MAX);
	maintenance_handle_completions(maintenance_results, maintenance_count);
}

static void run_activity_phase(game_loop_pulse_context &ctx)
{
	const uint64_t loop_tick = ctx.loop_tick;

	PROFILE_START(activities);
	const uint64_t activities_begin_us = loop_monotonic_us();
	if (maintenance_activity_due(ne_event_tick, WAIT_SEC, 1))
		ship_activity();

	if (!no_ferries && maintenance_activity_due(ne_event_tick, WAIT_SEC, 2))
		ferry_activity();

	if (maintenance_activity_due(ne_event_tick, WAIT_SEC * 120, 3))
		spawn_random_mapmob();

	//    if (!(pulse % WAIT_SEC))
	//      arena_activity();

	if (maintenance_activity_due(ne_event_tick, SHORT_AFFECT, 4))
		short_affect_update();

	if (maintenance_activity_due(ne_event_tick, WAIT_SEC * 300, 5))
		wimps_in_approve_queue();

	PROFILE_END(activities);
	const uint64_t activities_us =
		latency_trace_elapsed_us(activities_begin_us, loop_monotonic_us());
	latency_trace_record("activities", activities_us, loop_tick);

	ctx.activities_us = activities_us;
}

static void run_combat_phase(game_loop_pulse_context &ctx)
{
	const uint64_t loop_tick = ctx.loop_tick;
	P_desc point;
	P_char t_ch;

	PROFILE_START(combat);
	const uint64_t combat_begin_us = loop_monotonic_us();
	perform_violence();

	/* for action_delays[] related to combat --TAM 04/19/94 */
	for (point = descriptor_list; point; point = point->next)
	{
		if (point->character && point->connected == CON_PLAYING)
		{
			t_ch = point->character;

			if (!pulse)
			{
				if (IS_SET(t_ch->specials.act2, PLR2_HINT_CHANNEL))
				{
					tossHint(t_ch);
				}
			}
			if (t_ch->desc && t_ch->desc->last_map_update)
			{
				// For ship passengers: GMCP only (handler.c already filters to GMCP-enabled only)
				if (IS_SHIP_ROOM(t_ch->in_room))
				{
					if (GMCP_ENABLED(t_ch))
					{
						P_ship ship = get_ship_from_char(t_ch);
						if (ship && IS_MAP_ROOM(ship->location))
						{
							int n = map_view_distance(t_ch,
										  ship->location);
							if (n > 1)
							{
								// Render map and send via GMCP only (skip text by using websocket flag temporarily)
								bool was_websocket =
									t_ch->desc->websocket;
								t_ch->desc->websocket =
									1; // Force skip_text_output in display_map_room
								display_map_room(t_ch,
										 ship->location, n,
										 MAP_AUTOMAP, 0);
								t_ch->desc->websocket =
									was_websocket;
							}
						}
					}
				}
				else
				{
					map_look(t_ch, MAP_AUTOMAP);
				}
				t_ch->desc->last_map_update = 0;
			}
			if (t_ch->desc && t_ch->desc->last_group_update)
			{
				/* For GMCP clients, send structured data to group panel */
				if (GMCP_ENABLED(t_ch))
				{
					gmcp_send_group_status(t_ch);
				}
				/* For MSP clients, display text group output */
				if (t_ch->desc->term_type == TERM_MSP)
				{
					do_group(t_ch, writable_arg(""), 0);
				}
				t_ch->desc->last_group_update = 0;
			}
			if (t_ch->points.delay_move > 0)
				t_ch->points.delay_move -= BOUNDED(
					0,
					!IS_MAP_ROOM(t_ch->in_room) ? move_regen(t_ch, FALSE) :
								      move_regen(t_ch, FALSE) / 2,
					t_ch->points.delay_move);
		}
	}
	//      }
	PROFILE_END(combat);
	const uint64_t combat_us = latency_trace_elapsed_us(combat_begin_us, loop_monotonic_us());
	latency_trace_record("combat", combat_us, loop_tick);

	ctx.combat_us = combat_us;
}

static void run_pulse_reset_phase(game_loop_pulse_context &ctx)
{
	const uint64_t loop_time_begin_us = ctx.loop_time_begin_us;
	const uint64_t loop_tick = ctx.loop_tick;
	const uint64_t loop_start_mono_us = ctx.loop_start_mono_us;
	const uint64_t connections_us = ctx.connections_us;
	const uint64_t command_sweep_us = ctx.command_sweep_us;
	const uint64_t prompts_us = ctx.prompts_us;
	const uint64_t ne_events_us = ctx.ne_events_us;
	const uint64_t activities_us = ctx.activities_us;
	const uint64_t combat_us = ctx.combat_us;

	PROFILE_START(pulse_reset);
	// tics since last checkpoint signal
	tics = tics + 1;
	if (tics > static_cast<sig_atomic_t>(BIT_30))
	{
		tics = 1;
		debug("Huge value for tics, resetting to 1.");
		logit(LOG_SYS, "Huge value for tics, resetting to 1.");
	}
	nevent_advance_tick();
	const uint64_t affect_and_points_begin_us = loop_monotonic_us();
	uint64_t affect_us = 0;
	uint64_t point_us = 0;
	if (!pulse)
	{
		affect_update();
		const uint64_t affect_end_us = loop_monotonic_us();
		point_update();
		const uint64_t point_end_us = loop_monotonic_us();
		affect_us = latency_trace_elapsed_us(affect_and_points_begin_us, affect_end_us);
		point_us = latency_trace_elapsed_us(affect_end_us, point_end_us);
	}
	const uint64_t affect_and_points_us =
		latency_trace_elapsed_us(affect_and_points_begin_us, loop_monotonic_us());
	latency_trace_record("affect_and_points", affect_and_points_us, loop_tick);
	latency_trace_record("affect_update", affect_us, loop_tick);
	latency_trace_record("point_update", point_us, loop_tick);
	/* check out the time */
	const uint64_t loop_us = latency_trace_elapsed_us(loop_time_begin_us, loop_monotonic_us());
	if (loop_us != LATENCY_TRACE_DURATION_INVALID && loop_us >= 250000) // 4 ticks a sec
	{
		char tick_buffer[LATENCY_TRACE_TICK_STRING_LENGTH];
		char duration_buffers[9][LATENCY_TRACE_TICK_STRING_LENGTH];
		statuslog(
			56,
			"MUD TICK TOOK TOO LONG - loop time - %f: boot=%s tick=%s pulse_start_mono_us=%" PRIu64
			" connections_us=%s"
			" activities_us=%s"
			" combat_us=%s"
			" commands_us=%s"
			" ne_events_us=%s"
			" prompts_us=%s"
			" affect_and_points_us=%s"
			" affect_update_us=%s"
			" point_update_us=%s",
			(double)loop_us / 1000000.0, latency_trace_boot_id(),
			latency_trace_format_tick(loop_tick, tick_buffer), loop_start_mono_us,
			latency_trace_format_duration(connections_us, duration_buffers[0]),
			latency_trace_format_duration(activities_us, duration_buffers[1]),
			latency_trace_format_duration(combat_us, duration_buffers[2]),
			latency_trace_format_duration(command_sweep_us, duration_buffers[3]),
			latency_trace_format_duration(ne_events_us, duration_buffers[4]),
			latency_trace_format_duration(prompts_us, duration_buffers[5]),
			latency_trace_format_duration(affect_and_points_us, duration_buffers[6]),
			latency_trace_format_duration(affect_us, duration_buffers[7]),
			latency_trace_format_duration(point_us, duration_buffers[8]));
	}
	latency_trace_record("total_tick", loop_us, loop_tick);
	if (!(tics % 300))
	{
		world_activity_log_diagnostics();
		latency_trace_snapshot snapshot = {};
		latency_trace_snapshot_take_and_reset(&snapshot);
		FILE *_ltf = fopen("logs/latency_trace.log", "a");
		if (_ltf)
		{
			latency_trace_snapshot_dump(_ltf, &snapshot);
			fclose(_ltf);
		}
		else
			statuslog(56,
				  "LATENCY TRACE: could not open logs/latency_trace.log: errno=%d",
				  errno);
		latency_trace_snapshot_dump(stderr, &snapshot);
		fflush(stderr);
	}
	// Pace from the monotonic pulse start. On an overrun discard missed wall
	// slots and allow a full interval before the next logical simulation tick.
	// Network readiness and completion hints wake this wait without advancing
	// commands, combat, world events, or durable-completion publication.
	network_wait_until(ctx, network_next_pulse_us(loop_time_begin_us, loop_monotonic_us()));
	PROFILE_END(pulse_reset);

	ctx.affect_us = affect_us;
	ctx.point_us = point_us;
	ctx.loop_us = loop_us;
}

static void refuse_lifecycle_for_active_cutover_owner(const char *operation)
{
	logit(LOG_STATUS, "Refusing %s while a critical SQL cutover owner retains its session.",
	      operation);
	persistence_alert(AVATAR, "critical_command", operation, "none", "none",
			  "lifecycle_refused",
			  "shutdown_cancelled=1 retry_after_owner_terminal_cleanup=1");
	for (P_desc pending_desc = descriptor_list; pending_desc; pending_desc = pending_desc->next)
		if (pending_desc->descriptor > 0 && pending_desc->connected == CON_PLAYING)
			write_to_descriptor(
				pending_desc,
				"\r\nShutdown/copyover cancelled: an economic SQL cutover still owns its session. "
				"Retry after terminal cleanup.\r\n");
	shutdownflag = 0;
	_reboot = 0;
	_copyover = 0;
	_autoboot = 0;
	shutdownData.eShutdownType = TimedShutdownData::NONE;
}

/**
 * Run network and simulation pulses, including persistence deadlines
 * independent of world-event debt.
 */
void game_loop(int port, int sslport)
{
	char buf[MAX_STRING_LENGTH];
	char comm[MAX_INPUT_LENGTH];
	P_desc point;
	struct host_answer host_ans_buf;
	int s, S;
	int WS; /* WebSocket listener socket */
	bool copyover_recovered = false;
	int accept_debug = getenv("DURIS_ACCEPT_DEBUG") != NULL;
	unsigned long accept_debug_pulse = 0;

	sentbytes = 0;
	receivedbytes = 0;
	if (network_wakeup_fd() < 0)
		fatal_boot_error("comm", "Could not initialize network completion wakeups");

	avail_descs = MAX_CONNECTIONS;

	snprintf(buf, MAX_STRING_LENGTH, "avail_descs set to: %d", avail_descs);
	logit(LOG_STATUS, "%s", buf);

	dead_desc_pool = mm_create("SOCKET", sizeof(struct descriptor_data),
				   offsetof(struct descriptor_data, next),
				   mm_find_best_chunk(sizeof(struct descriptor_data), 25, 110));
	if (!transport_world_boot(copyover_boot != 0))
		fatal_boot_error("transport", "World transport handshake failed");

	// copyover recovery - pool must exist first
	if (copyover_boot)
	{
		copyover_recovered = copyover_recover(
			&recovered_mother_desc, &recovered_mother_desc_ssl, &recovered_ws_desc);
		if (copyover_recovered)
		{
			copyover_restore_combat();
			// recalculate avg mob level now that mobs are restored
			calc_zone_mob_level();
		}
	}

	// redis crash recovery - restore world state from redis snapshot
	if (redis_world_recovery_boot_active())
	{
		logit(LOG_STATUS, "Performing redis %s recovery...",
		      redis_world_clean_restart_boot() ? "clean restart" : "crash");
		if (redis_load_world_state())
		{
			copyover_restore_combat(); // reuse combat restoration logic
			calc_zone_mob_level();
			logit(LOG_STATUS, "%s recovery complete",
			      redis_world_clean_restart_boot() ? "Clean restart" : "Crash");
			if (!redis_consume_world_state())
				logit(LOG_STATUS,
				      "Recovered Redis generation could not be consumed safely");
		}
		else
		{
			logit(LOG_STATUS, "%s recovery failed; applying full normal zone boot",
			      redis_world_clean_restart_boot() ? "Clean restart" : "Crash");
			for (int zone = 0; zone <= top_of_zone_table; ++zone)
				reset_zone(zone, 2);
			if (!load_moonstone_fragments())
				logit(LOG_FILE, "Error initializing automatons quest!\r\n");
			/* boot_db() deferred legacy ground artifacts to Redis. Restore the SQL
			 * fallback now that the recovery generation could not be materialized. */
			if (!mini_mode)
				addOnGroundArtis_sql();
		}
		redis_world_recovery_boot_clear();
		// Enable the registry-owned world-state job now that recovery is done.
		const nevent_periodic_result world_state_job =
			nevent_periodic_set_enabled("world-state-save", true, 30 * WAIT_SEC);
		if (world_state_job != nevent_periodic_result::enabled)
			logit(LOG_EXIT,
			      "NEVENT PERIODIC: could not enable world-state-save after recovery (status=%u)",
			      static_cast<unsigned int>(world_state_job));
	}

	// Materialize missing transports only after recovery (or its cold-boot
	// fallback). Reconcile legacy duplicated generations before world ticks.
	if (!mini_mode)
	{
		reconcile_shopkeepers(copyover_boot != 0);
		initialize_transport();
	}

	/* Rebuild once after boot/recovery so delayed work never depends on stale
	 * room, corpse, or character indexes from a prior process. */
	world_activity_rebuild();

	PROFILES(RESET);
#ifdef DO_PROFILE
	init_func_call_info();
#endif

	// Decide the original failed-copyover case before taking the world cut or
	// opening listeners. A successful transport-owned copyover needs no native
	// recovered listener and must retain the original first-branch behavior.
	if (copyover_boot &&
	    !(transport_world_active() && (!copyover_boot || copyover_recovered)) &&
	    recovered_mother_desc < 0)
	{
		/* The inherited listeners are still open, so this process cannot safely
		 * bind replacements.  Return through normal shutdown to join every worker;
		 * exit(1) here left joinable std::threads and turned a rejected copyover into
		 * SIGABRT plus a core dump.  The supervisor then performs a cold restart. */
		logit(LOG_STATUS,
		      "FATAL: copyover recovery failed; requesting graceful cold restart");
		_reboot = 1;
		return;
	}

	// Real successful initialization: copyover/Redis fallback, shop/transport
	// reconciliation and the failed-copyover return are all behind this point.
	// Early SQL/native promotion remains intact; this owner reacquires genuine
	// selected exclusion and captures only AFTER original drain callbacks settle.
	if (!economic_initialized_world_owner::complete_boot())
	{
		logit(LOG_STATUS,
		      "FATAL: initialized accounting world capture refused; requesting graceful cold restart");
		_reboot = 1;
		return;
	}
	// The root's synchronous SQL/independent qualification consumer belongs
	// inside the held boot-cut interval. Raw census alone never selects an epoch.
	// Invalidate before transport readiness, replay/native callbacks or input.
	if (!economic_initialized_world_owner::before_world_callbacks())
	{
		logit(LOG_STATUS,
		      "FATAL: initialized accounting world exclusion unresolved; requesting graceful cold restart");
		_reboot = 1;
		return;
	}

	// use recovered sockets if copyover, otherwise create new ones
	if (transport_world_active() && (!copyover_boot || copyover_recovered))
	{
		s = S = WS = -1;
	}
	else if (copyover_boot && recovered_mother_desc >= 0)
	{
		logit(LOG_STATUS, "Using recovered sockets from copyover");
		s = recovered_mother_desc;
		S = recovered_mother_desc_ssl;
		WS = recovered_ws_desc;
	}
	else if (copyover_boot)
	{
		/* The inherited listeners are still open, so this process cannot safely
		 * bind replacements.  Return through normal shutdown to join every worker;
		 * exit(1) here left joinable std::threads and turned a rejected copyover into
		 * SIGABRT plus a core dump.  The supervisor then performs a cold restart. */
		logit(LOG_STATUS,
		      "FATAL: copyover recovery failed; requesting graceful cold restart");
		_reboot = 1;
		return;
	}
	else
	{
		logit(LOG_STATUS, "Opening mother connection.");
		s = init_socket(port);
		logit(LOG_STATUS, "Opening father connection.");
		S = init_socket(sslport);
		logit(LOG_STATUS, "Opening WebSocket connection.");
		WS = websocket_init(WS_PORT);
		if (WS < 0)
		{
			logit(LOG_STATUS, "WARNING: WebSocket server failed to start on port %d",
			      WS_PORT);
		}
	}

	// store in file-scope statics for copyover access
	mother_desc = s;
	mother_desc_ssl = S;
	ws_desc = WS;
	copyover_boot = 0;
	copyover_clear_boot();
	transport_world_ready();
	for (P_desc receipt_desc = descriptor_list; receipt_desc; receipt_desc = receipt_desc->next)
		if (receipt_desc->character && receipt_desc->connected == CON_PLAYING)
			locker_identify_replay(receipt_desc->character);

	long last_desc_per_hour_reset = time(0);
	/* Main loop */
resume_game_loop:
	// A refused lifecycle operation returns to a bounded running deadline.
	// This does not advance the completed-loop counter.
	game_loop_watchdog_lifecycle('R');
	while (!shutdownflag)
	{
		const uint64_t loop_time_begin_us = loop_monotonic_us();
		const uint64_t loop_tick = (uint64_t)ne_event_tick;
		const uint64_t loop_start_mono_us = loop_time_begin_us;
		latency_trace_begin_pulse(loop_tick, loop_start_mono_us);

		game_loop_pulse_context context{};
		context.telnet_listener = s;
		context.ssl_listener = S;
		context.websocket_listener = WS;
		context.network_buffer = buf;
		context.command_buffer = comm;
		context.host_answer = &host_ans_buf;
		context.accept_debug_pulse = &accept_debug_pulse;
		context.last_desc_per_hour_reset = &last_desc_per_hour_reset;
		context.accept_debug = accept_debug;
		context.loop_time_begin_us = loop_time_begin_us;
		context.loop_tick = loop_tick;
		context.loop_start_mono_us = loop_start_mono_us;

		if (!run_connection_phase(context))
			continue;
		run_session_input_phase(context);
		run_output_phase(context);
		run_event_phase(context);
		run_recurring_persistence_phase(context);
		run_activity_phase(context);
		run_combat_phase(context);
		run_pulse_reset_phase(context);
		game_loop_watchdog_completed();
	}

	// Cover the entire save/drain chain, including early refusal paths.
	game_loop_watchdog_lifecycle(_copyover ? 'C' :
						 (_reboot || _autoboot || _pwipe ? 'D' : 'S'));
	if (_copyover)
	{
		if (player_save_execution_guard::current_ownership_epoch())
		{
			persistence_alert(AVATAR, "player_save", "copyover", "none", "none",
					  "owned_lifecycle_pending", "copyover_cancelled=1");
			shutdownflag = 0;
			_reboot = 0;
			_copyover = 0;
			_autoboot = 0;
			goto resume_game_loop;
		}
		if (!critical_command_coordinator_try_acquire_lifecycle_guard())
		{
			refuse_lifecycle_for_active_cutover_owner("copyover");
			goto resume_game_loop;
		}
		/* Flush dirty realm records (harvested deposits) before the exec.
		 * There are two distinct kingdom flush paths, on purpose:
		 *   1. Normal shutdown: game_loop() returns and main() runs
		 *      kingdom_shutdown(), which flushes and then tears down.
		 *   2. Copyover: a successful copyover_save() execs the new binary
		 *      and never returns, so path 1 is never reached -- the flush
		 *      must happen HERE, beside the other pre-exec saves.
		 * Only the flush, not kingdom_shutdown(): if copyover_save()
		 * fails we resume the game loop below, and the shutdown's guard
		 * despawn and index clear would leave the live game with a dead
		 * kingdom subsystem. The flush is idempotent (it only writes
		 * realms still marked dirty), so the eventual kingdom_shutdown()
		 * after a failed copyover re-flushes nothing. */
		kingdom_flush_persistent_state();
		if (!copyover_save(s, S, WS))
		{
			transport_world_abort();
			persistence_alert(AVATAR, "player_save", "copyover", "none", "none",
					  "terminal_save_failed", "shutdown_cancelled=1");
			shutdownflag = 0;
			_reboot = 0;
			_copyover = 0;
			_autoboot = 0;
			critical_command_coordinator_release_lifecycle_guard();
			goto resume_game_loop;
		}
		return;
	}

	if (!_pwipe && item_creation_grant_batches_pending())
	{
		persistence_alert(AVATAR, "starter_grant", "shutdown", "none", "none",
				  "kit_pending",
				  "shutdown_cancelled=1 retry_after_kit_completion=1");
		shutdownflag = 0;
		_reboot = 0;
		_autoboot = 0;
		goto resume_game_loop;
	}

	if (!critical_command_coordinator_try_acquire_lifecycle_guard())
	{
		refuse_lifecycle_for_active_cutover_owner("shutdown");
		goto resume_game_loop;
	}
	critical_command_coordinator_quiesce();
	critical_outbox_quiesce();
	if (!_pwipe && !critical_command_coordinator_drain(3000))
	{
		critical_command_coordinator_resume();
		critical_outbox_resume();
		persistence_alert(AVATAR, "critical_command", "shutdown", "none", "none",
				  "pipeline_drain_failed", "shutdown_cancelled=1");
		shutdownflag = 0;
		_reboot = 0;
		_autoboot = 0;
		critical_command_coordinator_release_lifecycle_guard();
		goto resume_game_loop;
	}
	if (!_pwipe && !critical_outbox_drain(3000))
	{
		critical_command_coordinator_resume();
		critical_outbox_resume();
		persistence_alert(AVATAR, "critical_outbox", "shutdown", "none", "none",
				  "pipeline_drain_failed", "shutdown_cancelled=1");
		shutdownflag = 0;
		_reboot = 0;
		_autoboot = 0;
		critical_command_coordinator_release_lifecycle_guard();
		goto resume_game_loop;
	}
	if (!_pwipe && !persistence_save_all_characters_terminal(RENT_CRASH))
	{
		critical_command_coordinator_resume();
		critical_outbox_resume();
		persistence_alert(AVATAR, "player_save", "shutdown", "none", "none",
				  "terminal_save_failed", "shutdown_cancelled=1");
		for (P_desc pending_desc = descriptor_list; pending_desc;
		     pending_desc = pending_desc->next)
			if (pending_desc->descriptor > 0 && pending_desc->connected == CON_PLAYING)
				write_to_descriptor(
					pending_desc,
					"\r\nShutdown cancelled because a character save failed.\r\n");
		shutdownflag = 0;
		_reboot = 0;
		_autoboot = 0;
		critical_command_coordinator_release_lifecycle_guard();
		goto resume_game_loop;
	}
	if (!_pwipe && !(player_save_execution_guard::current_ownership_epoch() ?
				 player_save_pipeline_drain_owned(3000) :
				 player_save_pipeline_drain(3000)))
	{
		critical_command_coordinator_resume();
		critical_outbox_resume();
		player_save_pipeline_resume();
		persistence_alert(AVATAR, "player_save", "shutdown", "none", "none",
				  "pipeline_drain_failed", "shutdown_cancelled=1");
		shutdownflag = 0;
		_reboot = 0;
		_autoboot = 0;
		critical_command_coordinator_release_lifecycle_guard();
		goto resume_game_loop;
	}
	if (!_pwipe && !redis_world_recovery_drain(3000))
	{
		critical_command_coordinator_resume();
		critical_outbox_resume();
		player_save_pipeline_resume();
		persistence_alert(AVATAR, "world_recovery", "shutdown", "none", "none",
				  "pipeline_drain_failed", "shutdown_cancelled=1");
		shutdownflag = 0;
		_reboot = 0;
		_autoboot = 0;
		critical_command_coordinator_release_lifecycle_guard();
		goto resume_game_loop;
	}
	if (!_pwipe && !save_dirty_shopkeepers(true))
	{
		/* Dirty shopkeeper state is authoritative inventory.  Do not extract
		 * characters or tear down services while a forced save is unresolved. */
		critical_command_coordinator_resume();
		critical_outbox_resume();
		player_save_pipeline_resume();
		persistence_alert(AVATAR, "shopkeeper_save", "shutdown", "none", "none",
				  "dirty_save_failed", "shutdown_cancelled=1");
		shutdownData.eShutdownType = TimedShutdownData::NONE;
		for (P_desc pending_desc = descriptor_list; pending_desc;
		     pending_desc = pending_desc->next)
			if (pending_desc->descriptor > 0 && pending_desc->connected == CON_PLAYING)
				write_to_descriptor(
					pending_desc,
					"\r\nShutdown cancelled because shopkeeper inventory could not be saved.\r\n");
		shutdownflag = 0;
		_reboot = 0;
		_autoboot = 0;
		critical_command_coordinator_release_lifecycle_guard();
		goto resume_game_loop;
	}

	PROFILES(SAVE);
#ifdef DO_PROFILE
	save_func_call_info();
#endif

	// Don't want to save stuff just after we wiped all the tables in SQL.
	if (!_pwipe)
	{
		flush_pending_ship_saves();
		locker_async_drain(2000);

		if (no_ferries == 0)
		{
			shutdown_ferries();
		}

		shutdown_ships();

		shutdown_auction_houses();

		Guildhall::shutdown();
	}

	// skip character extraction during copyover - we need them intact
	if (!_copyover && !_pwipe)
	{
		for (point = descriptor_list; point; point = point->next)
		{
			if (point->character)
			{
				/* check for CON_PLAYING before extracting char. -DCL */
				if (point->connected == CON_PLAYING)
				{
					/* when you extract_char() a morph, it un_morph's first, which
					   results in another save.  Unfortunatly, the save_silent(...3)
					   has already nuked all the eq...  so.. just un_morph() them
					   before the save_silent */
					if (IS_MORPH(point->character))
					{
						if (IS_FIGHTING(point->character))
							stop_fighting(point->character);
						un_morph(point->character);
					}
					if (shutdown_message)
					{
						write_to_descriptor(point, shutdown_message);
					}
					// If it's not an immortal.
					if (GET_LEVEL(point->character) < MINLVLIMMORTAL)
					{
						update_ingame_racewar(
							-GET_RACEWAR(point->character));
					}
					extract_char(point->character);
				}
			}
		}
	}

	close_sockets(s);
	if (S >= 0)
		close(S);
	websocket_shutdown();
	mother_desc = -1;
	mother_desc_ssl = -1;
	ws_desc = -1;
}

/*
 * ****************************************************************** *
 * general utility stuff (for local use)
 *                                    *
 * ******************************************************************
 */

/** Remove the queue head and clear the tail when the queue becomes empty. */
int get_from_q(struct txt_q *queue, char *dest)
{
	struct txt_block *tmp;

	/* hmm, could it be this simple? JAB */
	if (!queue)
	{
		logit(LOG_COMM, "call to get_from_q with NULL queue");
		return (0);
	}
	if (!dest)
	{
		logit(LOG_COMM, "call to get_from_q with bogus string");
		return (0);
	}
	/*
	 * Q empty?
	 */
	if (!queue->head)
		return (0);

	tmp = queue->head;
	strcpy(dest, queue->head->text);
	queue->head = queue->head->next;
	if (!queue->head)
		queue->tail = NULL;

	queue->bytes -= strlen(tmp->text) + 1;
	--queue->entries;
	FREE(tmp->text);
	FREE(tmp);

	return (1);
}

/** Pull the first command accepted by a selective queue gate, leaving every
 * skipped entry linked in its original order. */
static int get_filtered_cmd_from_q(struct txt_q *queue, char *dest, bool (*allowed)(const char *))
{
	struct txt_block *prev = NULL;
	struct txt_block *tmp;

	if (!queue || !dest || !allowed)
	{
		logit(LOG_COMM, "call to get_filtered_cmd_from_q with bogus arguments");
		return (0);
	}

	for (tmp = queue->head; tmp; prev = tmp, tmp = tmp->next)
	{
		if (!allowed(tmp->text))
			continue;

		strcpy(dest, tmp->text);

		if (prev)
			prev->next = tmp->next;
		else
			queue->head = tmp->next;

		if (queue->tail == tmp)
			queue->tail = prev;

		queue->bytes -= strlen(tmp->text) + 1;
		--queue->entries;
		FREE(tmp->text);
		FREE(tmp);

		return (1);
	}

	return (0);
}

/** Dequeue the first command this character may run while casting. */
int get_casting_cmd_from_q(P_char ch, struct txt_q *queue, char *dest)
{
	struct txt_block *prev = NULL;
	struct txt_block *tmp;

	if (!ch || !queue || !dest)
	{
		logit(LOG_COMM, "call to get_casting_cmd_from_q with bogus arguments");
		return (0);
	}

	for (tmp = queue->head; tmp; prev = tmp, tmp = tmp->next)
	{
		if (!input_allowed_while_casting(ch, tmp->text))
			continue;

		strcpy(dest, tmp->text);

		if (prev)
			prev->next = tmp->next;
		else
			queue->head = tmp->next;

		if (queue->tail == tmp)
			queue->tail = prev;

		queue->bytes -= strlen(tmp->text) + 1;
		--queue->entries;
		FREE(tmp->text);
		FREE(tmp);

		return (1);
	}

	return (0);
}

/**
 * Ownership transactions publish live item moves asynchronously.  Pull safe
 * commands from behind item-dependent type-ahead while leaving the dependent
 * commands in FIFO order for the first pulse after publication.
 */
int get_item_movement_cmd_from_q(struct txt_q *queue, char *dest)
{
	return get_filtered_cmd_from_q(queue, dest, input_allowed_while_item_moving);
}

/** Dequeue against every unpublished state domain currently affecting play. */
int get_pending_transaction_cmd_from_q(struct txt_q *queue, char *dest, bool item_pending,
				       bool currency_pending)
{
	if (item_pending && currency_pending)
		return get_filtered_cmd_from_q(queue, dest,
					       input_allowed_while_item_and_currency_pending);
	if (item_pending)
		return get_item_movement_cmd_from_q(queue, dest);
	if (currency_pending)
		return get_filtered_cmd_from_q(queue, dest, input_allowed_while_currency_pending);
	return get_from_q(queue, dest);
}

/* flag 0 appends a command; nonzero flags may merge application output. */
void write_to_q(const char *txt, struct txt_q *queue, const int flag)
{
	if (!queue || !txt)
	{
		logit(LOG_COMM, "call to write_to_q with bogus arguments");
		return;
	}
	/* Output overflow is terminal; never grow a queue waiting to disconnect. */
	if (flag && queue->overflowed)
		return;

	const size_t string_limit = flag ? MAX_STRING_LENGTH : MAX_INPUT_LENGTH;
	const size_t txtlen = strnlen(txt, string_limit);
	if (txtlen == string_limit)
	{
		queue->overflowed = true;
		return;
	}
	const size_t taillen = queue->tail ? strlen(queue->tail->text) : 0;
	const bool merge = flag && queue->tail && taillen < MAX_INPUT_LENGTH &&
			   txtlen < MAX_INPUT_LENGTH - taillen;
	const size_t growth = txtlen + (merge ? 0 : 1);
	const size_t byte_limit = flag ? SESSION_OUTPUT_MAX_BYTES : SESSION_INPUT_MAX_BYTES;
	const size_t entry_limit = flag ? SESSION_OUTPUT_MAX_ENTRIES : SESSION_INPUT_MAX_ENTRIES;
	/* Check before CREATE/RECREATE, including a terminator for new entries.
	 * Subtraction avoids wrapping even if a counter is already out of range. */
	if (queue->bytes > byte_limit || growth > byte_limit - queue->bytes ||
	    queue->entries > entry_limit || (!merge && queue->entries == entry_limit))
	{
		queue->overflowed = true;
		return; /* Reject newest input; all accepted commands retain their order. */
	}
	/* Rearm notices only when input admission resumes after substantial drain. */
	if (!flag && queue->overflow_reported && queue->bytes <= SESSION_INPUT_MAX_BYTES / 2 &&
	    queue->entries <= SESSION_INPUT_MAX_ENTRIES / 2)
	{
		queue->overflowed = false;
		queue->overflow_reported = false;
	}
	if (merge)
	{
		RECREATE(queue->tail->text, char, taillen + txtlen + 1);
		memcpy(queue->tail->text + taillen, txt, txtlen + 1);
	}
	else
	{
		struct txt_block *n_new;
		CREATE(n_new, txt_block, 1, MEM_TAG_TXTBLK);
		CREATE(n_new->text, char, txtlen + 1, MEM_TAG_BUFFER);
		memcpy(n_new->text, txt, txtlen + 1);
		n_new->next = NULL;
		if (queue->tail)
			queue->tail->next = n_new;
		else
			queue->head = n_new;
		queue->tail = n_new;
		++queue->entries;
	}
	queue->bytes += growth;
}

/* Browser pastes may contain several commands in one message. Bound each line
 * before copying it, and use the same admission limits as Telnet typeahead. */
void queue_websocket_input(P_desc descriptor, const char *text)
{
	if (!descriptor || !text)
		return;
	do
	{
		/* Once full, discard the rest of this paste without scanning every line. */
		if (descriptor->input.bytes >= SESSION_INPUT_MAX_BYTES ||
		    descriptor->input.entries >= SESSION_INPUT_MAX_ENTRIES)
		{
			descriptor->input.overflowed = true;
			return;
		}
		const size_t length = strcspn(text, "\r\n");
		if (length >= MAX_INPUT_LENGTH)
			descriptor->input.overflowed = true;
		else
		{
			char line[MAX_INPUT_LENGTH];
			memcpy(line, text, length);
			line[length] = '\0';
			write_to_q(line, &descriptor->input, 0);
		}
		text += length;
		if (!*text)
			break;
		if (*text++ == '\r' && *text == '\n')
			++text;
	} while (*text);
}

/* OOB requests dispatch directly, without accumulating in the command queue.
 * Give their work a separate per-pulse budget, including login/service routes. */
bool admit_session_oob(P_desc descriptor, size_t bytes)
{
	if (!descriptor || descriptor->oob_input_overflowed)
		return false;
	if (descriptor->oob_input_tick != ne_event_tick)
	{
		descriptor->oob_input_tick = ne_event_tick;
		descriptor->oob_input_bytes = 0;
		descriptor->oob_input_entries = 0;
	}
	if (descriptor->oob_input_bytes > SESSION_OOB_MAX_BYTES ||
	    bytes > SESSION_OOB_MAX_BYTES - descriptor->oob_input_bytes ||
	    descriptor->oob_input_entries >= SESSION_OOB_MAX_ENTRIES)
	{
		descriptor->oob_input_overflowed = true;
		return false;
	}
	descriptor->oob_input_bytes += bytes;
	++descriptor->oob_input_entries;
	return true;
}

static void report_input_queue_overflow(P_desc descriptor)
{
	struct txt_q *queue = &descriptor->input;
	if (queue->overflowed && !queue->overflow_reported)
	{
		SEND_TO_Q(
			"Command queue limit reached; excess or overlong commands were discarded. "
			"Wait for queued commands to finish before sending more.\r\n",
			descriptor);
		queue->overflow_reported = true;
	}
}

/*
 * if b > a returns 0 secs, 0 usecs
 */

struct timeval timediff(struct timeval *a, struct timeval *b)
{
	static struct timeval rslt;

	rslt.tv_sec = a->tv_sec - b->tv_sec;
	rslt.tv_usec = a->tv_usec - b->tv_usec;

	while (rslt.tv_usec < 0)
	{
		rslt.tv_usec += 1000000;
		rslt.tv_sec--;
	}

	while (rslt.tv_usec > 1000000)
	{
		rslt.tv_usec -= 1000000;
		rslt.tv_sec++;
	}

	if (rslt.tv_sec < 0)
	{
		rslt.tv_usec = 0;
		rslt.tv_sec = 0;
	}
	return (rslt);
}

/*
 * Empty the queues before closing connection
 */

void flush_queues(P_desc d)
{
	char str[MAX_STRING_LENGTH];

	while (get_from_q(&d->output, str))
	{
	}
	while (get_from_q(&d->input, str))
	{
	}
	d->output = {};
	d->input = {};
	d->oob_input_tick = 0;
	d->oob_input_bytes = 0;
	d->oob_input_entries = 0;
	d->oob_input_overflowed = false;
}

int wizconnectsite(char *name, char *player, int flag)
{
	struct wizban_t *tmp;
	char buf[MAX_INPUT_LENGTH];
	char buff[MAX_INPUT_LENGTH];
	int i;

	if (name == NULL)
		return FALSE;

	/*
	 * lowercase the name string, since strstr is case sensitive
	 */
	for (i = 0; *(name + i) != '\0'; i++)
		buf[i] = LOWER(*(name + i));
	buf[i] = 0; /*
	             * to terminate buf
	             */
	/*
	 * lowercase the name string, since strstr is case sensitive
	 */
	for (i = 0; *(player + i) != '\0'; i++)
		buff[i] = LOWER(*(player + i));
	buff[i] = 0; /*
	              * to terminate buf
	              */
	i = 1;
	for (tmp = wizconnect; tmp; tmp = tmp->next)
	{
		if (strstr(buff, tmp->name))
		{
			i = 0;
			switch (flag)
			{
			case 0:
				if (strstr(buf, tmp->ban_str))
					return TRUE;
				break;
			case 1:
				if (tmp->ban_str[0] == '*')
					if (strstr(buf, (tmp->ban_str + 1)))
						return TRUE;
				break;
			}
		}
	}
	return i;
}

int bannedsite(char *name, int flag)
{
	struct ban_t *tmp;
	char buf[MAX_INPUT_LENGTH];
	int i;

	if (name == NULL)
		return FALSE;

	/*
	 * lowercase the name string, since strstr is case sensitive
	 */
	for (i = 0; *(name + i) != '\0'; i++)
		buf[i] = LOWER(*(name + i));
	buf[i] = 0; /*
	             * to terminate buf
	             */

	for (tmp = ban_list; tmp; tmp = tmp->next)
	{
		switch (flag)
		{
		case 0:
			if (match_pattern(tmp->ban_str, buf))
			{
				return TRUE;
			}
			break;
		case 1:
			if (tmp->ban_str[0] == '*')
				if (match_pattern((tmp->ban_str + 1), buf))
				{
					return TRUE;
				}
			break;
		}
	}
	return FALSE;
}

/*
 * ****************************************************************** *
 * socket handling                                                    *
 * ******************************************************************
 */

#if 0 /*                                                                                                                                                                                               \
       * old socket routines.  JAB                                                                                                                                                                     \
       */

/*
 * old socket code used to be here... but I fucking yanked it forever.
 * (Neb/Io/Gary/Whatever)
 *
 * Leaving the old "#if 0" here for memory sake.  You know... people can
 * sit around and say to their grandchildren: "When I was your age, there
 * was code here... it didn't work worth a damn... but we kept it there
 * anyway.  Not sure why, though".
 */

#else /*                                                                                                                                                                                               \
       * old/new socket code. JAB                                                                                                                                                                      \
       */

bool runtime_listener_address(sockaddr_in6 *address)
{
	if (!address)
		return false;
	memset(address, 0, sizeof(*address));
	address->sin6_family = AF_INET6;

	const char *configured = getenv("LISTEN_ADDRESS");
	if (!configured || !*configured || !strcmp(configured, "::"))
	{
		address->sin6_addr = in6addr_any;
		return true;
	}
	if (inet_pton(AF_INET6, configured, &address->sin6_addr) == 1)
		return true;

	in_addr ipv4;
	if (inet_pton(AF_INET, configured, &ipv4) != 1)
		return false;
	address->sin6_addr.s6_addr[10] = 0xff;
	address->sin6_addr.s6_addr[11] = 0xff;
	memcpy(&address->sin6_addr.s6_addr[12], &ipv4, sizeof(ipv4));
	return true;
}

int init_socket(int port)
{
	int s, bind_error;
	sockaddr_in6 sa;
	int value = 1;
	struct linger linger_values;

	/*
	 * struct linger ld;
	 */
	int buffsize, buffer;

	linger_values.l_onoff = 0;
	linger_values.l_linger = 0;

	bzero(&sa, sizeof sa);
	if (!runtime_listener_address(&sa))
	{
		logit(LOG_EXIT, "LISTEN_ADDRESS must be a numeric IPv4 or IPv6 address");
		exit(1);
	}
	/*
	  gethostname(hostname, MAX_HOSTNAME);
	  hp = gethostbyname(hostname);
	  if (hp == NULL) {
	    logit(LOG_EXIT, "gethostbyname");
	    exit(1);
	  }
	*/
	/*  sa.sin_family = hp->h_addrtype; */
	sa.sin6_port = htons((unsigned short int)port);
#ifdef IPPROTO_MPTCP
	/*
	 * Multipath TCP: if there are multiple routes available and enabled, they
	 * will be used together.  In our case (hardly any bandwidth used), the
	 * worse route will be kept on standby, to be used when lag happens.
	 *
	 * The kernel silently falls back to non-MPTCP extremely fast, thus broken
	 * routers or middleware rejecting packets with a flag they don't know
	 * doesn't require a retry from us.  Thus, the only concerns are platforms
	 * that don't support MPTCP (Windows, old BSDs) or have CONFIG_MPTCP=n.
	 */
	s = socket(AF_INET6, SOCK_STREAM, IPPROTO_MPTCP);
	if (s < 0)
#endif
		s = socket(AF_INET6, SOCK_STREAM, 0);
	if (s < 0)
	{
		logit(LOG_EXIT, "Init-socket");
		exit(1);
	}
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0)
	{
		logit(LOG_EXIT, "setsockopt REUSEADDR");
		exit(1);
	}
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger_values, sizeof(linger_values)) < 0)
	{
		logit(LOG_EXIT, "setsockopt REUSEADDR");
		exit(1);
	}
	buffsize = sizeof(int);

	if (getsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&buffer, (socklen_t *)&buffsize))
	{
		logit(LOG_EXIT, "getsockopt SNDBUF");
		exit(1);
	}
	if (buffer < MIN_SOCKET_BUFFER_SIZE)
	{
		buffer = MIN_SOCKET_BUFFER_SIZE;
		if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&buffer, sizeof(buffer)) < 0)
		{
			logit(LOG_EXIT, "setsockopt SNDBUF");
			exit(1);
		}
	}
	if ((bind_error = (bind(s, (struct sockaddr *)&sa, sizeof(sa))) < 0))
	{
		logit(LOG_EXIT, "bind error %d", bind_error);
		close(s);
		exit(1);
	}
	if (listen(s, SOMAXCONN) < 0)
	{
		logit(LOG_EXIT, "listen failed");
		close(s);
		exit(1);
	}
	nonblock(s);
	return (s);
}

int new_connection(int s)
{
	sockaddr_in6 isa;
	socklen_t i;
	int t;

	i = sizeof(isa);
	getsockname(s, (struct sockaddr *)&isa, &i);

	if ((t = accept(s, (struct sockaddr *)&isa, &i)) < 0)
		return (-1);
	nonblock(t);
	i = 1;
	setsockopt(t, SOL_TCP, TCP_NODELAY, &i, sizeof(i));

	// increase send buffer
	i = 65536;
	setsockopt(t, SOL_SOCKET, SO_SNDBUF, &i, sizeof(i));

	return (t);
}

/*
 * Check if a descriptor is valid (exists in descriptor_list)
 * Used to detect dangling pointers to freed/closing descriptors
 */
int is_desc_valid(struct descriptor_data *desc)
{
	struct descriptor_data *d;

	if (!desc)
		return 0;

	for (d = descriptor_list; d; d = d->next)
	{
		if (d == desc)
			return 1;
	}
	return 0;
}

void close_sockets(int s)
{
	logit(LOG_STATUS, "Closing all sockets.");
	while (descriptor_list)
		close_socket(descriptor_list);
	close(s);
}

void close_socket(struct descriptor_data *d)
{
	struct descriptor_data *tmp;
	snoop_by_data *snoop_by_ptr, *next;
	int is_morphed = d->character ? IS_MORPH(d->character) : 0;
	char Gbuf1[MAX_STRING_LENGTH];
	time_t ct;
	transport_descriptor_closed(d);
	if (d && d->player_load_request_id)
		player_load_pipeline_cancel(d->player_load_request_id);
	account_recovery_descriptor_closed(d);
	account_async_cancel(d);
	password_async_cancel(d);
	password_login_release(d->login_password_job);
	d->login_password_job = nullptr;

	compress_end(d, TRUE); /* does flushing out all output break anything ? */

	/* clean up poll wizard session if active */
	if (d->character && poll_wizard_active(d->character))
		poll_wizard_cancel(d->character);

	if (d->sslses)
		ssl_close(d->sslses);
	if (d->descriptor && !(d->transport_session && transport_world_active()))
		close(d->descriptor);
	flush_queues(d);
	--used_descs;

	/* Forget snooping */
	/*
	  if (d->snoop.snoop_by) {
	    send_to_char("Your victim is no longer among us.\r\n", d->snoop.snoop_by);
	    d->snoop.snoop_by->desc->snoop.snooping = 0;
	  }
	*/
	snoop_by_ptr = d->snoop.snoop_by_list;
	while (snoop_by_ptr)
	{
		if (is_morphed && affected_by_spell(d->character, SPELL_CHANNEL))
			send_to_char(
				"Your host has lost link... you can no longer maintain the sight link.\r\n",
				snoop_by_ptr->snoop_by);
		else
			send_to_char("Your victim is no longer among us.\r\n",
				     snoop_by_ptr->snoop_by);
		snoop_by_ptr->snoop_by->desc->snoop.snooping = 0;

		next = snoop_by_ptr->next;
		FREE(snoop_by_ptr);

		snoop_by_ptr = next;
	}

	d->snoop.snoop_by_list = 0;

	if (is_morphed && affected_by_spell(d->character, SPELL_CHANNEL))
		un_morph(d->character);

	if (d->snoop.snooping)
	{
		/*
		 * if !d->character, or they aren't playing, I can't get their
		 * level.. so I'll assume its better then 58 to be safe
		 */
		is_morphed = IS_MORPH(d->snoop.snooping);

		if (d->character && (d->connected == CON_PLAYING) && (GET_LEVEL(d->character) < 58))
			send_to_char("&+CYou are no longer being snooped.&N\r\n",
				     d->snoop.snooping);
		/*    d->snoop.snooping->desc->snoop.snoop_by = 0;*/
		if (is_morphed)
		{
			act("&+B$n has lost $s link and is unable to maintain $s part of the spell!&n",
			    FALSE, d->character, 0, d->snoop.snooping, TO_VICT);
			un_morph(d->snoop.snooping);
		}
		if (d->snoop.snooping)
		{
			rem_char_from_snoopby_list(&d->snoop.snooping->desc->snoop.snoop_by_list,
						   d->character);
			d->snoop.snooping = 0;
		}
	}
	if (d->str && (*d->str))
	{
		FREE(*d->str);
		if ((d->character) && (d->character->player.description == *d->str))
			/*
			 * okay... we have a fun situation here.  They just lost link
			 * while entering their description.  Before, the code would
			 * try to free() this piece of memory twice.  Once, when doing
			 * the free_char() call, and secondly when free()ing *d->str.
			 * The proper thing to do is to set their description to NULL
			 * (its not entered in yet anyway), and let the memory be
			 * freed with *d->str  (neb)
			 */
			d->character->player.description = NULL;
	}
	/* Okay, above sounds fine and dandy, but this is the real world,
	   and it's named duris. Shit happens. I want d->str cleared
	   God Damn it! So, I'll set it to null as well. It should already
	   be caught, but just in case, I'd rather leave a few bytes of
	   wasted memory lying around, than a potential bomb.
	 */
	if (d->str)
		*d->str = NULL;
	d->str = NULL;
	d->backstr = NULL;
	/* Gee, that wasn't so tough now, was it? */

	if (d->character)
	{
		if (d->connected == CON_PLAYING)
		{
			P_char telemetry_character =
				d->original && IS_PC(d->original) ? d->original : d->character;
			(void)telemetry_runtime_game_connection_transition(
				telemetry_character, d,
				telemetry_connection_transition_kind::detached);
			(void)telemetry_runtime_game_evidence(
				telemetry_character, nullptr,
				telemetry_runtime_evidence_kind::linkdead);
		}
		if (d->connected == CON_PLAYING)
		{
			sql_disconnectIP(d->character);
			redis_player_offline(d->character);
			act("$n has lost $s link.", TRUE, GET_PLYR(d->character), 0, 0, TO_ROOM);
			if ((NumAttackers(d->character) > 0) && !IS_TRUSTED(d->character))
			{
				logit(LOG_COMM, "Combat DropLink: %s [%s].",
				      GET_NAME(GET_PLYR(d->character)), d->host);
				statuslog(56, "Combat DropLink: %s [%s].",
					  GET_NAME(GET_PLYR(d->character)), d->host);
			}
			else
			{
				logit(LOG_COMM, "Closing link to: %s [%s].",
				      GET_NAME(GET_PLYR(d->character)), d->host);
				// Subtract 5 hrs: GMT -> EST.
				ct = time(0) - 5 * 60 * 60;
				snprintf(Gbuf1, MAX_STRING_LENGTH, "%s", asctime(localtime(&ct)));
				*(Gbuf1 + strlen(Gbuf1) - 1) = '\0';
				loginlog(d->character->player.level,
					 "%s [%s] has lost link @ %s EST.",
					 GET_NAME(GET_PLYR(d->character)), d->host, Gbuf1);
				sql_log(d->character, CONNECTLOG, "Lost Link");
			}
			if (!persistence_save_character_terminal(d->character, RENT_CRASH))
			{
				persistence_alert(AVATAR, "player_save", "link_loss", "none",
						  "none", "terminal_save_failed",
						  "retry_scheduled=1");
				persistence_schedule_character_save(d->character, RENT_CRASH, 4,
								    "link-loss-retry");
			}
			d->character->desc = 0;
		}
		else
		{
			logit(LOG_COMM, "Losing player: %s [%s].", GET_NAME(d->character), d->host);
			item_creation_grant_cancel_batch_before_entry(d->character);
			free_char(d->character);
			d->character = NULL;
		}
	}
	else
		logit(LOG_COMM,
		      "Losing descriptor without char [host=%s desc=%d connected=%d ssl=%s].",
		      *d->host ? d->host : "unknown", d->descriptor, d->connected,
		      d->sslses ? "yes" : "no");

	if (next_to_process == d)
		next_to_process = next_to_process->next;
	if (d == descriptor_list)
		descriptor_list = descriptor_list->next;
	else
	{
		/*
		 * Locate the previous element
		 */
		for (tmp = descriptor_list; tmp && (tmp->next != d); tmp = tmp->next)
			;
		if (tmp)
			tmp->next = d->next;
	}

	if (d->descriptor && !(d->transport_session && transport_world_active()))
		shutdown(d->descriptor, 2);

	if (d->showstr_head)
	{
		FREE(d->showstr_head);
	}
#ifdef I_REALLY_WANT_TO_CRASH_THE_GAME
	if (d->showstr_point)
	{
#ifdef MEM_DEBUG
		mem_use[MEM_STRINGS] -= strlen(d->showstr_point);
#endif
		FREE(d->showstr_point);
	}

	if (d->showstr_count)
#ifdef MEM_DEBUG
		mem_use[MEM_STRINGS] -= strlen(d->showstr_vector);
#endif
	FREE(d->showstr_vector);

	if (d->storage)
		FREE(d->storage);

#endif
		/* I really don't wanna crash it  */
#ifdef USE_ACCOUNT
	if (d->account)
		d->account = free_account(d->account);
#endif

	/* Clear service authorization before descriptor reuse. */
	d->durisweb_verified = 0;
	d->durisweb_backend = 0;
	d->durisweb_auth_window_start = 0;
	d->durisweb_auth_failures = 0;

	/* Free WebSocket fragment buffer if any */
	websocket_free(d);
	telnet_free_output(d);

	if (d)
	{
#if 0
#ifdef MEM_DEBUG
    mem_use[MEM_DESC] -= sizeof(struct descriptor_data);
#endif
    FREE((char *) d);
#endif
		mm_release(dead_desc_pool, d);
	}
}

void nonblock(int s)
{
	int flags;

	flags = fcntl(s, F_GETFL);
	flags |= O_NONBLOCK;
	if (fcntl(s, F_SETFL, flags) < 0)
	{
		logit(LOG_EXIT, "Nonblock");
		exit(1);
	}
}

#endif /*                                                                                                                                                                                              \
        * old/new socket code. 9/18/95  JAB                                                                                                                                                            \
        */

struct hostname_lookup_request
{
	char address[INET6_ADDRSTRLEN];
	int descriptor;
};

#define MAX_HOSTNAME_LOOKUP_WORKERS 8
static pthread_mutex_t hostname_lookup_mutex = PTHREAD_MUTEX_INITIALIZER;
static int hostname_lookup_workers = 0;

static void *hostname_lookup_worker(void *arg)
{
	struct hostname_lookup_request *request = (struct hostname_lookup_request *)arg;
	struct addrinfo hints, *result = NULL;
	char hostname[NI_MAXHOST];
	char temp_path[128], final_path[128];
	FILE *f;

	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICHOST;

	if (getaddrinfo(request->address, NULL, &hints, &result) == 0)
	{
		if (getnameinfo(result->ai_addr, result->ai_addrlen, hostname, sizeof(hostname),
				NULL, 0, NI_NAMEREQD) == 0)
		{
			snprintf(temp_path, sizeof(temp_path), "lib/etc/hosts/.%d.%s.%lu.tmp",
				 request->descriptor, request->address,
				 (unsigned long)pthread_self());
			snprintf(final_path, sizeof(final_path), "lib/etc/hosts/%d.%s",
				 request->descriptor, request->address);
			f = fopen(temp_path, "w");
			if (f != NULL)
			{
				int write_ok = fprintf(f, "%s\n", hostname) >= 0;
				int close_ok = fclose(f) == 0;
				if (write_ok && close_ok && rename(temp_path, final_path) == 0)
					network_wakeup_notify();
			}
		}
		freeaddrinfo(result);
	}

	pthread_mutex_lock(&hostname_lookup_mutex);
	hostname_lookup_workers--;
	pthread_mutex_unlock(&hostname_lookup_mutex);
	free(request);
	return NULL;
}

void resolve_descriptor_hostname_async(const char *address, int descriptor)
{
	struct hostname_lookup_request *request;
	pthread_t thread;
	pthread_attr_t attr;

	request = (struct hostname_lookup_request *)calloc(1, sizeof(*request));
	if (request == NULL)
		return;

	pthread_mutex_lock(&hostname_lookup_mutex);
	if (hostname_lookup_workers >= MAX_HOSTNAME_LOOKUP_WORKERS)
	{
		pthread_mutex_unlock(&hostname_lookup_mutex);
		free(request);
		return;
	}
	hostname_lookup_workers++;
	pthread_mutex_unlock(&hostname_lookup_mutex);

	strncpy(request->address, address, sizeof(request->address) - 1);
	request->descriptor = descriptor;
	{
		char stale_path[128];
		snprintf(stale_path, sizeof(stale_path), "lib/etc/hosts/%d.%s", descriptor,
			 request->address);
		unlink(stale_path);
	}
	if (pthread_attr_init(&attr) != 0)
	{
		pthread_mutex_lock(&hostname_lookup_mutex);
		hostname_lookup_workers--;
		pthread_mutex_unlock(&hostname_lookup_mutex);
		free(request);
		return;
	}
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&thread, &attr, hostname_lookup_worker, request) != 0)
	{
		pthread_mutex_lock(&hostname_lookup_mutex);
		hostname_lookup_workers--;
		pthread_mutex_unlock(&hostname_lookup_mutex);
		free(request);
	}
	pthread_attr_destroy(&attr);
}

int new_descriptor(int s, int conn_type)
{
	P_desc newd;
	int desc;
	socklen_t size;
	sockaddr_in6 sock;
	gnutls_session_t sslses = 0;

	if ((desc = new_connection(s)) < 0)
		return (-1);

	// Capacity counts live connections, never the numeric descriptor value.
	if (used_descs >= avail_descs)
	{
		shutdown(desc, 2);
		close(desc);
		return 0;
	}
	/* SSL connection - initialize TLS only after admission. */
	if (conn_type == 1 && !(sslses = ssl_new(desc)))
	{
		shutdown(desc, 2);
		close(desc);
		return 0;
	}
	used_descs++;
	if (used_descs > max_descs)
		max_descs = used_descs;

	if (used_descs > max_descs_this_hour)
		max_descs_this_hour = used_descs;

#if 0
#ifdef MEM_DEBUG
  mem_use[MEM_DESC] += sizeof(struct descriptor_data);
#endif
  CREATE(newd, struct descriptor_data, 1);
#endif
	newd = (struct descriptor_data *)mm_get(dead_desc_pool);
	bzero(newd, sizeof(struct descriptor_data));

	/*
	 * find info
	 */
	size = sizeof(sock);

	if (getpeername(desc, (struct sockaddr *)&sock, &size) < 0)
	{
		perror("getpeername");
		strcpy(newd->host, "&+RUNTRACEABLE&n");
	}
	else
	{
		inet_ntop(AF_INET6, &sock.sin6_addr, newd->host, sizeof newd->host);
		if (!strncmp(newd->host, "::ffff:", 7)) // mapped IPv4
		{
			/* Source and destination overlap, so this must be memmove:
			   strcpy() is undefined for overlapping ranges and aborts
			   under _FORTIFY_SOURCE.  Every IPv4 client arrives as an
			   IPv4-mapped address, so this ran on each connection. */
			char *mapped = newd->host + 7;
			memmove(newd->host, mapped, strlen(mapped) + 1);
		}

		/*
		 * things got ugly, 20k+ sites, so, split it into 2 files, a
		 * sorted historical one and an unsorted 'recent' one.  Rather
		 * than code in sorting routines, from time to time we combine the
		 * two, and resort (by hand, ie. 'sort').  Yes, this is an awful
		 * kludge, but, until we imp an asynch gethostbyaddr, it will have
		 * to do.  JAB
		 */

		/*
		 * first, we do a binary search of the sorted file.
		 */
		/*
		    if (!flag) {
		      char *t;
		      t = dnsdb_find(Gbuf1);
		      if (t) {
		        found = TRUE;
		        strcpy(Gbuf3, t);
		      }
		    }
		*/
	}

	//  if (!found)
	//    write_to_descriptor(desc, "Looking up your hostname...\r\n");
	/*
	 * init desc data
	 */
	newd->descriptor = desc;
	newd->network_input_remaining = conn_type == 2 ? WS_INPUT_BUFFER_SIZE :
							 MAX_QUEUE_LENGTH - 1;
	// newd->connected = CON_HOST_LOOKUP;
	newd->wait = 1;
	if (!transport_frontend_active())
		resolve_descriptor_hostname_async(strip_ansi(newd->host).c_str(), desc);
	*newd->host2 = '\0';
	newd->prompt_mode = FALSE;
	*newd->buf = '\0';
	newd->str = 0;
	newd->showstr_head = 0;
	newd->showstr_vector = 0;
	newd->showstr_count = 0;
	*newd->last_input = '\0';
	newd->output.head = NULL;
	newd->input.head = NULL;
	newd->next = descriptor_list;
	newd->character = 0;
	newd->original = 0;
	newd->snoop.snooping = 0;
	newd->snoop.snoop_by_list = 0;
	newd->tmp_val = 0; /*
	                                * SAM 7-94
	                                */
	newd->confirm_state = 0; /*
	                                * SAM 7-94
	                                */
	newd->editor = NULL;
	newd->out_compress = MCCP_NONE;
	newd->z_str = NULL;
	newd->sslses = sslses;
	*newd->client_str = '\0';
	newd->term_type = TERM_ANSI;

	/* WebSocket connection - set flags and wait for HTTP upgrade */
	if (conn_type == 2)
	{
		int ws_opt = 1;
		newd->websocket = 1;
		newd->ws_state = 0; /* WS_STATE_CONNECTING */
		newd->ws_handshake_done = 0;
		newd->ws_handshake_started = time(0);
		newd->ws_fragment_buffer = NULL;
		newd->ws_fragment_len = 0;
		newd->gmcp_enabled = 1; /* WebSocket clients always get GMCP */
		/* WebSocket needs non-blocking I/O and low latency */
		fcntl(desc, F_SETFL, O_NONBLOCK);
		setsockopt(desc, IPPROTO_TCP, TCP_NODELAY, &ws_opt, sizeof(ws_opt));
	}

	descriptor_list = newd;
	transport_frontend_accepted(newd);

	if (conn_type == 1) // ssl - always use CON_SSLNEGO, let game loop handle greet
	{
		newd->tls_read_interest = POLLIN;
		newd->tls_handshake_deadline_us =
			loop_monotonic_us() +
			static_cast<uint64_t>(TLS_HANDSHAKE_TIMEOUT_MS) * 1000;
		STATE(newd) = CON_SSLNEGO;
	}
	else if (conn_type == 2)
		STATE(newd) = CON_GET_TERM; /* WebSocket waits for HTTP handshake */
	else
	{
		/* Terminal discovery is optional metadata.  Start it before the
		 * greeting so responsive clients can answer immediately, but never
		 * hold the login screen behind an RFC 1091 response. */
		ttype_negotiate(newd);
		if (transport_frontend_active())
		{
			STATE(newd) = CON_GET_TERM;
			advertise_mccp(newd);
			gmcp_negotiate(newd);
		}
		else
			greet(newd);
	}

	return 0;
}

static void greet(P_desc newd)
{
	check_cp437(newd);
	if (bannedsite(newd->host, 0))
	{
		write_to_descriptor(
			newd,
			"Your site has been banned from being able to connect to Duris.\r\n"
			"You were banned because someone at your site has flagrantly violated\r\n"
			"the rules to a point where banning your site was necessary.  If you\r\n"
			"feel this is in error, please e-mail multiplay@durismud.com\r\n");
		banlog(56, "Reject Connect from %s, banned site.", newd->host);
		logit(LOG_STATUS, "Rejected Connect from %s, banned site.", newd->host);
		STATE(newd) = CON_EXIT;
		// flush_queues(newd);
		return;
	}

	if (newd->transport_session && transport_world_active() && newd->websocket)
	{
		STATE(newd) = CON_GET_ACCT_NAME;
		return;
	}
	select_terminal(newd, "");
	if (newd->transport_session && transport_world_active())
		return;

	advertise_mccp(newd);
	gmcp_negotiate(newd);
	/* sga disabled - causes ^? ^M on raw telnet in character mode */
	/* sga_negotiate(newd); */
}

void append_prompt(P_char ch, char *promptbuf)
{
	P_char t_ch_f;
	P_char tank;
	int percent = 0;

	if (!ch)
		return;

	if (!IS_TRUSTED(ch) &&
	    (ch->desc->connected == CON_PLAYING || ch->desc->connected == CON_MAIN_MENU))
	{
		;
	}
	else
	{
		return;
	}

	if (ch)
	{
		t_ch_f = GET_OPPONENT(ch);
	}

	if (IS_NPC(ch))
		return;

	strcat(promptbuf, "\n&+g<");
	if (GET_MAX_HIT(ch) > 0)
		percent = (100 * GET_HIT(ch)) / GET_MAX_HIT(ch);
	else
		percent = -1;

	if (percent >= 66)
	{
		snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf),
			 "&+g %dh", ch->points.hit);
	}
	else if (percent >= 33)
	{
		snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf),
			 "&+y %dh", ch->points.hit);
	}
	else if (percent >= 15)
	{
		snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf),
			 "&+r %dh", ch->points.hit);
	}
	else
	{
		snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf),
			 "&+R %dh", ch->points.hit);
	}
	snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+g/%dH",
		 GET_MAX_HIT(ch));

	if (GET_MAX_VITALITY(ch) > 0)
	{
		percent = (100 * GET_VITALITY(ch)) / GET_MAX_VITALITY(ch);
	}
	else
	{
		percent = -1;
	}

	if (percent >= 66)
	{
		snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf),
			 "&+g %dv", ch->points.vitality);
	}
	else if (percent >= 33)
	{
		snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf),
			 "&+y %dv", ch->points.vitality);
	}
	else
	{
		snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf),
			 "&+r %dv", ch->points.vitality);
	}
	snprintf(promptbuf + strlen(promptbuf), MAX_STRING_LENGTH - strlen(promptbuf), "&+g/%dV",
		 GET_MAX_VITALITY(ch));

	strcat(promptbuf, " &+CPos:&+g");
	if (GET_POS(ch) == POS_STANDING)
		strcat(promptbuf, " standing");
	else if (GET_POS(ch) == POS_SITTING)
		strcat(promptbuf, " sitting");
	else if (GET_POS(ch) == POS_KNEELING)
		strcat(promptbuf, " kneeling");
	else if (GET_POS(ch) == POS_PRONE)
		strcat(promptbuf, " on your ass");
	strcat(promptbuf, " &+g>&n\n");

	if (t_ch_f && (ch->in_room == t_ch_f->in_room))
	{
		strcat(promptbuf, "&+g<");

		/* TANK elements only active if... */
		if ((tank = GET_OPPONENT(t_ch_f)) && (ch->in_room == tank->in_room))
		{
			snprintf(promptbuf + strlen(promptbuf),
				 MAX_STRING_LENGTH - strlen(promptbuf), " &+BT: %s",
				 (ch != tank && !CAN_SEE(ch, tank)) ?
					 "someone" :
					 (IS_PC(tank) ? PERS(tank, ch, 0) :
							(FirstWord(GET_NAME(tank)))));
			strcat(promptbuf, " &+CTP:&+g");
			if (GET_POS(tank) == POS_STANDING)
				strcat(promptbuf, " sta");
			else if (GET_POS(tank) == POS_SITTING)
				strcat(promptbuf, " sit");
			else if (GET_POS(tank) == POS_KNEELING)
				strcat(promptbuf, " kne");
			else if (GET_POS(tank) == POS_PRONE)
				strcat(promptbuf, " ass");

			strcat(promptbuf, " &+cTC:");
			if (GET_MAX_HIT(tank) > 0)
			{
				percent = (100 * GET_HIT(tank)) / GET_MAX_HIT(tank);
			}
			else
			{
				percent = -1;
			}
			if (percent >= 100)
			{
				strcat(promptbuf, "&+gexcellent");
			}
			else if (percent >= 90)
			{
				strcat(promptbuf, "&+Yfew scratches");
			}
			else if (percent >= 75)
			{
				strcat(promptbuf, "&+Y small wounds");
			}
			else if (percent >= 50)
			{
				strcat(promptbuf, "&+M few wounds");
			}
			else if (percent >= 30)
			{
				strcat(promptbuf, "&+m nasty wounds");
			}
			else if (percent >= 15)
			{
				strcat(promptbuf, "&+Rpretty hurt");
			}
			else if (percent >= 0)
			{
				strcat(promptbuf, "&+r awful");
			}
			else
			{
				strcat(promptbuf, "&+r bleeding, close to death");
			}

			snprintf(promptbuf + strlen(promptbuf),
				 MAX_STRING_LENGTH - strlen(promptbuf), " &+rE: %s&+g",
				 (!CAN_SEE(ch, t_ch_f)) ?
					 "someone" :
					 (IS_PC(t_ch_f) ? PERS(t_ch_f, ch, 0) :
							  (FirstWord((t_ch_f)->player.name))));
			if (GET_POS(t_ch_f) == POS_STANDING)
				strcat(promptbuf, " sta");
			else if (GET_POS(t_ch_f) == POS_SITTING)
				strcat(promptbuf, " sit");
			else if (GET_POS(t_ch_f) == POS_KNEELING)
				strcat(promptbuf, " kne");
			else if (GET_POS(t_ch_f) == POS_PRONE)
				strcat(promptbuf, " ass");

			strcat(promptbuf, "&+C EP: ");
			if (GET_MAX_HIT(t_ch_f) > 0)
			{
				percent = (100 * GET_HIT(t_ch_f)) / GET_MAX_HIT(t_ch_f);
			}
			else
			{
				percent = -1;
			}
			if (percent >= 100)
			{
				strcat(promptbuf, "&+gexcellent");
			}
			else if (percent >= 90)
			{
				strcat(promptbuf, "&+Yfew scratches");
			}
			else if (percent >= 75)
			{
				strcat(promptbuf, "&+Y small wounds");
			}
			else if (percent >= 50)
			{
				strcat(promptbuf, "&+M few wounds");
			}
			else if (percent >= 30)
			{
				strcat(promptbuf, "&+m nasty wounds");
			}
			else if (percent >= 15)
			{
				strcat(promptbuf, "&+Rpretty hurt");
			}
			else if (percent >= 0)
			{
				strcat(promptbuf, "&+r awful");
			}
			else
			{
				strcat(promptbuf, "&+r bleeding, close to death");
			}
			strcat(promptbuf, " &+g>&n\n ");
		}
	}
}

void write_to_pc_log(P_char ch, const char *message, int log)
{
	if (!ch)
		return;

	ch = GET_PLYR(ch);

	if (!ch || !IS_PC(ch) || !IS_ALIVE(ch))
	{
		return;
	}

	if (!GET_PLAYER_LOG(ch))
	{
		initialize_logs(ch, false);
	}

	if (!GET_PLAYER_LOG(ch))
	{
		logit(LOG_DEBUG,
		      "Reloaded player log (%s) in write_to_pc_log(), but still not loaded.",
		      GET_NAME(ch));
		debug("Reloaded player log (%s) in write_to_pc_log(), but still not loaded.",
		      GET_NAME(ch));
		return;
	}

	if (log < 0 || log >= NUM_LOGS)
	{
		logit(LOG_DEBUG, "Invalid log (%d) in write_to_pc_log()", log);
		debug("Invalid log (%d) in write_to_pc_log()", log);
		return;
	}

	GET_PLAYER_LOG(ch)->write(log, message);
}

void initialize_logs(P_char ch, bool reset_logs)
{
	if (!ch || !IS_PC(ch))
		return;

	if (!reset_logs && GET_PLAYER_LOG(ch))
	{
		logit(LOG_DEBUG,
		      "Tried to initialize player log (%s) in initialize_logs(), but was not null!",
		      GET_NAME(ch));
		return;
	}

	if (reset_logs)
	{
		clear_logs(ch);
	}

	GET_PLAYER_LOG(ch) = new PlayerLog;
}

void clear_logs(P_char ch)
{
	if (!ch || !IS_PC(ch))
		return;

	if (!GET_PLAYER_LOG(ch))
	{
		//    logit(LOG_DEBUG, "Tried to clear player log (%s) in clear_logs(), but was null!", GET_NAME(ch));
		return;
	}

	GET_PLAYER_LOG(ch)->clear();
}

/*
 * **  Combine multiple entries in the output queue to go to the same file
 * **  descriptor. (Max of 2 * MAX_STRING_LENGTH (16K)) **  Also go through
 * the strings adding color codes when appropriate, and **  striping the
 * special symbols when needed.
 */

int process_output(P_desc t)
{
	report_input_queue_overflow(t);
	if (t->output.overflowed)
		return -1;
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	snoop_by_data *snoop_by_ptr;
	P_char realChar = t->original ? t->original : t->character;
	string descbuf;

	bool text = t->output.head;
	bool output_prompt_mode = t->prompt_mode;

#ifdef SMART_PROMPT
	if (t->character && (IS_PC(t->character) || IS_MORPH(t->character)))
	{
		if (IS_SET(GET_PLYR(t->character)->specials.act, PLR_OLDSMARTP) &&
		    !t->showstr_count && !t->str && !IS_FIGHTING(GET_PLYR(t->character)))
		{
			t->prompt_mode = FALSE;
		}
		else if (!IS_SET(GET_PLYR(t->character)->specials.act, PLR_SMARTPROMPT) && text)
		{
			t->prompt_mode = TRUE;
		}
	}
#endif
	if (realChar && item_creation_grant_blocks_commands(realChar))
		t->prompt_mode = FALSE;
	if (realChar && GET_STAT(realChar) == STAT_DEAD)
		t->prompt_mode = FALSE;

	// Keep ordinary command prompts pending until a transaction and its messages publish.
	// Pager and string-editor prompts remain available while unrelated work is in flight.
	bool defer_prompt = t->prompt_mode && realChar && !t->showstr_count && !t->str &&
			    (item_movement_transaction_player_busy(realChar) ||
			     currency_transaction_player_busy(realChar) ||
			     collector_transaction_player_busy(realChar) ||
			     collector_service_player_busy(realChar));
	if (defer_prompt)
		output_prompt_mode = FALSE;

	if (text && !defer_prompt && !t->connected && t->character &&
	    (IS_PC(t->character) || IS_MORPH(t->character)) &&
	    !IS_SET(GET_PLYR(t->character)->specials.act, PLR_COMPACT))
	{
		write_to_q("\r\n", &t->output, 1);
	}

	if (text && !defer_prompt && STATE(t) == CON_PLAYING && IS_PC(realChar) &&
	    ((output_prompt_mode == (PLR_FLAGGED(realChar, PLR_SMARTPROMPT)) ||
	      (output_prompt_mode != PLR_FLAGGED(realChar, PLR_OLDSMARTP)))))
	{
		if (!t->snoop.snooping || !t->snoop.snooping->desc ||
		    !t->snoop.snooping->desc->prompt_mode)
			descbuf += "\r\n";
	}

	bool had_prompt = t->prompt_mode && !defer_prompt;
	if (had_prompt)
		make_prompt(t);
	if (t->output.overflowed)
		return -1;

	/* Cycle thru output queue */
	while (get_from_q(&t->output, buf))
	{
#if 0
    if( PLR_FLAGGED(realChar, PLR_SMARTPROMPT) )
      format_text(buf, 1, t, MAX_STRING_LENGTH);
#endif

		if ((snoop_by_ptr = t->snoop.snoop_by_list) != NULL)
		{
			format_to_snoopers(buf, buf2);
		}
		while (snoop_by_ptr)
		{
			write_to_q(buf2, &snoop_by_ptr->snoop_by->desc->output, 1);

			snoop_by_ptr = snoop_by_ptr->next;
		}

		AnsiString abuf(buf);
		if (t->term_type == TERM_GENERIC)
			abuf.colorize(0);
		abuf.term(buf, t->character && PLR3_FLAGGED(t->character, PLR3_UNDERLINE) ?
				       TL_UNDERLINE :
				       TL_BLINK);
		delete_doubledollar(buf);

		/* Rendering can expand color markup. Bound the aggregate before the
		 * string append allocates, as well as bounding its queued source text. */
		const size_t rendered_bytes = strlen(buf);
		if (descbuf.size() > SESSION_OUTPUT_MAX_BYTES ||
		    rendered_bytes > SESSION_OUTPUT_MAX_BYTES - descbuf.size())
		{
			t->output.overflowed = true;
			return -1;
		}
		descbuf.append(buf, rendered_bytes);
	}

	{
		int output_result = write_to_descriptor(t, descbuf.c_str());
		if (output_result < 0)
			return (-1);
	}

	/* Telnet prompt framing is useful during login/account states too. Those
	 * screens queue their own prompt text instead of using make_prompt(). */
	if (had_prompt)
		if (send_ga(t) < 0)
			return (-1);

	return (1);
}

/*
 * this routine takes raw input from a socket (t->buf) and breaks it up
 * and massages and filters it before writing it to the input queue
 * (t->input) for actual parsing by the mud.  ALL input from sockets must
 * pass through this routine.
 */

int process_input(P_desc t)
{
	int thisround, begin;
	bool incomplete_telnet_command = false;
	char *buf, *bp;

	/* WebSocket connections use their own input processing */
	if (t->websocket)
	{
		return websocket_process_input(t);
	}

	begin = t->buflen;
	if (begin < 0 || begin >= MAX_QUEUE_LENGTH)
		panic_corruption("comm", "process_input: invalid buffer length %d", begin);
	if (begin >= MAX_QUEUE_LENGTH - 1)
	{
		logit(LOG_COMM, "process_input: input buffer exhausted for descriptor %d.",
		      t->descriptor);
		return -1;
	}
	buf = t->buf;
	const size_t read_capacity =
		MIN(static_cast<size_t>(MAX_QUEUE_LENGTH - begin - 1), t->network_input_remaining);
	if (!read_capacity)
		return 0;

	/*
	 * Read in some stuff
	 */
	if (t->sslses)
	{
		thisround = gnutls_record_recv(t->sslses, buf + begin, read_capacity);
		if (!thisround)
		{
			logit(LOG_COMM,
			      "EOF encountered on socket read for %s [host=%s desc=%d connected=%d ssl=%s].",
			      (t->character) ? GET_NAME(t->character) : "NOCHAR",
			      *t->host ? t->host : "unknown", t->descriptor, t->connected,
			      t->sslses ? "yes" : "no");
			return NETWORK_INPUT_EOF;
		}
		else if (thisround < 0)
		{
			if (thisround != GNUTLS_E_AGAIN && thisround != GNUTLS_E_INTERRUPTED)
			{
				logit(LOG_COMM, "process_input() CON_%d %s Read: %d Error: %s",
				      t->connected, (t->character) ? GET_NAME(t->character) : "",
				      thisround, gnutls_strerror(thisround));
				return (-1);
			}
			t->tls_read_interest = gnutls_record_get_direction(t->sslses) ? POLLOUT :
											POLLIN;
			return 0;
		}
		t->tls_read_interest = 0;
	}
	else
	{
		thisround = read(t->descriptor, buf + begin, read_capacity);
		if (!thisround)
		{
			logit(LOG_COMM,
			      "EOF encountered on socket read for %s [host=%s desc=%d connected=%d ssl=%s].",
			      (t->character) ? GET_NAME(t->character) : "NOCHAR",
			      *t->host ? t->host : "unknown", t->descriptor, t->connected,
			      t->sslses ? "yes" : "no");
			return NETWORK_INPUT_EOF;
		}
		else if (thisround < 0)
		{
			if (errno != EAGAIN && errno != EINTR)
			{
				logit(LOG_COMM, "process_input() CON_%d %s Read: %d Error: %d",
				      t->connected, (t->character) ? GET_NAME(t->character) : "",
				      thisround, errno);
				return (-1);
			}
			return 0;
		}
	}

	t->network_input_remaining -= thisround;
	int len = begin + thisround;
	buf[len] = 0; // safety vs broken code
	bp = buf;

	for (int i = 0; i < len; i++)
	{
		switch (buf[i])
		{
		case 0: // illegal; ignore
		case '\r':
			break;

		case '\n':
			*bp = 0;
			process_line(t, buf);
			bp = buf;
			break;

		case (char)IAC:
		{
			int consumed = parse_telnet_options(t, buf + i, len - i);
			if (consumed <= 0)
			{
				/* Preserve a fragmented Telnet command for the next socket read. */
				incomplete_telnet_command = true;
				memmove(bp, buf + i, len - i);
				bp += len - i;
				goto incomplete;
			}
			i += consumed - 1;
			break; /* prevent fall-through to backspace handler */
		}

		case '\b':
		case 127: // handle both ^H and DEL
			while (bp > buf)
			{
				// Eat whole Unicode characters, do no other
				// processing.  We don't support clusters thus
				// no need to consume multiple codepoints.
				if (!IS_UTF8_TAIL(*--bp) || t->cp437)
					break;
			}
			break;

		default:
			*bp++ = buf[i]; // possibly no-op if bp hasn't changed
		}
	}

incomplete:
	const int buffered_length = static_cast<int>(bp - buf);
	if (incomplete_telnet_command && buffered_length >= MAX_QUEUE_LENGTH - 1)
	{
		/* Do not strand a full, unterminated Telnet frame without read space. */
		logit(LOG_COMM, "process_input: input buffer exhausted for descriptor %d.",
		      t->descriptor);
		return -1;
	}
	if (!incomplete_telnet_command && buffered_length > MAX_INPUT_LENGTH - 1)
	{
		// is it even a good idea to process it anyway?
		*bp = 0;
		process_line(t, buf);
		bp = buf;
	}
	t->buflen = bp - buf;
	return 0;
}

/*
 * Count accepted player input as activity when it is queued, not only when the
 * command loop eventually executes it.  Combat waits and map movement can
 * legitimately delay get_from_q() across several point_update() ticks.
 */
static void note_player_input_activity(P_desc t, const char *input)
{
	if (!t || t->connected != CON_PLAYING || !t->character || !IS_PC(t->character) || !*input)
		return;

	t->character->specials.timer = 0;
	REMOVE_BIT(t->character->specials.act, PLR_AFK);
}

static void process_line(P_desc t, char *in)
{
	if (transport_frontend_input(t, 1, in, strlen(in)))
		return;
	char out[MAX_QUEUE_LENGTH * 3]; // max expansion
	char buffer[MAX_STRING_LENGTH];
#ifdef SMART_PROMPT
	if (t->character && IS_SET(t->character->specials.act, PLR_SMARTPROMPT))
		t->prompt_mode = TRUE;
#endif

	if (t->cp437)
		upgrade_cp437_and_dollars(out, in);
	else if (validate_utf8_and_dollars(out, in))
	{
		// During login (non-zero connected), bad bytes come from client negotiation;
		// silently discard and re-prompt instead of confusing the user.
		if (!t->connected)
			write_to_descriptor(t, "Bad characters in input, skipped.\r\n");
		out[0] = '\0';
	}

	note_player_input_activity(t, out);

	int k = strlen(out);
	if (k > (MAX_INPUT_LENGTH - 1))
	{
		k = MAX_INPUT_LENGTH - 1;
		while (IS_UTF8_TAIL(out[k])) // don't cut in the middle of an Unicode char
			k--; // max 3, we have validated
		out[k] = 0;

		checked_snprintf(buffer, sizeof buffer, "Line too long. Truncated to:\r\n%s\r\n",
				 out);
		if (write_to_descriptor(t, buffer) < 0)
			return;
	}

	/* handle '!' to repeat last command */
	if (*out == '!' && *t->last_input && t->character && !t->connected)
		strcpy(out, t->last_input);

	if (t && t->character && IS_PC(t->character))
	{
		t->character->only.pc->received_data += k;
		receivedbytes += k;
	}
	const size_t previous_entries = t->input.entries;
	write_to_q(out, &t->input, 0);
	if (t->input.entries == previous_entries)
		return;
	strcpy(t->last_input, out);

	snoop_by_data *snoop_by_ptr = t->snoop.snoop_by_list;

	while (snoop_by_ptr)
	{
		write_to_q("&+y%&n ", &snoop_by_ptr->snoop_by->desc->output, 1);
		write_to_q(out, &snoop_by_ptr->snoop_by->desc->output, 1);
		write_to_q("\r\n", &snoop_by_ptr->snoop_by->desc->output, 1);

		snoop_by_ptr = snoop_by_ptr->next;
	}
}

void comm_transport_line(P_desc d, char *line)
{
	process_line(d, line);
}

void comm_transport_greet(P_desc d)
{
	greet(d);
}

/*
 * **************************************************************** *
 * Public routines for system-to-player-communication        *
 * ****************************************************************
 */

static char send_to_char_f_buf[MAX_STRING_LENGTH];
void send_to_char_f(P_char ch, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vsnprintf(send_to_char_f_buf, sizeof(send_to_char_f_buf) - 1, fmt, args);
	va_end(args);

	send_to_char(send_to_char_f_buf, ch);
}

void send_to_char(const char *messg, P_char ch)
{
	send_to_char(messg, ch, LOG_PUBLIC);
}

void send_to_char_f(P_char ch, const OutputContext &context, const char *fmt, ...)
{
	char message[MAX_STRING_LENGTH];
	va_list args;
	va_start(args, fmt);
	vsnprintf(message, sizeof(message) - 1, fmt, args);
	va_end(args);
	send_to_char(message, ch, context);
}

void send_to_char(const char *messg, P_char ch, const OutputContext &context)
{
	send_to_char(messg, ch, LOG_PUBLIC, context);
}

void send_to_char(const char *messg, P_char ch, int log)
{
	send_to_char(messg, ch, log, OutputContext{});
}

void send_to_char(const char *messg, P_char ch, int log, const OutputContext &context)
{
	static bool bSwitched = FALSE;
	static bool bWarningAdded = false;

	if (ch && ch->desc && messg)
	{
		const char *original_message = context.original_message ? context.original_message :
									  messg;
		std::string rendered;
		bool paging = executing_ch == ch && IS_SET(ch->specials.act, PLR_PAGING_ON);
		if (paging && !output_length)
		{
			bWarningAdded = false;
			pager_original.clear();
			pager_style_fallback = false;
		}
		size_t capacity = paging ? MAX_COMMAND_OUTPUT - output_length - 1 :
					   MAX_STRING_LENGTH - 1;
		OutputContext recipient_context = context;
		if (context.resolve_recipient_preferences)
		{
			recipient_context = player_output_profile(ch, context).context;
			recipient_context.spans = context.spans;
		}
		size_t channel = (size_t)context.channel;
		bool sequenced = channel > 0 && channel < (size_t)OutputChannel::Count;
		if (sequenced)
			recipient_context.sequence = ch->desc->output_sequences[channel];
		bool animated_match = false;
		bool styled_frame = false;
		if ((!paging || (!pager_style_fallback && !bWarningAdded)) &&
		    render_output_message(messg, recipient_context, rendered, capacity,
					  &animated_match))
		{
			messg = rendered.c_str();
			styled_frame = true;
			if (sequenced && animated_match)
				++ch->desc->output_sequences[channel];
		}
		else if (context.original_message)
			messg = original_message;
		if (paging && !bWarningAdded &&
		    (!pager_original.empty() || messg != original_message))
		{
			if (pager_original.empty())
				pager_original.assign(command_output, output_length);
			size_t original_length = strlen(original_message);
			if (original_length < MAX_COMMAND_OUTPUT - pager_original.size())
			{
				// Earlier decoration must not displace later visible output. If
				// necessary, fall back for the whole accumulated command before paging.
				if (strlen(messg) >= MAX_COMMAND_OUTPUT - output_length)
				{
					strcpy(command_output, pager_original.c_str());
					output_length = pager_original.size();
					messg = original_message;
					pager_style_fallback = true;
				}
				pager_original += original_message;
			}
		}
		if (executing_ch != ch || !IS_SET(ch->specials.act, PLR_PAGING_ON))
		{
			if (SWITCHED(ch) && !bSwitched)
			{
				char buf[30];
				snprintf(buf, 30, "&+M@&+W%s&n: ", J_NAME(ch));
				bSwitched = TRUE;
				write_to_q(buf, &ch->desc->output, 1);
				bSwitched = FALSE;
			}
			write_to_q(messg, &ch->desc->output, 1);
		}
		else
		{
			size_t len = strlen(messg);

			// once a 'warning' is appended, no more is added to the pager
			if (!bWarningAdded)
			{
				if (len < (MAX_COMMAND_OUTPUT - output_length))
				{
					strncat(command_output, messg,
						MAX_COMMAND_OUTPUT - output_length);
					output_length += len;
				}
				else
				{
					const char *warning =
						"\r\n\r\n&+W *** ...and the list goes on... ***&n\r\n";
					strncat(command_output, warning, PAD_COMMAND_OUTPUT);
					if (!pager_original.empty())
						pager_original += warning;
					bWarningAdded = true;
				}
			}
		}

		if (context.chat && (!paging || !bWarningAdded))
		{
			if (!styled_frame || pager_style_fallback)
				recipient_context.policy = OutputPolicy::Preserve;
			gmcp_comm_channel_output(ch, *context.chat, recipient_context, messg);
		}

		if ((!IS_TRUSTED(ch) || log != LOG_PUBLIC) && log != LOG_NONE &&
		    (ch->desc->connected == CON_PLAYING || ch->desc->connected == CON_MAIN_MENU))
		{
			write_to_pc_log(ch, original_message, log);
		}
	}
}

bool send_to_pid(const char *str, int pid)
{
	for (P_desc d = descriptor_list; d; d = d->next)
	{
		if (d->connected == CON_PLAYING && IS_PC(d->character) &&
		    GET_PID(d->character) == pid)
		{
			send_to_char(str, d->character);
			return TRUE;
		}
	}
	return FALSE;
}

void send_to_all(const char *messg)
{
	P_desc i;

	if (messg)
		for (i = descriptor_list; i; i = i->next)
			if (!i->connected)
				write_to_q(messg, &i->output, 2);
}

void send_to_outdoor(const char *messg)
{
	P_desc i;

	if (messg)
		for (i = descriptor_list; i; i = i->next)
			if (!i->connected && OUTSIDE(i->character))
				if (IS_TRUSTED(i->character) ||
				    !IS_ROOM(i->character->in_room, ROOM_SILENT))
					write_to_q(messg, &i->output, 2);
}

void send_to_nearby_rooms([[maybe_unused]] int from_room, [[maybe_unused]] const char *messg)
{
	/* lags mud to hell and back */

#if 0
  P_desc   i;

  if (messg)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected && OUTSIDE(i->character))
        if (!IS_ROOM(i->character->in_room, ROOM_SILENT) &&
            (how_close(from_room, i->character->in_room, 10) >= 0))
          write_to_q(messg, &i->output, 2);
#endif
}

void send_to_zone_outdoor(int z_num, const char *messg)
{
	send_to_zone_func(z_num, (int)(-ROOM_INDOORS), messg);
}

void send_to_zone_indoor(int z_num, const char *messg)
{
	send_to_zone_func(z_num, (int)ROOM_INDOORS, messg);
}

void send_to_zone(int z_num, const char *msg)
{
	send_to_zone_func(z_num, 0, msg);
}

void send_to_except(const char *messg, P_char ch)
{
	P_desc i;

	if (messg)
		for (i = descriptor_list; i; i = i->next)
			if (ch->desc != i && !i->connected)
				if (IS_TRUSTED(i->character) ||
				    !IS_ROOM(i->character->in_room, ROOM_SILENT))
					write_to_q(messg, &i->output, 2);
}

static char send_to_room_f_buf[MAX_STRING_LENGTH];
void send_to_room_f(int room, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vsnprintf(send_to_room_f_buf, sizeof(send_to_room_f_buf) - 1, fmt, args);
	va_end(args);

	send_to_room(send_to_room_f_buf, room);
}
void send_to_room(const char *messg, int room)
{
	P_char i;

	if ((room < 0) || (room > top_of_world))
	{
		logit(LOG_DEBUG, "send_to_room(): room numb out of range (%d)", room);
		return;
	}

	if (messg)
		for (i = world[room].people; i; i = i->next_in_room)
			if (i->desc)
				if (IS_TRUSTED(i) || !IS_ROOM(i->in_room, ROOM_SILENT) ||
				    i->specials.z_cord == 0)
					write_to_q(messg, &i->desc->output, 2);
}

void send_to_room_except(const char *messg, int room, P_char ch)
{
	P_char i;
	char Gbuf4[MAX_STRING_LENGTH];

	if ((room < 0) || (room > top_of_world))
	{
		logit(LOG_DEBUG, "send_to_room_except(): room numb out of range (%d)", room);
		return;
	}

	if (messg)
		for (i = world[room].people; i; i = i->next_in_room)
			if ((i != ch) && i->desc)
			{
				if (GET_LEVEL(i) >= GET_LEVEL(ch))
				{
					snprintf(Gbuf4, MAX_STRING_LENGTH, "R[%s]", GET_NAME(ch));
					write_to_q(Gbuf4, &i->desc->output, 1);
				}
				write_to_q(messg, &i->desc->output, 2);
			}
}

void send_to_room_except_two(const char *messg, int room, P_char ch1, P_char ch2)
{
	P_char i;

	if ((room < 0) || (room > top_of_world))
	{
		logit(LOG_DEBUG, "send_to_room_except_two(): room numb out of range (%d)", room);
		return;
	}

	if (messg)
		for (i = world[room].people; i; i = i->next_in_room)
			if ((i != ch1) && (i != ch2) && i->desc)
				if (IS_TRUSTED(i) || !IS_ROOM(i->in_room, ROOM_SILENT))
					write_to_q(messg, &i->desc->output, 2);
}

void act_convert(char *buf, const char *str, P_char ch, P_char to, P_obj obj, void *vict_obj,
		 int type)
{
	char tbuf[MAX_STRING_LENGTH];
	bool found;
	int j, tbp, skip;
	char *point;
	const char *strp, *i;
	bool no_eol = FALSE;

	for (strp = str, point = buf;;)
	{
		if (*strp == '$')
		{
			j = 0;

			switch (*(++strp))
			{
			case 'n':
				if (ch && to)
					i = PERS(ch, to, FALSE);
				else
					i = NULL;

				break;

			case 'N':
				if (vict_obj && to)
					i = PERS((P_char)vict_obj, to, FALSE);
				else
					i = NULL;

				break;

			case 'm':
				if (ch)
					i = HMHR(ch);
				else
					i = NULL;

				break;

			case 'M':
				if (vict_obj)
					i = HMHR((P_char)vict_obj);
				else
					i = NULL;

				break;

			case 's':
				if (ch)
					i = HSHR(ch);
				else
					i = NULL;

				break;

			case 'S':
				if (vict_obj)
				{
					if (type == TO_VICT)
						i = "your";
					else
						i = HSHR((P_char)vict_obj);
				}
				else
					i = NULL;

				break;

			case 'e':
				if (ch)
					i = HSSH(ch);
				else
					i = NULL;

				break;

			case 'E':
				if (vict_obj)
					i = HSSH((P_char)vict_obj);
				else
					i = NULL;

				break;

			case 'o':
				if (obj && to)
					i = OBJN(obj, to);
				else
					i = NULL;

				break;

			case 'O':
				if (vict_obj && to)
					i = OBJN((P_obj)vict_obj, to);
				else
					i = NULL;

				break;

			case 'p':
				if (obj && to)
					i = OBJS(obj, to);
				else
					i = NULL;

				break;

			case 'P':
				if (vict_obj && to)
					i = OBJS((P_obj)vict_obj, to);
				else
					i = NULL;

				break;

				/*
					 * 'q's' are same as p's except it kills 'A |An
					 * |The' from the start of the string, it's ugly,
					 * cause we have to skip leading ansi stuff. JAB
					 */
			case 'q':
			case 'Q':
				*tbuf = '\0';
				tbp = 0;
				skip = 0;
				found = FALSE;

				if (*strp == 'Q')
				{
					if (vict_obj && to)
						i = OBJS((P_obj)vict_obj, to);
					else
						i = NULL;
				}
				else
				{
					if (obj && to)
						i = OBJS(obj, to);
					else
						i = NULL;
				}

				if (i == NULL)
					break;

				for (; *i; i++)
				{
					if (skip)
					{
						skip--;
					}
					else
					{
						/*
							 * ANSI skipping
							 */
						if (!found && (*i == '&'))
						{
							if ((*(i + 1) == 'N') || (*(i + 1) == 'n'))
								skip = 1;
							else if ((*(i + 1) == '-') ||
								 (*(i + 1) == '+'))
								skip = 2;
							else if (*(i + 1) == '=')
								skip = 3;
						}

						// a and an

						if (!found && (LOWER(*i) == 'a') && (*(i + 1)))
						{
							if (*(i + 1) == ' ')
							{
								found = TRUE;
								i++;
								continue;
							}

							if ((LOWER(*(i + 1)) == 'n') && *(i + 2) &&
							    (*(i + 2) == ' '))
							{
								found = TRUE;
								i += 2;
								continue;
							}
						}

						// the

						if (!found && (LOWER(*i) == 't'))
						{
							if ((LOWER(*(i + 1)) == 'h') &&
							    (LOWER(*(i + 2)) == 'e') &&
							    (*(i + 3) == ' '))
							{
								found = TRUE;
								i += 3;
								continue;
							}
						}

						// some

						if (!found && (LOWER(*i) == 's') &&
						    (LOWER(*(i + 1)) == 'o') &&
						    (LOWER(*(i + 2)) == 'm') &&
						    (LOWER(*(i + 3)) == 'e') &&
						    (LOWER(*(i + 4)) == ' '))
						{
							found = TRUE;
							i += 4;
							continue;
						}
					}

					tbuf[tbp++] = *i;
				}

				tbuf[tbp++] = 0;
				i = tbuf;

				break;

			case 'a':
				if (obj)
					i = SANA(obj);
				else
					i = NULL;

				break;

			case 'A':
				if (vict_obj)
					i = SANA((P_obj)vict_obj);
				else
					i = NULL;

				break;

			case 'T':
				if (vict_obj)
					i = (char *)vict_obj;
				else
					i = NULL;

				break;

			case 'F':
				if (vict_obj)
					i = FirstWord((char *)vict_obj);
				else
					i = NULL;

				break;

			case 'w': /* complicated crap, I use it for dam_messages() */
				if (type == TO_VICT)
				{
					if (ch && to)
						i = "you";
					else
						i = NULL;
				}
				else if (type == TO_CHAR)
				{
					if (vict_obj && to)
						i = PERS((P_char)vict_obj, to, FALSE);
					else
						i = NULL;
				}
				else if (type == TO_NOTVICT)
				{
					if (ch && to)
						i = PERS((P_char)vict_obj, to, FALSE);
					else
						i = NULL;
				}

				break;

			case 'W':
				if (type == TO_VICT)
					i = "r"; /* changes you to your */
				else
					i = "'s"; /* changes joe to joe's */

				break;

			case '$':
				i = "$";

				break;

			default:
				logit(LOG_DEBUG, "Invalid $-code, act(): $%c %s", *strp, str);
				i = NULL;

				break;
			}

			if (i)
				while (*(i + j))
					*(point++) = *(i + j++);

			++strp;
		}
		else if (!(*(point++) = *(strp++)))
			break;
	}

	if (!no_eol)
	{
		*(--point) = '\n';
		*(++point) = '\r';
		*(++point) = '\0';
	}

	CAP(buf);
}

/*
 * higher-level communication

 n: ch name ("the boy")
 m: pronoun object ("him")
 s: possessive ("his")
 e: pronoun subject ("he")
 o: obj name ("totem")
 p: obj short description ("the lime-green totem")
 q: obj short description w/o article (a/an/the) ("lime-green totem")
 a: obj article (a/an/the) ("the")
 */

void escape_act_dollars(char *dst, size_t dst_size, const char *src)
{
	if (!dst || dst_size == 0)
		return;
	if (!src)
	{
		dst[0] = '\0';
		return;
	}
	size_t di = 0;
	for (size_t si = 0; src[si] && di < dst_size - 2; si++)
	{
		if (src[si] == '$')
		{
			dst[di++] = '$';
			dst[di++] = '$';
		}
		else
			dst[di++] = src[si];
	}
	dst[di] = '\0';
}

// LATENT: no output buffer bounds checking on 'buf'/'tbuf' - safe only
// because format strings are code constants, not player-controlled.
// Would need snprintf-style length tracking to harden.
void act(const char *str, int hide_invisible, P_char ch, P_obj obj, void *vict_obj, int type)
{
	act(str, hide_invisible, ch, obj, vict_obj, type, OutputContext{});
}

void act(const char *str, int hide_invisible, P_char ch, P_obj obj, void *vict_obj, int type,
	 const OutputContext &context)
{
	P_char to, vict;
	// The array buf contains our primary string (final to be sent to target).
	char buf[MAX_STRING_LENGTH], tbuf[MAX_STRING_LENGTH], tbuf2[MAX_STRING_LENGTH];
	/* Debugging
	char     mybuf[MAX_STRING_LENGTH];
	int      mycheck;
	 */
	int j, tbp, which_z, sil = type & ACT_SILENCEABLE;
	bool ignore_zcoord = type & ACT_IGNORE_ZCOORD;
	char *point;
	const char *strp, *i = nullptr;
	int terseonly = type & ACT_TERSE;
	int notterse = type & ACT_NOTTERSE;
	bool no_eol = type & ACT_NOEOL;
	unsigned int flags = type & ~7;

	type &= 7;

	if (!str || !*str)
		return;

	which_z = (ch ? ch->specials.z_cord : 0);

	if (type == TO_VICT)
	{
		to = (P_char)vict_obj;
		if (to == NULL || !to->desc)
			return;
		/*    which_z = (to ? to->specials.z_cord : 0); */
	}
	else if (type == TO_CHAR)
	{
		if (ch == NULL || !ch->desc)
		{
			return;
		}
		to = ch;
	}
	else if (type == TO_VICTROOM || type == TO_NOTVICTROOM)
	{
		vict = (P_char)vict_obj;
		if (vict)
		{
			if (vict->in_room == NOWHERE)
			{
				logit(LOG_DEBUG, "act TO_VICTROOM in NOWHERE %s (%s).",
				      GET_NAME(vict), str);
				return;
			}
			to = world[vict->in_room].people;
		}
		else
		{
			return;
		}
		/*   which_z = (to ? to->specials.z_cord : 0); */
	}
	else
	{
		if (!ch && obj)
		{
			if (!OBJ_ROOM(obj))
			{
				logit(LOG_DEBUG,
				      "Comm.c act: no ch, has obj, but obj (%d) not in a room.",
				      OBJ_VNUM(obj));
				return;
			}
			to = world[obj->loc.room].people;
			which_z = obj->z_cord;
		}
		else
		{
			if (!ch || ch->in_room == NOWHERE)
			{
				/*        logit(LOG_DEBUG, "act TO_ROOM in NOWHERE %s (%s).", GET_NAME(ch), str);*/
				return;
			}
			to = world[ch->in_room].people;
			which_z = ch->specials.z_cord;
		}
	}

	if (!to)
		return; /* if a tree falls in the forest... */

	for (; to; to = to->next_in_room)
	{
		// Viewing character needs a descriptor to send to, needs to be awake, and match z-requirements...
		if (to->desc && IS_AWAKE(to) &&
		    (ignore_zcoord || (to->specials.z_cord == which_z))
		    //   needs to not be ignoring target ch (also check only.pc not null - can be null during disconnect)
		    && (IS_NPC(to) || !to->only.pc || !to->only.pc->ignored ||
			(to->only.pc->ignored != ch))
		    //   needs to match the target type: Only TO_CHAR is shown to ch, NOTVICT/NOTVICTROOM doesn't show to victim.
		    && ((type == TO_CHAR) || (to != ch)) &&
		    !((type == TO_NOTVICT || type == TO_NOTVICTROOM) && (to == (P_char)vict_obj))
		    //   needs to have terse toggled appropriately
		    && (IS_NPC(to) || (!terseonly && !notterse) ||
			((terseonly && IS_SET(to->specials.act2, PLR2_TERSE)) ||
			 (notterse && !IS_SET(to->specials.act2, PLR2_TERSE))))
		    //   and message shouldn't be hidden due to an invisible ch or obj.
		    && (!hide_invisible || (to == ch) ||
			(ch ? CAN_SEE(to, ch) : CAN_SEE_OBJ(to, obj))))
		{
			// If it's silencable, and not to an Imm, and there is a flag flag to not show it.
			if (sil && !IS_TRUSTED(to) &&
			    (IS_ROOM(to->in_room, ROOM_SILENT) || IS_AFFECTED4(to, AFF4_DEAF)))
			{
				continue;
			}

			OutputContext selected_context = context;
			int sender_attr = 0, entity_attr = 0;
			if (context.resolve_recipient_preferences)
			{
				auto profile = player_output_profile(to, context);
				selected_context = profile.context;
				selected_context.spans = context.spans;
				selected_context.chat = context.chat;
				sender_attr = profile.sender_attr;
				entity_attr = profile.entity_attr;
			}
			const bool style_output =
				selected_context.policy != OutputPolicy::Preserve &&
				selected_context.spans.size() <= MAX_STRING_LENGTH;
			std::vector<OutputStyleSpan> entity_spans;
			for (strp = str, point = buf;;)
			{
				if (*strp == '$')
				{
					j = 0;
					i = nullptr;

					switch (*(++strp))
					{
					case 'n':
						if (ch)
						{
							if (ch == to)
							{
								i = "you";
							}
							else
							{
								i = PERS(ch, to, FALSE);
							}
						}
						else
						{
							i = "(NULL)";
						}
						break;

					case 'N':
						if (vict_obj)
						{
							if ((P_char)vict_obj == to)
							{
								i = "you";
							}
							else
							{
								i = PERS((P_char)vict_obj, to,
									 FALSE);
							}
						}
						else
						{
							i = "(NULL)";
						}
						break;

					case 'm':
						if (ch)
							i = HMHR(ch);
						else
							i = "(NULL)";
						break;

					case 'M':
						if (vict_obj)
							i = HMHR((P_char)vict_obj);
						else
							i = "(NULL)";
						break;

					case 's':
						if (ch)
						{
							if (type == TO_CHAR)
								i = "your";
							else
								i = HSHR(ch);
						}
						else
							i = "(NULL)'s";
						break;

					case 'S':
						if (vict_obj)
						{
							if (type == TO_VICT)
								i = "your";
							else
								i = HSHR((P_char)vict_obj);
						}
						else
							i = "(NULL)'s";
						break;

					case 'e':
						if (ch)
							i = HSSH(ch);
						else
							i = "(NULL)";
						break;

					case 'E':
						if (vict_obj)
							i = HSSH((P_char)vict_obj);
						else
							i = "(NULL)";
						break;

					case 'o':
						if (obj)
							i = OBJN(obj, to);
						else
							i = "(NULL)";

						break;

					case 'O':
						if (vict_obj)
							i = OBJN((P_obj)vict_obj, to);
						else
							i = "(NULL)";
						break;

					case 'p':
						if (obj)
							i = OBJS(obj, to);
						else
							i = "(NULL)";
						break;

					case 'P':
						if (vict_obj)
							i = OBJS((P_obj)vict_obj, to);
						else
							i = "(NULL)";
						break;

						/*
							 * 'q's' are same as p's except it kills 'A | An | The | Some' from
							 * the start of the string, it's ugly, cause we have to skip leading ansi stuff. JAB
							 */
					case 'q':
					case 'Q':
					{
						if (*strp == 'Q')
						{
							if (vict_obj)
								i = OBJS((P_obj)vict_obj, to);
							else
								i = NULL;
						}
						else
						{
							if (obj)
								i = OBJS(obj, to);
							else
								i = NULL;
						}

						if (i == NULL)
						{
							i = "(NULL)";
							break;
						}

						tbuf[0] = '\0';
						tbp = 0;
						// safety limit for tbuf to prevent buffer overflow
						const int tbuf_limit = MAX_STRING_LENGTH - 10;
						// First _copy_ ansi to tbuf.
						while (*i == '&' && tbp < tbuf_limit)
						{
							if (i[1] == 'n' || i[1] == 'N')
							{
								tbuf[tbp++] = '&';
								i++;
								tbuf[tbp++] = *(i++);
							}
							// Begins with && -> actual & to target.  What to do here is debatable.
							// Decided to just break and leave && as beginning of i.
							else if (i[1] == '&')
							{
								break;
							}
							else if ((i[1] == '+' || i[1] == '-') &&
								 is_ansi_char(i[2]))
							{
								tbuf[tbp++] = '&';
								i++;
								tbuf[tbp++] = *(i++);
								tbuf[tbp++] = *(i++);
							}
							else if (i[1] == '=' &&
								 is_ansi_char(i[2]) &&
								 is_ansi_char(i[3]))
							{
								tbuf[tbp++] = '&';
								i++;
								tbuf[tbp++] = *(i++);
								tbuf[tbp++] = *(i++);
								tbuf[tbp++] = *(i++);
							}
							// Not an ansi code.
							else
								break;
						}

						// Now, if the rest starts with "A " or "An " skip those chars.
						if ((LOWER(*i) == 'a'))
						{
							if (i[1] == ' ')
							{
								i += 2;
							}
							else if ((LOWER(i[1]) == 'n') &&
								 (i[2] == ' '))
							{
								i += 3;
							}
						}
						// Same for "The "
						else if ((LOWER(*i) == 't') &&
							 (LOWER(i[1]) == 'h') &&
							 (LOWER(i[2]) == 'e') &&
							 (LOWER(i[3]) == ' '))
						{
							i += 4;
						}
						// And same for "Some "
						else if ((LOWER(*i) == 's') &&
							 (LOWER(i[1]) == 'o') &&
							 (LOWER(i[2]) == 'm') &&
							 (LOWER(i[3]) == 'e') &&
							 (LOWER(i[3]) == ' '))
						{
							i += 5;
						}

						// If the whole string was just ansi chars with optional article (smh.. but zone writers).
						if (*i == '\0')
							i = "(NULL)";
						// Otherwise, add the rest of the string to the end of tbuf (contains ansi).
						else
						{
							checked_snprintf(tbuf + tbp,
									 MAX_STRING_LENGTH - tbp,
									 "%s", i);
							i = tbuf;
						}
						break;
					}

					case 'a':
						if (obj)
							i = SANA(obj);
						else
							i = "(NULL)";
						break;

					case 'A':
						if (vict_obj)
							i = SANA((P_obj)vict_obj);
						else
							i = "(NULL)";
						break;

					case 'T':
						if (vict_obj)
							i = (char *)vict_obj;
						else
							i = "(NULL)";
						break;

					case 'F':
						if (vict_obj)
							i = FirstWord((char *)vict_obj);
						else
							i = "(NULL)";
						break;

					case 'w': /* complicated crap, I use it for dam_messages() */
						if (type == TO_VICT)
						{
							if (ch)
								i = "you";
							else
								i = "(NULL)";
						}
						else if (type == TO_CHAR)
						{
							if (vict_obj)
								i = PERS((P_char)vict_obj, to,
									 FALSE);
							else
								i = "(NULL)";
						}
						else if (type == TO_NOTVICT)
						{
							if (ch)
								i = PERS((P_char)vict_obj, to,
									 FALSE);
							else
								i = "(NULL)";
						}
						else if (type == TO_NOTVICTROOM)
						{
							if (ch)
								i = PERS((P_char)vict_obj, to,
									 FALSE);
							else
								i = "(NULL)";
						}
						break;

					case 'W':
						if (type == TO_VICT)
							i = "r"; /* changes you to your */
						else
							i = "'s"; /* changes joe to joe's */
						break;

					case '$':
						i = "$$"; // will be squashed later
						break;

					default:
						logit(LOG_DEBUG,
						      "act(): Invalid $-code: '$%c' in '%s'.",
						      *strp, str);
						i = "(NULL)";
						break;
					}

					if (i)
					{
						size_t span_begin = point - buf;
						// Note: This doesn't handle ansi in the middle of the lower-cased words.
						// Making it so we don't get 'A', 'An', 'The', or 'Some' in the middle of a sentence (removing caps)!
						// For each word,
						// safety limit to prevent buffer overflow (leave room for null terminator)
						const int tbuf2_limit = MAX_STRING_LENGTH - 10;
						for (tbp = 0; *i;)
						{
							// bounds check before ansi processing
							if (tbp >= tbuf2_limit)
							{
								logit(LOG_DEBUG,
								      "act(): tbuf2 overflow prevented in ansi processing");
								break;
							}

							// Copy beginning ansi.
							while (*i == '&' && tbp < tbuf2_limit)
							{
								if (i[1] == 'n' || i[1] == 'N')
								{
									tbuf2[tbp++] = '&';
									i++;
									tbuf2[tbp++] = *(i++);
								}
								// Begins with && -> actual & to target.  We just copy the && over in this case.
								else if (i[1] == '&')
								{
									tbuf2[tbp++] = '&';
									tbuf2[tbp++] = '&';
									i += 2;
								}
								else if ((i[1] == '+' ||
									  i[1] == '-') &&
									 is_ansi_char(i[2]))
								{
									tbuf2[tbp++] = '&';
									i++;
									tbuf2[tbp++] = *(i++);
									tbuf2[tbp++] = *(i++);
								}
								else if (i[1] == '=' &&
									 is_ansi_char(i[2]) &&
									 is_ansi_char(i[3]))
								{
									tbuf2[tbp++] = '&';
									i++;
									tbuf2[tbp++] = *(i++);
									tbuf2[tbp++] = *(i++);
									tbuf2[tbp++] = *(i++);
								}
								else
								{
									break; // not a recognized ansi code, exit loop
								}
							}

							// bounds check before article processing
							if (tbp >= tbuf2_limit)
								break;

							// "A " or "An "
							if (*i == 'A')
							{
								if (i[1] == ' ')
								{
									tbuf2[tbp++] = 'a';
									i++;
								}
								else if ((LOWER(i[1]) == 'n') &&
									 (i[2] == ' '))
								{
									tbuf2[tbp++] = 'a';
									tbuf2[tbp++] = 'n';
									i += 2;
								}
								else
								{
									tbuf2[tbp++] = 'A';
									i++;
								}
							}
							// "The "
							else if ((*i == 'T') &&
								 (LOWER(i[1]) == 'h') &&
								 (LOWER(i[2]) == 'e') &&
								 (i[3] == ' '))
							{
								tbuf2[tbp++] = 't';
								tbuf2[tbp++] = 'h';
								tbuf2[tbp++] = 'e';
								i += 3;
							}
							// "Some "
							else if ((*i == 'S') &&
								 (LOWER(i[1]) == 'o') &&
								 (LOWER(i[2]) == 'm') &&
								 (LOWER(i[3]) == 'e') &&
								 (LOWER(i[4]) == ' '))
							{
								tbuf2[tbp++] = 's';
								tbuf2[tbp++] = 'o';
								tbuf2[tbp++] = 'm';
								tbuf2[tbp++] = 'e';
								i += 4;
							}
							// Any other word, just copy.
							else
							{
								while (!isspace(*i) && *i != '\0' &&
								       tbp < tbuf2_limit)
								{
									tbuf2[tbp++] = *(i++);
								}
							}

							// Copy following white-space
							while (isspace(*i) && tbp < tbuf2_limit)
							{
								tbuf2[tbp++] = *(i++);
							}
						}
						tbuf2[tbp] = '\0';
						i = tbuf2;

						for (j = 0; *(i + j) != '\0'; j++)
						{
							*(point++) = *(i + j);
						}
						// Text substitutions ($T/$F) remain eligible body text;
						// recipient-resolved names/pronouns/items retain their role.
						if (style_output && *strp != 'T' && *strp != 'F' &&
						    *strp != '$')
							entity_spans.push_back(
								{ span_begin, (size_t)(point - buf),
								  *strp == 'n' ?
									  StyleOrigin::Sender :
									  StyleOrigin::Entity,
								  *strp == 'n' ? sender_attr :
										 entity_attr });
					}
					// Move past the character following the $.
					++strp;
				}
				// If it's not a $, just copy it, breaking at end of char *.
				else if ((*(point++) = *(strp++)) == '\0')
					break;
			}

			// Add \n\r to end of char *.
			if (!no_eol)
			{
				*(--point) = '\n';
				*(++point) = '\r';
				*(++point) = '\0';
			}

			/* CAP does this now...
			// Skip beginning ansi(s) and capitalize the first char.
			point = buf;
			while( *point == '&' && ( LOWER(*(point+1)) == 'n'
			  || (*(point+1) == '+' && is_ansi_char( *(point+2) ))
			  || (*(point+1) == '-' && is_ansi_char( *(point+2) ))
			  || (*(point+1) == '=' && is_ansi_char( *(point+2) ) && is_ansi_char( *(point+3) )) ) )
			{
			  if( *(point+1) == '+' || *(point+1) == '-' )
			  {
			    point += 3;
			  }
			  else if( *(point+1) == '=' )
			  {
			    point += 4;
			  }
			  else
			  {
			    point += 2;
			  }
			}
			*/

			//      act_convert(mybuf, str, ch, to, obj, vict_obj, type);
			//      mycheck = strcmp(mybuf, buf);

			CAP(buf);
			OutputContext recipient_context = selected_context;
			// Caller spans for act must address the final recipient message, never
			// the template. Added entity spans protect each recipient's expansion.
			if (style_output)
				entity_spans.insert(entity_spans.end(), context.spans.begin(),
						    context.spans.end());
			else
				recipient_context.policy = OutputPolicy::Preserve;
			recipient_context.spans = entity_spans;
			send_to_char(buf, to, (flags & ACT_PRIVATE) ? LOG_PRIVATE : LOG_PUBLIC,
				     recipient_context);
		}

		// If there's only one recipient and we've sent the message to them, go ahead and return.
		if ((type == TO_VICT) || (type == TO_CHAR))
			return;
	}
}

void delete_doubledollar(char *string)
{
	char *out = string;

	for (;;)
	{
		switch (*out++ = *string++)
		{
		case '$':
			if (*string == '$')
				string++;
			break;
		case 0:
			return;
		}
	}
}

// Puts a Cyan % in front of each line.
void format_to_snoopers(char *from_string, char *to_string)
{
	char *index, *index2;

	//  debug( "From: '%s'.", from_string );
	index2 = to_string;
	snprintf(index2, MAX_STRING_LENGTH, "&+C%%&N ");
	index2 += 7;
	index = from_string;
	while (*index != '\0')
	{
		if (*index == '\r')
		{
			index++;
			continue;
		}
		if (index[0] == '\n' && index[1] != '\0')
		{
			snprintf(index2, MAX_STRING_LENGTH, "\n&+C%%&N ");
			index2 += 8;
			index++;
		}
		else
		{
			*(index2++) = *(index++);
		}
	}
	*index2 = '\0';
}

namespace
{
constexpr size_t diagnostic_allocation_header_bytes() noexcept
{
#ifdef MEMCHK
	return sizeof(ALLOCATION_HEADER);
#else
	return 0;
#endif
}
bool diagnostic_string_heap(const std::string &value, size_t *bytes) noexcept
{
	const uintptr_t data = reinterpret_cast<uintptr_t>(value.data());
	const uintptr_t object = reinterpret_cast<uintptr_t>(&value);
	if (data >= object && data - object < sizeof(value))
	{
		*bytes = 0;
		return true;
	}
	if (value.capacity() == SIZE_MAX)
		return false;
	*bytes = value.capacity() + 1;
	return true;
}
bool diagnostic_queue_storage(const txt_q &queue, size_t *bytes) noexcept
{
	size_t value = sizeof(queue), count = 0, text = 0;
	const txt_block *last = nullptr;
	for (const auto *node = queue.head; node; node = node->next)
	{
		if (count >= queue.entries)
			return false; // Actual recorded count bounds corrupt cycles.
		const size_t node_request =
			sizeof(*node) + 2 * diagnostic_allocation_header_bytes();
		if (!node->text || node_request > SIZE_MAX - value)
			return false;
		value += node_request;
		const size_t length = strlen(node->text);
		if (length == SIZE_MAX || length + 1 > SIZE_MAX - value ||
		    length + 1 > SIZE_MAX - text)
			return false;
		value += length + 1;
		text += length + 1;
		++count;
		last = node;
	}
	if (count != queue.entries || text != queue.bytes || last != queue.tail)
		return false;
	*bytes = value;
	return true;
}
bool diagnostic_recipient(P_desc d) noexcept
{
	return d && !d->connected && d->character && IS_TRUSTED(d->character) &&
	       IS_SET(d->character->specials.act, PLR_DEBUG) && d->character->desc;
}
struct diagnostic_queue_phase
{
	size_t bytes, entries, tail_length;
	bool tail, overflowed;
};
bool diagnostic_queue_request(diagnostic_queue_phase &queue, const char *text, size_t &live,
			      size_t &peak) noexcept
{
	if (queue.overflowed)
		return true;
	const size_t length = strnlen(text, MAX_STRING_LENGTH);
	if (length == MAX_STRING_LENGTH)
	{
		queue.overflowed = true;
		return true;
	}
	const bool merge = queue.tail && queue.tail_length < MAX_INPUT_LENGTH &&
			   length < MAX_INPUT_LENGTH - queue.tail_length;
	const size_t growth = length + (merge ? 0 : 1);
	if (queue.bytes > SESSION_OUTPUT_MAX_BYTES ||
	    growth > SESSION_OUTPUT_MAX_BYTES - queue.bytes ||
	    queue.entries > SESSION_OUTPUT_MAX_ENTRIES ||
	    (!merge && queue.entries == SESSION_OUTPUT_MAX_ENTRIES))
	{
		queue.overflowed = true;
		return true;
	}
	size_t request;
	if (merge)
	{
		const size_t header = diagnostic_allocation_header_bytes();
		if (length >= SIZE_MAX - header || queue.tail_length >= SIZE_MAX - header - length)
			return false;
		request = queue.tail_length + length + 1 + header;
	}
	else
	{
		const size_t overhead =
			sizeof(txt_block) + 2 * diagnostic_allocation_header_bytes();
		if (length >= SIZE_MAX - overhead)
			return false;
		request = overhead + length + 1;
	}
	if (request > SIZE_MAX - live)
		return false;
	peak = std::max(peak, live + request); // realloc old/new coexist; old already live.
	const size_t retained_growth = merge ? growth : request;
	if (retained_growth > SIZE_MAX - live)
		return false;
	live += retained_growth;
	queue.bytes += growth;
	if (!merge)
		++queue.entries;
	queue.tail_length = merge ? queue.tail_length + length : length;
	queue.tail = true;
	return true;
}
struct diagnostic_pager_phase
{
	size_t size, capacity, heap, maximum;
};
bool diagnostic_pager_append(diagnostic_pager_phase &pager, size_t length, size_t &live,
			     size_t &peak) noexcept
{
	if (length > pager.maximum - pager.size)
		return false;
	const size_t next = pager.size + length;
	if (next > pager.capacity)
	{
		size_t capacity = next;
		// Owning libstdc++13 basic_string::_M_create's exact geometric request.
		if (next < 2 * pager.capacity)
			capacity = std::min(2 * pager.capacity, pager.maximum);
		if (capacity == SIZE_MAX || capacity + 1 > SIZE_MAX - live)
			return false;
		peak = std::max(peak, live + capacity + 1);
		live -= pager.heap;
		live += capacity + 1;
		pager.heap = capacity + 1;
		pager.capacity = capacity;
	}
	pager.size = next;
	return true;
}
} // private original diagnostic output request accounting

bool diagnostic_output_storage_bytes(size_t *output) noexcept
{
	if (!output || !nevent_is_game_thread())
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	size_t bytes = sizeof(pager_original) + sizeof(command_output) + sizeof(output_length) +
		       sizeof(pager_style_fallback);
	size_t heap;
	if (!diagnostic_string_heap(pager_original, &heap) || heap > SIZE_MAX - bytes)
		return false;
	bytes += heap;
	// Repeated registered descriptor nodes imply a cycle; refuse rather than
	// count one live queue twice or hang an allocation-free observation.
	P_desc slow = descriptor_list, fast = descriptor_list;
	while (fast && fast->next)
	{
		slow = slow->next;
		fast = fast->next->next;
		if (slow == fast)
			return false;
	}
	for (P_desc d = descriptor_list; d; d = d->next)
	{
		// Queue ownership survives PLR_DEBUG/trust/connection changes until drain
		// or descriptor destruction. Count ALL current registered outputs once.
		if (!diagnostic_queue_storage(d->output, &heap) || heap > SIZE_MAX - bytes)
			return false;
		bytes += heap;
	}
	*output = bytes;
	return true;
#endif
}

bool diagnostic_send_to_char_bounded(const char *message, P_char ch,
				     bool (*reserve)(size_t, void *) noexcept, void *context,
				     size_t outer_live) noexcept
{
	if (!reserve || !message || !ch || !ch->desc || !nevent_is_game_thread() ||
	    !IS_TRUSTED(ch) || !IS_SET(ch->specials.act, PLR_DEBUG))
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)context;
	(void)outer_live;
	return false;
#else
	struct workspace
	{
		size_t storage, live, peak, without_prefix_live, without_prefix_peak, pager_heap;
		diagnostic_queue_phase queue, without_prefix;
		diagnostic_pager_phase pager;
		char prefix[30];
	};
	// Actual default context temporary + recipient copy + empty rendered string.
	// Preserve returns before every owning renderer allocation; trusted/Public
	// skips PC logging, default chat/resolve flags skip their callbacks entirely.
	const size_t frame = sizeof(workspace) + 2 * sizeof(OutputContext) + sizeof(std::string) +
			     sizeof(((workspace *)nullptr)->prefix);
	if (frame > SIZE_MAX - outer_live || !reserve(outer_live + frame, context))
		return false;
	workspace work{};
	if (!diagnostic_output_storage_bytes(&work.storage) || outer_live < work.storage)
		return false;
	bool registered = false;
	for (P_desc d = descriptor_list; d; d = d->next)
		if (d == ch->desc)
		{
			registered = true;
			break;
		}
	if (!registered)
		return false;
	bool selected = false;
	for (P_desc d = descriptor_list; d; d = d->next)
		if (diagnostic_recipient(d) && d->character == ch)
		{
			selected = true;
			break;
		}
	if (!selected)
		return false;
	work.live = outer_live + frame;
	work.peak = work.live;
	const bool paging = executing_ch == ch && IS_SET(ch->specials.act, PLR_PAGING_ON);
	if (!paging)
	{
		const auto &queue = ch->desc->output;
		work.queue = { queue.bytes, queue.entries,
			       queue.tail ? strlen(queue.tail->text) : 0, queue.tail != nullptr,
			       queue.overflowed };
		work.without_prefix = work.queue;
		work.without_prefix_live = work.live;
		work.without_prefix_peak = work.peak;
		if (!diagnostic_queue_request(work.without_prefix, message,
					      work.without_prefix_live, work.without_prefix_peak))
			return false;
		if (SWITCHED(ch))
		{
			snprintf(work.prefix, sizeof(work.prefix), "&+M@&+W%s&n: ", J_NAME(ch));
			// Original private recursion guard may skip this prefix. Admit BOTH exact
			// alternatives; neither can invoke an unadmitted allocator afterward.
			if (!diagnostic_queue_request(work.queue, work.prefix, work.live,
						      work.peak))
				return false;
		}
		if (!diagnostic_queue_request(work.queue, message, work.live, work.peak))
			return false;
		work.peak = std::max(work.peak, work.without_prefix_peak);
	}
	else
	{
		if (!diagnostic_string_heap(pager_original, &work.pager_heap))
			return false;
		work.pager = { output_length ? pager_original.size() : 0, pager_original.capacity(),
			       work.pager_heap, pager_original.max_size() };
		const size_t length = strlen(message);
		// bWarningAdded is intentionally left in the original function. Its true
		// branch requests nothing; admit the complete false branch before calling it.
		if (work.pager.size && length < MAX_COMMAND_OUTPUT - work.pager.size)
		{
			if (!diagnostic_pager_append(work.pager, length, work.live, work.peak))
				return false;
			// If original visible capacity was exhausted, fallback resets output_length
			// to the PRE-append original size; this successful-fit branch cannot then
			// append the warning. Those allocation phases are alternatives.
		}
		else if (length >= MAX_COMMAND_OUTPUT - output_length && work.pager.size)
		{
			static constexpr char warning[] =
				"\r\n\r\n&+W *** ...and the list goes on... ***&n\r\n";
			if (!diagnostic_pager_append(work.pager, sizeof(warning) - 1, work.live,
						     work.peak))
				return false;
		}
	}
	if (!reserve(work.peak, context))
		return false;
	try
	{
		// Exact original default/Public function keeps all original static flags,
		// limits, merges, paging/fallback, counters and recipient behavior unchanged.
		send_to_char(message, ch);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}
