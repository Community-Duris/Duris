#include "cmd/divine_refusal_content.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

static void expect_generic(const DivineRefusalContentEntry *entry)
{
	char rendered[DIVINE_REFUSAL_CONTENT_MAX_MESSAGE + 1];
	assert(!divine_refusal_content_render(entry, rendered, sizeof(rendered)));
	assert(std::strcmp(rendered, DIVINE_REFUSAL_GENERIC_LINE) == 0);
}

static void expect_rejected_and_retained(DivineRefusalContentRegistry &registry,
					 const std::string &json, uint32_t revision)
{
	const auto rejected = registry.reload_json(json);
	assert(!rejected.ok);
	assert(rejected.revision == revision);
	const auto snapshot = registry.snapshot();
	assert(snapshot && snapshot->revision() == revision);
}

int main()
{
	auto &registry = divine_refusal_content_registry();
	const auto loaded = registry.reload_json(R"JSON({
        "version": 1,
        "revision": 7,
        "entries": {
            "66026": {
                "patron": "Garl",
                "message": "{patron} has forbidden it. The price is $5."
            },
            "66031": { "enabled": false, "percent": 0 }
        }
    })JSON");
	assert(loaded.ok && loaded.revision == 7);
	const auto snapshot = registry.snapshot();
	assert(snapshot && snapshot->revision() == 7);
	const auto *garl = snapshot->find(66026);
	assert(garl && garl->has_enabled == false && garl->has_percent == false);
	assert(snapshot->find(66027) == nullptr);

	char rendered[DIVINE_REFUSAL_CONTENT_MAX_MESSAGE + 1];
	assert(divine_refusal_content_render(garl, rendered, sizeof(rendered)));
	assert(std::strcmp(rendered, "Garl has forbidden it. The price is $5.") == 0);
	expect_generic(nullptr);
	expect_generic(snapshot->find(66031));

	const auto *disabled = snapshot->find(66031);
	assert(disabled && disabled->has_enabled && !disabled->enabled);
	assert(disabled->has_percent && disabled->percent == 0.0f);

	expect_rejected_and_retained(
		registry,
		R"JSON({"version":1,"revision":8,"entries":{"66026":{"patron":"Garl","message":"bad {name}"}}})JSON",
		7);
	expect_rejected_and_retained(
		registry,
		R"JSON({"version":1,"revision":8,"entries":{"66026":{"patron":"Garl","message":"bad\u0001"}}})JSON",
		7);
	expect_rejected_and_retained(
		registry,
		R"JSON({"version":1,"revision":8,"entries":{"66026":{"patron":"Garl","message":"ok","percent":101}}})JSON",
		7);
	expect_rejected_and_retained(
		registry,
		R"JSON({"version":1,"revision":8,"entries":{"66026":{"patron":"Garl","message":"ok","unexpected":true}}})JSON",
		7);
	expect_rejected_and_retained(
		registry,
		R"JSON({"version":1,"revision":8,"entries":{"66026":{"patron":"Garl","message":"ok"}}} trailing)JSON",
		7);
	const auto unknown_patron = registry.reload_json(
		R"JSON({"version":1,"revision":8,"entries":{"66026":{"patron":"Unknown god","message":"should fall back"}}})JSON");
	assert(unknown_patron.ok && unknown_patron.revision == 8);
	expect_generic(registry.snapshot()->find(66026));
	const auto restored = registry.reload_json(R"JSON({
        "version": 1,
        "revision": 7,
        "entries": {
            "66026": {
                "patron": "Garl",
                "message": "{patron} has forbidden it. The price is $5."
            },
            "66031": { "enabled": false, "percent": 0 }
        }
    })JSON");
	assert(restored.ok && restored.revision == 7);

	std::string oversized(DIVINE_REFUSAL_CONTENT_MAX_BYTES + 1, ' ');
	expect_rejected_and_retained(registry, oversized, 7);

	const auto *still_garl = registry.snapshot()->find(66026);
	assert(still_garl && still_garl->message.find("$5") != std::string::npos);
	const auto file_loaded = registry.reload_file(DIVINE_REFUSAL_CONTENT_FILE);
	assert(file_loaded.ok && file_loaded.revision == 1);
	const auto file_snapshot = registry.snapshot();
	assert(file_snapshot && file_snapshot->find(66026) && file_snapshot->find(66031));
	assert(file_snapshot->find(66027) == nullptr);
	std::puts("divine refusal content: ok");
	return 0;
}
