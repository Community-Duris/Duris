#include "item/quest_reward_continuation.h"
#include "world/native_quest_recovery_context.h"

#include <algorithm>
#include <charconv>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

// Passive original exported values only. Never opens/initializes a journal or
// calls a SQL, physical publication, save-hold, ACK or coordinator owner.
namespace
{
constexpr size_t output_limit = 64 * 1024;
void need(bool condition)
{
	if (!condition)
		throw std::runtime_error("refused");
}

std::string digest(std::span<const uint8_t> bytes)
{
	uint8_t hash[SHA256_DIGEST_LENGTH]{};
	SHA256(bytes.data(), bytes.size(), hash);
	std::string result;
	for (uint8_t byte : hash)
	{
		result += "0123456789abcdef"[byte >> 4];
		result += "0123456789abcdef"[byte & 15];
	}
	return result;
}

std::string hex(const critical_operation_id &id)
{
	char result[CRITICAL_COMMAND_ID_HEX_SIZE]{};
	need(critical_operation_id_to_hex(id, result, sizeof(result)));
	return result;
}

uint64_t integer(const char *text)
{
	const std::string input(text);
	uint64_t value = 0;
	const auto parsed = std::from_chars(input.data(), input.data() + input.size(), value);
	need(!input.empty() && input[0] != '0' && parsed.ec == std::errc{} &&
	     parsed.ptr == input.data() + input.size() && value);
	return value;
}

struct descriptor
{
	int fd;
	~descriptor()
	{
		if (fd >= 0)
			close(fd);
	}
};

bool same(const struct stat &a, const struct stat &b)
{
	return a.st_dev == b.st_dev && a.st_ino == b.st_ino && a.st_size == b.st_size &&
	       a.st_mtim.tv_sec == b.st_mtim.tv_sec && a.st_mtim.tv_nsec == b.st_mtim.tv_nsec &&
	       a.st_ctim.tv_sec == b.st_ctim.tv_sec && a.st_ctim.tv_nsec == b.st_ctim.tv_nsec;
}

std::vector<uint8_t> read_file(const char *path, size_t limit)
{
	descriptor file{ open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK) };
	struct stat before
	{
	}, after{};
	need(file.fd >= 0 && !fstat(file.fd, &before) && S_ISREG(before.st_mode) &&
	     !(before.st_mode & 0222) && before.st_size > 0 &&
	     static_cast<uint64_t>(before.st_size) <= limit);
	auto read_exact = [&]()
	{
		std::vector<uint8_t> bytes(static_cast<size_t>(before.st_size));
		size_t offset = 0;
		while (offset < bytes.size())
		{
			const ssize_t count =
				read(file.fd, bytes.data() + offset, bytes.size() - offset);
			need(count > 0);
			offset += static_cast<size_t>(count);
		}
		uint8_t extra = 0;
		need(read(file.fd, &extra, 1) == 0);
		return bytes;
	};
	auto bytes = read_exact();
	need(!fstat(file.fd, &after) && same(before, after) && lseek(file.fd, 0, SEEK_SET) == 0);
	need(read_exact() == bytes && !fstat(file.fd, &after) && same(before, after));
	const int fd = file.fd;
	file.fd = -1;
	need(close(fd) == 0);
	return bytes;
}

struct original
{
	critical_native_recovery_envelope envelope;
	item_transfer_payload payload{};
	native_quest_recovery_context context;
	std::vector<uint8_t> command_bytes;
};

std::unique_ptr<original> load(char **args)
{
	auto result = std::make_unique<original>();
	auto &out = *result;
	out.command_bytes = read_file(args[0], CRITICAL_COMMAND_MAX_ENCODED_BYTES);
	out.envelope.attachment = read_file(args[1], CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES);
	out.envelope.revision = integer(args[2]);
	const uint64_t phase = integer(args[3]);
	need(phase == 1 || phase == 2);
	out.envelope.phase = static_cast<critical_native_recovery_phase>(phase);
	auto &command = out.envelope.command;
	std::vector<uint8_t> canonical;
	need(critical_command_decode(out.command_bytes.data(), out.command_bytes.size(),
				     &command) == critical_command_codec_result::ok &&
	     critical_command_encode(command, &canonical) == critical_command_codec_result::ok &&
	     canonical == out.command_bytes &&
	     command.payload_version == ITEM_TRANSFER_NATIVE_MOBILE_RECOVERY_PAYLOAD_VERSION &&
	     item_transfer_command_decode_payload(command, &out.payload) &&
	     native_quest_recovery_context_decode(command, out.envelope.attachment, &out.context) ==
		     player_snapshot_codec_result::ok);
	return result;
}

