# Makefile for Project Duris!

SHELL = /bin/sh

CC	= g++
DEBUG	= -ggdb3
#LDFLAGS	= -ggdb3

HARDENING_FLAGS = -Og -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection
WARNING_FLAGS = -Wall -Wextra -Wpedantic -Werror \
	-Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 \
	-Wnull-dereference -Wduplicated-cond -Wduplicated-branches -Wlogical-op \
	-Wimplicit-fallthrough=5 -Walloca -Wvla -Wdate-time -Wundef \
	-Wshadow=compatible-local -Wcast-align=strict -Wstringop-overflow=4 \
	-Warray-bounds=2 -Wstrict-overflow=2 -Wuse-after-free=3 \
	-Wwrite-strings -Wunused-parameter -Wunused-variable \
	-Wunused-but-set-variable -Wmissing-field-initializers -Wunused-function

# No legacy warning exceptions remain.  If a diagnostic in one of the six
# categories above reappears, resolve it rather than reintroducing a -Wno- flag:
# see docs/guides/BUILDING.md ("Warning profile") and
# tests/async/test_compiler_warning_profile.py, which fails if one is added.

# EXTRA_CFLAGS / EXTRA_LDFLAGS are appended last so a wrapper such as
# scripts/build-san.sh can add instrumentation without discarding the warning
# profile or the feature defines below.
EXTRA_CFLAGS =
EXTRA_LDFLAGS =
PERSISTENCE_BACKEND ?= mariadb
BUILD_PROFILE ?= development

CFLAGS = -g -std=c++20 $(HARDENING_FLAGS) $(WARNING_FLAGS)
ifeq ($(BUILD_PROFILE),development)
CFLAGS += -DTEST_MUD
else ifneq ($(BUILD_PROFILE),production)
$(error BUILD_PROFILE must be development or production)
endif
CFLAGS += -D__NO_TESTS__    # exclude built-in test hooks from maintained builds
CFLAGS += $(EXTRA_CFLAGS)
LDFLAGS += $(EXTRA_LDFLAGS)

ifeq ($(PERSISTENCE_BACKEND),mariadb)
MYSQL_INCLUDES = -I/usr/include/mysql
MYSQL_LIBS = -lmysqlclient
else ifeq ($(PERSISTENCE_BACKEND),flatfile)
CFLAGS += -D__NO_MYSQL__
MYSQL_INCLUDES = -I./no_mysql
MYSQL_LIBS =
else
$(error PERSISTENCE_BACKEND must be mariadb or flatfile)
endif

#CFLAGS += -DCTF_MUD=1

INCLUDES	= -I. -I../tests/async -I/usr/include/libxml2 $(MYSQL_INCLUDES) -I./ships

GENERAL_LIBS    = -lm -lstdc++ -lz -lxml2 -lcrypt $(MYSQL_LIBS) -lgnutls -lssl -lcrypto -lcjson -lhiredis -lhiredis_ssl -lbsd -pthread

# GENERAL_LIBS += -lcygipc
LIBS += $(GENERAL_LIBS)

# Keep every compiled artifact under the repository's bin/ tree.  Callers may
# override these paths for specialized builds, such as the sanitizer wrapper.
ROOT_DIR          := $(abspath ..)
BIN_ROOT          ?= $(ROOT_DIR)/bin
SERVER_BIN_DIR    ?= $(BIN_ROOT)/server
TOOL_BIN_DIR      ?= $(BIN_ROOT)/tools
MIGRATION_BIN_DIR ?= $(BIN_ROOT)/migrations
OBJDIR            ?= $(BIN_ROOT)/objects/server/$(PERSISTENCE_BACKEND)/$(BUILD_PROFILE)
DMS_BINARY        ?= $(SERVER_BIN_DIR)/dms_new
PFILE_BINARY      ?= $(TOOL_BIN_DIR)/pfile
MIGRATE_BINARY    ?= $(MIGRATION_BIN_DIR)/migrate_pfiles
BACKEND_STAMP     := $(dir $(DMS_BINARY)).$(notdir $(DMS_BINARY))-backend
DEPFLAGS := -MMD -MP

