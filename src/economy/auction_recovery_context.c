#include "economy/auction_recovery_context.h"
#include "economy/auction_native_publication.h"
#include "economy/auction_native_command_context.h"
#include "economy/auction_repository.h"
#include <openssl/sha.h>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace
{
constexpr std::array<uint8_t, 4> magic = { 'N', 'A', 'R', '1' };
constexpr size_t limit = CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES;
struct writer
{
	std::vector<uint8_t> bytes;
	void raw(std::span<const uint8_t> value)
	{
		if (value.size() > limit - bytes.size())
			throw std::length_error("auction context");
		bytes.insert(bytes.end(), value.begin(), value.end());
	}
	template <class T> void number(T value)
	{
		std::array<uint8_t, sizeof(T)> data{};
		for (size_t i = 0; i < data.size(); ++i)
			data[i] = static_cast<uint8_t>(static_cast<uint64_t>(value) >> (8 * i));
		raw(data);
	}
	void blob(std::span<const uint8_t> value)
	{
		if (value.size() > limit)
			throw std::length_error("auction blob");
		number<uint32_t>(static_cast<uint32_t>(value.size()));
		raw(value);
	}
	void effect(const auction_recovery_effect &value)
	{
		number<uint8_t>(value.state);
		number<uint8_t>(value.periodic);
	}
};
struct reader
{
	std::span<const uint8_t> bytes;
	size_t cursor = 0;
	bool raw(size_t size, std::span<const uint8_t> *value)
	{
		if (size > bytes.size() - cursor)
			return false;
		*value = bytes.subspan(cursor, size);
		cursor += size;
		return true;
	}
	template <class T> bool number(T *value)
	{
		std::span<const uint8_t> part;
		if (!raw(sizeof(T), &part))
			return false;
		uint64_t number = 0;
		for (size_t i = 0; i < sizeof(T); ++i)
			number |= uint64_t(part[i]) << (8 * i);
		*value = static_cast<T>(number);
		return true;
	}
	bool blob(std::span<const uint8_t> *value)
	{
		uint32_t size = 0;
		return number(&size) && raw(size, value);
	}
	bool flag(bool *value)
	{
		uint8_t b = 0;
		if (!number(&b) || b > 1)
			return false;
		*value = b;
		return true;
	}
	bool effect(auction_recovery_effect *value)
	{
		return number(&value->state) && flag(&value->periodic);
	}
};
bool effect_valid(const auction_recovery_effect &value)
{
	return value.state <= 2 && (!value.periodic || value.state);
}
bool unused(const auction_recovery_context &context, bool permit_notice = false)
{
	if (context.ownership.state || context.balances.state ||
	    (!permit_notice && context.notice.state))
		return false;
	for (const auto &root : context.roots)
		for (const auto &effect : root)
			if (effect.state)
				return false;
	for (const auto &root : context.reload)
		for (const auto &effect : root)
			if (effect.state)
				return false;
	for (const auto &root : context.proclib)
		for (const auto &effect : root)
			if (effect.state)
				return false;
	return true;
}
bool same_forest(const std::vector<player_item_snapshot> &,
		 const std::vector<player_item_snapshot> &);
bool digest_forest(const std::vector<player_item_snapshot> &items,
		   const std::array<uint8_t, 32> &expected)
{
	std::vector<uint8_t> encoded;
	std::array<uint8_t, 32> digest{};
	return player_item_snapshot_list_encode(items, &encoded) ==
		       player_snapshot_codec_result::ok &&
	       SHA256(encoded.data(), encoded.size(), digest.data()) && digest == expected;
}
bool shape(const critical_command &command, const auction_recovery_context &context)
{
	auction_command_payload payload{};
	auction_native_command_context accepted;
	if (command.type != critical_command_type::auction || command.schema_version != 2 ||
	    !command.publication_required || !command.accepted_at_usec ||
	    !auction_repository_frozen_accounting_valid(command) ||
	    !auction_command_decode_payload(command, &payload) ||
	    payload.actor_pid != context.actor_pid || static_cast<uint8_t>(context.stage) > 3 ||
	    !effect_valid(context.ownership) || !effect_valid(context.balances) ||
	    !effect_valid(context.notice) || context.ownership.periodic ||
	    context.balances.periodic || context.notice.periodic ||
	    (!payload.actor_pid && context.balances.state))
		return false;
	if (auction_native_command_decode(command, &accepted) != economic_accounting_error::ok ||
	    accepted.original_level != context.original_level ||
	    accepted.acknowledged_save_revision != context.before_save_revision ||
	    context.player_before.size() != accepted.before_item_uids.size() ||
	    !std::equal(context.player_before.begin(), context.player_before.end(),
			accepted.before_item_uids.begin(),
			[](const player_item_snapshot &item, uint64_t uid)
			{ return item.object_uid == uid; }) ||
	    !digest_forest(context.player_before, accepted.before_digest) ||
	    !digest_forest(context.selected_literals, accepted.selected_digest) ||
	    context.selected_literals.size() != accepted.selected_node_count ||
	    std::count_if(context.selected_literals.begin(), context.selected_literals.end(),
			  [](const player_item_snapshot &item)
			  { return item.parent_index == -1; }) != accepted.selected_root_count)
		return false;
	std::vector<player_item_snapshot> admitted_after;
	if (!auction_native_expected_player_forest(payload, context.player_before,
						   context.selected_literals, false,
						   context.original_level, &admitted_after) ||
	    !digest_forest(admitted_after, accepted.after_digest))
		return false;
	if (payload.actor_pid ? (!context.before_present || !context.before_save_revision ||
				 !context.original_level) :
				(context.before_present || context.after_present ||
				 context.before_save_revision || context.original_level ||
				 !context.player_before.empty() || !context.player_after.empty()))
		return false;
	if (!context.after_present && !context.player_after.empty())
		return false;
	const bool items = payload.action == auction_action::list ||
			   payload.action == auction_action::claim_item;
	const bool ownership = payload.action == auction_action::bid ||
			       payload.action == auction_action::finalize ||
			       payload.action == auction_action::remove;
	if (!ownership && context.ownership.state)
		return false;
	if (items ? !auction_native_selected_forest_valid(payload, context.selected_literals) :
		    !context.selected_literals.empty())
		return false;
	if (context.reload.size() != context.selected_literals.size() ||
	    context.proclib.size() != context.selected_literals.size())
		return false;
	for (size_t i = 0; i < AUCTION_COMMAND_MAX_ITEMS; ++i)
	{
		for (size_t step = 0; step < context.roots[i].size(); ++step)
		{
			const auto &effect = context.roots[i][step];
			const bool supported =
				payload.action == auction_action::list ?
					(step == 0 || step == 1 || step == 4) :
					(payload.action == auction_action::claim_item &&
					 (step == 2 || step == 3 || step == 4));
			if (!effect_valid(effect) || effect.periodic ||
			    (effect.state && (!supported || i >= payload.item_count)))
				return false;
		}
	}
	for (size_t i = 0; i < context.selected_literals.size(); ++i)
	{
		if (context.proclib[i].size() > PLAYER_SNAPSHOT_MAX_ROWS ||
		    (!context.proclib[i].empty() &&
		     context.proclib[i].size() !=
			     context.selected_literals[i].extra_descriptions.size()))
			return false;
		for (const auto &effect : context.reload[i])
			if (!effect_valid(effect) ||
			    (payload.action != auction_action::claim_item && effect.state))
				return false;
		for (const auto &effect : context.proclib[i])
			if (!effect_valid(effect) || payload.action != auction_action::claim_item)
				return false;
	}
	if (!context.receipt_present)
		return context.stage == auction_recovery_stage::captured && unused(context) &&
		       !context.result_code && !context.durable_revision &&
		       context.failure_stage == critical_failure_stage::none &&
		       context.outcome == critical_apply_outcome::applied &&
		       std::all_of(context.result.begin(), context.result.end(),
				   [](uint8_t b) { return !b; });
	if (context.failure_stage != critical_failure_stage::none ||
	    (context.result_code ? context.outcome != critical_apply_outcome::terminal_failure :
				   (context.outcome != critical_apply_outcome::applied &&
				    context.outcome != critical_apply_outcome::already_applied)))
		return false;
	auction_command_result result{};
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> encoded{};
	if (!auction_command_decode_result(context.result.data(), context.result.size(), &result) ||
	    !auction_command_encode_result(result, &encoded) || encoded != context.result ||
	    result.action != payload.action || result.item_count > payload.item_count)
		return false;
	const uint64_t revision =
		std::max({ result.wallet_revision, result.bank_revision, result.auction_revision,
			   result.player_owner_revision, result.auction_owner_revision });
	if (context.durable_revision != revision)
		return false;
	if (!context.result_code)
	{
		if (result.item_count != (items ? payload.item_count : 0) ||
		    (payload.action == auction_action::list ?
			     !result.auction_id :
			     result.auction_id != payload.auction_id))
			return false;
		if (payload.action == auction_action::list &&
		    result.seller_pid != payload.actor_pid)
			return false;
		if (payload.action == auction_action::claim_item &&
		    result.winner_pid != payload.actor_pid)
			return false;
		for (size_t i = 0; i < result.item_count; ++i)
			if (payload.items[i].expected_item_revision == UINT64_MAX ||
			    result.item_uids[i] != payload.items[i].item_uid ||
			    result.item_revisions[i] != payload.items[i].expected_item_revision + 1)
				return false;
	}
	if (context.result_code && !unused(context, true))
		return false;
	if (context.after_present)
	{
		std::vector<player_item_snapshot> expected;
		if (!auction_native_expected_player_forest(
			    payload, context.player_before, context.selected_literals,
			    context.result_code != 0, context.original_level, &expected) ||
		    !same_forest(expected, context.player_after))
			return false;
	}
	if ((context.stage == auction_recovery_stage::physically_proven ||
	     context.stage == auction_recovery_stage::restored_after_proven) &&
	    payload.actor_pid && !context.after_present)
		return false;
	if (context.stage == auction_recovery_stage::restored_after_proven &&
	    (!payload.actor_pid || !context.after_present || !context.receipt_present))
		return false;
	if (context.stage == auction_recovery_stage::physically_proven && !context.result_code)
	{
		if (ownership && context.ownership.state != 2)
			return false;
		if (payload.actor_pid && result.wallet_revision && context.balances.state != 2)
			return false;
		for (size_t i = 0; i < payload.item_count; ++i)
		{
			if (payload.action == auction_action::list &&
			    (context.roots[i][0].state != 2 || context.roots[i][1].state != 2 ||
			     context.roots[i][4].state != 2))
				return false;
			if (payload.action == auction_action::claim_item)
			{
				if (context.roots[i][2].state != 2 ||
				    context.roots[i][3].state != 2 ||
				    context.roots[i][4].state != 2)
					return false;
			}
		}
		if (payload.action == auction_action::claim_item)
			for (size_t i = 0; i < context.selected_literals.size(); ++i)
			{
				if (context.proclib[i].size() !=
				    context.selected_literals[i].extra_descriptions.size())
					return false;
				for (const auto &effect : context.reload[i])
					if (effect.state != 2)
						return false;
				for (const auto &effect : context.proclib[i])
					if (effect.state != 2)
						return false;
			}
	}

	return true;
}
bool same_forest(const std::vector<player_item_snapshot> &a,
		 const std::vector<player_item_snapshot> &b)
{
	std::vector<uint8_t> x, y;
	return player_item_snapshot_list_encode(a, &x) == player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode(b, &y) == player_snapshot_codec_result::ok &&
	       x == y;
}
bool advances(const auction_recovery_effect &a, const auction_recovery_effect &b)
{
	return b.state >= a.state && b.state - a.state <= 1 && (a.state != 2 || a == b) &&
	       (!a.periodic || b.periodic);
}
} // namespace