template <typename T> void array(std::ostream &out, const T &values)
{
	out << '[';
	bool first = true;
	for (const auto value : values)
	{
		if (!first)
			out << ',';
		first = false;
		out << static_cast<uint64_t>(value);
	}
	out << ']';
}

void summary(std::ostream &out, const original &value)
{
	const auto &envelope = value.envelope;
	const auto &context = value.context;
	const auto &native = value.payload.native_mobile.reference;
	const auto &receipt = context.receipt;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source{};
	need(economic_source_event_encode(native.birth_source, &source) ==
	     economic_accounting_error::ok);
	out << "{\"operation\":\"" << hex(envelope.command.operation_id)
	    << "\",\"command_sha256\":\"" << digest(value.command_bytes)
	    << "\",\"attachment_sha256\":\"" << digest(envelope.attachment)
	    << "\",\"command_bytes\":" << value.command_bytes.size()
	    << ",\"attachment_bytes\":" << envelope.attachment.size()
	    << ",\"revision\":" << envelope.revision
	    << ",\"phase\":" << static_cast<int>(envelope.phase)
	    << ",\"payload_version\":" << envelope.command.payload_version
	    << ",\"pid\":" << value.payload.native_recovery.player_pid
	    << ",\"instance\":" << native.mobile_instance_id << ",\"birth\":\""
	    << hex(native.birth_operation) << "\",\"source_sha256\":\"" << digest(source)
	    << "\",\"save_revision\":" << value.payload.native_recovery.acknowledged_save_revision
	    << ",\"publication_stage\":" << static_cast<int>(context.publication_stage)
	    << ",\"handoff\":" << static_cast<int>(context.child_handoff_stage)
	    << ",\"branch\":" << context.next_branch << ",\"publication_steps\":";
	array(out, context.publication_steps);
	out << ",\"give_messages\":";
	array(out, context.give_messages);
	out << ",\"give_hooks\":";
	array(out, context.give_hooks);
	out << ",\"consumed_steps\":";
	array(out, context.consumed_root_steps);
	out << ",\"receipt\":{\"present\":" << (receipt.present ? "true" : "false")
	    << ",\"outcome\":" << static_cast<int>(receipt.outcome)
	    << ",\"durable_revision\":" << receipt.durable_revision
	    << ",\"error_code\":" << receipt.error_code
	    << ",\"failure_stage\":" << static_cast<int>(receipt.failure_stage)
	    << ",\"result_size\":" << receipt.result_size << ",\"result_sha256\":\""
	    << digest(std::span(receipt.result_payload).first(receipt.result_size))
	    << "\",\"result_array_sha256\":\"" << digest(receipt.result_payload) << "\"}}";
}