# files that would be compiled as dynamic libs if USE_SHARED was defined
#    make sure that they are NOT included in ANY form in OBJS!!!
# The makefile will deal with compiling SH_LIBS as normal files
# if USE_SHARED isn't defined

SH_LIBS 	=

SHIP_SRCS = \
	ship_auto.c \
	ship_base.c \
	ship_cargo.c \
	ship_combat.c \
	ship_control.c \
	ship_identity.c \
	ship_npc.c \
	ship_npc_ai.c \
	ship_shop.c \
	ship_utils.c \
	ship_variables.c \

SHIP_OBJS = $(SHIP_SRCS:%.c=ships/%.o)

OBJS =  $(SHIP_OBJS) \
				 magic.o \
				 achievements.o \
				 account.o \
				 password_hash.o \
				 account_reward.o \
				 account_reward_snapshot.o \
				 account_reward_config.o \
				 hardcore_config.o \
				 creation_availability_config.o \
				 frag_cap_config.o \
				 actcomm.o \
				 actinf.o \
				 actmove.o \
				 actnew.o \
				 actobj.o \
				 actoff.o \
				 actoth.o \
				 actset.o \
				 actwiz.o \
				 ansi.o \
				 specs.ailvio.o \
				 alliances.o \
				 automatic_rules.o \
				 affects.o \
				 arena.o \
				 artifact.o \
				 artifact_cache_codec.o \
				 report_cache_codec.o \
				 donation_event.o \
				 redis_connection.o \
				 redis_command_observability.o \
				 redis_donation_runtime.o \
				 redis_key_registry.o \
				 redis_namespace.o \
				 world_recovery_codec.o \
				 redis_donation_worker.o \
				 assocs.o \
				 auction_houses.o \
				 bard.o \
				 beh_magic.o \
         blispells.o \
				 boards.o \
				 boon.o \
				 breath_weapons.o \
				 buildings.o \
         cardgames.o \
         chaos.o \
				 chaos_config.o \
				 comm.o \
				 copyover.o \
				 common.o \
				 condition.o \
				 constant.o \
				 ctf.o \
				 dam_mods.o \
				 db.o \
				 character_identity.o \
				 debug.o \
				 disguise.o \
				 drannak.o \
				 enhance.o \
				 crafting.o \
				 mining.o \
				 mining_config.o \
				 random_equipment_config.o \
				 safe_format.o \
				 safe_io.o \
				 salvage.o \
				 material_rarity.o \
				 dreadlord.o \
				 editor.o \
				 encounters.o \
				 epic.o \
				 epic_bonus.o \
				 epic_bonus_state.o \
				 deferred_save_policy.o \
				 epic_skills.o \
				 ethermancer.o \
				 events.o \
				 event_names.o \
				 new_events.o \
				 nevent_periodic.o \
				 fight.o \
				 files.o \
				 fraglist.o \
				 forge_items.o \
				 graph.o \
				 group.o \
				 guild.o \
				 guildhall.o \
				 guildhall_rooms.o \
				 guildhall_cmds.o \
				 guildhall_procs.o \
				 guildhall_db.o \
				 grapple.o \
				 handler.o \
				 hardcore.o \
				 innates.o \
				 interp.o \
				 justice.o \
				 kick.o \
				 language.o \
				 limits.o \
				 linked_objects.o\
				 listen.o\
				 mail.o \
				 map.o \
				 mccp.o \
				 memorize.o \
				 memory.o \
				 mm.o \
				 mobact.o \
				 world_quest.o \
				 mobcombat.o \
				 mobconv.o \
				 mobpatrol.o \
				 modify.o \
				 mount.o \
				 name_gen.o \
				 nanny.o \
				 necromancy.o \
				 new_skills.o \
				 nq.o \
				 objconv.o \
				 objmisc.o \
				 outposts.o \
				 persistence_checkpoint.o \
				 persistence_observability.o \
				 persistence_mode.o \
				 env_file.o \
				 flatfile_store.o \
				 flatfile_authority_transaction.o \
				 flatfile_account_repository.o \
				 flatfile_account_reward_summon_repository.o \
				 flatfile_account_adapter.o \
				 flatfile_identity_repository.o \
				 flatfile_identity_adapter.o \
				 flatfile_character_delete.o \
				 flatfile_player_repository.o \
				 flatfile_item_uid_allocator.o \
				 flatfile_item_repository.o \
				 flatfile_artifact_repository.o \
				 flatfile_corpse_repository.o \
				 flatfile_corpse_ownership.o \
				 flatfile_corpse_restore.o \
				 flatfile_association_repository.o \
				 flatfile_locker_repository.o \
				 flatfile_frag_leaderboard_repository.o \
				 flatfile_help_catalog.o \
				 flatfile_ip_activity_repository.o \
				 flatfile_nexus_repository.o \
				 flatfile_auction_repository.o \
				 flatfile_boon_repository.o \
				 flatfile_offline_message_repository.o \
				 flatfile_player_domain_repository.o \
				 flatfile_recipe_repository.o \
				 flatfile_spellbook_repository.o \
				 flatfile_ship_repository.o \
				 flatfile_shopkeeper_repository.o \
				 flatfile_shopkeeper_capture.o \
				 flatfile_shopkeeper_ownership.o \
				 flatfile_shopkeeper_materialize.o \
				 flatfile_shopkeeper_restore.o \
				 flatfile_shopkeeper_save.o \
				 flatfile_shop_trade_repository.o \
				 flatfile_shop_trade_materialization.o \
				 flatfile_shop_trophy_history.o \
				 flatfile_world_item_repository.o \
				 flatfile_world_quest_history.o \
				 corpse_lifecycle_transaction.o \
				 critical_command.o \
				 critical_command_journal.o \
				 critical_command_coordinator.o \
				 critical_command_repository.o \
				 corpse_lifecycle_command.o \
				 item_transfer_command.o \
				 item_ownership_runtime.o \
				 item_movement_transaction.o \
				 shop_trade_transaction.o \
				 shop_trade_runtime.o \
				 item_transfer_repository.o \
				 item_transfer_synthetic.o \
				 item_uid_allocator.o \
				 critical_outbox.o \
				 epic_command.o \
				 epic_transaction.o \
				 currency_command.o \
				 auction_command.o \
				 auction_repository.o \
				 auction_transaction.o \
				 combat_outcome_command.o \
				 combat_outcome_repository.o \
				 combat_outcome_transaction.o \
				 artifact_guild_command.o \
				 artifact_guild_repository.o \
				 artifact_guild_state.o \
				 artifact_guild_transaction.o \
				 boon_reward_command.o \
				 boon_reward_repository.o \
				 boon_reward_transaction.o \
				 boon_shop_command.o \
				 boon_shop_transaction.o \
				 shop_trade_command.o \
				 zone_touch_command.o \
				 zone_touch_repository.o \
				 zone_touch_transaction.o \
				 session_audit_command.o \
				 session_audit_repository.o \
				 session_audit_transaction.o \
				 player_load_repository.o \
				 player_load_topology.o \
				 player_load_pipeline.o \
				 player_load_items.o \
				 player_load_pets.o \
				 gameplay_read_state.o \
				 epic_task_catalog.o \
				 maintenance_scheduler.o \
				 maintenance_repository.o \
				 maintenance_snapshot.o \
				 player_load_materialize.o \
				 presence_policy.o \
				 redis_cache_store.o \
				 redis_report_cache.o \
				 redis_runtime_config.o \
				 redis_maintenance.o \
				 redis_ship_legacy.o \
				 redis_floor_runtime.o \
				 redis_floor_store.o \
				 redis_presence_payload.o \
				 redis_presence_runtime.o \
				 redis_presence_worker.o \
				 redis_world_runtime.o \
				 redis_world_store.o \
				 currency_transaction.o \
				 player_revision_state.o \
				 player_save_worker.o \
				 player_save_journal.o \
				 player_save_pipeline.o \
				 world_recovery_pipeline.o \
				 player_snapshot_repository.o \
				 player_snapshot_capture.o \
				 player_snapshot_codec.o \
				 persistence_queue.o \
				 latency_trace.o \
				 sql_persistence_raw.o \
				 sql_pool.o \
				 plushit.o \
				 poll.o \
				 prompt.o \
				 properties.o \
				 psionics.o \
				 quest.o \
				 random.o \
				 randomeq.o \
				 random.mob.o \
				 random.zone.o \
				 randobj.o \
				 range.o \
				 rogues.o \
				 shaman.o \
				 shop.o \
				 signals.o \
				 skills.o \
				 smagic.o \
				 salchemist.o \
				 sillusionist.o \
				 siege.o \
				 sparser.o \
				 specials.o \
				 specializations.o \
				 specs.assign.o \
				 specs.barovia.o \
				 specs.clwcvrn.o \
				 specs.dragonnia.o \
				 specs.eth2.o \
				 specs.halfcut.o \
				 specs.ravenloft.o \
				 specs.snogres.o \
				 specs.ioun.o \
				 specs.jot.o \
				 specs.library.o \
				 studioproc.o \
				 studioproclib.o \
				 specs.lohrr.o \
				 specs.mobile.o \
				 specs.morgs.o \
				 specs.mist.o \
				 specs.object.o \
				 specs.realm.o \
				 specs.room.o \
				 specs.snikzone.o \
				 specs.grove.o \
				 specs.hoa.o \
				 specs.highway.o \
				 specs.tharnadia.o \
				 specs.twintowers.o \
				 specs.undermountain.o \
				 specs.underworld.o \
				 specs.vecna.o \
				 specs.venthix.o \
				 specs.verzanan.o \
				 specs.zalrix.o \
				 specs.set.o \
				 specs.shabo.o \
				 specs.juiblex.o \
				 specs.keleks.o \
				 specs.caertannad.o \
				 specs.firep.o \
				 specs.winterhaven.o \
				 specs.zion.o \
				 spells.o \
				 redis.o \
				 wizredis.o \
				 sql.o \
				 sql_player.o \
				 ssl.o \
				 statistics.o \
				 storage_lockers.o \
				 locker_async.o \
				 timers.o \
				 track.o \
				 tradeskill.o \
				 trap.o \
				 transport.o \
				 trophy.o \
				 unicode.o \
				 utility.o \
				 weather.o \
				 ferry.o \
				 ferryact.o \
				 paladins.o \
				 guard.o \
				 makeexit.o \
				 racewar_stat_mods.o \
				 nexus_stones.o \
				 test_async.o \
				 testcmd.o \
				 wikihelp.o \
				 reavers.o \
				 avengers.o \
				 specs.lucrot.o \
				 specs.gellz.o \
				 smoke.o \
				 multiplay_whitelist.o \
				 home.o \
				 websocket.o \
				 ws_handlers.o \
				 json_utils.o \
				 gmcp.o \
				 ttype.o \
				 wiz_newchar.o \
				 zcheck.o


