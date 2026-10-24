// Actual teacher command, purchase callback and epic balance transaction.
// Cash/stat/save/display adapters observe charges and publication order. These
// tests do not exercise live HP calculations, client packets or restart durability.
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "cmd/interp.h"
#include "magic/spells.h"
#include "classes/epic_skills.h"
#include "world/epic.h"
#include "world/epic_transaction.h"
#include "net/gmcp.h"
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <string>
#include <vector>

index_data indexes[1] = {};
P_index mob_index = indexes;
Skill skills[MAX_SKILLS] = {};
struct race_names race_names_table[LAST_RACE + 1] = {};
static char_data pupil = {}, teacher = {};
static pc_only_data pc = {};
static npc_only_data npc = {};
static int minimum = 56;
static bool online = true;
static critical_submit_result admission = critical_submit_result::accepted;
static int cash_charges = 0, saves = 0, waits = 0, refreshes = 0, vitals = 0;
static int refreshed_at_save = 0, refreshed_at_vitals = 0;
static std::string feedback;
static std::vector<critical_command> ledger;

float get_property(const char *key, double fallback)
{
	return std::string(key) == "epic.skills.minLevel" ? minimum : fallback;
}
int get_property(const char *, int fallback)
{
	return fallback;
}
int GET_CHAR_SKILL_P(P_char ch, int skill)
{
	return ch->only.pc->skills[skill].learned;
}
int SUB_MONEY(P_char ch, int amount, int mode)
{
	assert(mode == 0 && amount > 0 && GET_MONEY(ch) >= amount);
	++cash_charges;
	ch->points.cash[0] -= amount;
	return 0;
}
char *coin_stringv(int, int)
{
	static char text[] = "fixture coins";
	return text;
}
// These cases submit a full skill name, never the informational quote page.
void CAP(char *)
{
	std::abort();
}
void send_to_char(const char *text, P_char)
{
	feedback += text;
}
void send_to_char_f(P_char, const char *format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	feedback += text;
}
char affect_total(P_char ch, int)
{
	assert(ch == &pupil && pc.skills[SKILL_TOUGHNESS].learned > 0);
	++refreshes;
	return false;
}
void gmcp_char_vitals(P_char ch)
{
	assert(ch == &pupil);
	refreshed_at_vitals = refreshes;
	++vitals;
}
bool do_save_silent(P_char ch, int)
{
	assert(ch == &pupil);
	refreshed_at_save = refreshes;
	++saves;
	return true;
}
void CharWait(P_char ch, int)
{
	assert(ch == &pupil);
	++waits;
}
critical_submit_result critical_command_coordinator_submit(critical_command command)
{
	if (admission == critical_submit_result::accepted)
		ledger.push_back(std::move(command));
	return admission;
}

bool critical_command_coordinator_get_completed(const critical_operation_id &,
						critical_completion *)
{
	return false;
}
P_char find_player_by_pid(int pid)
{
	return online && pid == 42 ? &pupil : nullptr;
}

// PRODUCTION

