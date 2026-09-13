#pragma once

#include "player/output_preferences.h"
#include <vector>

// Build at the existing recipient delivery point. Template fragments have known
// provenance; player text and entity names are never stripped or guessed.
class PlayerOutputMessage
{
    public:
	PlayerOutputMessage(char_data *recipient, OutputChannel channel);
	PlayerOutputMessage &literal(std::string_view legacy_template);
	PlayerOutputMessage &body(std::string_view authored_text);
	PlayerOutputMessage &entity(std::string_view authored_name,
				    StyleOrigin origin = StyleOrigin::Entity);
	void send(int log);

    private:
	char_data *recipient_;
	ResolvedOutputProfile profile_;
	std::string legacy_, selected_;
	std::vector<OutputStyleSpan> spans_;
	bool candidate_valid_ = true;
	void append_legacy(std::string_view text);
};

// Only for fixed, known command template wrappers. Use after recipient selection.
const char *player_output_template(char_data *recipient, OutputChannel channel, const char *legacy,
				   const char *selected);

// World descriptions can include authored layout. Conservative layout detection
// is a presentation veto, never a source of gameplay or visibility information.
OutputContext preserve_authored_layout(const char *message, const OutputContext &context);
void send_authored_output(const char *message, char_data *recipient, const OutputContext &context);