# Name lists (no paths) for pfile-related objs
SHARED_PFILE_OBJS_NAMES = pf-skills.o pf-files.o
PFILE_OBJS_NAMES        = pfile-stubs.o pfile.o

MIGRATE_OBJ_NAMES = migrate_main.o \
							migrate_common.o \
							migrate_objects.o \
							migrate_accounts.o \
							migrate_players.o \
							migrate_lockers.o \
							migrate_ships.o \
							migrate_guilds.o \
							migrate_misc.o \
							migrate_stubs.o

#******************************************************************************
#******************************************************************************
#
# Don't mess around below this point.  Everything past here are makefile rules,
# and NOT dependency lists that need to be changed

PROGS = $(DMS_BINARY)

# Build object paths under OBJDIR (preserve subdirs like ships/)
DMS_OBJS = $(addprefix $(OBJDIR)/,$(OBJS))
PFILE_OBJS        = $(addprefix $(OBJDIR)/,$(PFILE_OBJS_NAMES))
SHARED_PFILE_OBJS = $(addprefix $(OBJDIR)/,$(SHARED_PFILE_OBJS_NAMES))
MIGRATE_OBJS      = $(addprefix $(OBJDIR)/migrations/,$(MIGRATE_OBJ_NAMES))

ifndef USE_SHARED
	DMS_OBJS   += $(addprefix $(OBJDIR)/,$(SH_LIBS))
	PFILE_OBJS += $(addprefix $(OBJDIR)/,$(SH_LIBS))
