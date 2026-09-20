/*
 *  guildhall_db.h
 *  Implementation of saving guildhalls/rooms to DB
 *
 *  Created by Torgal on 1/30/10.
 *
 */

#ifndef _GUILDHALLS_DB_H_
#define _GUILDHALLS_DB_H_

#include "guild/guildhall.h"
struct Guildhall;
struct GuildhallRoom;

int next_guildhall_id();
int next_guildhall_room_id();
int next_guildhall_room_vnum();

/* The room object for a guildhall room of `type`: the typed subclass, or a
 * plain GuildhallRoom for GENERIC and anything unknown. ONE factory, used by
 * both backends' loaders and by construction, so a new room type cannot be
 * known to one of them and fall back to a generic room in another. */
GuildhallRoom *make_guildhall_room(int type);

void load_guildhalls(vector<Guildhall *> &);
void load_guildhall(int id, Guildhall *);
void load_guildhall_rooms(Guildhall *);
vector<GuildhallRoom *> load_guildhall_rooms(int guildhall_id);

bool save_guildhall(Guildhall *);
bool save_guildhall_room(GuildhallRoom *);

bool delete_guildhall(Guildhall *);
bool delete_guildhall_room(GuildhallRoom *);

#endif // _GUILDHALLS_DB_H_