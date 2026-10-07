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
