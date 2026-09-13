#pragma once

#include "net/ansi.h"
#include <map>
#include <memory>
#include <span>

class OutputProfileSnapshot;

enum class OutputPolicy
{
	Preserve,
	Static,
	Animated
};

// Stable registry identifiers. Append new entries; never renumber existing ones.
// Routing only: no channel is automatically adopted by the output queue.
enum class OutputChannel
{
	Unspecified = 0,
	RoomDescription = 1,
	Chat = 2,
	Combat = 3,
	SystemFeedback = 4,
	RoomTitle = 5,
	RoomInspect = 6,
	RoomExits = 7,
	RoomAuras = 8,
	RoomOccupants = 9,
	ItemsList = 10,
	ChatSay = 11,
	ChatTell = 12,
	ChatWhisper = 13,
	ChatAsk = 14,
	ChatShout = 15,
	ChatYell = 16,
	ChatGroup = 17,
	ChatGuild = 18,
	ChatAlliance = 19,
	ChatPetition = 20,
	ChatProject = 21,
	ChatPage = 22,
	ChatRacewar = 23,
	ChatImmortal = 24,
	Social = 25,
	Weather = 26,
	CombatIncoming = 27,
	CombatOutgoing = 28,
	CombatObserved = 29,
	Prompt = 30,
	ChatAuction = 31,
	ChatNchat = 32,
	ChatJchat = 33,
	ChatWizmsg = 34,
	Count = 35
};
enum class StyleOrigin
{
	ChannelBase,
	Sender,
	Entity,
	Authored
};

// Half-open visible-character offsets into an AnsiString.
struct AnsiStyleSpan
{
	size_t begin = 0;
	size_t end = 0;
	StyleOrigin origin = StyleOrigin::Authored;
	int attr = 0;
};

// Half-open byte offsets into the completed Duris-markup message, after formatting.
struct OutputStyleSpan
{
	size_t begin = 0;
	size_t end = 0;
	StyleOrigin origin = StyleOrigin::Authored;
	int attr = 0;
};

// Lowercase ASCII whole-word keys, foreground attributes only. Construct once
// in the caller/config snapshot, then borrow for the duration of a send.
using WordColorDictionary = std::map<std::string, int, std::less<>>;

struct OutputContext
{
	OutputChannel channel = OutputChannel::Unspecified;
	OutputPolicy policy = OutputPolicy::Preserve;
	const WordColorDictionary *words = nullptr;
	int base_attr = 0;
	std::span<const OutputStyleSpan> spans{};
	// Registry contexts retain the immutable dictionary through copies and reloads.
	// Hand-built contexts may continue to borrow a caller-owned dictionary.
	std::shared_ptr<const OutputProfileSnapshot> snapshot_owner{};
};

// Pure transformation. Existing attributes and protected words are never erased.
// Invalid bounds/styles leave the entire input unchanged. No per-word match cap.
AnsiString style_dictionary_words(const AnsiString &input, const WordColorDictionary &words,
				  std::span<const AnsiStyleSpan> spans = {}, int base_attr = 0);

// Freeze markup only if both legacy serializers can emit every character and
// the caller's remaining pager capacity admits it. false means use original bytes.
// The output is assigned only on success, so it can also own the input bytes.
// Animated uses the supplied fixed frame here; recipe/sequence ownership is separate.
bool render_output_message(const char *message, const OutputContext &context, std::string &rendered,
			   size_t capacity = MAX_STRING_LENGTH - 1);
