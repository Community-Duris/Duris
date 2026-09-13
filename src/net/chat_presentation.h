#pragma once

#include "net/output_style.h"

struct char_data;

// Called only from an accepted terminal delivery. Serializes the frozen frame;
// never resolves a second profile, transforms language, or advances animation.
void gmcp_comm_channel_output(char_data *recipient, const OutputChatMessage &chat,
			      const OutputContext &context, const char *frozen);