player_snapshot_codec_result
auction_recovery_context_encode(const critical_command &command,
				const auction_recovery_context &context,
				std::vector<uint8_t> *out) noexcept
{
	try
	{
		if (!out || !shape(command, context))
			return player_snapshot_codec_result::invalid_value;
		std::vector<uint8_t> original;
		if (critical_command_encode(command, &original) !=
		    critical_command_codec_result::ok)
			return player_snapshot_codec_result::invalid_value;
		writer w;
		w.raw(magic);
		w.blob(original);
		w.number<uint32_t>(context.actor_pid);
		w.number<uint32_t>(context.original_level);
		w.number<uint64_t>(context.before_save_revision);
		w.number<uint8_t>(context.before_present);
		w.number<uint8_t>(context.after_present);
		for (const auto *items :
		     { &context.player_before, &context.player_after, &context.selected_literals })
		{
			std::vector<uint8_t> encoded;
			const auto result = player_item_snapshot_list_encode(*items, &encoded);
			if (result != player_snapshot_codec_result::ok)
				return result;
			w.blob(encoded);
		}
		w.number<uint8_t>(context.receipt_present);
		w.number<uint8_t>(static_cast<uint8_t>(context.outcome));
		w.number<uint32_t>(context.result_code);
		w.number<uint16_t>(static_cast<uint16_t>(context.failure_stage));
		w.number<uint64_t>(context.durable_revision);
		w.raw(context.result);
		w.number<uint8_t>(static_cast<uint8_t>(context.stage));
		for (const auto &root : context.roots)
			for (const auto &effect : root)
				w.effect(effect);
		for (const auto &root : context.reload)
			for (const auto &effect : root)
				w.effect(effect);
		for (const auto &root : context.proclib)
		{
			w.number<uint32_t>(static_cast<uint32_t>(root.size()));
			for (const auto &effect : root)
				w.effect(effect);
		}
		w.effect(context.ownership);
		w.effect(context.balances);
		w.effect(context.notice);
		*out = std::move(w.bytes);
		return player_snapshot_codec_result::ok;
	}
	catch (const std::length_error &)
	{
		return player_snapshot_codec_result::limit_exceeded;
	}
	catch (...)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
}

