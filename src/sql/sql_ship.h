#ifndef DURIS_SQL_SHIP_H_INCLUDED
#define DURIS_SQL_SHIP_H_INCLUDED

struct ShipData;

bool sql_save_ship(struct ShipData *ship);
struct ShipData *sql_load_ship(const char *owner_name);
bool sql_load_all_ships(void);
bool sql_delete_ship(const char *owner_name);

#endif // DURIS_SQL_SHIP_H_INCLUDED