std::string inspect(const original &parent, const original &child)
{
	const auto &p = parent.payload;
	const auto &c = child.payload;
	const auto &pc = parent.context;
	need(parent.envelope.phase == critical_native_recovery_phase::continuation_pending &&
	     p.native_mobile.action == item_native_mobile_action::acceptance &&
	     c.native_mobile.action == item_native_mobile_action::consumption &&
	     p.native_mobile.reference.mobile_vnum == 16006 &&
	     c.native_mobile.reference.mobile_vnum == 16006 &&
	     p.native_mobile.reference.birthplace_vnum == 16077 && pc.branch_program_frozen &&
	     pc.next_branch < pc.branches.size() && pc.child_handoff_stage &&
	     pc.next_child_command == child.command_bytes &&
	     pc.next_child_operation.bytes == child.envelope.command.operation_id.bytes &&
	     child.context.parent_acceptance.bytes == parent.envelope.command.operation_id.bytes &&
	     c.continuation.kind == item_transfer_continuation_kind::quest_offering);
	const auto &branch = pc.branches[pc.next_branch];
	need(branch.give == std::vector<native_quest_recovery_goal>{ { 1, 16080 },
								     { 1, 16014 },
								     { 1, 16013 } } &&
	     branch.receive ==
		     std::vector<native_quest_recovery_goal>{ { 1, 16075 }, { 1, 16015 } } &&
	     branch.disappear && c.native_recovery.publication_terms.disappear &&
	     c.native_recovery.publication_terms.message == branch.message &&
	     c.native_recovery.publication_terms.disappear_message == branch.disappear_message &&
	     c.native_recovery.publication_terms.echo_all == branch.echo_all);
	quest_reward_continuation terms;
	need(quest_reward_continuation_decode(c.continuation.data.data(),
					      c.continuation.data.size(), &terms) &&
	     terms.version == 5 && terms.player_pid == c.native_recovery.player_pid &&
	     terms.mobile_vnum == 16006 && terms.room_vnum == 16077 &&
	     terms.completion_index == pc.next_branch &&
	     terms.definition_id == branch.definition_id && terms.reward_count == 2 &&
	     terms.root_count == 3 && terms.rewards[0].type == 1 &&
	     terms.rewards[0].number == 16075 && terms.rewards[1].type == 1 &&
	     terms.rewards[1].number == 16015 && terms.xp_award_count == 0 &&
	     c.native_recovery.consumed_root_order.size() == terms.root_count &&
	     std::equal(c.native_recovery.consumed_root_order.begin(),
			c.native_recovery.consumed_root_order.end(), terms.roots.begin()));
	const std::array<int32_t, 3> kinds{ 16080, 16014, 16013 };
	for (size_t index = 0; index < kinds.size(); ++index)
	{
		const auto end = c.items.begin() + c.item_count;
		const auto found = std::find_if(c.items.begin(), end,
						[&](const item_transfer_entry &entry)
						{ return entry.item_uid == terms.roots[index]; });
		need(found != end && !found->parent_item_uid &&
		     found->root_item_uid == found->item_uid && found->vnum == kinds[index]);
	}
	const bool checked = child.envelope.phase ==
			     critical_native_recovery_phase::continuation_pending;
	const bool valid = checked && native_quest_recovery_pair_context_valid(
					      parent.envelope, child.envelope, nullptr);
	// Phase1 is a held value cut, not a failed phase2 transition. Phase2 must
	// meet the maintained structural validator, still without owner authority.
	need(!checked || valid);
	std::ostringstream out;
	out << "{\"scope\":\"original-pair value correlation only\",\"external_proof_required\":["
	       "\"authentic owner export, binary/candidate binding and actual guarded owner execution\","
	       "\"real quest-D authority, full physical retirement, literal custody/world census and chronology\","
	       "\"original save-hold/publication and SQL/ACK/session/journal observations; caller values are not capabilities\"],\"pair_checked\":"
	    << (checked ? "true" : "false") << ",\"pair_valid\":" << (valid ? "true" : "false")
	    << ",\"continuation_sha256\":\"" << digest(c.continuation.data)
	    << "\",\"required_xp_mask\":0,\"required_economic_mask\":3,\"parent\":";
	summary(out, parent);
	out << ",\"child\":";
	summary(out, child);
	out << '}';
	need(out.str().size() < output_limit);
	return out.str();
}
} // namespace

int main(int argc, char **argv)
{
	try
	{
		need(argc == 9);
		auto parent = load(argv + 1), child = load(argv + 5);
		const auto output = inspect(*parent, *child);
		std::cout << output << '\n'; // No partial output on any prior refusal.
		return std::cout ? 0 : 3;
	}
	catch (...)
	{
		std::cerr << "Original QP03 pair values refused; no owner authority inferred.\n";
		return 2;
	}
}