player_snapshot_codec_result auction_recovery_context_decode(const critical_command &command,
							     std::span<const uint8_t> bytes,
							     auction_recovery_context *out) noexcept
{
	try
	{
		if (!out || bytes.size() > limit)
			return player_snapshot_codec_result::limit_exceeded;
		reader r{ bytes };
		std::span<const uint8_t> part;
		std::vector<uint8_t> original;
		if (critical_command_encode(command, &original) !=
			    critical_command_codec_result::ok ||
		    !r.raw(magic.size(), &part) ||
		    !std::equal(part.begin(), part.end(), magic.begin()) || !r.blob(&part) ||
		    part.size() != original.size() ||
		    !std::equal(part.begin(), part.end(), original.begin()))
			return player_snapshot_codec_result::invalid_value;
		auction_recovery_context c;
		if (!r.number(&c.actor_pid) || !r.number(&c.original_level) ||
		    !r.number(&c.before_save_revision) || !r.flag(&c.before_present) ||
		    !r.flag(&c.after_present))
			return player_snapshot_codec_result::truncated;
		for (auto *items : { &c.player_before, &c.player_after, &c.selected_literals })
		{
			if (!r.blob(&part))
				return player_snapshot_codec_result::truncated;
			const auto result =
				player_item_snapshot_list_decode(part.data(), part.size(), items);
			if (result != player_snapshot_codec_result::ok)
				return result;
		}
		uint8_t outcome = 0, stage = 0;
		uint16_t failure = 0;
		if (!r.flag(&c.receipt_present) || !r.number(&outcome) ||
		    !r.number(&c.result_code) || !r.number(&failure) ||
		    !r.number(&c.durable_revision) || !r.raw(c.result.size(), &part))
			return player_snapshot_codec_result::truncated;
		std::copy(part.begin(), part.end(), c.result.begin());
		c.outcome = static_cast<critical_apply_outcome>(outcome);
		c.failure_stage = static_cast<critical_failure_stage>(failure);
		if (!r.number(&stage))
			return player_snapshot_codec_result::truncated;
		c.stage = static_cast<auction_recovery_stage>(stage);
		for (auto &root : c.roots)
			for (auto &effect : root)
				if (!r.effect(&effect))
					return player_snapshot_codec_result::truncated;
		c.reload.resize(c.selected_literals.size());
		c.proclib.resize(c.selected_literals.size());
		for (auto &root : c.reload)
			for (auto &effect : root)
				if (!r.effect(&effect))
					return player_snapshot_codec_result::truncated;
		for (auto &root : c.proclib)
		{
			uint32_t count = 0;
			if (!r.number(&count) || count > PLAYER_SNAPSHOT_MAX_ROWS ||
			    count > (bytes.size() - r.cursor) / 2)
				return player_snapshot_codec_result::limit_exceeded;
			root.resize(count);
			for (auto &effect : root)
				if (!r.effect(&effect))
					return player_snapshot_codec_result::truncated;
		}
		if (!r.effect(&c.ownership) || !r.effect(&c.balances) || !r.effect(&c.notice) ||
		    r.cursor != bytes.size() || !shape(command, c))
			return player_snapshot_codec_result::invalid_value;
		std::vector<uint8_t> canonical;
		const auto encoded = auction_recovery_context_encode(command, c, &canonical);
		if (encoded != player_snapshot_codec_result::ok ||
		    canonical.size() != bytes.size() ||
		    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
			return player_snapshot_codec_result::invalid_value;
		*out = std::move(c);
		return player_snapshot_codec_result::ok;
	}
	catch (...)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
}

bool auction_recovery_envelope_valid(const critical_native_recovery_envelope &e) noexcept
{
	auction_recovery_context c;
	return e.revision &&
	       (e.phase == critical_native_recovery_phase::execution_pending ||
		e.phase == critical_native_recovery_phase::continuation_pending) &&
	       auction_recovery_context_decode(e.command, e.attachment, &c) ==
		       player_snapshot_codec_result::ok &&
	       (e.phase != critical_native_recovery_phase::execution_pending ||
		(!c.notice.state && !c.notice.periodic)) &&
	       (e.phase != critical_native_recovery_phase::continuation_pending ||
		(c.stage == auction_recovery_stage::physically_proven ||
		 c.stage == auction_recovery_stage::restored_after_proven));
}
bool auction_recovery_initial_valid(const critical_native_recovery_envelope &e) noexcept
{
	auction_recovery_context c;
	return e.revision == 1 && e.phase == critical_native_recovery_phase::execution_pending &&
	       auction_recovery_context_decode(e.command, e.attachment, &c) ==
		       player_snapshot_codec_result::ok &&
	       c.stage == auction_recovery_stage::captured && !c.receipt_present &&
	       !c.after_present && unused(c);
}
bool auction_recovery_successor_valid(const critical_native_recovery_envelope &a,
				      const critical_native_recovery_envelope &b) noexcept
{
	try
	{
		auction_recovery_context x, y;
		if (!auction_recovery_envelope_valid(a) || !auction_recovery_envelope_valid(b) ||
		    a.revision == UINT64_MAX || b.revision != a.revision + 1 ||
		    !critical_command_equal(a.command, b.command) ||
		    (a.phase == critical_native_recovery_phase::continuation_pending &&
		     b.phase != a.phase) ||
		    auction_recovery_context_decode(a.command, a.attachment, &x) !=
			    player_snapshot_codec_result::ok ||
		    auction_recovery_context_decode(b.command, b.attachment, &y) !=
			    player_snapshot_codec_result::ok ||
		    x.actor_pid != y.actor_pid || x.original_level != y.original_level ||
		    x.before_save_revision != y.before_save_revision ||
		    x.before_present != y.before_present ||
		    !same_forest(x.player_before, y.player_before) ||
		    !same_forest(x.selected_literals, y.selected_literals) ||
		    (x.after_present &&
		     (!y.after_present || !same_forest(x.player_after, y.player_after))) ||
		    static_cast<uint8_t>(y.stage) < static_cast<uint8_t>(x.stage) ||
		    (x.stage == auction_recovery_stage::physically_proven &&
		     y.stage == auction_recovery_stage::restored_after_proven) ||
		    (x.receipt_present &&
		     (!y.receipt_present || x.result_code != y.result_code ||
		      x.result != y.result || x.durable_revision != y.durable_revision ||
		      x.failure_stage != y.failure_stage || x.outcome != y.outcome)) ||
		    !advances(x.ownership, y.ownership) || !advances(x.balances, y.balances) ||
		    !advances(x.notice, y.notice))
			return false;
		if (y.stage == auction_recovery_stage::restored_after_proven &&
		    x.stage != y.stage &&
		    (a.phase != critical_native_recovery_phase::execution_pending ||
		     b.phase != a.phase || x.roots != y.roots || x.reload != y.reload ||
		     x.proclib != y.proclib || x.ownership != y.ownership ||
		     x.balances != y.balances || x.notice != y.notice))
			return false;
		for (size_t i = 0; i < AUCTION_COMMAND_MAX_ITEMS; ++i)
			for (size_t j = 0; j < x.roots[i].size(); ++j)
				if (!advances(x.roots[i][j], y.roots[i][j]))
					return false;
		if (x.reload.size() != y.reload.size() || x.proclib.size() != y.proclib.size())
			return false;
		for (size_t i = 0; i < x.reload.size(); ++i)
		{
			for (size_t j = 0; j < x.reload[i].size(); ++j)
				if (!advances(x.reload[i][j], y.reload[i][j]))
					return false;
			if (!x.proclib[i].empty() && x.proclib[i].size() != y.proclib[i].size())
				return false;
			if (x.proclib[i].empty())
				for (const auto &effect : y.proclib[i])
					if (effect.state || effect.periodic)
						return false;
			for (size_t j = 0; j < x.proclib[i].size(); ++j)
				if (!advances(x.proclib[i][j], y.proclib[i][j]))
					return false;
		}

		return true;
	}
	catch (...)
	{
		return false;
	}
}
bool auction_recovery_publication_context_valid(const critical_native_recovery_envelope &e,
						const critical_completion &receipt) noexcept
{
	auction_recovery_context c;
	return auction_recovery_envelope_valid(e) &&
	       receipt.disposition == critical_completion_disposition::execution &&
	       critical_operation_id_equal(e.command.operation_id, receipt.operation_id) &&
	       auction_recovery_context_decode(e.command, e.attachment, &c) ==
		       player_snapshot_codec_result::ok &&
	       c.receipt_present &&
	       (c.stage == auction_recovery_stage::physically_proven ||
		c.stage == auction_recovery_stage::restored_after_proven) &&
	       receipt.result_size == c.result.size() && c.result_code == receipt.error_code &&
	       c.failure_stage == receipt.failure_stage &&
	       c.durable_revision == receipt.durable_revision &&
	       (c.outcome == receipt.outcome ||
		(c.outcome == critical_apply_outcome::applied &&
		 receipt.outcome == critical_apply_outcome::already_applied)) &&
	       std::equal(c.result.begin(), c.result.end(), receipt.result_payload.begin()) &&
	       std::all_of(receipt.result_payload.begin() + c.result.size(),
			   receipt.result_payload.end(), [](uint8_t b) { return !b; });
}
bool auction_recovery_terminal_valid(const critical_native_recovery_envelope &e) noexcept
{
	auction_recovery_context c;
	return e.phase == critical_native_recovery_phase::continuation_pending &&
	       auction_recovery_envelope_valid(e) &&
	       auction_recovery_context_decode(e.command, e.attachment, &c) ==
		       player_snapshot_codec_result::ok &&
	       (c.stage == auction_recovery_stage::physically_proven ||
		c.stage == auction_recovery_stage::restored_after_proven) &&
	       c.notice.state == 2;
}

#if defined(__linux__) && defined(__x86_64__) && __cplusplus == 202002L &&                        \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_SANITIZE_VECTOR)
namespace
{
constexpr size_t auction_codec_allocator_frames =
	// _M_allocate, allocator_traits::allocate, allocator::allocate (C++20):
	// each this/allocator reference, n and returned pointer; new_allocator
	// adds its genuine hint pointer; operator new n and returned pointer.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) +
	// _M_deallocate/traits/allocator/new_allocator: allocator/this+p+n,
	// then sized operator delete p+n. Trivial element _Destroy closures.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	(3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *)) +
	// vector max_size/_S_max_size/traits max_size/new_allocator::_M_max_size
	// references/results and actual diffmax/allocmax locals. C++20 allocator
	// has no max_size member; that inactive C++17 branch is not counted.
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) +
	// traits::construct -> construct_at -> forward -> placement-new; all
	// constructor arguments here are real references to trivial values.
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
constexpr size_t auction_codec_copy_frames =
	// __uninitialized_move_if_noexcept_a and __uninitialized_copy_a: 3
	// iterators+allocator-reference+returned iterator each. Runtime ordinary
	// uninitialized_copy's two boolean locals and __uninit_copy carrier.
	2 * (4 * sizeof(void *) + sizeof(void *)) + 3 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(void *) +
	// copy/copy_move_a/a1/a2/copy_m, each3 iterator params+return; real
	// miter/niter/wrap/assign_one and memmove argument/result scopes.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// distance/__distance and normal-iterator subtraction/base/dereference/
	// ++/comparison/constructor source parameter/return scopes.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) + sizeof(char) +
	6 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) + sizeof(bool) +
	// Fitting forward insert reaches advance(__mid,__elems_after), even zero.
	// advance: iterator-reference, size_t n, real local difference_type __d;
	// __iterator_category: iterator-reference and actual returned RA tag;
	// __advance: iterator-reference, difference n and by-value RA tag;
	// actual += this/n/reference-return, plus source ++/-- alternatives.
	sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	4 * sizeof(void *);
