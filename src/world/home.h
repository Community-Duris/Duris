/*
Home Construction System,
Copyright 2025, Marissa du Bois
MIT License

This copy is for the exclusive use of Duris: Land of the Bloodlust
*/

#include "core/structs.h"

#define PLOT_SIZE 11
#define MAX_PLOTS 11

#define ROOM_TYPE_NONE 0
#define ROOM_TYPE_DIRT 1
#define ROOM_TYPE_GRASS 2
#define ROOM_TYPE_WALL 3
#define ROOM_TYPE_WATER 4
#define ROOM_TYPE_FLOOR 5
#define ROOM_TYPE_FARM 6
#define ROOM_TYPE_FARM_PLANTED 7
#define ROOM_TYPE_ROAD 8
#define ROOM_TYPE_EXIT 9
#define ROOM_TYPE_TREE 10
#define ROOM_TYPE_DOOR_WOOD 11
#define ROOM_TYPE_DOOR_IRON 12
#define ROOM_TYPE_MAX 13

#define HAS_HOME(ch) (FALSE)
#define IS_IN_HOUSE(ch) (FALSE)
#define IS_IN_HOME(ch) (FALSE)
#define GET_CHUNK_IN(ch) (NULL)

struct home_data;

static home_data *home_list = NULL;
static home_data *last_home = NULL;

const char *room_type_ansi[ROOM_TYPE_MAX] = {
	"&n", // NONE
	"&+y", // DIRT
	"&-G", // GRASS
	"&+L", // WALL
	"&-B", // WATER
	"&-Y", // FLOOR
	"&-G&+y", // FARM
	"&-G&+y", // FARM Planted
	"&+L+", // ROAD
	"&-W&+L", // EXIT
	"&+g", // TREE
	"&+y", // DOOR WOOD
	"&+L", // DOOR WOOD
};

const char *room_types[ROOM_TYPE_MAX] = {
	" ", // NONE
	".", // DIRT
	"°", // GRASS
	"Û", // WALL
	"Û", // WATER
	" ", // FLOOR
	".", // FARM
	"*", // FARM Planted
	"+", // ROAD
	"^", // EXIT
	"*", // TREE
	"#", // DOOR WOOD
	"#", // DOOR WOOD
};

struct home_room_data
{
	room_data *room;
	int positionX, positionY;
	int room_type = ROOM_TYPE_NONE;
};

struct home_plot_data
{
	int positionX, positionY;

	home_room_data *rooms[PLOT_SIZE * PLOT_SIZE];

	home_plot_data *north;
	home_plot_data *east;
	home_plot_data *south;
	home_plot_data *west;
};

struct home_data
{
	P_char owner;

	zone_data *zone;

	int exit_to;

	home_plot_data *plots[MAX_PLOTS * MAX_PLOTS];

	home_data *next_home = NULL;

	char fileName[MAX_STRING_LENGTH];
};
