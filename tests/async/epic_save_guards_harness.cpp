// Requirement EPIC-REFUND: refusal cannot mutate skills or claim a refund;
// a committed refund attempts exactly one checkpoint and diagnoses its refusal.
// Include the entire production unit so the actual internal callback is exercised.
#include "world/epic.c"
#include <cassert>
#include <cstdarg>

Skill skills[MAX_AFFECT_TYPES + 1] = {};
epic_reward epic_rewards[1] = {};
namespace
{
std::string terminal_text, diagnostics;
bool save_result = false;
unsigned saves = 0, pickups = 0;
int saved_learned = -1;
std::string formatted(const char *format, va_list args)
{
	char buffer[4096];
	vsnprintf(buffer, sizeof(buffer), format, args);
	return buffer;
}
} // namespace
void send_to_char(const char *text, P_char)
{
	terminal_text += text;
}
void send_to_char_f(P_char, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	terminal_text += formatted(format, args);
	va_end(args);
}
void logit(const char *, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	diagnostics += formatted(format, args);
	va_end(args);
}
bool do_save_silent(P_char ch, int intent)
{
	assert(intent == 1);
	++saves;
	if (save_result)
		saved_learned = ch->only.pc->skills[FIRST_SKILL].learned;
	return save_result;
}
bool insert_money_pickup(int pid, int amount)
{
	assert(pid == 42 && amount == 123);
	++pickups;
	return true;
}
bool isname(const char *name, const char *names)
{
	return strstr(names, name) != nullptr;
}
int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}
bool currency_transaction_submit_wallet_value(P_char, int64_t, currency_reason_type, int64_t,
					      critical_source_site, critical_deadline_class,
					      currency_completion_fn, const void *, size_t)
{
	std::abort();
}
void argument_interpreter(char *, char *first, char *second)
{
	first[0] = second[0] = 0;
}
P_char get_char_vis(P_char, const char *)
{
	std::abort();
}
int get_property(const char *, int value)
{
	return value;
}
float get_property(const char *, double value)
{
	return value;
}
bool send_to_pid(const char *, int)
{
	return true;
}
void send_to_pid_offline(const char *, int)
{
	std::abort();
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	char_data ch = {};
	pc_only_data pc = {};
	ch.only.pc = &pc;
	ch.player.name = const_cast<char *>("SyntheticEpic");
	ch.player.level = MAXLVLMORTAL + 1;
	pc.pid = 42;
	skills[FIRST_SKILL].name = const_cast<char *>("synthetic");
	skills[FIRST_SKILL].targets = TAR_EPIC;
	pc.skills[FIRST_SKILL].learned = 50;
	pc.skills[FIRST_SKILL].taught = 60;
	const epic_skill_refund_context context = { 7, 123 };
	const auto *raw = reinterpret_cast<const uint8_t *>(&context);
	const epic_command_result result = {};
	if (std::string(argv[1]) == "refused")
	{
		for (const auto &[committed, length] :
		     { std::pair{ false, sizeof(context) }, std::pair{ true, size_t{ 0 } } })
		{
			terminal_text.clear();
			epic_skill_refund_committed(&ch, committed, result, EIO, raw, length);
			assert(pc.skills[FIRST_SKILL].learned == 50 &&
			       pc.skills[FIRST_SKILL].taught == 60 && saves == 0 && pickups == 0);
			assert(terminal_text.find("could not be completed") != std::string::npos &&
			       terminal_text.find("were refunded") == std::string::npos);
		}
	}
	else
	{
		assert(std::string(argv[1]) == "checkpoint");
		for (bool successful_save : { false, true })
		{
			save_result = successful_save;
			saves = pickups = 0;
			terminal_text.clear();
			diagnostics.clear();
			pc.skills[FIRST_SKILL].learned = 50;
			epic_skill_refund_committed(&ch, true, result, 0, raw, sizeof(context));
			assert(saves == 1 && pickups == 1 && pc.skills[FIRST_SKILL].learned == 0);
			assert(terminal_text.find("7 epics were refunded") != std::string::npos);
			assert((diagnostics.find("Failed to save SyntheticEpic") !=
				std::string::npos) == !successful_save);
			assert(successful_save ? saved_learned == 0 : saved_learned == -1);
		}
		for (auto reset : { do_epic_reset, do_epic_reset_norefund })
		{
			for (bool successful_save : { false, true })
			{
				save_result = successful_save;
				saves = 0;
				diagnostics.clear();
				char argument[] = "";
				reset(&ch, argument, 0);
				assert(saves == 1);
				assert((diagnostics.find("Failed to save SyntheticEpic") !=
					std::string::npos) == !successful_save);
			}
		}
	}
}
