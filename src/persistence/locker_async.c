/*
 * locker_async.c -- main-thread snapshot, persistence writer apply.
 *
 * Design:
 *  - Per-locker dirty slots coalesce many save triggers into one generation.
 *  - At most LOCKER_ASYNC_SNAPSHOTS_PER_PULSE new snapshots start per pulse.
 *  - A snapshot captures what the locker character carries, with the locker's
 *    name and owner, while that player is object-command locked.
 *  - The persistence writer applies it in capture order with every other save:
 *    it finds or creates the locker's row and public chest, claims the items and
 *    replaces the rows. The game thread never queries the database for a locker
 *    save.
 *  - Completion runs on the next main pulse (extract terminal locker char, unlock).
 *    A terminal save that fails is retried through the writer later; the locker
 *    character keeps the items until one lands.
 *
 * Private chests are saved by StorageLocker::LockerToPFile(), which queues each on
 * the writer as its own job.
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/utility.h"
#include "item/storage_lockers.h"
#include "persistence/locker_async.h"
#include "player/player_save_worker.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_repository.h"
#include "flatfile/flatfile_player_repository.h"
#include "persistence/persistence_mode.h"
#include "sql/sql_pool.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"

#include <deque>
#include <memory>
#include <new>
#include <string>
#include <unordered_map>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern P_char character_list;
extern P_room world;

#define LOCKER_ASYNC_NAME_LEN 128
/* How long a failed terminal save waits before the writer tries it again. */
#define LOCKER_ASYNC_RETRY_SECONDS 30

enum locker_async_state
{
	LCHK_FREE = 0,
	LCHK_DIRTY,
	LCHK_INFLIGHT
};

struct locker_async_slot
{
	enum locker_async_state state;
	char locker_name[LOCKER_ASYNC_NAME_LEN];
	int terminal;
	int rebuild_objects; /* another dirty landed while inflight */
	unsigned long gen;
	time_t dirty_at;
	time_t retry_at; /* a failed terminal save waits for this before the pulse starts it */
	/* The user by pid and the locker character by name, found again when a snapshot
	 * starts: either may be extracted before then (a user saved on the way out). */
	int user_pid;
};

struct locker_async_job
{
	char locker_name[LOCKER_ASYNC_NAME_LEN];
	unsigned long gen;
	int terminal;
	int user_pid;
	// What the public chest holds in memory, with the locker's name and owner.
	std::shared_ptr<locker_snapshot> snapshot;
};

struct locker_async_result
{
	char locker_name[LOCKER_ASYNC_NAME_LEN];
	unsigned long gen;
	int ok;
	int terminal;
	int user_pid;
};

/* A deque never moves its elements, so slot pointers stay valid as it grows. */
static std::deque<struct locker_async_slot> g_slots;
static unsigned long g_gen_seq = 1;
static int g_snapshots_started_this_pulse = 0;
/* Each locker's writer key, stable for the life of the process. */
static std::unordered_map<std::string, uint64_t> g_job_keys;

static pthread_mutex_t g_q_mu = PTHREAD_MUTEX_INITIALIZER;
static std::deque<struct locker_async_result> g_results;
static int g_inited = 0;

static int locker_async_worker_available(void)
{
	return player_save_worker_health_copy().running;
}

/* ---------------- slot helpers (main only) ---------------- */

static struct locker_async_slot *slot_find(const char *name)
{
	if (!name || !*name)
		return NULL;
	for (struct locker_async_slot &slot : g_slots)
		if (slot.state != LCHK_FREE && !str_cmp(slot.locker_name, name))
			return &slot;
	return NULL;
}

static struct locker_async_slot *slot_alloc(const char *name)
{
	struct locker_async_slot *s = slot_find(name);
	if (s)
		return s;
	for (struct locker_async_slot &slot : g_slots)
		if (slot.state == LCHK_FREE)
		{
			s = &slot;
			break;
		}
	if (!s)
	{
		try
		{
			g_slots.emplace_back();
		}
		catch (const std::bad_alloc &)
		{
			return NULL;
		}
		s = &g_slots.back();
	}
	memset(s, 0, sizeof(*s));
	snprintf(s->locker_name, sizeof(s->locker_name), "%s", name);
	s->state = LCHK_DIRTY;
	return s;
}