constexpr size_t auction_codec_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t auction_codec_default_frames =
	// Runtime default_n_a/default_n/default_n_1<true>: real first/n/allocator
	// reference, can_fill and val locals, actual returned pointer carriers.
	(3 * sizeof(void *) + sizeof(size_t)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(3 * sizeof(void *) + sizeof(size_t)) +
	// _Construct's real location plus placement-new n/location/result.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// fill_n/__fill_n_a<random_access>: first/n/value/result/tag;
	// __size_to_integer argument/result; __fill_a/__fill_a1 scalar __tmp.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(char) + 2 * sizeof(size_t) +
	2 * (3 * sizeof(void *)) + sizeof(uint64_t);
constexpr size_t auction_codec_vector_frames =
	auction_codec_allocator_frames + auction_codec_copy_frames + auction_codec_relocate_frames +
	auction_codec_default_frames +
	// reserve this/n/old_size/tmp; assign public/forward-aux and exact
	// _M_allocate_and_copy's this/n/first/last/result/returned pointer.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 7 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(char) + 5 * sizeof(void *) + sizeof(size_t) +
	// push_back/emplace_back and real realloc_insert old/new start/finish,
	// len/elems_before/position/forward value reference; _M_check_len.
	2 * sizeof(void *) + 3 * sizeof(void *) + 7 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(void *) + 3 * sizeof(size_t) +
	// C++20 forward insert public/range-insert (no old dispatch), offset/elems_after/
	// len/old-start/finish/mid/new-start/finish/iterator return/tag scopes.
	15 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(char) +
	// default_append's n/size/navail/len and real old/new/destroy pointers.
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	// begin/end/cbegin/size/capacity/get-allocator declared carriers and
	// iterator-category/std::max arguments/results on the real call paths.
	7 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(char) + 3 * sizeof(void *);
constexpr size_t auction_codec_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + auction_codec_allocator_frames;
constexpr size_t auction_codec_vector_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<uint8_t>) + 2 * sizeof(void *) +
	sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	8 * (sizeof(void *) + sizeof(size_t)) + auction_codec_vector_frames;

// Genuine additional selected typed library scopes beside the vector's
// reserve/forward-insert profile. The owning inline DTOs remain in the actual
// enclosing object sizes, rather than a fabricated encoded-envelope baseline.
static_assert(std::is_trivially_copyable_v<economic_source_event>);
static_assert(std::is_trivially_destructible_v<economic_source_event>);
static_assert(std::is_trivially_copyable_v<auction_command_payload>);
static_assert(std::is_trivially_copyable_v<economic_account_key>);
constexpr size_t auction_codec_optional_frames =
	// Actual metadata/frozen/admission default/generated move/copy member
	// functions: this/source refs; optional/_Optional_base/_payload/_Storage
	// default constructors and trivial storage destructor this carriers.
	6 * sizeof(void *) + 5 * sizeof(void *) + sizeof(void *) +
	// source_event assignment operator=(T&&): this/u/ref-result; real
	// is_engaged/get/construct wrappers, payload _M_construct and forward.
	3 * sizeof(void *) + (sizeof(void *) + sizeof(bool)) + 2 * (2 * sizeof(void *)) +
	2 * sizeof(void *) + 2 * sizeof(void *) +
	// __addressof -> _Construct -> forward -> placement new -> trivial
	// economic_source_event generated move this/source. No extra DTO copy.
	2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(void *) +
	sizeof(size_t) + sizeof(void *) + 2 * sizeof(void *) +
	// Actual optional operator bool/operator->/base get/payload get and
	// addressof parameter and reference/pointer/bool result carriers.
	2 * (sizeof(void *) + sizeof(bool)) + 3 * (2 * sizeof(void *));
constexpr size_t auction_codec_equal_frames =
	// array/vector operator== actual lhs/rhs and returned bool; genuine
	// container size/begin/end and array_traits::_S_ptr pointer returns.
	2 * sizeof(void *) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t)) +
	6 * (2 * sizeof(void *)) +
	// equal/__equal_aux/__equal_aux1/__equal<true>::equal each three
	// iterator arguments and returned bool; __simple and __len locals;
	// niter_base calls and __memcmp's genuine pointers/length/int result.
	4 * (3 * sizeof(void *) + sizeof(bool)) + sizeof(bool) + sizeof(size_t) +
	3 * (2 * sizeof(void *)) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(int) +
	// Actual normal_iterator copied argument/ctor/base source carriers.
	4 * (2 * sizeof(void *));
constexpr size_t auction_codec_copy_n_frames =
	// Original copy_n(count literal16) owns first/count/result/__n2/result,
	// __size_to_integer(int), iterator_category and __copy_n<RA> tag.
	3 * sizeof(void *) + 2 * sizeof(int) + 2 * sizeof(int) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + 3 * sizeof(void *) + sizeof(int) +
	sizeof(std::random_access_iterator_tag);
constexpr size_t auction_codec_scalar_source_frames =
	// Original read lambda (facts-reference capture/this, offset,width,
	// byte index,value/result); typed read_number alternatives; span data,
	// index,size/constructor parameters and returned pointer/reference.
	2 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(uint64_t) +
	sizeof(std::span<const uint8_t>) + 2 * sizeof(size_t) + 2 * sizeof(uint64_t) +
	4 * (sizeof(void *) + sizeof(size_t) + sizeof(void *)) +
	// Actual account/empty/key_valid/kind_valid helper params/results,
	// operation_id_equal two refs/result and zero's byte range loop.
	3 * sizeof(void *) + sizeof(economic_account_kind) + sizeof(uint64_t) + sizeof(bool) +
	2 * (sizeof(void *) + sizeof(bool)) + sizeof(economic_account_kind) + sizeof(bool) +
	2 * sizeof(void *) + sizeof(bool) + 4 * sizeof(void *) + sizeof(uint8_t) + sizeof(bool) +
	// Original append_u64/u32/u16 and native_fact_append<T> pointer/value/
	// index/byte result lifetimes, plus initializer-list begin/end/size.
	4 * (sizeof(void *) + sizeof(uint64_t) + sizeof(size_t) + sizeof(uint8_t)) +
	3 * (sizeof(void *) + sizeof(void *)) + sizeof(std::initializer_list<uint64_t>) +
	// Original metadata assignment generated function this/source and the
	// returned source_for event temporary, optional typed path above.
	2 * sizeof(void *) + sizeof(economic_source_event) + auction_codec_optional_frames +
	auction_codec_equal_frames + auction_codec_copy_n_frames;
bool auction_codec_add(size_t &total, size_t value) noexcept
{
	if (value > SIZE_MAX - total)
		return false;
	total += value;
	return true;
}

struct auction_recovery_budget_refusal
{
};
struct auction_recovery_codec_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const auction_recovery_context *candidate = nullptr;
	const auction_native_command_context *native = nullptr;
	const std::vector<player_item_snapshot> *forest = nullptr, *second_forest = nullptr;
	const std::vector<uint8_t> *original = nullptr, *writer_bytes = nullptr, *encoded = nullptr,
				   *canonical = nullptr;
	bool denied = false;
	static bool forward(size_t amount, void *opaque) noexcept
	{
		auto &b = *static_cast<auction_recovery_codec_budget *>(opaque);
		if (b.denied || !b.reserve || !b.reserve(amount, b.context))
		{
			b.denied = true;
			return false;
		}
		return true;
	}
	bool rows(const std::vector<player_item_snapshot> &items, size_t &bytes) const noexcept
	{
		if (items.capacity() > SIZE_MAX / sizeof(player_item_snapshot) ||
		    !auction_codec_add(bytes, items.capacity() * sizeof(player_item_snapshot)))
			return false;
		for (const auto &row : items)
		{
			size_t heap = 0;
			if (!player_item_snapshot_current_heap_bytes(row, &heap) ||
			    !auction_codec_add(bytes, heap))
				return false;
		}
		return true;
	}
	bool prefix(size_t &bytes, size_t extra = 0) noexcept
	{
		bytes = outer;
		if (!auction_codec_add(bytes, sizeof(*this)) || !auction_codec_add(bytes, frames) ||
		    !auction_codec_add(bytes, extra))
		{
			denied = true;
			return false;
		}
		if (candidate)
		{
			if (!rows(candidate->player_before, bytes) ||
			    !rows(candidate->player_after, bytes) ||
			    !rows(candidate->selected_literals, bytes) ||
			    candidate->reload.capacity() >
				    SIZE_MAX / sizeof(std::array<auction_recovery_effect, 4>) ||
			    !auction_codec_add(
				    bytes,
				    candidate->reload.capacity() *
					    sizeof(std::array<auction_recovery_effect, 4>)) ||
			    candidate->proclib.capacity() >
				    SIZE_MAX / sizeof(std::vector<auction_recovery_effect>) ||
			    !auction_codec_add(
				    bytes, candidate->proclib.capacity() *
						   sizeof(std::vector<auction_recovery_effect>)))
			{
				denied = true;
				return false;
			}
			for (const auto &effects : candidate->proclib)
				if (effects.capacity() >
					    SIZE_MAX / sizeof(auction_recovery_effect) ||
				    !auction_codec_add(bytes,
						       effects.capacity() *
							       sizeof(auction_recovery_effect)))
				{
					denied = true;
					return false;
				}
		}
		if (native && (!auction_codec_add(bytes, native->base_v1_payload.capacity()) ||
			       native->before_item_uids.capacity() > SIZE_MAX / sizeof(uint64_t) ||
			       !auction_codec_add(bytes, native->before_item_uids.capacity() *
								 sizeof(uint64_t))))
		{
			denied = true;
			return false;
		}
		const bool observed =
			(!forest || rows(*forest, bytes)) &&
			(!second_forest || rows(*second_forest, bytes)) &&
			(!original || auction_codec_add(bytes, original->capacity())) &&
			(!writer_bytes || auction_codec_add(bytes, writer_bytes->capacity())) &&
			(!encoded || auction_codec_add(bytes, encoded->capacity())) &&
			(!canonical || auction_codec_add(bytes, canonical->capacity()));
		if (!observed)
			denied = true;
		return observed;
	}
	bool peak(size_t extra = 0) noexcept
	{
		size_t bytes = 0;
		if (!prefix(bytes, extra) || !forward(bytes, this))
		{
			denied = true;
			return false;
		}
		return true;
	}
	template <class T> bool resize(std::vector<T> &v, size_t count)
	{
		size_t request = 0;
		if (count > v.max_size())
			return false;
		if (count > v.capacity())
		{
			const size_t additional = count - v.size();
			const size_t growth = std::max(v.size(), additional);
			const size_t capacity =
				growth > v.max_size() - v.size() ? v.max_size() : v.size() + growth;
			if (capacity > SIZE_MAX / sizeof(T))
				return false;
			request = capacity * sizeof(T);
		}
		if (!peak(request))
			return false;
		v.resize(count);
		return true;
	}
};
struct auction_recovery_denial_latch
{
	const auction_recovery_codec_budget &budget;
	bool *output;
	~auction_recovery_denial_latch() noexcept
	{
		if (output && budget.denied)
			*output = true;
	}
};
bool auction_recovery_query_prepare(bool (*reserve)(size_t, void *) noexcept, void *context,
				    size_t outer, size_t owning_query) noexcept
{
	// This helper's two pointers/two size arguments, local total, checked-add
	// signature and actual callback signature/returns coexist with the caller.
	size_t total = outer;
	return reserve && auction_codec_add(total, owning_query) &&
	       auction_codec_add(total,
				 4 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(bool)) &&
	       reserve(total, context);
}
bool auction_recovery_codec_policy() noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	return sizeof(void *) == 8 && sizeof(size_t) == 8;
#else
	return false;
