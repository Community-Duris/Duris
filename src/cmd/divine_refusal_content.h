#ifndef DURIS_DIVINE_REFUSAL_CONTENT_H
#define DURIS_DIVINE_REFUSAL_CONTENT_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>

inline constexpr size_t DIVINE_REFUSAL_CONTENT_MAX_BYTES = 64 * 1024;
inline constexpr size_t DIVINE_REFUSAL_CONTENT_MAX_ENTRIES = 64;
inline constexpr int DIVINE_REFUSAL_CONTENT_MIN_VNUM = 1;
inline constexpr int DIVINE_REFUSAL_CONTENT_MAX_VNUM = 1000000;
inline constexpr size_t DIVINE_REFUSAL_CONTENT_MAX_PATRON = 64;
inline constexpr size_t DIVINE_REFUSAL_CONTENT_MAX_MESSAGE = 256;
inline constexpr char DIVINE_REFUSAL_CONTENT_FILE[] = "lib/misc/divine_refusal.json";
inline constexpr char DIVINE_REFUSAL_GENERIC_LINE[] =
	"My deity has warned me against completing that action.";

struct DivineRefusalContentEntry
{
	std::string patron;
	std::string message;
	bool has_enabled = false;
	bool enabled = true;
	bool has_percent = false;
	float percent = 0.0f;
};

class DivineRefusalContentSnapshot
{
    public:
	uint32_t revision() const { return revision_; }
	const DivineRefusalContentEntry *find(int vnum) const;

    private:
	uint32_t revision_ = 0;
	std::map<int, DivineRefusalContentEntry> entries_;
	friend class DivineRefusalContentRegistry;
	friend std::shared_ptr<DivineRefusalContentSnapshot>
	divine_refusal_content_parse(std::string_view json);
};

struct DivineRefusalContentLoadResult
{
	bool ok = false;
	uint32_t revision = 0;
	std::string diagnostic;
};

class DivineRefusalContentRegistry
{
    public:
	std::shared_ptr<const DivineRefusalContentSnapshot> snapshot() const;
	// Reload is an explicit boot/admin boundary. Runtime order handling only
	// borrows an immutable snapshot and never parses content.
	DivineRefusalContentLoadResult reload_json(std::string_view json);
	DivineRefusalContentLoadResult reload_file(const std::string &path);

    private:
	std::shared_ptr<const DivineRefusalContentSnapshot> current_{};
};

DivineRefusalContentRegistry &divine_refusal_content_registry();

// Render one authored line, or the generic fallback when the row is missing,
// incomplete, or names a patron outside the reviewed canonical catalog.
// Returns true only when an authored line was rendered successfully.
bool divine_refusal_content_render(const DivineRefusalContentEntry *entry, char *out,
				   size_t out_len);

#endif // DURIS_DIVINE_REFUSAL_CONTENT_H
