#ifndef __LISTEN_H__

#include <string>
#include <vector>
using namespace std;

#define LISTEN_SAY 0
#define LISTEN_GSAY 1
#define LISTEN_EMOTE 2

void listen_broadcast(P_char ch, const char *buf, int cmd);
void listen_say(P_char ch, P_char tch, const char *buf);
void listen_gsay(P_char ch, P_char tch, const char *buf);
void listen_emote(P_char ch, P_char tch, const char *buf);
void muddle_listened_string(P_char ch, P_char vict, const char *arg, char *muddled, int extr_depth);

#endif