static void slot_clear(struct locker_async_slot *s)
{
	if (!s)
		return;
	memset(s, 0, sizeof(*s));
	s->state = LCHK_FREE;
}

/* A failed terminal save goes back to the writer later. The locker character keeps
 * the items until one lands; its user has left, so nothing stays object-locked. */
static void slot_retry_later(struct locker_async_slot *s)
{
	s->state = LCHK_DIRTY;
	s->gen = g_gen_seq++;
	s->dirty_at = time(NULL);
	s->retry_at = s->dirty_at + LOCKER_ASYNC_RETRY_SECONDS;
	s->user_pid = 0;
}

int locker_async_player_obj_locked(P_char ch)
{
	int pid;

	if (!ch || IS_NPC(ch))
		return 0;
	pid = GET_PID(ch);
	if (pid <= 0)
		return 0;
	/* Only lock while DIRTY (waiting for / during the main-thread snapshot
	 * start). Once INFLIGHT the snapshot is sealed; unlock the player. */
	for (const struct locker_async_slot &slot : g_slots)
	{
		if (slot.state == LCHK_DIRTY && slot.user_pid == pid)
			return 1;
	}
	return 0;
}

int locker_async_name_busy(const char *locker_name)
{
	struct locker_async_slot *s = slot_find(locker_name);
	return (s && s->state != LCHK_FREE) ? 1 : 0;
}

/* ---------------- snapshot (main only) ---------------- */

/* The owner, from the locker's name: guild.<id>.locker, account.<name>.locker or
 * <player>.locker. The writer looks a player up only to create a new locker's row. */
static void locker_owner(P_char chLocker, locker_snapshot *snapshot)
{
	const char *name = GET_NAME(chLocker);

	snapshot->racewar = GET_RACEWAR(chLocker);
	snapshot->race = GET_RACE(chLocker);
	if (strncmp(name, "guild.", 6) == 0)
		snapshot->owner_assoc_id = atoi(name + 6);
	else if (strncmp(name, "account.", 8) != 0)
	{
		snapshot->owner_name = name;
		const size_t dot = snapshot->owner_name.find(".locker");
		if (dot != std::string::npos)
			snapshot->owner_name.resize(dot);
	}
}

/* Each locker keeps one writer key, so a newer save can replace its queued one. Keys
 * stay below 2^32 and never match a private chest's (chest_id << 32 | locker_id). */
static uint64_t locker_job_key(const char *name)
{
	std::string key = name;
	for (char &c : key)
		c = LOWER(c);
	const auto found = g_job_keys.find(key);
	if (found != g_job_keys.end())
		return found->second;
	const uint64_t assigned = g_job_keys.size() + 1;
	g_job_keys.emplace(std::move(key), assigned);
	return assigned;
}

const char *locker_async_job_name(unsigned long long key)
{
	for (const auto &entry : g_job_keys)
		if (entry.second == key)
			return entry.first.c_str();
	return NULL;
}

/* ---------------- writer ---------------- */

/* Flat-file writes the public chest into its locker catalog; MariaDB into its rows. */
static player_save_apply_result write_locker(const locker_snapshot &snapshot)
{
	const char *root = persistence_mode_flatfile_root();
	if (!root)
		return locker_snapshot_repository_apply_from_pool(snapshot);
	flatfile_locker_save save;
	save.locker_name = snapshot.locker_name;
	save.owner_assoc_id = snapshot.owner_assoc_id;
	save.racewar = static_cast<int8_t>(snapshot.racewar);
	save.race = static_cast<int8_t>(snapshot.race);
	save.items = snapshot.items;
	for (player_item_snapshot &item : save.items)
		item.equipment_slot = -1;
	std::string error;
	return flatfile_locker_snapshot_apply(root, save, &error);
}

