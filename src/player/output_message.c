#include "player/output_message.h"
#include "core/prototypes.h"
#include <algorithm>
#include <cstring>

PlayerOutputMessage::PlayerOutputMessage(char_data *recipient, OutputChannel channel)
	: recipient_(recipient)
	, profile_(player_output_profile(recipient, channel, OutputPolicy::Static))
{
}

PlayerOutputMessage &PlayerOutputMessage::literal(std::string_view legacy_template)
{
	append_legacy(legacy_template);
	if (profile_.context.policy == OutputPolicy::Preserve)
		return *this;
	if (legacy_template.size() >= MAX_STRING_LENGTH ||
	    selected_.size() + legacy_template.size() * 2 + 2 >= MAX_STRING_LENGTH)
	{
		candidate_valid_ = false;
		return *this;
	}
	// The input here is a caller-owned template fragment, never a message body.
	char plain[MAX_STRING_LENGTH];
	AnsiString(std::string(legacy_template).c_str()).plain(plain);
	selected_ += "&n";
	// Re-escape literal ampersands before the final markup parser sees them.
	for (const char *p = plain; *p; ++p)
	{
		selected_ += *p;
		if (*p == '&')
			selected_ += '&';
	}
	return *this;
}

PlayerOutputMessage &PlayerOutputMessage::body(std::string_view authored_text)
{
	append_legacy(authored_text);
	if (authored_text.size() >= MAX_STRING_LENGTH - selected_.size())
	{
		candidate_valid_ = false;
		return *this;
	}
	if (profile_.context.policy != OutputPolicy::Preserve)
		selected_ += authored_text;
	return *this;
}

PlayerOutputMessage &PlayerOutputMessage::entity(std::string_view authored_name, StyleOrigin origin)
{
	const size_t begin = selected_.size();
	body(authored_name);
	if (profile_.context.policy != OutputPolicy::Preserve)
		spans_.push_back({ begin, selected_.size(), origin,
				   origin == StyleOrigin::Sender ? profile_.sender_attr :
								   profile_.entity_attr });
	return *this;
}

void PlayerOutputMessage::send(int log)
{
	if (profile_.context.policy == OutputPolicy::Preserve || !candidate_valid_)
	{
		send_to_char(legacy_.c_str(), recipient_, log);
		return;
	}
	profile_.context.spans = spans_;
	profile_.context.original_message = legacy_.c_str();
	// Delivery retains original logging, bounded serialization and pager fallback.
	send_to_char(selected_.c_str(), recipient_, log, profile_.context);
}

void PlayerOutputMessage::append_legacy(std::string_view text)
{
	// Match the old bounded snprintf templates. A decorated candidate may not
	// consume space that the original visible message could have occupied.
	legacy_.append(
		text.substr(0, std::min(text.size(), MAX_STRING_LENGTH - 1 - legacy_.size())));
}

const char *player_output_template(char_data *recipient, OutputChannel channel, const char *legacy,
				   const char *selected)
{
	return player_output_profile(recipient, channel, OutputPolicy::Static).context.policy ==
			       OutputPolicy::Preserve ?
		       legacy :
		       selected;
}

OutputContext preserve_authored_layout(const char *message, const OutputContext &context)
{
	if (context.policy == OutputPolicy::Preserve || !message)
		return context;
	auto preserved = context;
	preserved.policy = OutputPolicy::Preserve;
	if (strlen(message) >= MAX_STRING_LENGTH)
		return preserved;
	AnsiString visible(message);
	size_t spaces = 0, drawing = 0;
	for (size_t i = 0; i < visible.size(); ++i)
	{
		const auto ch = visible.ch(i);
		spaces = ch == ' ' ? spaces + 1 : 0;
		drawing = ch == '|' || ch == '/' || ch == '\\' || ch == '_' || ch == '+' ||
					  ch == '-' || ch == '*' ?
				  drawing + 1 :
				  0;
		if (ch == '\t' || (ch >= 0x2500 && ch <= 0x259f) || spaces >= 3 || drawing >= 3)
			return preserved;
	}
	return context;
}

void send_authored_output(const char *message, char_data *recipient, const OutputContext &context)
{
	auto resolved =
		context.resolve_recipient_preferences ?
			player_output_profile(recipient, context.channel, context.policy).context :
			context;
	resolved.spans = context.spans;
	resolved.original_message = context.original_message;
	send_to_char(message, recipient, preserve_authored_layout(message, resolved));
}
