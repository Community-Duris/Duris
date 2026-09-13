#include "combat/damage.h"
P_index mob_index = indexes;
static int combat_random_calls = 0;
int number(int, int)
{
	++combat_random_calls;
	return 1;
}
void debug(const char *, ...) {}
char *PERS(P_char ch, P_char to, int mode, bool)
{
	return PERS(ch, to, mode);
}
affected_type *get_ward_from_char(P_char)
{
	return nullptr;
}
void append_prompt(P_char, char *) {}
void UpdateScreen(P_char, int) {}
int process_output(P_desc)
{
	return 1;
}
