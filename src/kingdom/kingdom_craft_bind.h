/*
 *  kingdom_craft_bind.h
 *  Duris
 *
 *  THE BINDING TOKEN on guild-store gear, and nothing else.
 *
 *  Store gear is soulbound to the one character who bought it (ruled
 *  2026-09-15: not to an account, not to an alt, not to a trade partner).
 *  The engine's legacy soulbind test asks whether the wearer's NAME is one of
 *  the object's keywords, and a store piece's keywords are ordinary words --
 *  "steel", "vambraces", "kingdom", "strength" -- so any character named after
 *  one of them would pass it. A store piece therefore carries a token keyed to
 *  the buyer's PLAYER ID instead, and the wear check for store pieces
 *  (kingdom_store_piece.h) looks for that token and nothing else.
 *
 *  THE TOKEN LIVES IN THE PIECE'S ACTION DESCRIPTION, not among its keywords.
 *  Keywords are what player commands target, so a token there would let
 *  anyone type `get kingdom-bound-1042 bag` and read player ids off other
 *  people's gear. An action description is shown only for notes and corpses,
 *  is never matched by isname(), and is saved with the object everywhere
 *  (STRUNG_DESC3).
 *
 *  THE TOKEN is "kingdom-bound-<pid>", as in "kingdom-bound-1042". No
 *  character name can equal it: names are letters only (_parse_name(),
 *  account/nanny.c), and the token carries hyphens and digits. A player id
 *  names one character for good, so a later character who takes a deleted
 *  buyer's name does not inherit the binding either.
 *
 *  Pure string code with no engine dependency, on purpose:
 *  tests/async/kingdom_craft_math_harness.cpp executes it without a server.
 */

#ifndef _KINGDOM_CRAFT_BIND_H_
#define _KINGDOM_CRAFT_BIND_H_

#include <cstddef>
#include <cstring>
#include <string>

#define KINGDOM_CRAFT_BIND_PREFIX "kingdom-bound-"

/* Room for the prefix, any long, and the terminator. */
constexpr size_t KINGDOM_CRAFT_BIND_TOKEN_LEN = 40;

/* Write the token for player id `pid` into `out`. False, with `out` left
 * empty, when `pid` is not a real player id (zero or negative) or `out` is
 * too small to hold the whole token.
 *
 * Built as a std::string and copied only once it is known to fit, rather
 * than through snprintf(): under -Wformat-truncation=2 GCC flags a bounded
 * snprintf into a caller's buffer whenever truncation is possible at all,
 * even with the return value checked, and that is fatal in this build. */
inline bool kingdom_craft_bind_token(long pid, char *out, size_t out_len)
{
	if (!out || out_len == 0)
		return false;
	out[0] = '\0';
	if (pid <= 0)
		return false;

	const std::string token = std::string(KINGDOM_CRAFT_BIND_PREFIX) + std::to_string(pid);

	if (token.size() >= out_len)
		return false;
	std::memcpy(out, token.c_str(), token.size() + 1);
	return true;
}

/* What ends a word in a binding: whitespace, or the end of the string. Not a
 * space alone -- an action description that has been through anything that
 * normalises line endings (a database export, an immortal's edit) can carry
 * "\r\n" around the token, and the buyer must still be its owner. */
inline bool kingdom_craft_bind_boundary(char c)
{
	return c == '\0' || c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/* True when `binding` -- the string the store writes the token into, which is
 * the piece's action description -- carries the token for `pid` as a WHOLE
 * word: that exact token, with the start of the string or a space before it
 * and the end of the string or a space after it. The token for id 10 is
 * therefore not found inside "kingdom-bound-104", and no plain word matches at
 * all. Case is compared exactly: the writer emits the token in one case only. */
inline bool kingdom_craft_binding_is(const char *binding, long pid)
{
	char token[KINGDOM_CRAFT_BIND_TOKEN_LEN];

	if (!binding || !kingdom_craft_bind_token(pid, token, sizeof(token)))
		return false;

	const size_t length = std::strlen(token);

	for (const char *at = std::strstr(binding, token); at; at = std::strstr(at + 1, token))
	{
		const bool starts = at == binding || kingdom_craft_bind_boundary(at[-1]);
		const bool ends = kingdom_craft_bind_boundary(at[length]);

		if (starts && ends)
			return true;
	}
	return false;
}

/* True when `binding` carries ANY binding token: a word that begins with the
 * prefix, whatever follows it. This recognises store gear by its token alone,
 * failing closed: any such object is judged by kingdom_craft_binding_is() and
 * never by the legacy name test. */
inline bool kingdom_craft_binding_present(const char *binding)
{
	if (!binding)
		return false;

	for (const char *at = std::strstr(binding, KINGDOM_CRAFT_BIND_PREFIX); at;
	     at = std::strstr(at + 1, KINGDOM_CRAFT_BIND_PREFIX))
	{
		if (at == binding || kingdom_craft_bind_boundary(at[-1]))
			return true;
	}
	return false;
}

#endif /* _KINGDOM_CRAFT_BIND_H_ */