static void reset(int level, int learned = 0, int teacher_vnum = 80862)
{
	epic_transaction_reset_for_tests();
	online = true;
	admission = critical_submit_result::accepted;
	pupil = {};
	teacher = {};
	pc = {};
	npc = {};
	pupil.only.pc = &pc;
	teacher.only.npc = &npc;
	pc.pid = 42;
	pc.epics = 10000;
	pc.skills[SKILL_TOUGHNESS].learned = pc.skills[SKILL_TOUGHNESS].taught = learned;
	pupil.player.level = level;
	pupil.player.m_class = CLASS_WARRIOR;
	pupil.player.race = RACE_HUMAN;
	SET_POS(&pupil, POS_STANDING + STAT_NORMAL);
	pupil.player.name = const_cast<char *>("Trainfixture");
	pupil.points.cash[0] = 10000000;
	teacher.specials.act = ACT_ISNPC;
	indexes[0].virtual_number = teacher_vnum;
	cash_charges = saves = waits = refreshes = vitals = 0;
	refreshed_at_save = refreshed_at_vitals = 0;
	feedback.clear();
	ledger.clear();
}
static void practice(const char *skill = "toughness")
{
	assert(epic_teacher(&teacher, &pupil, CMD_PRACTICE, const_cast<char *>(skill)));
}
static void unchanged(int learned = 0)
{
	assert(pc.epics == 10000 && GET_MONEY(&pupil) == 10000000);
	assert(pc.skills[SKILL_TOUGHNESS].learned == learned);
	assert(!cash_charges && !saves && !waits && !refreshes && !vitals && ledger.empty());
}
static critical_completion completion_for(size_t index, bool committed = true)
{
	const auto &command = ledger.at(index);
	epic_command_payload payload{};
	assert(epic_command_decode_payload(command, &payload));
	critical_completion completion{};
	completion.operation_id = command.operation_id;
	completion.outcome = committed ? critical_apply_outcome::applied :
					 critical_apply_outcome::terminal_failure;
	epic_command_result result{};
	result.balance = pc.epics + (committed ? payload.delta : 0);
	result.revision = pc.epic_revision + (committed ? 1 : 0);
	result.delta = payload.delta;
	completion.result_size = EPIC_RESULT_PAYLOAD_BYTES;
	std::array<uint8_t, EPIC_RESULT_PAYLOAD_BYTES> encoded{};
	assert(epic_command_encode_result(result, &encoded));
	std::copy(encoded.begin(), encoded.end(), completion.result_payload.begin());
	return completion;
}
static void bought(int before, int learned)
{
	// Admission alone must not publish the purchase.
	assert(pc.skills[SKILL_TOUGHNESS].learned == before && !cash_charges && !refreshes &&
	       !vitals && !saves);
	assert(pc.epics == 10000 && GET_MONEY(&pupil) == 10000000 && ledger.size() == 1);
	const auto completion = completion_for(0);
	epic_transaction_handle_completions(&completion, 1);
	// Repeated worker completions must not charge or refresh again.
	epic_transaction_handle_completions(&completion, 1);
	const int multiplier = 1 + before / 30;
	assert(pc.epics == 10000 - 150 * multiplier);
	assert(GET_MONEY(&pupil) == 10000000 - 1000000 * multiplier);
	assert(pc.skills[SKILL_TOUGHNESS].learned == learned &&
	       pc.skills[SKILL_TOUGHNESS].taught == learned);
	assert(cash_charges == 1 && saves == 1 && waits == 1 && ledger.size() == 1);
}
int main(int argc, char **argv)
{
	assert(argc == 2);
	skills[SKILL_TOUGHNESS].name = const_cast<char *>("toughness");
	skills[SKILL_ANATOMY].name = const_cast<char *>("anatomy");
	skills[SKILL_IMPROVED_LISTEN].name = const_cast<char *>("improved listen");
	skills[SKILL_LISTEN].name = const_cast<char *>("listen");
	const std::string mode = argv[1];
	if (mode == "boundaries")
	{
		for (int gate : { 56, 60 })
		{
			minimum = gate;
			reset(gate - 1);
			practice();
			unchanged();
			assert(feedback.find("reach level " + std::to_string(gate)) !=
			       std::string::npos);
			for (int level : { gate, gate + 1 })
			{
				reset(level);
				practice();
				bought(0, 10);
			}
		}
		minimum = 56;
		reset(56, 0, 19141);
		pupil.player.m_class = CLASS_NECROMANCER;
		practice("anatomy");
		unchanged();
		assert(feedback.find("people of your class") != std::string::npos);
		reset(56, 0, 27035);
		pupil.player.m_class = CLASS_ROGUE;
		pc.skills[SKILL_LISTEN].learned = 79;
		practice("improved listen");
		unchanged();
		assert(feedback.find("not yet mastered") != std::string::npos);
		reset(56);
		pc.epics = 149;
		practice();
		assert(pc.epics == 149 && GET_MONEY(&pupil) == 10000000 && !cash_charges && !saves);
		reset(56);
		pupil.points.cash[0] = 999999;
		practice();
		assert(pc.epics == 10000 && GET_MONEY(&pupil) == 999999 && !cash_charges && !saves);
		for (auto *entry = epic_teachers; entry->vnum; ++entry)
			if (entry->vnum == 80862)
			{
				const int maximum = entry->max;
				entry->max = 50;
				reset(56, 50);
				practice();
				unchanged(50);
				assert(feedback.find("cannot teach you anything more") !=
				       std::string::npos);
				entry->max = maximum;
			}
	}
	else if (mode == "mastery")
	{
		reset(56, 80);
		practice();
		bought(80, 90);
		assert(feedback.find("improve your mastery") != std::string::npos);
		assert(feedback.find("You have mastered") == std::string::npos);
		reset(56, 95);
		practice();
		bought(95, 100);
		assert(feedback.find("You have mastered") != std::string::npos);
		assert(feedback.find("improve your mastery") == std::string::npos);
		reset(56, 100);
		practice();
		unchanged(100);
		assert(feedback.find("already mastered this skill") != std::string::npos);
	}
	else if (mode == "refresh")
	{
		reset(56);
		practice();
		bought(0, 10);
		assert(refreshes == 1 && refreshed_at_save == 1);
		assert(vitals == 1 && refreshed_at_vitals == 1);
	}
	else
	{
		assert(mode == "failures");
		reset(56);
		admission = critical_submit_result::overloaded;
		practice();
		unchanged();
		assert(feedback.find("service is busy") != std::string::npos);
		reset(56);
		practice();
		const auto rejected = completion_for(0, false);
		epic_transaction_handle_completions(&rejected, 1);
		assert(!cash_charges && !saves && !refreshes && !vitals && !waits);
		assert(pc.skills[SKILL_TOUGHNESS].learned == 0 && pc.epics == 10000);
		assert(feedback.find("declined") != std::string::npos);
		for (bool change_cash : { false, true })
		{
			reset(56);
			practice();
			const auto committed = completion_for(0);
			if (change_cash)
				pupil.points.cash[0] = 0;
			else
				pc.skills[SKILL_TOUGHNESS].learned = 20;
			epic_transaction_handle_completions(&committed, 1);
			assert(!cash_charges && !saves && !refreshes && !vitals && !waits &&
			       ledger.size() == 2);
			assert(feedback.find("being refunded") != std::string::npos);
			epic_command_payload refund{};
			assert(epic_command_decode_payload(ledger.back(), &refund));
			assert(refund.delta == 150 &&
			       refund.reason == epic_reason_type::epic_skill_refund);
		}
		reset(56);
		practice();
		const auto committed = completion_for(0);
		online = false;
		epic_transaction_handle_completions(&committed, 1);
		assert(!cash_charges && !saves && !refreshes && !vitals && !waits);
		online = true;
		epic_transaction_player_ready(&pupil);
		assert(cash_charges == 1 && saves == 1 && refreshes == 1 && vitals == 1);
		assert(pc.skills[SKILL_TOUGHNESS].learned == 10);
	}
	printf("PASS real epic teacher: %s\n", mode.c_str());
}