/* Runs on the persistence writer thread. A lost connection goes back to the
 * writer, which retries this job before any later save. */
static player_save_apply_result locker_write_job(const struct locker_async_job &job)
{
	const player_save_apply_result applied = write_locker(*job.snapshot);
	if (applied.outcome == player_save_apply_outcome::retryable_failure ||
	    applied.outcome == player_save_apply_outcome::ambiguous_commit)
		return applied;

	struct locker_async_result res;
	memset(&res, 0, sizeof(res));
	snprintf(res.locker_name, sizeof(res.locker_name), "%s", job.locker_name);
	res.gen = job.gen;
	res.ok = applied.outcome == player_save_apply_outcome::applied;
	res.terminal = job.terminal;
	res.user_pid = job.user_pid;
	pthread_mutex_lock(&g_q_mu);
	try
	{
		g_results.push_back(res);
	}
	catch (const std::bad_alloc &)
	{
		logit(LOG_FILE, "locker_async: result lost for %s gen=%lu", res.locker_name,
		      res.gen);
	}
	pthread_mutex_unlock(&g_q_mu);
	return applied;
}

static int job_push(const struct locker_async_job &job, size_t bytes)
{
	const player_save_submit_result submitted = persistence_writer_submit(
		persistence_job_kind::locker, locker_job_key(job.locker_name), bytes,
		[job]() { return locker_write_job(job); });
	return submitted == player_save_submit_result::accepted ||
	       submitted == player_save_submit_result::replaced;
}

/* ---------------- mark dirty / pulse / completion ---------------- */

int locker_async_mark_dirty(P_char chLocker, P_char chUser, int terminal, const char *reason)
{
	struct locker_async_slot *s;
	const char *name;

	if (!g_inited || !locker_async_worker_available())
		return 0;
	if (!chLocker || !GET_NAME(chLocker))
		return 0;
	name = GET_NAME(chLocker);

	s = slot_alloc(name);
	if (!s)
	{
		persistence_alert(AVATAR, "locker_async", name, "none", "none", "slots_full",
				  "dirty table allocation failed; reason=%s",
				  reason ? reason : "unknown");
		return 0;
	}

	if (terminal)
		s->terminal = 1;
	if (chUser && !IS_NPC(chUser))
		s->user_pid = GET_PID(chUser);

	if (s->state == LCHK_INFLIGHT)
	{
		/* Coalesce: ask for another pass after current gen finishes. */
		s->rebuild_objects = 1;
		if (terminal)
			s->terminal = 1;
	}
	else
	{
		s->state = LCHK_DIRTY;
		s->gen = g_gen_seq++;
		s->dirty_at = time(NULL);
		s->retry_at = 0;
	}

	logit(LOG_DEBUG, "locker_async: mark dirty name=%s terminal=%d gen=%lu reason=%s state=%d",
	      name, terminal ? 1 : 0, s->gen, reason ? reason : "?", s->state);
	return 1;
}

static P_char find_char_by_pid(int pid)
{
	P_char ch;
	if (pid <= 0)
		return NULL;
	for (ch = character_list; ch; ch = ch->next)
		if (IS_PC(ch) && GET_PID(ch) == pid)
			return ch;
	return NULL;
}

static P_char find_locker_char_by_name(const char *name)
{
	P_char ch;
	P_char found = NULL;
	int matches = 0;

	if (!name)
		return NULL;
	for (ch = character_list; ch; ch = ch->next)
	{
		if (!GET_NAME(ch) || str_cmp(GET_NAME(ch), name))
			continue;
		/* Prefer dedicated locker temporary chars (no real PID in play). */
		matches++;
		if (!found)
			found = ch;
		/* Prefer non-descriptor temporary locker avatars if present. */
		if (!ch->desc && (!found || found->desc))
			found = ch;
	}
	if (matches > 1)
		logit(LOG_DEBUG,
		      "locker_async: name lookup ambiguous name=%s matches=%d; using preferential temp/locker char",
		      name, matches);
	return found;
}