#endif
}
struct auction_bounded_context_writer
{
	std::vector<uint8_t> bytes;
	auction_recovery_codec_budget &budget;
	void raw(std::span<const uint8_t> value)
	{
		if (value.size() > limit - bytes.size())
			throw std::length_error("auction context");
		size_t request = 0;
		if (value.size() > bytes.capacity() - bytes.size())
		{
			const size_t growth = std::max(bytes.size(), value.size());
			const size_t capacity = growth > bytes.max_size() - bytes.size() ?
							bytes.max_size() :
							bytes.size() + growth;
			request = capacity;
		}
		if (!budget.peak(request))
			throw auction_recovery_budget_refusal{};
		bytes.insert(bytes.end(), value.begin(), value.end());
	}
	template <class T> void number(T value)
	{
		std::array<uint8_t, sizeof(T)> data{};
		for (size_t i = 0; i < data.size(); ++i)
			data[i] = static_cast<uint8_t>(static_cast<uint64_t>(value) >> (8 * i));
		raw(data);
	}
	void blob(std::span<const uint8_t> value)
	{
		if (value.size() > limit)
			throw std::length_error("auction blob");
		number<uint32_t>(static_cast<uint32_t>(value.size()));
		raw(value);
	}
	void effect(const auction_recovery_effect &value)
	{
		number<uint8_t>(value.state);
		number<uint8_t>(value.periodic);
	}
};
}

size_t auction_recovery_shape_source_frame_bytes() noexcept
{
	using iterator = std::vector<player_item_snapshot>::const_iterator;
	using uid_iterator = std::vector<uint64_t>::const_iterator;
	// equal(first1,last1,first2,predicate) -> __equal3/__equal: real
	// iterator params/local first copies and captureless UID comparison body.
	constexpr size_t equal = 8 * sizeof(iterator) + 4 * sizeof(uid_iterator) +
				 8 * sizeof(void *) + 4 * sizeof(bool) + sizeof(uint64_t) +
				 2 * sizeof(char);
	// count_if/__count_if: first/last/predicate/count, _Iter_pred's captured
	// empty closure object, actual caller item reference and returned bool.
	constexpr size_t count = 6 * sizeof(iterator) + 4 * sizeof(void *) +
				 2 * sizeof(std::ptrdiff_t) + 4 * sizeof(bool) + 2 * sizeof(char);
	// Original unused/effect_valid and full roots/reload/proclib/receipt scans:
	// complete range iterators, root/effect references, step/index locals,
	// array size/index/data, enum/scalar comparison and returned boolean.
	constexpr size_t effects =
		14 * sizeof(void *) + 8 * sizeof(size_t) + 8 * sizeof(bool) +
		2 * sizeof(std::vector<std::array<auction_recovery_effect, 4>>::const_iterator) +
		2 * sizeof(std::vector<std::vector<auction_recovery_effect>>::const_iterator) +
		2 * sizeof(std::vector<auction_recovery_effect>::const_iterator);
	// Full result original decode/encode comparison, max initializer-list
	// and raw SHA256 declaration call carriers. Executable stack qualification
	// remains separate from these source-declared carrier scopes.
	constexpr size_t result = 12 * sizeof(void *) + 8 * sizeof(size_t) + 6 * sizeof(bool) +
				  sizeof(std::initializer_list<uint64_t>) + 5 * sizeof(uint64_t) +
				  4 * sizeof(uint32_t) + 3 * sizeof(uint16_t) +
				  2 * sizeof(uint8_t) + 2 * sizeof(void *) + sizeof(size_t) +
				  sizeof(void *);
	// Own rows/prefix/query scopes accompany the genuine integrated178 CURRENT
	// export, including const-capacity descendants. Fresh-copy observation is
	// not a substitute for this real CURRENT closure.
	constexpr size_t heap_observer =
		6 * sizeof(void *) + 4 * sizeof(size_t) + 4 * sizeof(bool) + 2 * sizeof(iterator);
	return equal + count + effects + result + heap_observer +
	       player_item_snapshot_current_heap_observer_frame_bytes();
}
size_t auction_recovery_wire_source_frame_bytes() noexcept
{
	// Original reader raw/blob/number/flag/effect signatures and live part,
	// cursor/number/i/size/b temporaries; actual span subspan constructors,
	// data/size/index access and equality/copy controls. Helpers do not recurse.
	constexpr size_t reader_frames = 14 * sizeof(void *) + 12 * sizeof(size_t) +
					 2 * sizeof(std::span<const uint8_t>) + sizeof(uint64_t) +
					 sizeof(uint32_t) + sizeof(uint8_t) + 8 * sizeof(bool);
	// Original writer raw/blob/effect/typed number, real typed local data
	// array (largest number width), i/count/request/capacity/growth and span
	// range constructors. Budget raw adds no unrecorded staging byte buffer.
	constexpr size_t writer_frames =
		16 * sizeof(void *) + 15 * sizeof(size_t) + 3 * sizeof(std::span<const uint8_t>) +
		sizeof(std::array<uint8_t, sizeof(uint64_t)>) + sizeof(uint64_t) +
		sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t) + 6 * sizeof(bool);
	// Context nested-vector resize enters real default construction of each
	// inner vector and each array<effect,4>; generic byte-vector profiles alone
	// would omit those typed default/cleanup carriers.
	constexpr size_t nested =
		4 * sizeof(void *) + 2 * sizeof(std::allocator<auction_recovery_effect>) +
		4 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + sizeof(size_t) +
		2 * sizeof(void *) + 4 * (2 * sizeof(void *) + sizeof(uint8_t) + sizeof(bool));
	// Genuine own budget/rows/resize/query/add footprints, joined with the
	// complete integrated178 row CURRENT observer rather than an older copy
	// inventory. Its getter-return scope is included in query preflight.
	constexpr size_t observers = 18 * sizeof(void *) + 11 * sizeof(size_t) + 8 * sizeof(bool);
	return reader_frames + writer_frames + nested + observers +
	       player_item_snapshot_current_heap_observer_frame_bytes();
}

namespace
{
constexpr size_t auction_digest_sha_assembly_frames =
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) + 2 * sizeof(void *);
constexpr size_t auction_digest_sha_c_small_frames =
	16 * sizeof(unsigned int) + 12 * sizeof(unsigned int) + sizeof(unsigned int) + sizeof(int) +
	sizeof(const uint8_t *);
constexpr size_t auction_digest_sha_c_normal_frames = 16 * sizeof(unsigned int) +
						      11 * sizeof(unsigned int) + 2 * sizeof(int) +
						      2 * sizeof(void *);
constexpr size_t auction_digest_sha_init_frames = sizeof(void *) + sizeof(int);
constexpr size_t auction_digest_sha_update_frames = 2 * sizeof(void *) + sizeof(size_t) +
						    2 * sizeof(void *) + sizeof(unsigned int) +
						    sizeof(size_t) + sizeof(int);
constexpr size_t auction_digest_sha_final_frames = 3 * sizeof(void *) + sizeof(size_t) +
						   sizeof(unsigned long) + sizeof(unsigned int) +
						   sizeof(int);
