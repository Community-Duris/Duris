#include "net/chat_presentation.h"
#include "net/gmcp.h"
#include "core/utils.h"
#include "net/unicode.h"
#include <cjson/cJSON.h>
#include <cstdlib>
#include <cstring>
#include <ctime>

void gmcp_comm_channel_output(char_data *recipient, const OutputChatMessage &chat,
			      const OutputContext &context, const char *frozen)
{
	if (!GMCP_ENABLED(recipient) || !frozen)
		return;
	const char *canonical = nullptr;
	if (context.channel == OutputChannel::ChatSay && chat.channel == "say")
		canonical = "chat.say";
	else if (context.channel == OutputChannel::ChatTell && chat.channel == "tell")
		canonical = "chat.tell";
	else if (context.channel == OutputChannel::ChatGuild && chat.channel == "gcc")
		canonical = "chat.guild";
	if (!canonical)
		return;

	cJSON *root = cJSON_CreateObject();
	if (!root)
		return;
	cJSON_AddStringToObject(root, "channel", chat.channel.c_str());
	cJSON_AddStringToObject(root, "sender", chat.sender.c_str());
	cJSON_AddStringToObject(root, "text", chat.text.c_str());
	cJSON_AddNumberToObject(root, "timestamp", std::time(nullptr));

	// Existing packets remain usable if an exceptional input exceeds the
	// presentation budget or contains a raw terminal control sequence.
	if (std::strlen(frozen) < MAX_STRING_LENGTH && !std::strchr(frozen, '\x1b'))
	{
		AnsiString visible(frozen);
		while (!visible.empty() && (visible.ch(visible.size() - 1) == '\n' ||
					    visible.ch(visible.size() - 1) == '\r'))
			visible.pop_back();
		if (visible.size() <= 16384)
		{
			std::string plain;
			cJSON *runs = cJSON_CreateArray();
			size_t count = 0;
			for (size_t start = 0; start < visible.size();)
			{
				if (count == 4096)
				{
					++count;
					break;
				}
				size_t end = start;
				const int attr = visible.attr(start);
				while (end < visible.size() && visible.attr(end) == attr)
				{
					char bytes[8];
					char *out = bytes;
					put_utf8(out, visible.ch(end++));
					plain.append(bytes, out);
				}
				const int tuple[] = { (int)start, (int)end, GET_FG(attr),
						      GET_BG(attr) };
				cJSON_AddItemToArray(runs, cJSON_CreateIntArray(tuple, 4));
				++count;
				start = end;
			}
			if (count <= 4096)
			{
				cJSON *presentation = cJSON_AddObjectToObject(root, "presentation");
				cJSON_AddNumberToObject(presentation, "version", 1);
				cJSON_AddNumberToObject(presentation, "channelId",
							(int)context.channel);
				cJSON_AddStringToObject(presentation, "channel", canonical);
				cJSON_AddStringToObject(
					presentation, "policy",
					context.policy == OutputPolicy::Preserve ? "preserve" :
					context.policy == OutputPolicy::Animated ? "animated" :
										   "static");
				cJSON_AddStringToObject(presentation, "text", plain.c_str());
				cJSON_AddBoolToObject(presentation, "underline",
						      PLR3_FLAGGED(recipient, PLR3_UNDERLINE));
				cJSON_AddItemToObject(presentation, "runs", runs);
			}
			else
				cJSON_Delete(runs);
		}
	}
	char *json = cJSON_PrintUnformatted(root);
	if (json)
	{
		gmcp_send(recipient->desc, GMCP_PKG_COMM_CHANNEL, json);
		std::free(json);
	}
	cJSON_Delete(root);
}
