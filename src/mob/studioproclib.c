/*
   ***************************************************************************
   *  File: studioproclib.c                                   Part of Duris *
   *  Usage: the 'sayresponse' and 'transporter' object proclibs            *
   ***************************************************************************
   *
   * Ported in behaviour from a 2020-era uncommitted patch that carried
   * both procs inside specs.library.c, with three corrections:
   *
   *  1. sprintf() -> snprintf() on both parameter builders.  The originals
   *     wrote two MAX_STRING_LENGTH inputs into one fixed buffer; the
   *     build runs -D_FORTIFY_SOURCE=0, so nothing would have caught it.
   *  2. proclibobj_transporter called char_light()/room_light() after
   *     char_to_room(); char_to_room() already does the light bookkeeping,
   *     and the second call on a NOWHERE char is a crash.  The redundant
   *     calls are gone.
   *  3. The original reached these procs by patching special() in
   *     interp.c to walk every ground object on every command.  Instead
   *     proclibObj_add() installs proclib_obj_cmd_bridge() as the
   *     prototype proc of any vnum that gains a proclib, so the engine's
   *     OWN dispatch (interp.c:2229, "special in object present?") does
   *     the work and interp.c is not touched.  It is also cheaper: only
   *     vnums that actually carry a proclib are ever consulted.
   */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "core/prototypes.h"
#include "item/objmisc.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include "core/utility.h"
#include "mob/studioproclib.h"
#include <climits>
#include <algorithm>
#include <limits>
#include <utility>

extern P_index obj_index;
extern P_room world;

/* defined in specs.library.c */
extern char *proclib_getNext_string(char *source, char *nextString);
extern int proclib_obj_proc(P_obj obj, P_char ch, int cmd, char *argument);

/* ------------------------------------------------------------------ */
/* sayresponse: 'keywords' 'reply text'                                */
/* ------------------------------------------------------------------ */

char *proclibobj_parse_sayresponse(char *argument)
{
	char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
	char params[MAX_STRING_LENGTH * 2 + 2];
	char *pRet = NULL;

	argument = proclib_getNext_string(argument, arg1);
	if (arg1[0])
	{
		argument = proclib_getNext_string(argument, arg2);
		if (arg2[0])
		{
			checked_snprintf(params, sizeof(params), "%s\xFF%s", arg1, arg2);
			CREATE(pRet, char, strlen(params) + 1, MEM_TAG_EXDESCD);
			strcpy(pRet, params);
			return pRet;
		}
	}
	return NULL;
}

/* Object in the room (or held by the speaker) replies when a player says
   any of the keywords.  Reached from studioproc_speech() with CMD_SAY,
   AFTER the say text has landed, so the reply reads as a reply. */
int proclibobj_sayresponse(P_obj obj, P_char ch, int cmd, char *argument)
{
	struct extra_descr_data *ed;
	char low[MAX_STRING_LENGTH];
	int room = -1, li, replied = FALSE;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE; /* command-driven only */
	if (cmd != CMD_SAY || !obj || !ch || !argument || !*argument)
		return FALSE;

	if (OBJ_ROOM(obj))
		room = obj->loc.room;
	else if (OBJ_CARRIED(obj) && obj->loc.carrying)
		room = obj->loc.carrying->in_room;
	else if (OBJ_WORN(obj) && obj->loc.wearing)
		room = obj->loc.wearing->in_room;
	if (room < 0 || room != ch->in_room)
		return FALSE;

	for (li = 0; argument[li] && li < (int)sizeof(low) - 1; li++)
		low[li] = LOWER(argument[li]);
	low[li] = '\0';

	for (ed = obj->ex_description; ed; ed = ed->next)
	{
		char *delim;
		char kw[MAX_INPUT_LENGTH];
		const char *p;
		int hit = FALSE, k;
		char buf[MAX_STRING_LENGTH];

		if (!ed->keyword || !ed->description ||
		    strn_cmp(ed->keyword, "_proclib_sayresponse", 20))
			continue;

		delim = strchr(ed->description, '\xFF');
		if (!delim || !*(delim + 1))
			continue;

		p = ed->description;
		while (p < delim && !hit)
		{
			while (p < delim && *p == ' ')
				p++;
			for (k = 0; p < delim && *p != ' ' && k < (int)sizeof(kw) - 1; p++, k++)
				kw[k] = LOWER(*p);
			kw[k] = '\0';
			if (k && strstr(low, kw))
				hit = TRUE;
		}
		if (!hit)
			continue;

		snprintf(buf, sizeof(buf) - 3, "%s replies, '%s'",
			 obj->short_description ? obj->short_description : "something", delim + 1);
		CAP(buf);
		strcat(buf, "\r\n");
		send_to_room(buf, room);
		replied = TRUE;
	}
	return replied;
}

