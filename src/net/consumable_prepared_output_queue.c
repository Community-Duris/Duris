#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

// Kind9 original output queue preparation. Original write_to_q/act/send_to_char
// stay byte-exact. Every allocation below precedes native mutation/publication.
#include "net/consumable_prepared_output_queue.h"
#include <utility>

struct consumable_prepared_output_queue::state
{
	struct original_block
	{
		const txt_block *node;
		std::string text;
	};
	const txt_q *original = nullptr;
	txt_q before{};
	std::vector<original_block> original_blocks;
	txt_block *first = nullptr, *last = nullptr;
	char *replacement = nullptr;
	size_t replacement_size = 0;
	size_t bytes = 0, entries = 0;
	bool overflowed = false;
	enum class phase : unsigned char
	{
		prepared,
		transferred
	} stage = phase::prepared;
	~state() noexcept
	{
		if (replacement)
			__free(replacement, __FILE__, __LINE__);
		while (first)
		{
			txt_block *next = first->next;
			__free(first->text, __FILE__, __LINE__);
			__free(first, __FILE__, __LINE__);
			first = next;
		}
	}
};

consumable_prepared_output_queue::consumable_prepared_output_queue(
	std::unique_ptr<state> state) noexcept
	: state_(std::move(state))
{
}
consumable_prepared_output_queue::~consumable_prepared_output_queue() noexcept = default;

std::unique_ptr<consumable_prepared_output_queue>
consumable_prepared_output_queue::prepare(const txt_q &queue,
					  std::span<const std::string_view> messages) noexcept
try
{
	auto owned = std::make_unique<state>();
	owned->original = &queue;
	owned->before = queue;
	owned->bytes = queue.bytes;
	owned->entries = queue.entries;
	owned->overflowed = queue.overflowed;
	if ((queue.head == nullptr) != (queue.tail == nullptr) ||
	    queue.bytes > SESSION_OUTPUT_MAX_BYTES || queue.entries > SESSION_OUTPUT_MAX_ENTRIES)
		return nullptr;
	size_t actual_bytes = 0;
	for (const txt_block *node = queue.head; node; node = node->next)
	{
		if (!node->text || owned->original_blocks.size() >= SESSION_OUTPUT_MAX_ENTRIES ||
		    std::any_of(owned->original_blocks.begin(), owned->original_blocks.end(),
				[node](const auto &seen) { return seen.node == node; }))
			return nullptr;
		const size_t length = strnlen(node->text, SESSION_OUTPUT_MAX_BYTES);
		if (length == SESSION_OUTPUT_MAX_BYTES ||
		    length + 1 > SESSION_OUTPUT_MAX_BYTES - actual_bytes)
			return nullptr;
		actual_bytes += length + 1;
		owned->original_blocks.push_back({ node, std::string(node->text, length) });
	}
	if (actual_bytes != queue.bytes || owned->original_blocks.size() != queue.entries ||
	    (!owned->original_blocks.empty() && owned->original_blocks.back().node != queue.tail))
		return nullptr;
	// Exact original output admission and merge predicates, applied in original
	// message order. Existing tail changes are staged in separate real storage.
	for (const auto message : messages)
	{
		if (owned->overflowed)
			break;
		// Match strnlen: embedded NUL terminates the original C-string input.
		const size_t zero = message.find('\0');
		const size_t length = zero == std::string_view::npos ? message.size() : zero;
		if (length >= MAX_STRING_LENGTH)
		{
			owned->overflowed = true;
			break;
		}
		const size_t tail_length = owned->last	      ? strlen(owned->last->text) :
					   owned->replacement ? owned->replacement_size - 1 :
					   queue.tail ? owned->original_blocks.back().text.size() :
							0;
		const bool tail_exists = owned->last || queue.tail;
		const bool merge = tail_exists && tail_length < MAX_INPUT_LENGTH &&
				   length < MAX_INPUT_LENGTH - tail_length;
		const size_t growth = length + (merge ? 0 : 1);
		if (owned->bytes > SESSION_OUTPUT_MAX_BYTES ||
		    growth > SESSION_OUTPUT_MAX_BYTES - owned->bytes ||
		    owned->entries > SESSION_OUTPUT_MAX_ENTRIES ||
		    (!merge && owned->entries == SESSION_OUTPUT_MAX_ENTRIES))
		{
			owned->overflowed = true;
			break;
		}
		if (merge)
		{
			char *text = static_cast<char *>(__try_malloc(
				tail_length + length + 1, MEM_TAG_BUFFER, __FILE__, __LINE__));
			if (!text)
				return nullptr;
			const char *prior =
				owned->last	   ? owned->last->text :
				owned->replacement ? owned->replacement :
						     owned->original_blocks.back().text.c_str();
			memcpy(text, prior, tail_length);
			if (length)
				memcpy(text + tail_length, message.data(), length);
			text[tail_length + length] = 0;
			if (owned->last)
			{
				__free(owned->last->text, __FILE__, __LINE__);
				owned->last->text = text;
			}
			else
			{
				if (owned->replacement)
					__free(owned->replacement, __FILE__, __LINE__);
				owned->replacement = text;
				owned->replacement_size = tail_length + length + 1;
			}
		}
		else
		{
			auto *node = static_cast<txt_block *>(__try_malloc(
				sizeof(txt_block), MEM_TAG_TXTBLK, __FILE__, __LINE__));
			if (!node)
				return nullptr;
			node->next = nullptr;
			node->text = static_cast<char *>(
				__try_malloc(length + 1, MEM_TAG_BUFFER, __FILE__, __LINE__));
			if (!node->text)
			{
				__free(node, __FILE__, __LINE__);
				return nullptr;
			}
			if (length)
				memcpy(node->text, message.data(), length);
			node->text[length] = 0;
			if (owned->last)
				owned->last->next = node;
			else
				owned->first = node;
			owned->last = node;
			++owned->entries;
		}
		owned->bytes += growth;
	}
	return std::unique_ptr<consumable_prepared_output_queue>(
		new consumable_prepared_output_queue(std::move(owned)));
}
catch (...)
{
	return nullptr;
}