static void apply_result(struct locker_async_result *r)
{
	struct locker_async_slot *s = slot_find(r->locker_name);
	P_char chLocker;
	P_char chUser;

	if (!s)
	{
		logit(LOG_DEBUG, "locker_async: result for unknown slot %s gen=%lu ok=%d",
		      r->locker_name, r->gen, r->ok);
		return;
	}

	if (r->gen != s->gen)
	{
		logit(LOG_DEBUG, "locker_async: stale result name=%s res_gen=%lu slot_gen=%lu",
		      s->locker_name, r->gen, s->gen);
		if (s->rebuild_objects)
			s->state = LCHK_DIRTY;
		return;
	}

	chLocker = find_locker_char_by_name(s->locker_name);
	chUser = find_char_by_pid(s->user_pid);

	if (!r->ok)
		persistence_alert(AVATAR, "locker_async", s->locker_name, "none", "none",
				  "worker_failed",
				  "locker save failed gen=%lu terminal=%d; the writer tries again",
				  s->gen, s->terminal ? 1 : 0);

	if (s->terminal)
	{
		/* A save that landed while a newer one waits (the user left during an
		 * in-stay save) is not the one holding what the locker character carries. */
		if (r->ok && chLocker && !s->rebuild_objects)
		{
			chLocker->specials.timer = 0;
			extract_char(chLocker);
		}
	}
	else if (chLocker && chUser)
	{
		locker_async_request_resort(chLocker, chUser);
	}

	if (s->rebuild_objects)
	{
		s->rebuild_objects = 0;
		s->state = LCHK_DIRTY;
		s->gen = g_gen_seq++;
		s->dirty_at = time(NULL);
		s->retry_at = 0;
	}
	else if (r->ok || !s->terminal)
	{
		/* A failed in-stay save is carried by the locker's next save. */
		slot_clear(s);
	}
	else
	{
		slot_retry_later(s);
	}
}

static void drain_results(void)
{
	for (;;)
	{
		struct locker_async_result r;
		int found = 0;

		pthread_mutex_lock(&g_q_mu);
		if (!g_results.empty())
		{
			r = g_results.front();
			g_results.pop_front();
			found = 1;
		}
		pthread_mutex_unlock(&g_q_mu);
		if (!found)
			break;
		apply_result(&r);
	}
}

static int start_one_snapshot(struct locker_async_slot *s)
{
	struct locker_async_job job = {};
	size_t bytes = 0;
	P_char chLocker;
	P_char chUser;

	if (!s || s->state != LCHK_DIRTY)
		return 0;
	if (g_snapshots_started_this_pulse >= LOCKER_ASYNC_SNAPSHOTS_PER_PULSE)
		return 0;

	chLocker = find_locker_char_by_name(s->locker_name);
	chUser = find_char_by_pid(s->user_pid);
	if (!chLocker)
	{
		logit(LOG_FILE, "locker_async: dirty locker char missing for %s -- clearing",
		      s->locker_name);
		slot_clear(s);
		return 0;
	}

	/* Always re-prepare live inventory onto the locker char before sealing.
	 * Terminal leave already did LockerToPFile; a second call is mostly no-op.
	 * Non-terminal rebuild gens MUST re-walk after a prior restore. */
	if (chUser && chUser->in_room != NOWHERE && IS_ROOM(chUser->in_room, ROOM_LOCKER))
	{
		if (!locker_async_prepare_snapshot(chUser))
		{
			persistence_alert(AVATAR, "locker_async", s->locker_name, "none", "none",
					  "prepare_failed",
					  "LockerToPFile failed before snapshot; aborting gen");
			slot_clear(s);
			return 0;
		}
	}

	/* Player is obj-locked via user_pid while DIRTY. Capture the snapshot now. */
	snprintf(job.locker_name, sizeof(job.locker_name), "%s", s->locker_name);
	job.gen = s->gen;
	job.terminal = s->terminal;
	job.user_pid = s->user_pid;
	try
	{
		auto snapshot = std::make_shared<locker_snapshot>();
		snapshot->locker_name = s->locker_name;
		locker_owner(chLocker, snapshot.get());
		if (player_item_snapshot_list_capture(chLocker, false, true, false,
						      &snapshot->items,
						      &bytes) == player_snapshot_capture_result::ok)
			job.snapshot = std::move(snapshot);
	}
	catch (const std::bad_alloc &)
	{
		job.snapshot.reset();
	}

	if (!job.snapshot || !job_push(job, sizeof(locker_snapshot) + bytes))
	{
		persistence_alert(AVATAR, "locker_async", s->locker_name, "none", "none",
				  job.snapshot ? "job_queue_full" : "snapshot_failed",
				  "locker save not queued terminal=%d; %s", s->terminal ? 1 : 0,
				  s->terminal ? "the writer tries again later" :
						"the locker's next save carries it");
		if (s->terminal)
			slot_retry_later(s);
		else
		{
			if (chUser)
				locker_async_restore_snapshot_view(chUser);
			slot_clear(s);
		}
		return 0;
	}

	s->state = LCHK_INFLIGHT;
	g_snapshots_started_this_pulse++;

	/* Non-terminal mid-stay: restore chests/floor from the sealed snapshot
	 * copy (items still on chLocker). Main-thread walk is done; unlock
	 * happens because state is no longer DIRTY. */
	if (!s->terminal && chUser)
		locker_async_restore_snapshot_view(chUser);

	logit(LOG_DEBUG, "locker_async: enqueued name=%s gen=%lu terminal=%d", s->locker_name,
	      s->gen, s->terminal ? 1 : 0);
	return 1;
}

