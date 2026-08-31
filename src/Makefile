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

INCLUDES	= -I. -I../tests/async -I/usr/include/libxml2 $(MYSQL_INCLUDES)

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
				 world/achievements.o \
				 account/account.o \
				 account/password_hash.o \
				 account/account_reward.o \
				 account/account_reward_snapshot.o \
				 account/account_reward_config.o \
				 world/hardcore_config.o \
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
				 guild/alliances.o \
				 mob/automatic_rules.o \
				 magic/affects.o \
				 combat/arena.o \
				 guild/artifact.o \
				 guild/artifact_cache_codec.o \
				 persistence/report_cache_codec.o \
				 economy/donation_event.o \
				 redis/redis_connection.o \
				 redis/redis_command_observability.o \
				 redis/redis_donation_runtime.o \
				 redis/redis_key_registry.o \
				 redis/redis_namespace.o \
				 world/world_recovery_codec.o \
				 redis/redis_donation_worker.o \
				 guild/assocs.o \
				 economy/auction_houses.o \
				 classes/bard.o \
				 magic/beh_magic.o \
         magic/blispells.o \
				 cmd/boards.o \
				 economy/boon.o \
				 combat/breath_weapons.o \
				 world/buildings.o \
         economy/cardgames.o \
         combat/chaos.o \
				 combat/chaos_config.o \
				 net/comm.o \
				 persistence/copyover.o \
				 core/common.o \
				 world/condition.o \
				 core/constant.o \
				 combat/ctf.o \
				 combat/dam_mods.o \
				 world/db.o \
				 account/character_identity.o \
				 core/debug.o \
				 classes/disguise.o \
				 classes/drannak.o \
				 item/enhance.o \
				 economy/crafting.o \
				 economy/mining.o \
				 economy/mining_config.o \
				 item/random_equipment_config.o \
				 core/safe_format.o \
				 core/safe_io.o \
				 item/salvage.o \
				 item/material_rarity.o \
				 classes/dreadlord.o \
				 net/editor.o \
				 mob/encounters.o \
				 world/epic.o \
				 world/epic_bonus.o \
				 world/epic_bonus_state.o \
				 persistence/deferred_save_policy.o \
				 classes/epic_skills.o \
				 classes/ethermancer.o \
				 world/events.o \
				 world/event_names.o \
				 world/new_events.o \
				 world/nevent_periodic.o \
				 combat/fight.o \
				 core/files.o \
				 combat/fraglist.o \
				 item/forge_items.o \
				 world/graph.o \
				 guild/group.o \
				 guild/guild.o \
				 guild/guildhall.o \
				 guild/guildhall_rooms.o \
				 guild/guildhall_cmds.o \
				 guild/guildhall_procs.o \
				 guild/guildhall_db.o \
				 combat/grapple.o \
				 world/handler.o \
				 world/hardcore.o \
				 classes/innates.o \
				 cmd/interp.o \
				 combat/justice.o \
				 combat/kick.o \
				 classes/language.o \
				 world/limits.o \
				 item/linked_objects.o\
				 net/listen.o\
				 cmd/mail.o \
				 world/map.o \
				 net/mccp.o \
				 classes/memorize.o \
				 core/memory.o \
				 core/mm.o \
				 mob/mobact.o \
				 world/world_quest.o \
				 combat/mobcombat.o \
				 mob/mobconv.o \
				 mob/mobpatrol.o \
				 net/modify.o \
				 classes/mount.o \
				 mob/name_gen.o \
				 account/nanny.o \
				 classes/necromancy.o \
				 classes/new_skills.o \
				 cmd/nq.o \
				 item/objconv.o \
				 item/objmisc.o \
				 world/outposts.o \
				 persistence/persistence_checkpoint.o \
				 persistence/persistence_observability.o \
				 persistence/persistence_mode.o \
				 core/env_file.o \
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
				 item/item_transfer_command.o \
				 item/item_ownership_runtime.o \
				 item/item_movement_transaction.o \
				 economy/shop_trade_transaction.o \
				 economy/shop_trade_runtime.o \
				 item/item_transfer_repository.o \
				 item/item_transfer_synthetic.o \
				 item/item_uid_allocator.o \
				 persistence/critical_outbox.o \
				 world/epic_command.o \
				 world/epic_transaction.o \
				 economy/currency_command.o \
				 economy/auction_command.o \
				 economy/auction_repository.o \
				 economy/auction_transaction.o \
				 combat/combat_outcome_command.o \
				 combat/combat_outcome_repository.o \
				 combat/combat_outcome_transaction.o \
				 guild/artifact_guild_command.o \
				 guild/artifact_guild_repository.o \
				 guild/artifact_guild_state.o \
				 guild/artifact_guild_transaction.o \
				 economy/boon_reward_command.o \
				 economy/boon_reward_repository.o \
				 economy/boon_reward_transaction.o \
				 economy/boon_shop_command.o \
				 economy/boon_shop_transaction.o \
				 economy/shop_trade_command.o \
				 world/zone_touch_command.o \
				 world/zone_touch_repository.o \
				 world/zone_touch_transaction.o \
				 account/session_audit_command.o \
				 account/session_audit_repository.o \
				 account/session_audit_transaction.o \
				 player/player_load_repository.o \
				 player/player_load_topology.o \
				 player/player_load_pipeline.o \
				 player/player_load_items.o \
				 player/player_load_pets.o \
				 persistence/gameplay_read_state.o \
				 world/epic_task_catalog.o \
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
				 economy/currency_transaction.o \
				 player/player_revision_state.o \
				 player/player_save_worker.o \
				 player/player_save_journal.o \
				 player/player_save_pipeline.o \
				 world/world_recovery_npc_items.o \
				 world/world_recovery_pipeline.o \
				 player/player_snapshot_repository.o \
				 player/player_snapshot_capture.o \
				 player/player_snapshot_codec.o \
				 persistence/persistence_queue.o \
				 persistence/latency_trace.o \
				 sql/sql_persistence_raw.o \
				 sql/sql_pool.o \
				 item/plushit.o \
				 net/poll.o \
				 net/prompt.o \
				 world/properties.o \
				 classes/psionics.o \
				 world/quest.o \
				 core/random.o \
				 item/randomeq.o \
				 world/random.mob.o \
				 world/random.zone.o \
				 item/randobj.o \
				 combat/range.o \
				 classes/rogues.o \
				 classes/shaman.o \
				 economy/shop.o \
				 core/signals.o \
				 classes/skills.o \
				 magic/smagic.o \
				 classes/salchemist.o \
				 classes/sillusionist.o \
				 combat/siege.o \
				 net/sparser.o \
				 mob/specials.o \
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
				 mob/studioproc.o \
				 mob/studioproclib.o \
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
				 world/statistics.o \
				 item/storage_lockers.o \
				 persistence/locker_async.o \
				 world/timers.o \
				 cmd/track.o \
				 economy/tradeskill.o \
				 combat/trap.o \
				 world/transport.o \
				 item/trophy.o \
				 net/unicode.o \
				 core/utility.o \
				 world/weather.o \
				 world/ferry.o \
				 world/ferryact.o \
				 classes/paladins.o \
				 combat/guard.o \
				 world/makeexit.o \
				 combat/racewar_stat_mods.o \
				 economy/nexus_stones.o \
				 core/test_async.o \
				 cmd/testcmd.o \
				 cmd/wikihelp.o \
				 classes/reavers.o \
				 classes/avengers.o \
				 specs/specs.lucrot.o \
				 specs/specs.gellz.o \
				 core/smoke.o \
				 account/multiplay_whitelist.o \
				 world/home.o \
				 net/websocket.o \
				 net/ws_handlers.o \
				 core/json_utils.o \
				 net/gmcp.o \
				 net/ttype.o \
				 account/wiz_newchar.o \
				 world/zcheck.o


# Name lists (no paths) for pfile-related objs
SHARED_PFILE_OBJS_NAMES = classes/skills.pf.o core/files.pf.o
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

$(PFILE_BINARY): $(PFILE_OBJS) $(SHARED_PFILE_OBJS) $(OBJDIR)/core/mm.o $(OBJDIR)/core/common.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -g -o $@ \
		$(PFILE_OBJS) $(SHARED_PFILE_OBJS) \
		$(OBJDIR)/core/common.o $(OBJDIR)/core/mm.o $(LIBS)

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
	rm -f *~ */*~ .depend */.depend
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
