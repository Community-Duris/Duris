#ifndef DURIS_CMD_TRACK_H
#define DURIS_CMD_TRACK_H

#include "core/structs.h"

char *sickprocess(const char *argument);
int MaxTrackDist(P_char ch);
void track_move(P_char ch);
void add_track(P_char ch, int direction);
void do_track(P_char ch, char *argument, int command);
void show_tracks(P_char ch, int room);
void show_tracking_map(P_char ch);

#endif