bool consumable_prepared_output_queue::copy_proposed_bytes(
	std::vector<std::string> *output) const noexcept
try
{
	if (!output || !state_ || state_->stage != state::phase::prepared)
		return false;
	std::vector<std::string> result;
	result.reserve(state_->entries);
	for (const auto &block : state_->original_blocks)
		result.push_back(block.node == state_->before.tail && state_->replacement ?
					 state_->replacement :
					 block.text);
	for (const auto *node = state_->first; node; node = node->next)
		result.emplace_back(node->text);
	*output = std::move(result);
	return true;
}
catch (...)
{
	return false;
}

bool consumable_prepared_output_queue::retained_exact(const txt_q &queue) const noexcept
{
	if (!state_ || state_->stage != state::phase::prepared || state_->original != &queue ||
	    queue.head != state_->before.head || queue.tail != state_->before.tail ||
	    queue.bytes != state_->before.bytes || queue.entries != state_->before.entries ||
	    queue.overflowed != state_->before.overflowed ||
	    queue.overflow_reported != state_->before.overflow_reported)
		return false;
	const txt_block *node = queue.head;
	for (const auto &before : state_->original_blocks)
	{
		if (node != before.node || !node->text || strcmp(node->text, before.text.c_str()))
			return false;
		node = node->next;
	}
	if (node)
		return false;
	return true;
}

bool consumable_prepared_output_queue::transfer(txt_q &queue) noexcept
{
	if (!retained_exact(queue))
		return false;
	if (state_->replacement)
	{
		char *old = queue.tail->text;
		queue.tail->text = std::exchange(state_->replacement, nullptr);
		__free(old, __FILE__, __LINE__);
	}
	if (state_->first)
	{
		if (queue.tail)
			queue.tail->next = state_->first;
		else
			queue.head = state_->first;
		queue.tail = state_->last;
		state_->first = state_->last = nullptr;
	}
	queue.bytes = state_->bytes;
	queue.entries = state_->entries;
	queue.overflowed = state_->overflowed;
	state_->stage = state::phase::transferred;
	return true;
}

bool consumable_prepared_output_queue::proposed_state(
	consumable_proposed_output_queue_state *output) const noexcept
{
	if (!output || !state_ || state_->stage != state::phase::prepared)
		return false;
	*output = { state_->bytes, state_->entries, state_->overflowed };
	return true;
}

bool consumable_prepared_output_queue::current_storage_bytes(size_t *output) const noexcept
{
	if (!output || !state_)
		return false;
	size_t bytes = sizeof(*this) + sizeof(state);
	const auto add = [&bytes](size_t count, size_t width = 1) noexcept
	{
		if (width && (count > SIZE_MAX / width || count * width > SIZE_MAX - bytes))
			return false;
		bytes += count * width;
		return true;
	};
	if (!add(state_->original_blocks.capacity(), sizeof(state::original_block)) ||
	    !add(state_->replacement ? state_->replacement_size : 0))
		return false;
	for (const auto &entry : state_->original_blocks)
	{
		const auto address = reinterpret_cast<uintptr_t>(&entry.text);
		const auto data = reinterpret_cast<uintptr_t>(entry.text.data());
		if (!(data >= address && data - address < sizeof(entry.text)) &&
		    (entry.text.capacity() == SIZE_MAX || !add(entry.text.capacity() + 1)))
			return false;
	}
	for (const auto *node = state_->first; node; node = node->next)
		if (!add(sizeof(txt_block)) || !add(strlen(node->text) + 1))
			return false;
	// Real memory-tag headers/allocator bookkeeping and all recipient/current/
	// caller profiles remain separate Source obligations, never inferred here.
	*output = bytes;
	return true;
}

// Private complete declared allocation requests for this prepared queue. The
// real memory-tag header belongs to each actual staged buffer/block exactly once.
bool consumable_prepared_output_queue::owned_allocation_request_bytes(size_t *output) const noexcept
{
	if (!output || !state_)
		return false;
	size_t bytes = 0;
	if (!current_storage_bytes(&bytes))
		return false;
#ifdef MEMCHK
	size_t headers = state_->replacement ? 1 : 0;
	for (const auto *node = state_->first; node; node = node->next)
	{
		if (headers > SIZE_MAX - 2)
			return false;
		headers += 2;
	}
	if (headers > (SIZE_MAX - bytes) / sizeof(ALLOCATION_HEADER))
		return false;
	bytes += headers * sizeof(ALLOCATION_HEADER);
#endif
	*output = bytes;
	// Requested retained heap only. Full allocator/library/Source peaks and the
	// old queue/current G remain the original admission owner's separate gates.
	return true;
}
