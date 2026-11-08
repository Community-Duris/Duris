#ifndef ARTIFACT_FEED_RATES_H
#define ARTIFACT_FEED_RATES_H

#include <cstddef>
#include <cstdint>

/*
 * How much time an epic award adds to an artifact's timer.
 *
 *   seconds fed = epics x point seconds x the source's rate
 *                 x the artifact-feeding difficulty dial
 *                 x 1.5 within epic.frag.thrill.duration of a frag
 *
 * PvP awards can fill the timer to the full ARTIFACT_BLOOD_DAYS. Every other
 * source can lift it no higher than now + the non-PvP ceiling, and never
 * lowers a timer that is already above that ceiling.
 *
 * Every value here is a property, so staff can retune it in game with the
 * artifeed command, which also saves the new value to lib/duris.properties.
 * The defaults are the rates the epic levelling reform proposes.
 */
struct artifact_feed_setting
{
	const char *name; // the name the artifeed command uses
	const char *label; // what the setting is, for the display
	int epic_type; // the EPIC_* award type it prices, or -1 for a global setting
	const char *property; // the key in lib/duris.properties
	double default_value; // the proposed rate
	double minimum;
	double maximum;
};

extern const artifact_feed_setting ARTIFACT_FEED_SOURCES[];
extern const size_t ARTIFACT_FEED_SOURCE_COUNT;
extern const artifact_feed_setting ARTIFACT_FEED_POINT_SECONDS;
extern const artifact_feed_setting ARTIFACT_FEED_NONPVP_CEILING_HOURS;

/** Finds a source or global setting by its artifeed name, or returns nullptr. */
const artifact_feed_setting *artifact_feed_setting_find(const char *name);

/** The setting's live value: the property if set, otherwise its default. */
double artifact_feed_setting_value(const artifact_feed_setting &setting);

/** The rate for an EPIC_* award type, or 0 for a type that never feeds. */
double artifact_feed_type_mod(int epic_type);

/** True for the award types that fill the timer to the full ARTIFACT_BLOOD_DAYS. */
bool artifact_feed_is_pvp(int epic_type);

/** Seconds of timer one epic point is worth before the source's rate. */
int artifact_feed_point_seconds(void);

/** How far above now a non-PvP feed may lift an artifact's timer, in seconds. */
int64_t artifact_feed_nonpvp_ceiling_seconds(void);

#endif
