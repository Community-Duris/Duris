#include "guild/artifact_feed_rates.h"

#include "core/prototypes.h"
#include "world/db.h"
#include "world/epic.h"

#include <algorithm>
#include <strings.h>

/*
 * The proposed rates size zoning so that a dedicated feeder replaces at most
 * a quarter of an artifact's daily decay:
 *
 *   zone rate = 0.25 x 86,400 / (1,200 x 800 zone epics a day) = 0.0225 -> 0.02
 *
 * and every other non-PvP source is cut by the same factor (x0.133) from its
 * previous value. PvP and ship PvP keep their previous rates.
 */
const artifact_feed_setting ARTIFACT_FEED_SOURCES[] = {
	{ "zone", "Epic stone touch", EPIC_ZONE, "artifact.feeding.epic.typeMod.zone", 0.02, 0.0,
	  50.0 },
	{ "quest", "Bartender quest", EPIC_QUEST, "artifact.feeding.epic.typeMod.quest", 0.02, 0.0,
	  50.0 },
	{ "elitemob", "Elite mob kill", EPIC_ELITE_MOB, "artifact.feeding.epic.typeMod.eliteMob",
	  0.01, 0.0, 50.0 },
	{ "randomzone", "Random zone", EPIC_RANDOM_ZONE, "artifact.feeding.epic.typeMod.randomZone",
	  0.04, 0.0, 50.0 },
	{ "nexus", "Nexus stone", EPIC_NEXUS_STONE, "artifact.feeding.epic.typeMod.nexusStone",
	  0.067, 0.0, 50.0 },
	{ "boon", "Boon", EPIC_BOON, "artifact.feeding.epic.typeMod.boon", 0.033, 0.0, 50.0 },
	{ "randommob", "Random mob or dragon kill", EPIC_RANDOMMOB,
	  "artifact.feeding.epic.typeMod.randomMob", 0.01, 0.0, 50.0 },
	{ "strahd", "Strahd achievement", EPIC_STRAHDME, "artifact.feeding.epic.typeMod.strahdMe",
	  0.02, 0.0, 50.0 },
	{ "pvp", "PvP kill", EPIC_PVP, "artifact.feeding.epic.typeMod.pvp", 7.2, 0.0, 50.0 },
	{ "pvpship", "Ship PvP", EPIC_SHIP_PVP, "artifact.feeding.epic.typeMod.pvpShip", 0.5, 0.0,
	  50.0 },
};
const size_t ARTIFACT_FEED_SOURCE_COUNT =
	sizeof(ARTIFACT_FEED_SOURCES) / sizeof(ARTIFACT_FEED_SOURCES[0]);

const artifact_feed_setting ARTIFACT_FEED_POINT_SECONDS = {
	"point", "Seconds per epic point", -1, "artifact.feeding.epic.point.seconds", 1200.0, 1.0,
	86400.0
};

const artifact_feed_setting ARTIFACT_FEED_NONPVP_CEILING_HOURS = {
	"ceiling",
	"Non-PvP ceiling, hours",
	-1,
	"artifact.feeding.nonPvp.ceilingHours",
	72.0,
	1.0,
	static_cast<double>(ARTIFACT_BLOOD_DAYS * 24)
};

const artifact_feed_setting *artifact_feed_setting_find(const char *name)
{
	if (!name || !*name)
		return nullptr;
	for (size_t index = 0; index < ARTIFACT_FEED_SOURCE_COUNT; ++index)
		if (!strcasecmp(name, ARTIFACT_FEED_SOURCES[index].name))
			return &ARTIFACT_FEED_SOURCES[index];
	if (!strcasecmp(name, ARTIFACT_FEED_POINT_SECONDS.name))
		return &ARTIFACT_FEED_POINT_SECONDS;
	if (!strcasecmp(name, ARTIFACT_FEED_NONPVP_CEILING_HOURS.name))
		return &ARTIFACT_FEED_NONPVP_CEILING_HOURS;
	return nullptr;
}

double artifact_feed_setting_value(const artifact_feed_setting &setting)
{
	const double value = get_property(setting.property, setting.default_value, false);
	return std::clamp(value, setting.minimum, setting.maximum);
}

double artifact_feed_type_mod(int epic_type)
{
	for (size_t index = 0; index < ARTIFACT_FEED_SOURCE_COUNT; ++index)
		if (ARTIFACT_FEED_SOURCES[index].epic_type == epic_type)
			return artifact_feed_setting_value(ARTIFACT_FEED_SOURCES[index]);
	return 0.0;
}

bool artifact_feed_is_pvp(int epic_type)
{
	return epic_type == EPIC_PVP || epic_type == EPIC_SHIP_PVP;
}

int artifact_feed_point_seconds(void)
{
	return static_cast<int>(artifact_feed_setting_value(ARTIFACT_FEED_POINT_SECONDS));
}

int64_t artifact_feed_nonpvp_ceiling_seconds(void)
{
	return static_cast<int64_t>(
		artifact_feed_setting_value(ARTIFACT_FEED_NONPVP_CEILING_HOURS) * 3600.0);
}
