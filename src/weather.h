/**********************************************************************
 * Weather.h -- Definitions for new zone-based weather routines. Made
 * 12/23/91 by Smandoggi/Dbra/abradfor. This is for DikuMUD, and whatever
 * legal things that apply to it, apply here as well, so there.
 *********************************************************************/

#ifndef _SOJ_WEATHER_H_
#define _SOJ_WEATHER_H_

/* Season patterns */
#define ONE_SEASON              1
#define TWO_SEASONS_EQUAL       2
#define TWO_SEASONS_FIRST_LONG  3
#define TWO_SEASONS_SECOND_LONG 4
#define THREE_SEASONS_EQUAL     5
#define FOUR_SEASONS_EQUAL      6
#define FOUR_SEASONS_EVEN_LONG  7
#define FOUR_SEASONS_ODD_LONG   8

/* Seasonal Wind characteristics */
#define SEASON_CALM      1
#define SEASON_BREEZY    2
#define SEASON_UNSETTLED 3
#define SEASON_WINDY     4
#define SEASON_CHINOOK   5
#define SEASON_VIOLENT   6
#define SEASON_HURRICANE 7

/* Seasonal Precipitation characteristics */
#define SEASON_NO_PRECIP_EVER  1
#define SEASON_ARID            2
#define SEASON_DRY             3
#define SEASON_LOW_PRECIP      4
#define SEASON_AVG_PRECIP      5
#define SEASON_HIGH_PRECIP     6
#define SEASON_STORMY          7
#define SEASON_TORRENT         8
#define SEASON_CONSTANT_PRECIP 9 /* Interesting if it's cold enough to snow */

/* Seasonal Temperature characteristics */
#define SEASON_FROSTBITE  1 /* Need to keep warm...*/
#define SEASON_NIPPY      2
#define SEASON_FREEZING   3
#define SEASON_COLD       4
#define SEASON_COOL       5
#define SEASON_MILD       6
#define SEASON_WARM       7
#define SEASON_HOT        8
#define SEASON_BLUSTERY   9
#define SEASON_HEATSTROKE 10 /* Definite HP loss for these two */
#define SEASON_BOILING    11

/* Flags */
#define NO_MOON_EVER     1
#define NO_SUN_EVER      2
#define NON_CONTROLLABLE 4
#define AFFECTS_INDOORS  8

/* Weather flags */
#define MOON_VISIBLE       1
#define SUN_VISIBLE        2
#define WEATHER_CONTROLLED 4 /* Idea for expansion */

// Days : 8am to 4pm
#define IS_DAY ((time_info.hour >= 8) && (time_info.hour <= 16))
// Twilight : 4am to 8am and 4pm to 7pm
#define IS_TWILIGHT (((time_info.hour > 4) && (time_info.hour < 8)) || ((time_info.hour > 16) && (time_info.hour <= 19)))
// Night: 8pm to 4am
#define IS_NIGHT ((time_info.hour <= 4) || (time_info.hour > 19))

#endif /* _SOJ_WEATHER_H_ */
