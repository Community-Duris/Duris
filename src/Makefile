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
				 magic/magic.o \
				 achievements.o \
				 account/account.o \
				 account/password_hash.o \
				 account/account_reward.o \
				 account/account_reward_snapshot.o \
				 account/account_reward_config.o \
				 hardcore_config.o \
				 account/creation_availability_config.o \
				 combat/frag_cap_config.o \
				 cmd/actcomm.o \
				 cmd/actinf.o \
				 cmd/actmove.o \
				 cmd/actnew.o \
				 cmd/actobj.o \
				 cmd/actoff.o \
				 cmd/actoth.o \
				 cmd/actset.o \
				 cmd/actwiz.o \
				 net/ansi.o \
				 specs/specs.ailvio.o \
				 alliances.o \
				 automatic_rules.o \
				 magic/affects.o \
				 combat/arena.o \
				 artifact.o \
				 artifact_cache_codec.o \
				 persistence/report_cache_codec.o \
				 donation_event.o \
				 redis/redis_connection.o \
				 redis/redis_command_observability.o \
				 redis/redis_donation_runtime.o \
				 redis/redis_key_registry.o \
				 redis/redis_namespace.o \
				 world_recovery_codec.o \
				 redis/redis_donation_worker.o \
				 assocs.o \
				 auction_houses.o \
				 classes/bard.o \
				 magic/beh_magic.o \
         magic/blispells.o \
				 cmd/boards.o \
				 boon.o \
				 combat/breath_weapons.o \
				 buildings.o \
         cardgames.o \
         combat/chaos.o \
				 combat/chaos_config.o \
				 net/comm.o \
				 persistence/copyover.o \
				 common.o \
				 condition.o \
				 constant.o \
				 combat/ctf.o \
				 combat/dam_mods.o \
				 db.o \
				 account/character_identity.o \
				 debug.o \
				 classes/disguise.o \
				 classes/drannak.o \
				 enhance.o \
				 crafting.o \
				 mining.o \
				 mining_config.o \
				 random_equipment_config.o \
				 safe_format.o \
				 safe_io.o \
				 salvage.o \
				 material_rarity.o \
				 classes/dreadlord.o \
				 net/editor.o \
				 encounters.o \
				 epic.o \
				 epic_bonus.o \
				 epic_bonus_state.o \
				 persistence/deferred_save_policy.o \
				 classes/epic_skills.o \
				 classes/ethermancer.o \
				 events.o \
				 event_names.o \
				 new_events.o \
				 nevent_periodic.o \
				 combat/fight.o \
				 files.o \
				 combat/fraglist.o \
				 forge_items.o \
				 graph.o \
				 group.o \
				 guild.o \
				 guildhall.o \
				 guildhall_rooms.o \
				 guildhall_cmds.o \
				 guildhall_procs.o \
				 guildhall_db.o \
				 combat/grapple.o \
				 handler.o \
				 hardcore.o \
				 classes/innates.o \
				 cmd/interp.o \
				 combat/justice.o \
				 combat/kick.o \
				 classes/language.o \
				 limits.o \
				 linked_objects.o\
				 net/listen.o\
				 cmd/mail.o \
				 map.o \
				 net/mccp.o \
				 classes/memorize.o \
				 memory.o \
				 mm.o \
				 mobact.o \
				 world_quest.o \
				 combat/mobcombat.o \
				 mobconv.o \
				 mobpatrol.o \
				 net/modify.o \
				 classes/mount.o \
				 name_gen.o \
				 account/nanny.o \
				 classes/necromancy.o \
				 classes/new_skills.o \
				 cmd/nq.o \
				 objconv.o \
				 objmisc.o \
				 outposts.o \
				 persistence/persistence_checkpoint.o \
				 persistence/persistence_observability.o \
				 persistence/persistence_mode.o \
				 env_file.o \
				 flatfile/flatfile_store.o \
				 flatfile/flatfile_authority_transaction.o \
				 flatfile/flatfile_account_repository.o \
				 flatfile/flatfile_account_reward_summon_repository.o \
				 flatfile/flatfile_account_adapter.o \
				 flatfile/flatfile_identity_repository.o \
				 flatfile/flatfile_identity_adapter.o \
				 flatfile/flatfile_character_delete.o \
				 flatfile/flatfile_player_repository.o \
				 flatfile/flatfile_item_uid_allocator.o \
				 flatfile/flatfile_item_repository.o \
				 flatfile/flatfile_artifact_repository.o \
				 flatfile/flatfile_corpse_repository.o \
				 flatfile/flatfile_corpse_ownership.o \
				 flatfile/flatfile_corpse_restore.o \
				 flatfile/flatfile_association_repository.o \
				 flatfile/flatfile_locker_repository.o \
				 flatfile/flatfile_frag_leaderboard_repository.o \
				 flatfile/flatfile_help_catalog.o \
				 flatfile/flatfile_ip_activity_repository.o \
				 flatfile/flatfile_nexus_repository.o \
				 flatfile/flatfile_auction_repository.o \
				 flatfile/flatfile_boon_repository.o \
				 flatfile/flatfile_offline_message_repository.o \
				 flatfile/flatfile_player_domain_repository.o \
				 flatfile/flatfile_recipe_repository.o \
				 flatfile/flatfile_spellbook_repository.o \
				 flatfile/flatfile_ship_repository.o \
				 flatfile/flatfile_shopkeeper_repository.o \
				 flatfile/flatfile_shopkeeper_capture.o \
				 flatfile/flatfile_shopkeeper_ownership.o \
				 flatfile/flatfile_shopkeeper_materialize.o \
				 flatfile/flatfile_shopkeeper_restore.o \
				 flatfile/flatfile_shopkeeper_save.o \
				 flatfile/flatfile_shop_trade_repository.o \
				 flatfile/flatfile_shop_trade_materialization.o \
				 flatfile/flatfile_shop_trophy_history.o \
				 flatfile/flatfile_world_item_repository.o \
				 flatfile/flatfile_world_quest_history.o \
				 persistence/corpse_lifecycle_transaction.o \
				 persistence/critical_command.o \
				 persistence/critical_command_journal.o \
				 persistence/critical_command_coordinator.o \
				 persistence/critical_command_repository.o \
				 persistence/corpse_lifecycle_command.o \
				 item_transfer_command.o \
				 item_ownership_runtime.o \
				 item_movement_transaction.o \
				 shop_trade_transaction.o \
				 shop_trade_runtime.o \
				 item_transfer_repository.o \
				 item_transfer_synthetic.o \
				 item_uid_allocator.o \
				 persistence/critical_outbox.o \
				 epic_command.o \
				 epic_transaction.o \
				 currency_command.o \
				 auction_command.o \
				 auction_repository.o \
				 auction_transaction.o \
				 combat/combat_outcome_command.o \
				 combat/combat_outcome_repository.o \
				 combat/combat_outcome_transaction.o \
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
				 account/session_audit_command.o \
				 account/session_audit_repository.o \
				 account/session_audit_transaction.o \
				 player/player_load_repository.o \
				 player/player_load_topology.o \
				 player/player_load_pipeline.o \
				 player/player_load_items.o \
				 player/player_load_pets.o \
				 persistence/gameplay_read_state.o \
				 epic_task_catalog.o \
				 persistence/maintenance_scheduler.o \
				 persistence/maintenance_repository.o \
				 persistence/maintenance_snapshot.o \
				 player/player_load_materialize.o \
				 persistence/presence_policy.o \
				 redis/redis_cache_store.o \
				 redis/redis_report_cache.o \
				 redis/redis_runtime_config.o \
				 redis/redis_maintenance.o \
				 redis/redis_ship_legacy.o \
				 redis/redis_floor_runtime.o \
				 redis/redis_floor_store.o \
				 redis/redis_presence_payload.o \
				 redis/redis_presence_runtime.o \
				 redis/redis_presence_worker.o \
				 redis/redis_world_runtime.o \
				 redis/redis_world_store.o \
				 currency_transaction.o \
				 player/player_revision_state.o \
				 player/player_save_worker.o \
				 player/player_save_journal.o \
				 player/player_save_pipeline.o \
				 world_recovery_npc_items.o \
				 world_recovery_pipeline.o \
				 player/player_snapshot_repository.o \
				 player/player_snapshot_capture.o \
				 player/player_snapshot_codec.o \
				 persistence/persistence_queue.o \
				 persistence/latency_trace.o \
				 sql/sql_persistence_raw.o \
				 sql/sql_pool.o \
				 plushit.o \
				 net/poll.o \
				 net/prompt.o \
				 properties.o \
				 classes/psionics.o \
				 quest.o \
				 random.o \
				 randomeq.o \
				 random.mob.o \
				 random.zone.o \
				 randobj.o \
				 combat/range.o \
				 classes/rogues.o \
				 classes/shaman.o \
				 shop.o \
				 signals.o \
				 classes/skills.o \
				 magic/smagic.o \
				 classes/salchemist.o \
				 classes/sillusionist.o \
				 combat/siege.o \
				 net/sparser.o \
				 specials.o \
				 classes/specializations.o \
				 specs/specs.assign.o \
				 specs/specs.barovia.o \
				 specs/specs.clwcvrn.o \
				 specs/specs.dragonnia.o \
				 specs/specs.eth2.o \
				 specs/specs.halfcut.o \
				 specs/specs.ravenloft.o \
				 specs/specs.snogres.o \
				 specs/specs.ioun.o \
				 specs/specs.jot.o \
				 specs/specs.library.o \
				 studioproc.o \
				 studioproclib.o \
				 specs/specs.lohrr.o \
				 specs/specs.mobile.o \
				 specs/specs.morgs.o \
				 specs/specs.mist.o \
				 specs/specs.object.o \
				 specs/specs.realm.o \
				 specs/specs.room.o \
				 specs/specs.snikzone.o \
				 specs/specs.grove.o \
				 specs/specs.hoa.o \
				 specs/specs.highway.o \
				 specs/specs.tharnadia.o \
				 specs/specs.twintowers.o \
				 specs/specs.undermountain.o \
				 specs/specs.underworld.o \
				 specs/specs.vecna.o \
				 specs/specs.venthix.o \
				 specs/specs.verzanan.o \
				 specs/specs.zalrix.o \
				 specs/specs.set.o \
				 specs/specs.shabo.o \
				 specs/specs.juiblex.o \
				 specs/specs.keleks.o \
				 specs/specs.caertannad.o \
				 specs/specs.firep.o \
				 specs/specs.winterhaven.o \
				 specs/specs.zion.o \
				 magic/spells.o \
				 redis/redis.o \
				 redis/wizredis.o \
				 sql/sql.o \
				 sql/sql_player.o \
				 net/ssl.o \
				 statistics.o \
				 storage_lockers.o \
				 persistence/locker_async.o \
				 timers.o \
				 cmd/track.o \
				 tradeskill.o \
				 combat/trap.o \
				 transport.o \
				 trophy.o \
				 net/unicode.o \
				 utility.o \
				 weather.o \
				 ferry.o \
				 ferryact.o \
				 classes/paladins.o \
				 combat/guard.o \
				 makeexit.o \
				 combat/racewar_stat_mods.o \
				 nexus_stones.o \
				 test_async.o \
				 cmd/testcmd.o \
				 cmd/wikihelp.o \
				 classes/reavers.o \
				 classes/avengers.o \
				 specs/specs.lucrot.o \
				 specs/specs.gellz.o \
				 smoke.o \
				 account/multiplay_whitelist.o \
				 home.o \
				 net/websocket.o \
				 net/ws_handlers.o \
				 json_utils.o \
				 net/gmcp.o \
				 net/ttype.o \
				 account/wiz_newchar.o \
				 zcheck.o


# Name lists (no paths) for pfile-related objs
SHARED_PFILE_OBJS_NAMES = skills.pf.o files.pf.o
PFILE_OBJS_NAMES        = account/pfile-stubs.o account/pfile.o

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

# Shared pfile objects (*.pf.o) also go into OBJDIR
$(OBJDIR)/%.pf.o: %.c
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
