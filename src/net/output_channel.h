#pragma once

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