[[maybe_unused]] constexpr size_t auction_digest_sha_frames =
	std::max(auction_digest_sha_assembly_frames,
		 std::max(auction_digest_sha_c_small_frames, auction_digest_sha_c_normal_frames)) +
	std::max(auction_digest_sha_init_frames,
		 std::max(auction_digest_sha_update_frames, auction_digest_sha_final_frames));

template <class Budget> bool auction_digest_fixed_bounded(std::span<const uint8_t> bytes,
							  std::array<uint8_t, 32> &output,
							  Budget &budget) noexcept
{
#if defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 &&      \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&  \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	if (!budget.peak(sizeof(SHA256_CTX) + auction_digest_sha_frames +
			 sizeof(std::span<const uint8_t>) + 3 * sizeof(void *) + 4 * sizeof(bool)))
		return false;
	// Original auction digest has no tag or domain prefix. Hash exactly the
	// same full canonical forest bytes, using caller-owned fixed SHA state.
	SHA256_CTX digest_context;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	const bool ok = SHA256_Init(&digest_context) == 1 &&
			SHA256_Update(&digest_context, bytes.data(), bytes.size()) == 1 &&
			SHA256_Final(output.data(), &digest_context) == 1;
#pragma GCC diagnostic pop
	return ok;
#else
	(void)bytes;
	(void)output;
	budget.denied = true;
	return false;
#endif
}