/* ------------------------------------------------------------------ */
/* transporter: keyword roomvnum                                       */
/* ------------------------------------------------------------------ */

char *proclibobj_parse_transporter(char *argument)
{
	char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
	char params[MAX_STRING_LENGTH + 16];
	char *pRet = NULL;

	argument = proclib_getNext_string(argument, arg1);
	if (arg1[0] && !is_number(arg1))
	{
		argument = proclib_getNext_string(argument, arg2);
		if (arg2[0] && is_number(arg2) && atoi(arg2) > 0)
		{
			checked_snprintf(params, sizeof(params), "%s\xFF%d", arg1, atoi(arg2));
			CREATE(pRet, char, strlen(params) + 1, MEM_TAG_EXDESCD);
			strcpy(pRet, params);
			return pRet;
		}
	}
	return NULL;
}

/* 'enter <keyword>' teleports the actor to the configured room, which is
   validated at fire time (an area can be renumbered under our feet). */
int proclibobj_transporter(P_obj obj, P_char ch, int cmd, char *argument)
{
	struct extra_descr_data *ed;
	char word[MAX_INPUT_LENGTH];
	int was_in;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE; /* command-driven only */
	if (cmd != CMD_ENTER || !obj || !ch || !argument)
		return FALSE;

	/* the transporter must be on the ground in the actor's room */
	if (!OBJ_ROOM(obj) || obj->loc.room != ch->in_room)
		return FALSE;

	one_argument(argument, word);
	if (!*word)
		return FALSE;

	for (ed = obj->ex_description; ed; ed = ed->next)
	{
		char *delim;
		char kw[MAX_INPUT_LENGTH];
		int klen, rnum;

		if (!ed->keyword || !ed->description ||
		    strn_cmp(ed->keyword, "_proclib_transporter", 20))
			continue;

		delim = strchr(ed->description, '\xFF');
		if (!delim || !*(delim + 1))
			continue;

		klen = (int)(delim - ed->description);
		if (klen <= 0 || klen >= (int)sizeof(kw))
			continue;
		strncpy(kw, ed->description, klen);
		kw[klen] = '\0';
		if (str_cmp(word, kw))
			continue;

		rnum = real_room(atoi(delim + 1));
		if (rnum < 0)
		{
			logit(LOG_STATUS,
			      "proclib transporter: obj %d keyword '%s' leads to missing room %d",
			      obj_index[obj->R_num].virtual_number, kw, atoi(delim + 1));
			send_to_char("It doesn't seem to lead anywhere.\r\n", ch);
			return TRUE;
		}
		if (rnum == ch->in_room)
		{
			send_to_char("You are already there.\r\n", ch);
			return TRUE;
		}

		act("$n steps into $p and vanishes.", TRUE, ch, obj, 0, TO_ROOM);
		act("You step into $p...", FALSE, ch, obj, 0, TO_CHAR);
		was_in = ch->in_room;
		char_from_room(ch);
		/* char_to_room() is bool and returns TRUE on SUCCESS
		   (handler.c:1039), so only a successful arrival gets the message
		   and the look. */
		if (char_to_room(ch, rnum, -1))
		{
			act("$n arrives in a swirl of mist.", TRUE, ch, 0, 0, TO_ROOM);
			if (IS_PC(ch))
			{
				char empty[2];

				empty[0] = '\0';
				do_look(ch, empty, CMD_LOOK);
			}
		}
		else if (char_in_list(ch) && IS_ALIVE(ch) && ch->in_room == NOWHERE)
		{
			/* char_to_room() may free ch before returning FALSE. char_in_list()
			   compares pointer values without dereferencing ch, so the state
			   checks are safe only after that liveness gate. A live character
			   still at NOWHERE was refused before placement and must be put
			   back; one already in the destination must not be inserted twice. */
			char_to_room(ch, was_in, -1);
			send_to_char("Something bars the way, and you step back out.\r\n", ch);
		}
		return TRUE;
	}
	return FALSE;
}