else
	PROGS += $(addprefix $(SERVER_BIN_DIR)/,$(SH_LIBS:.o=.so))
endif

all: $(PROGS)

.PHONY: dms_new pfile migrate_pfiles clean clean-server debug valgrind FORCE

dms_new: $(DMS_BINARY)

FORCE:

# The server binary has one stable runtime path, but its object graph and link
# dependencies differ by persistence backend and build profile. Keep each
# combination's objects isolated and update this stamp when either selection
# changes so a switch cannot leave an up-to-date binary linked for the prior
# runtime role.
$(BACKEND_STAMP): FORCE
	@mkdir -p $(dir $@)
	@set -eu; \
	backend='$(PERSISTENCE_BACKEND)'; \
	profile='$(BUILD_PROFILE)'; \
	identity="$$backend/$$profile"; \
	if [ ! -f "$@" ] || [ "$$(cat "$@")" != "$$identity" ]; then \
		printf '%s\n' "$$identity" >"$@.tmp"; \
		mv "$@.tmp" "$@"; \
	fi

$(DMS_BINARY): $(DMS_OBJS) $(BACKEND_STAMP)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -rdynamic -g -o $@ $(DMS_OBJS) $(LIBS)
	chmod a+rx $@

pfile: $(PFILE_BINARY)

