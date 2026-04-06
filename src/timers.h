#ifndef _TIMERS_H_
#define _TIMERS_H_

void set_timer(const char *name);
void set_timer(const char *name, int date);
int  get_timer(const char *name);
bool has_elapsed(const char *name, int seconds);

void timers_activity();

#endif