/* ------------------------------------------------------------------ */
/* prototype bridge                                                    */
/* ------------------------------------------------------------------ */

/* Installed on the vnum of any object that gains a proclib, so that
   interp.c's existing "special in object present?" walk delivers real
   commands to instance proclibs.  Deliberately silent for:
     cmd <= 0   - CMD_PERIODIC / CMD_SET_PERIODIC etc.  Periodic ticking
                  is already owned by proclib_obj_event (scheduled inside
                  proclibObj_add); returning TRUE here would make db.c
                  schedule a SECOND periodic event and double-tick.
     CMD_SAY    - speech is delivered from studioproc_speech() after the
                  say text has landed, so replies read as replies and the
                  player's say is never swallowed. */
/* THE DISPLACED-PROC CHAIN.

   A vnum can already own an object proc - a hand-written one, or
   studioproc_obj from the world.trg engine.  Refusing to install the
   bridge in that case (the original behaviour) kept the existing proc
   safe but left every instance proclib on that vnum unreachable: a
   transporter added at runtime to such a vnum would never see CMD_ENTER,
   silently.  Refusing is not the only way to be safe, though - we can
   install the bridge AND keep the displaced proc, calling it first.

   Order is deliberate and matches the rule world.trg already documents:
   the hand-written C proc runs first and a TRUE return means the data
   never runs.  So existing content cannot change behaviour; a proclib
   only sees commands the incumbent declined.

   Sized by distinct vnums that gain a proclib, which is small (boot-time
   _proclib_ edescs plus whatever an immortal adds), and only ever grown.
   Main game thread only, like the rest of this file. */
struct proclib_chain_ent
{
	int rnum;
	int (*prev)(P_obj, P_char, int, char *);
};
static struct proclib_chain_ent *proclib_chain = NULL;
static int proclib_chain_top = 0;
static int proclib_chain_cap = 0;

void proclib_chain_install(int rnum, int (*prev)(P_obj, P_char, int, char *))
{
	int i;

	if (rnum < 0 || !prev || prev == proclib_obj_cmd_bridge)
		return;
	for (i = 0; i < proclib_chain_top; i++)
		if (proclib_chain[i].rnum == rnum)
			return; /* already chained - never double-wrap */
	if (proclib_chain_top == proclib_chain_cap)
	{
		int newcap = proclib_chain_cap ? proclib_chain_cap * 2 : 32;
		struct proclib_chain_ent *grown =
			(struct proclib_chain_ent *)realloc(proclib_chain, newcap * sizeof(*grown));

		if (!grown)
			return; /* out of memory: leave the incumbent alone */
		proclib_chain = grown;
		proclib_chain_cap = newcap;
	}
	proclib_chain[proclib_chain_top].rnum = rnum;
	proclib_chain[proclib_chain_top].prev = prev;
	proclib_chain_top++;
}