bool digest_forest_bounded(const std::vector<player_item_snapshot> &items,
			   const std::array<uint8_t, 32> &expected,
			   bool (*reserve)(size_t, void *) noexcept, void *opaque, size_t outer,
			   bool *budget_denied)
{
	const size_t frames = sizeof(std::vector<uint8_t>) + sizeof(std::array<uint8_t, 32>) +
			      10 * sizeof(void *) + 4 * sizeof(size_t) + 4 * sizeof(bool) +
			      auction_codec_vector_constructor_frames + auction_codec_move_frames;
	auction_recovery_codec_budget budget{ reserve, opaque, outer,
					      frames + sizeof(auction_recovery_denial_latch) };
	auction_recovery_denial_latch denial{ budget, budget_denied };
	size_t nested = 0;
	if (!auction_recovery_codec_policy() || !budget.peak())
		return false;

	std::vector<uint8_t> encoded;
	std::array<uint8_t, 32> digest{};
	budget.encoded = &encoded;
	return budget.prefix(nested) &&
	       player_item_snapshot_list_encode_bounded(
		       items, &encoded, auction_recovery_codec_budget::forward, &budget, nested) ==
		       player_snapshot_codec_result::ok &&
	       auction_digest_fixed_bounded(encoded, digest, budget) && digest == expected;
}
bool same_forest_bounded(const std::vector<player_item_snapshot> &a,
			 const std::vector<player_item_snapshot> &b,
			 bool (*reserve)(size_t, void *) noexcept, void *opaque, size_t outer,
			 bool *budget_denied)
{
	const size_t frames = 2 * sizeof(std::vector<uint8_t>) + 10 * sizeof(void *) +
			      5 * sizeof(size_t) + 5 * sizeof(bool) +
			      auction_codec_vector_constructor_frames + auction_codec_move_frames;
	auction_recovery_codec_budget budget{ reserve, opaque, outer,
					      frames + sizeof(auction_recovery_denial_latch) };
	auction_recovery_denial_latch denial{ budget, budget_denied };
	size_t nested = 0;
	if (!auction_recovery_codec_policy() || !budget.peak())
		return false;

	std::vector<uint8_t> x, y;
	budget.original = &x;
	budget.encoded = &y;
	return budget.prefix(nested) &&
	       player_item_snapshot_list_encode_bounded(
		       a, &x, auction_recovery_codec_budget::forward, &budget, nested) ==
		       player_snapshot_codec_result::ok &&
	       budget.prefix(nested) &&
	       player_item_snapshot_list_encode_bounded(
		       b, &y, auction_recovery_codec_budget::forward, &budget, nested) ==
		       player_snapshot_codec_result::ok &&
	       x == y;
}
bool auction_recovery_shape_bounded(const critical_command &command,
				    const auction_recovery_context &context,
				    bool (*reserve)(size_t, void *) noexcept, void *opaque,
				    size_t outer, bool *budget_denied)
{
	if (!auction_recovery_query_prepare(reserve, opaque, outer,
					    5 * sizeof(void *) + 8 * sizeof(size_t) + sizeof(bool)))
	{
		if (budget_denied)
			*budget_denied = true;
		return false;
	}
	const size_t frames =
		sizeof(auction_command_payload) + sizeof(auction_native_command_context) +
		2 * sizeof(std::vector<player_item_snapshot>) + sizeof(auction_command_result) +
		sizeof(std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES>) + 25 * sizeof(void *) +
		14 * sizeof(size_t) + 15 * sizeof(bool) + 4 * sizeof(uint64_t) +
		auction_codec_vector_constructor_frames + auction_codec_move_frames +
		auction_recovery_shape_source_frame_bytes();
	auction_recovery_codec_budget budget{ reserve, opaque, outer,
					      frames + sizeof(auction_recovery_denial_latch) };
	auction_recovery_denial_latch denial{ budget, budget_denied };
	size_t nested = 0;
	if (!auction_recovery_codec_policy() || !budget.peak())
		return false;

	auction_command_payload payload{};
	auction_native_command_context accepted;
	budget.native = &accepted;
	if (command.type != critical_command_type::auction || command.schema_version != 2 ||
	    !command.publication_required || !command.accepted_at_usec || !budget.prefix(nested) ||
	    !auction_repository_frozen_accounting_valid_bounded(
		    command, auction_recovery_codec_budget::forward, &budget, nested) ||
	    !budget.prefix(nested) ||
	    !auction_command_decode_payload_bounded(
		    command, &payload, auction_recovery_codec_budget::forward, &budget, nested) ||
	    payload.actor_pid != context.actor_pid || static_cast<uint8_t>(context.stage) > 3 ||
	    !effect_valid(context.ownership) || !effect_valid(context.balances) ||
	    !effect_valid(context.notice) || context.ownership.periodic ||
	    context.balances.periodic || context.notice.periodic ||
	    (!payload.actor_pid && context.balances.state))
		return false;
	if ((!budget.prefix(nested) ?
		     economic_accounting_error::capacity :
		     auction_native_command_decode_bounded(
			     command, &accepted, auction_recovery_codec_budget::forward, &budget,
			     nested)) != economic_accounting_error::ok ||
	    accepted.original_level != context.original_level ||
	    accepted.acknowledged_save_revision != context.before_save_revision ||
	    context.player_before.size() != accepted.before_item_uids.size() ||
	    !std::equal(context.player_before.begin(), context.player_before.end(),
			accepted.before_item_uids.begin(),
			[](const player_item_snapshot &item, uint64_t uid)
			{ return item.object_uid == uid; }) ||
	    !budget.prefix(nested) ||
	    !digest_forest_bounded(context.player_before, accepted.before_digest,
				   auction_recovery_codec_budget::forward, &budget, nested,
				   &budget.denied) ||
	    !budget.prefix(nested) ||
	    !digest_forest_bounded(context.selected_literals, accepted.selected_digest,
				   auction_recovery_codec_budget::forward, &budget, nested,
				   &budget.denied) ||
	    context.selected_literals.size() != accepted.selected_node_count ||
	    std::count_if(context.selected_literals.begin(), context.selected_literals.end(),
			  [](const player_item_snapshot &item)
			  { return item.parent_index == -1; }) != accepted.selected_root_count)
		return false;
	std::vector<player_item_snapshot> admitted_after;
	budget.forest = &admitted_after;
	if (!budget.prefix(nested) ||
	    !auction_native_expected_player_forest_bounded(
		    payload, context.player_before, context.selected_literals, false,
		    context.original_level, &admitted_after, auction_recovery_codec_budget::forward,
		    &budget, nested) ||
	    !budget.prefix(nested) ||
	    !digest_forest_bounded(admitted_after, accepted.after_digest,
				   auction_recovery_codec_budget::forward, &budget, nested,
				   &budget.denied))
		return false;
	if (payload.actor_pid ? (!context.before_present || !context.before_save_revision ||
				 !context.original_level) :
				(context.before_present || context.after_present ||
				 context.before_save_revision || context.original_level ||
				 !context.player_before.empty() || !context.player_after.empty()))
		return false;
	if (!context.after_present && !context.player_after.empty())
		return false;
	const bool items = payload.action == auction_action::list ||
			   payload.action == auction_action::claim_item;
	const bool ownership = payload.action == auction_action::bid ||
			       payload.action == auction_action::finalize ||
			       payload.action == auction_action::remove;
	if (!ownership && context.ownership.state)
		return false;
	if (items ? (!budget.prefix(nested) ||
		     !auction_native_selected_forest_valid_bounded(
			     payload, context.selected_literals,
			     auction_recovery_codec_budget::forward, &budget, nested)) :
		    !context.selected_literals.empty())
		return false;
	if (context.reload.size() != context.selected_literals.size() ||
	    context.proclib.size() != context.selected_literals.size())
		return false;
	for (size_t i = 0; i < AUCTION_COMMAND_MAX_ITEMS; ++i)
	{
		for (size_t step = 0; step < context.roots[i].size(); ++step)
		{
			const auto &effect = context.roots[i][step];
			const bool supported =
				payload.action == auction_action::list ?
					(step == 0 || step == 1 || step == 4) :
					(payload.action == auction_action::claim_item &&
					 (step == 2 || step == 3 || step == 4));
			if (!effect_valid(effect) || effect.periodic ||
			    (effect.state && (!supported || i >= payload.item_count)))
				return false;
		}
	}
	for (size_t i = 0; i < context.selected_literals.size(); ++i)
	{
		if (context.proclib[i].size() > PLAYER_SNAPSHOT_MAX_ROWS ||
		    (!context.proclib[i].empty() &&
		     context.proclib[i].size() !=
			     context.selected_literals[i].extra_descriptions.size()))
			return false;
		for (const auto &effect : context.reload[i])
			if (!effect_valid(effect) ||
			    (payload.action != auction_action::claim_item && effect.state))
				return false;
		for (const auto &effect : context.proclib[i])
			if (!effect_valid(effect) || payload.action != auction_action::claim_item)
				return false;
	}
	if (!context.receipt_present)
		return context.stage == auction_recovery_stage::captured && unused(context) &&
		       !context.result_code && !context.durable_revision &&
		       context.failure_stage == critical_failure_stage::none &&
		       context.outcome == critical_apply_outcome::applied &&
		       std::all_of(context.result.begin(), context.result.end(),
				   [](uint8_t b) { return !b; });
	if (context.failure_stage != critical_failure_stage::none ||
	    (context.result_code ? context.outcome != critical_apply_outcome::terminal_failure :
				   (context.outcome != critical_apply_outcome::applied &&
				    context.outcome != critical_apply_outcome::already_applied)))
		return false;
	auction_command_result result{};
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> encoded{};
	if (!auction_command_decode_result(context.result.data(), context.result.size(), &result) ||
	    !budget.prefix(nested) ||
	    !auction_command_encode_result_bounded(
		    result, &encoded, auction_recovery_codec_budget::forward, &budget, nested) ||
	    encoded != context.result || result.action != payload.action ||
	    result.item_count > payload.item_count)
		return false;
	const uint64_t revision =
		std::max({ result.wallet_revision, result.bank_revision, result.auction_revision,
			   result.player_owner_revision, result.auction_owner_revision });
	if (context.durable_revision != revision)
		return false;
	if (!context.result_code)
	{
		if (result.item_count != (items ? payload.item_count : 0) ||
		    (payload.action == auction_action::list ?
			     !result.auction_id :
			     result.auction_id != payload.auction_id))
			return false;
		if (payload.action == auction_action::list &&
		    result.seller_pid != payload.actor_pid)
			return false;
		if (payload.action == auction_action::claim_item &&
		    result.winner_pid != payload.actor_pid)
			return false;
		for (size_t i = 0; i < result.item_count; ++i)
			if (payload.items[i].expected_item_revision == UINT64_MAX ||
			    result.item_uids[i] != payload.items[i].item_uid ||
			    result.item_revisions[i] != payload.items[i].expected_item_revision + 1)
				return false;
	}
	if (context.result_code && !unused(context, true))
		return false;
	if (context.after_present)
	{
		std::vector<player_item_snapshot> expected;
		budget.second_forest = &expected;
		if (!budget.prefix(nested) ||
		    !auction_native_expected_player_forest_bounded(
			    payload, context.player_before, context.selected_literals,
			    context.result_code != 0, context.original_level, &expected,
			    auction_recovery_codec_budget::forward, &budget, nested) ||
		    !budget.prefix(nested) ||
		    !same_forest_bounded(expected, context.player_after,
					 auction_recovery_codec_budget::forward, &budget, nested,
					 &budget.denied))
			return false;
		budget.second_forest = nullptr;
	}
	if ((context.stage == auction_recovery_stage::physically_proven ||
	     context.stage == auction_recovery_stage::restored_after_proven) &&
	    payload.actor_pid && !context.after_present)
		return false;
	if (context.stage == auction_recovery_stage::restored_after_proven &&
	    (!payload.actor_pid || !context.after_present || !context.receipt_present))
		return false;
	if (context.stage == auction_recovery_stage::physically_proven && !context.result_code)
	{
		if (ownership && context.ownership.state != 2)
			return false;
		if (payload.actor_pid && result.wallet_revision && context.balances.state != 2)
			return false;
		for (size_t i = 0; i < payload.item_count; ++i)
		{
			if (payload.action == auction_action::list &&
			    (context.roots[i][0].state != 2 || context.roots[i][1].state != 2 ||
			     context.roots[i][4].state != 2))
				return false;
			if (payload.action == auction_action::claim_item)
			{
				if (context.roots[i][2].state != 2 ||
				    context.roots[i][3].state != 2 ||
				    context.roots[i][4].state != 2)
					return false;
			}
		}
		if (payload.action == auction_action::claim_item)
			for (size_t i = 0; i < context.selected_literals.size(); ++i)
			{
				if (context.proclib[i].size() !=
				    context.selected_literals[i].extra_descriptions.size())
					return false;
				for (const auto &effect : context.reload[i])
					if (effect.state != 2)
						return false;
				for (const auto &effect : context.proclib[i])
					if (effect.state != 2)
						return false;
			}
	}

	return true;
}
}
player_snapshot_codec_result auction_recovery_context_encode_bounded(
	const critical_command &command, const auction_recovery_context &context,
	std::vector<uint8_t> *out, bool (*reserve)(size_t, void *) noexcept, void *opaque,
	size_t outer) noexcept
{
	if (!auction_recovery_query_prepare(reserve, opaque, outer,
					    5 * sizeof(void *) + 7 * sizeof(size_t) +
						    sizeof(player_snapshot_codec_result)))
		return player_snapshot_codec_result::limit_exceeded;
	const size_t frames = sizeof(auction_bounded_context_writer) +
			      2 * sizeof(std::vector<uint8_t>) + 20 * sizeof(void *) +
			      12 * sizeof(size_t) + 8 * sizeof(bool) + 4 * sizeof(uint64_t) +
			      auction_codec_vector_frames + auction_codec_move_frames +
			      auction_recovery_wire_source_frame_bytes();
	auction_recovery_codec_budget budget{ reserve, opaque, outer, frames };
	size_t nested = 0;
	if (!auction_recovery_codec_policy() || !budget.peak())
		return player_snapshot_codec_result::limit_exceeded;
	try
	{
		if (!out || !budget.prefix(nested) ||
		    !auction_recovery_shape_bounded(command, context,
						    auction_recovery_codec_budget::forward, &budget,
						    nested, &budget.denied))
			return budget.denied ? player_snapshot_codec_result::limit_exceeded :
					       player_snapshot_codec_result::invalid_value;
		std::vector<uint8_t> original;
		budget.original = &original;
		if ((!budget.prefix(nested) ?
			     critical_command_codec_result::overflow :
			     critical_command_encode_bounded(
				     command, &original, auction_recovery_codec_budget::forward,
				     &budget, nested)) != critical_command_codec_result::ok)
			return budget.denied ? player_snapshot_codec_result::limit_exceeded :
					       player_snapshot_codec_result::invalid_value;
		auction_bounded_context_writer w{ {}, budget };
		budget.writer_bytes = &w.bytes;
		w.raw(magic);
		w.blob(original);
		w.number<uint32_t>(context.actor_pid);
		w.number<uint32_t>(context.original_level);
		w.number<uint64_t>(context.before_save_revision);
		w.number<uint8_t>(context.before_present);
		w.number<uint8_t>(context.after_present);
		for (const auto *items :
		     { &context.player_before, &context.player_after, &context.selected_literals })
		{
			std::vector<uint8_t> encoded;
			budget.encoded = &encoded;
			const auto result = (!budget.prefix(nested) ?
						     player_snapshot_codec_result::limit_exceeded :
						     player_item_snapshot_list_encode_bounded(
							     *items, &encoded,
							     auction_recovery_codec_budget::forward,
							     &budget, nested));
			if (result != player_snapshot_codec_result::ok)
				return result;
			w.blob(encoded);
			budget.encoded = nullptr;
		}
		w.number<uint8_t>(context.receipt_present);
		w.number<uint8_t>(static_cast<uint8_t>(context.outcome));
		w.number<uint32_t>(context.result_code);
		w.number<uint16_t>(static_cast<uint16_t>(context.failure_stage));
		w.number<uint64_t>(context.durable_revision);
		w.raw(context.result);
		w.number<uint8_t>(static_cast<uint8_t>(context.stage));
		for (const auto &root : context.roots)
			for (const auto &effect : root)
				w.effect(effect);
		for (const auto &root : context.reload)
			for (const auto &effect : root)
				w.effect(effect);
		for (const auto &root : context.proclib)
		{
			w.number<uint32_t>(static_cast<uint32_t>(root.size()));
			for (const auto &effect : root)
				w.effect(effect);
		}
		w.effect(context.ownership);
		w.effect(context.balances);
		w.effect(context.notice);
		*out = std::move(w.bytes);
		return player_snapshot_codec_result::ok;
	}
	catch (const auction_recovery_budget_refusal &)
	{
		return player_snapshot_codec_result::limit_exceeded;
	}
	catch (const std::length_error &)
	{
		return player_snapshot_codec_result::limit_exceeded;
	}
	catch (...)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
}
player_snapshot_codec_result auction_recovery_context_decode_bounded(
	const critical_command &command, std::span<const uint8_t> bytes,
	auction_recovery_context *out, bool (*reserve)(size_t, void *) noexcept, void *opaque,
	size_t outer) noexcept
{
	if (!auction_recovery_query_prepare(reserve, opaque, outer,
					    5 * sizeof(void *) + 7 * sizeof(size_t) +
						    sizeof(player_snapshot_codec_result)))
		return player_snapshot_codec_result::limit_exceeded;
	const size_t frames = sizeof(auction_recovery_context) + sizeof(reader) +
			      2 * sizeof(std::vector<uint8_t>) + sizeof(std::span<const uint8_t>) +
			      20 * sizeof(void *) + 12 * sizeof(size_t) + 8 * sizeof(bool) +
			      sizeof(uint32_t) + 2 * sizeof(uint8_t) + sizeof(uint16_t) +
			      auction_codec_vector_frames + auction_codec_move_frames +
			      auction_recovery_wire_source_frame_bytes();
	auction_recovery_codec_budget budget{ reserve, opaque, outer, frames };
	size_t nested = 0;
	if (!auction_recovery_codec_policy() || !budget.peak())
		return player_snapshot_codec_result::limit_exceeded;
	try
	{
		if (!out || bytes.size() > limit)
			return player_snapshot_codec_result::limit_exceeded;
		reader r{ bytes };
		std::span<const uint8_t> part;
		std::vector<uint8_t> original;
		budget.original = &original;
		if ((!budget.prefix(nested) ?
			     critical_command_codec_result::overflow :
			     critical_command_encode_bounded(
				     command, &original, auction_recovery_codec_budget::forward,
				     &budget, nested)) != critical_command_codec_result::ok ||
		    !r.raw(magic.size(), &part) ||
		    !std::equal(part.begin(), part.end(), magic.begin()) || !r.blob(&part) ||
		    part.size() != original.size() ||
		    !std::equal(part.begin(), part.end(), original.begin()))
			return budget.denied ? player_snapshot_codec_result::limit_exceeded :
					       player_snapshot_codec_result::invalid_value;
		auction_recovery_context c;
		budget.candidate = &c;
		if (!r.number(&c.actor_pid) || !r.number(&c.original_level) ||
		    !r.number(&c.before_save_revision) || !r.flag(&c.before_present) ||
		    !r.flag(&c.after_present))
			return player_snapshot_codec_result::truncated;
		for (auto *items : { &c.player_before, &c.player_after, &c.selected_literals })
		{
			if (!r.blob(&part))
				return player_snapshot_codec_result::truncated;
			const auto result = (!budget.prefix(nested) ?
						     player_snapshot_codec_result::limit_exceeded :
						     player_item_snapshot_list_decode_bounded(
							     part.data(), part.size(), items,
							     auction_recovery_codec_budget::forward,
							     &budget, nested));
			if (result != player_snapshot_codec_result::ok)
				return result;
		}
		uint8_t outcome = 0, stage = 0;
		uint16_t failure = 0;
		if (!r.flag(&c.receipt_present) || !r.number(&outcome) ||
		    !r.number(&c.result_code) || !r.number(&failure) ||
		    !r.number(&c.durable_revision) || !r.raw(c.result.size(), &part))
			return player_snapshot_codec_result::truncated;
		std::copy(part.begin(), part.end(), c.result.begin());
		c.outcome = static_cast<critical_apply_outcome>(outcome);
		c.failure_stage = static_cast<critical_failure_stage>(failure);
		if (!r.number(&stage))
			return player_snapshot_codec_result::truncated;
		c.stage = static_cast<auction_recovery_stage>(stage);
		for (auto &root : c.roots)
			for (auto &effect : root)
				if (!r.effect(&effect))
					return player_snapshot_codec_result::truncated;
		if (!budget.resize(c.reload, c.selected_literals.size()))
			return player_snapshot_codec_result::limit_exceeded;
		if (!budget.resize(c.proclib, c.selected_literals.size()))
			return player_snapshot_codec_result::limit_exceeded;
		for (auto &root : c.reload)
			for (auto &effect : root)
				if (!r.effect(&effect))
					return player_snapshot_codec_result::truncated;
		for (auto &root : c.proclib)
		{
			uint32_t count = 0;
			if (!r.number(&count) || count > PLAYER_SNAPSHOT_MAX_ROWS ||
			    count > (bytes.size() - r.cursor) / 2)
				return player_snapshot_codec_result::limit_exceeded;
			if (!budget.resize(root, count))
				return player_snapshot_codec_result::limit_exceeded;
			for (auto &effect : root)
				if (!r.effect(&effect))
					return player_snapshot_codec_result::truncated;
		}
		if (!r.effect(&c.ownership) || !r.effect(&c.balances) || !r.effect(&c.notice) ||
		    r.cursor != bytes.size() ||
		    (!budget.prefix(nested) ||
		     !auction_recovery_shape_bounded(command, c,
						     auction_recovery_codec_budget::forward,
						     &budget, nested, &budget.denied)))
			return budget.denied ? player_snapshot_codec_result::limit_exceeded :
					       player_snapshot_codec_result::invalid_value;
		std::vector<uint8_t> canonical;
		budget.canonical = &canonical;
		const auto encoded =
			(!budget.prefix(nested) ?
				 player_snapshot_codec_result::limit_exceeded :
				 auction_recovery_context_encode_bounded(
					 command, c, &canonical,
					 auction_recovery_codec_budget::forward, &budget, nested));
		if (encoded != player_snapshot_codec_result::ok ||
		    canonical.size() != bytes.size() ||
		    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
			return budget.denied ? player_snapshot_codec_result::limit_exceeded :
					       player_snapshot_codec_result::invalid_value;
		*out = std::move(c);
		return player_snapshot_codec_result::ok;
	}
	catch (...)
	{
		return player_snapshot_codec_result::allocation_failure;
	}
}
bool auction_recovery_envelope_valid_bounded(const critical_native_recovery_envelope &e,
					     bool (*reserve)(size_t, void *) noexcept, void *opaque,
					     size_t outer) noexcept
{
	const size_t frames = sizeof(auction_recovery_context) + 8 * sizeof(void *) +
			      3 * sizeof(size_t) + 5 * sizeof(bool) +
			      auction_codec_vector_constructor_frames + auction_codec_move_frames;
	auction_recovery_codec_budget budget{ reserve, opaque, outer, frames };
	size_t nested = 0;
	if (!auction_recovery_codec_policy() || !budget.peak())
		return false;

	auction_recovery_context c;
	return e.revision &&
	       (e.phase == critical_native_recovery_phase::execution_pending ||
		e.phase == critical_native_recovery_phase::continuation_pending) &&
	       (!budget.prefix(nested) ?
			player_snapshot_codec_result::limit_exceeded :
			auction_recovery_context_decode_bounded(
				e.command, e.attachment, &c, auction_recovery_codec_budget::forward,
				&budget, nested)) == player_snapshot_codec_result::ok &&
	       (e.phase != critical_native_recovery_phase::execution_pending ||
		(!c.notice.state && !c.notice.periodic)) &&
	       (e.phase != critical_native_recovery_phase::continuation_pending ||
		(c.stage == auction_recovery_stage::physically_proven ||
		 c.stage == auction_recovery_stage::restored_after_proven));
}
#else
size_t auction_recovery_shape_source_frame_bytes() noexcept
{
	return 0;
}
size_t auction_recovery_wire_source_frame_bytes() noexcept
{
	return 0;
}
player_snapshot_codec_result
auction_recovery_context_encode_bounded(const critical_command &, const auction_recovery_context &,
					std::vector<uint8_t> *, bool (*)(size_t, void *) noexcept,
					void *, size_t) noexcept
{
	return player_snapshot_codec_result::limit_exceeded;
}
player_snapshot_codec_result
auction_recovery_context_decode_bounded(const critical_command &, std::span<const uint8_t>,
					auction_recovery_context *,
					bool (*)(size_t, void *) noexcept, void *, size_t) noexcept
{
	return player_snapshot_codec_result::limit_exceeded;
}
bool auction_recovery_envelope_valid_bounded(const critical_native_recovery_envelope &,
					     bool (*)(size_t, void *) noexcept, void *,
					     size_t) noexcept
{
	return false;
}
#endif