$(PFILE_BINARY): $(PFILE_OBJS) $(SHARED_PFILE_OBJS) $(OBJDIR)/mm.o $(OBJDIR)/common.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -g -o $@ \
		$(PFILE_OBJS) $(SHARED_PFILE_OBJS) \
		$(OBJDIR)/common.o $(OBJDIR)/mm.o $(LIBS)

migrate_pfiles: $(MIGRATE_BINARY)

$(MIGRATE_BINARY): $(MIGRATE_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -g -o $@ $(MIGRATE_OBJS) $(LIBS)

# Build any C object into OBJDIR, creating folders as needed.
$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) $(DEPFLAGS) -c $< -o $@

# Build any C++ object into OBJDIR, creating folders as needed.
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) $(DEPFLAGS) -c $< -o $@

$(OBJDIR)/migrations/%.o: ../src-migrate/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEBUG) $(DEPFLAGS) -c $< -o $@

# Shared pfile objects (pf-*.o) also go into OBJDIR
$(OBJDIR)/pf-%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -D_PFILE_ $(INCLUDES) $(DEBUG) $(DEPFLAGS) -c $< -o $@

# Shared libs (if you ever use SH_LIBS / USE_SHARED)
$(SERVER_BIN_DIR)/%.so: %.c
	@mkdir -p $(OBJDIR) $(dir $@)
	$(CC) $(CFLAGS) $(DEBUG) -fpic -c $< -o $(OBJDIR)/$*.o
	ld -shared -o $@ /usr/lib/crtbeginS.o $(OBJDIR)/$*.o /usr/lib/crtendS.o

clean-server:
	rm -f $(DMS_BINARY) $(BACKEND_STAMP)
	rm -rf $(OBJDIR)

clean: clean-server
	rm -f *~ ships/*~ .depend ships/.depend
	rm -f $(PFILE_BINARY) $(MIGRATE_BINARY)

debug: $(PROGS)
	cd ..; ./scripts/gdbdms

# Memory/thread checking; see docs/guides/valgrind.md.
valgrind: $(PROGS)
	cd ..; ./scripts/valgrind_mud.sh

# Auto-generated dependency files alongside objects
DEPFILES := $(DMS_OBJS:.o=.d) $(PFILE_OBJS:.o=.d) $(SHARED_PFILE_OBJS:.o=.d)
-include $(DEPFILES)

# DO NOT DELETE THIS LINE -- make depend depends on it.