void locker_async_pulse(void)
{
	struct locker_async_slot *oldest_terminal = NULL;
	struct locker_async_slot *oldest_any = NULL;
	const time_t now = time(NULL);

	if (!g_inited)
		return;

	g_snapshots_started_this_pulse = 0;
	drain_results();

	for (struct locker_async_slot &slot : g_slots)
	{
		struct locker_async_slot *s = &slot;
		if (s->state != LCHK_DIRTY || s->retry_at > now)
			continue;
		if (s->terminal)
		{
			if (!oldest_terminal || s->dirty_at < oldest_terminal->dirty_at)
				oldest_terminal = s;
		}
		if (!oldest_any || s->dirty_at < oldest_any->dirty_at)
			oldest_any = s;
	}

	if (oldest_terminal)
		start_one_snapshot(oldest_terminal);
	else if (oldest_any)
		start_one_snapshot(oldest_any);
}

int locker_async_drain(int wait_ms)
{
	int spins = 0;
	int max_spins = (wait_ms > 0) ? (wait_ms / 10) + 1 : 1;

	if (!g_inited)
		return 1;

	while (spins++ < max_spins)
	{
		int pending = 0;

		/* A drain starts every dirty locker, retries included, without pacing. */
		for (struct locker_async_slot &slot : g_slots)
		{
			if (slot.state == LCHK_DIRTY)
			{
				g_snapshots_started_this_pulse = 0;
				start_one_snapshot(&slot);
			}
		}
		drain_results();
		for (const struct locker_async_slot &slot : g_slots)
			if (slot.state != LCHK_FREE)
				pending = 1;
		if (!pending)
			return 1;
		usleep(10000);
	}
	return 0;
}

void locker_async_init(void)
{
	if (g_inited)
		return;
	if (!sql_pool_is_active() && !persistence_mode_flatfile_root())
	{
		logit(LOG_STATUS,
		      "locker_async: connection pool unavailable; async locker saves disabled");
		persistence_alert(AVATAR, "locker_async", "worker", "none", "none",
				  "pool_unavailable", "locker async saves disabled");
		return;
	}
	g_slots.clear();
	pthread_mutex_lock(&g_q_mu);
	g_results.clear();
	pthread_mutex_unlock(&g_q_mu);
	g_inited = 1;
	logit(LOG_STATUS, "Locker saves go through the persistence writer.");
}

void locker_async_shutdown(void)
{
	if (!g_inited)
		return;
	if (locker_async_worker_available())
		locker_async_drain(2000);
	g_inited = 0;
}