static int (*proclib_chain_prev(int rnum))(P_obj, P_char, int, char *)
{
	int i;

	for (i = 0; i < proclib_chain_top; i++)
		if (proclib_chain[i].rnum == rnum)
			return proclib_chain[i].prev;
	return NULL;
}

proclib_recovery_chain_stage::~proclib_recovery_chain_stage() noexcept
{
	reset();
}
proclib_recovery_chain_stage::proclib_recovery_chain_stage(
	proclib_recovery_chain_stage &&other) noexcept
{
	*this = std::move(other);
}
proclib_recovery_chain_stage &
proclib_recovery_chain_stage::operator=(proclib_recovery_chain_stage &&other) noexcept
{
	if (this != &other)
	{
		reset();
		allocation_ = std::exchange(other.allocation_, nullptr);
		expected_ = other.expected_;
		expected_top_ = other.expected_top_;
		expected_cap_ = other.expected_cap_;
		next_top_ = other.next_top_;
		next_cap_ = other.next_cap_;
		prepared_ = std::exchange(other.prepared_, false);
		requests_ = std::move(other.requests_);
	}
	return *this;
}
void proclib_recovery_chain_stage::reset() noexcept
{
	free(allocation_);
	allocation_ = nullptr;
	prepared_ = false;
	requests_.clear();
}
bool proclib_recovery_chain_stage::predecessor_matches(int rnum, obj_proc_type previous) noexcept
{
	return rnum >= 0 && proclib_chain_prev(rnum) == previous;
}
bool proclib_recovery_chain_stage::prepare(std::span<const request> requests,
					   proclib_recovery_chain_stage &output) noexcept
{
	if (!nevent_is_game_thread() || proclib_chain_top < 0 ||
	    proclib_chain_cap < proclib_chain_top || (proclib_chain_cap && !proclib_chain))
		return false;
	try
	{
		proclib_recovery_chain_stage candidate;
		candidate.expected_ = proclib_chain;
		candidate.expected_top_ = proclib_chain_top;
		candidate.expected_cap_ = proclib_chain_cap;
		candidate.requests_.assign(requests.begin(), requests.end());
		size_t additions = 0;
		for (size_t i = 0; i < requests.size(); ++i)
		{
			const auto &value = requests[i];
			if (value.rnum < 0 || value.previous == proclib_obj_cmd_bridge)
				return false;
			for (size_t j = 0; j < i; ++j)
				if (requests[j].rnum == value.rnum)
					return false;
			bool present = false;
			for (int j = 0; j < proclib_chain_top; ++j)
				if (proclib_chain[j].rnum == value.rnum)
				{
					if (proclib_chain[j].prev != value.previous)
						return false;
					present = true;
					break;
				}
			if (!present && value.previous)
				++additions;
		}
		if (additions > static_cast<size_t>(INT_MAX - proclib_chain_top))
			return false;
		candidate.next_top_ = proclib_chain_top + static_cast<int>(additions);
		candidate.next_cap_ = std::max(proclib_chain_cap, candidate.next_top_);
		if (additions)
		{
			if (static_cast<size_t>(candidate.next_cap_) >
			    std::numeric_limits<size_t>::max() / sizeof(proclib_chain_ent))
				return false;
			candidate.allocation_ = malloc(static_cast<size_t>(candidate.next_cap_) *
						       sizeof(proclib_chain_ent));
			if (!candidate.allocation_)
				return false;
			auto *entries = static_cast<proclib_chain_ent *>(candidate.allocation_);
			for (int j = 0; j < proclib_chain_top; ++j)
				entries[j] = proclib_chain[j];
			int next = proclib_chain_top;
			for (const auto &value : requests)
			{
				bool present = false;
				for (int j = 0; j < proclib_chain_top; ++j)
					if (proclib_chain[j].rnum == value.rnum)
					{
						present = true;
						break;
					}
				if (!present && value.previous)
					entries[next++] = { value.rnum, value.previous };
			}
			if (next != candidate.next_top_)
				return false;
		}
		candidate.prepared_ = true;
		output = std::move(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}
size_t proclib_recovery_chain_stage::retained_bytes() const noexcept
{
	size_t bytes = sizeof(*this);
	if (requests_.capacity() > (SIZE_MAX - bytes) / sizeof(request))
		return 0;
	bytes += requests_.capacity() * sizeof(request);
	if (allocation_)
	{
		if (next_cap_ < 0 ||
		    static_cast<size_t>(next_cap_) > (SIZE_MAX - bytes) / sizeof(proclib_chain_ent))
			return 0;
		bytes += static_cast<size_t>(next_cap_) * sizeof(proclib_chain_ent);
	}
	return bytes;
}
bool proclib_recovery_chain_stage::valid() const noexcept
{
	if (!prepared_ || !nevent_is_game_thread() || expected_ != proclib_chain ||
	    expected_top_ != proclib_chain_top || expected_cap_ != proclib_chain_cap)
		return false;
	if (allocation_)
	{
		const auto *entries = static_cast<const proclib_chain_ent *>(allocation_);
		for (int i = 0; i < expected_top_; ++i)
			if (entries[i].rnum != proclib_chain[i].rnum ||
			    entries[i].prev != proclib_chain[i].prev)
				return false;
	}
	for (const auto &value : requests_)
	{
		const auto previous = proclib_chain_prev(value.rnum);
		if (previous && previous != value.previous)
			return false;
	}
	return true;
}
void proclib_recovery_chain_stage::commit_unchecked() noexcept
{
	// Caller has validated the complete catalog/chain batch on the serialized
	// game thread. No allocation, callback, parser or checked failure follows.
	if (allocation_)
	{
		auto *old = proclib_chain;
		proclib_chain =
			static_cast<proclib_chain_ent *>(std::exchange(allocation_, nullptr));
		proclib_chain_top = next_top_;
		proclib_chain_cap = next_cap_;
		free(old);
	}
	prepared_ = false;
}

int proclib_obj_cmd_bridge(P_obj obj, P_char ch, int cmd, char *argument)
{
	if (item_restricted_for_player_pet(ch, obj))
		return FALSE;
	int (*prev)(P_obj, P_char, int, char *);

	if (!obj)
		return FALSE;

	/* the displaced proc keeps its original semantics, including the cmd
	   values this bridge itself ignores, so chaining cannot regress it */
	if (obj->R_num >= 0 && (prev = proclib_chain_prev(obj->R_num)) != NULL)
	{
		if (prev(obj, ch, cmd, argument))
			return TRUE;
	}

	if (cmd <= 0 || cmd == CMD_SAY)
		return FALSE;
	if (!IS_SET(obj->extra_flags, ITEM_PROCLIB))
		return FALSE;
	return proclib_obj_proc(obj, ch, cmd, argument);
}

#include <cerrno>
#include <new>
#include <type_traits>

bool proclib_recovery_chain_stage::prepare_bounded(
	const std::span<const request> &requests, proclib_recovery_chain_stage &output,
	bool (*reserve_scratch_peak)(size_t, void *) noexcept, void *context,
	size_t outer_live_scratch) noexcept
{
	if (!reserve_scratch_peak)
	{
		errno = EINVAL;
		return false;
	}
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)requests;
	(void)output;
	(void)context;
	(void)outer_live_scratch;
	errno = ENOTSUP;
	return false;
#else
	if (!nevent_is_game_thread() || proclib_chain_top < 0 ||
	    proclib_chain_cap < proclib_chain_top || (proclib_chain_cap && !proclib_chain))
	{
		errno = EINVAL;
		return false;
	}
	// Actual distinct candidate and fresh forward-range assign. Caller already
	// owns the source span/backing storage and old destination in outer.
	if (sizeof(proclib_recovery_chain_stage) > SIZE_MAX - outer_live_scratch ||
	    requests.size() > SIZE_MAX / sizeof(request))
	{
		errno = ENOBUFS;
		return false;
	}
	size_t live = outer_live_scratch + sizeof(proclib_recovery_chain_stage);
	const size_t clone_bytes = requests.size() * sizeof(request);
	if (clone_bytes > SIZE_MAX - live || !reserve_scratch_peak(live + clone_bytes, context))
	{
		errno = ENOBUFS;
		return false;
	}
	live += clone_bytes;
	try
	{
		proclib_recovery_chain_stage candidate;
		candidate.expected_ = proclib_chain;
		candidate.expected_top_ = proclib_chain_top;
		candidate.expected_cap_ = proclib_chain_cap;
		// Pinned libstdc++13 forward assign into a fresh vector requests EXACTLY
		// requests.size() rows. It allocates before the original row checks.
		candidate.requests_.assign(requests.begin(), requests.end());
		size_t additions = 0;
		for (size_t i = 0; i < requests.size(); ++i)
		{
			const auto &value = requests[i];
			if (value.rnum < 0 || value.previous == proclib_obj_cmd_bridge)
			{
				errno = EINVAL;
				return false;
			}
			for (size_t j = 0; j < i; ++j)
				if (requests[j].rnum == value.rnum)
				{
					errno = EINVAL;
					return false;
				}
			bool present = false;
			for (int j = 0; j < proclib_chain_top; ++j)
				if (proclib_chain[j].rnum == value.rnum)
				{
					if (proclib_chain[j].prev != value.previous)
					{
						errno = EINVAL;
						return false;
					}
					present = true;
					break;
				}
			if (!present && value.previous)
				++additions;
		}
		if (additions > static_cast<size_t>(INT_MAX - proclib_chain_top))
		{
			errno = EOVERFLOW;
			return false;
		}
		candidate.next_top_ = proclib_chain_top + static_cast<int>(additions);
		candidate.next_cap_ = std::max(proclib_chain_cap, candidate.next_top_);
		if (additions)
		{
			if (static_cast<size_t>(candidate.next_cap_) >
			    SIZE_MAX / sizeof(proclib_chain_ent))
			{
				errno = ENOBUFS;
				return false;
			}
			const size_t chain_bytes = static_cast<size_t>(candidate.next_cap_) *
						   sizeof(proclib_chain_ent);
			// The original global array stays live and unchanged. The distinct
			// candidate clone, replacement and row assignment temporary coexist.
			if (chain_bytes > SIZE_MAX - live ||
			    sizeof(proclib_chain_ent) > SIZE_MAX - live - chain_bytes ||
			    !reserve_scratch_peak(live + chain_bytes + sizeof(proclib_chain_ent),
						  context))
			{
				errno = ENOBUFS;
				return false;
			}
			candidate.allocation_ = malloc(chain_bytes);
			if (!candidate.allocation_)
			{
				errno = ENOMEM;
				return false;
			}
			auto *entries = static_cast<proclib_chain_ent *>(candidate.allocation_);
			for (int j = 0; j < proclib_chain_top; ++j)
				entries[j] = proclib_chain[j];
			int next = proclib_chain_top;
			for (const auto &value : requests)
			{
				bool present = false;
				for (int j = 0; j < proclib_chain_top; ++j)
					if (proclib_chain[j].rnum == value.rnum)
					{
						present = true;
						break;
					}
				if (!present && value.previous)
					entries[next++] = { value.rnum, value.previous };
			}
			if (next != candidate.next_top_)
			{
				errno = EINVAL;
				return false;
			}
		}
		candidate.prepared_ = true;
		static_assert(std::is_nothrow_move_assignable_v<proclib_recovery_chain_stage>);
		output = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
		return false;
	}
	catch (...)
	{
		errno = EFAULT;
		return false;
	}
#endif
}
