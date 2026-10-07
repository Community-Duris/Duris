#!/usr/bin/env python3
"""Genuine bid producer versus actual retained seller fee predicate.

Extract the current verifier fee subsection at execution. No MySQL/runtime/world
qualification: original two-engine component supplies native SQL proof coverage.
The original bid test suite runs first, preserving positive-fee and bad-source
bindings. This focused seam adds zero fee and malformed seller payout controls.
"""
import argparse
import ast
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[2])
    args = parser.parse_args()
    root = args.source_root.resolve()
    source = (root/'src/persistence/economic_sql_auction_claim_endpoint.c').read_text()
    owner = source.index('unsigned int economic_sql_pending_claim_endpoint_verify_retained_creator(')
    begin = source.index('\t\t\tconst auto sink = std::find_if(', owner)
    end = source.index('\n\t\t}\n\t\tif (!row(connection,', begin)
    literal = source[begin:end]
    assert literal.count('ECONOMIC_AUCTION_CLOSING_FEE_SINK_ID') == 1
    assert 'fee + amount != static_cast<uint64_t>(result.final_price)' in literal
    harness = r"""
#define main original_bid_accounting_main
#include "tests/async/auction_bid_accounting_test.cpp"
#undef main
#include <algorithm>
#include <cerrno>
#include <cstdio>
unsigned int actual_retained_fee(const economic_accounting_plan &plan,
                                 const economic_account_key &key,
                                 const auction_command_result &result)
{
    const auto effect = std::find_if(plan.accounts.begin(), plan.accounts.end(),
        [&](const auto &value) { return economic_account_key_equal(value.key, key); });
    assert(effect != plan.accounts.end());
    const auto amount = static_cast<uint64_t>(effect->after[0]);
@@ACTUAL_FEE_PREDICATE@@
    return 0;
}
int main()
{
    assert(original_bid_accounting_main() == 0);
    for (const uint16_t fee : { 300, 0 })
    {
        auto payload = bid_payload(20, 5000, 5);
        payload.closing_fee_basis_points = fee;
        const auto prior = listing(20, 3000, 2);
        const auto keys = accounts(20, false, true);
        economic_frozen_intent intent;
        const auto command = accepted(payload, 6, prior, keys, &intent);
        auto before = authority(prior, keys, 5);
        before.seller_claim_before = {};
        const auto result = result_for(payload, prior, 5);
        economic_accounting_plan plan;
        assert(auction_bid_accounting_plan(command, intent, before, result, &plan) ==
               economic_accounting_error::ok);
        assert(actual_retained_fee(plan, keys.seller_claim, result) == 0);
        auto damaged = plan;
        auto credit = std::find_if(damaged.accounts.begin(), damaged.accounts.end(),
            [&](const auto &value) { return economic_account_key_equal(value.key, keys.seller_claim); });
        assert(credit != damaged.accounts.end());
        const auto index = static_cast<uint16_t>(credit - damaged.accounts.begin());
        --credit->after[0];
        for (auto &posting : damaged.postings)
            if (posting.account_index == index) { --posting.copper; --posting.delta[0]; }
        assert(actual_retained_fee(damaged, keys.seller_claim, result) == EILSEQ);
        damaged = plan;
        for (auto &posting : damaged.postings)
            if (posting.account_index == index) posting.copper = -1;
        assert(actual_retained_fee(damaged, keys.seller_claim, result) == EILSEQ);
        if (fee)
        {
            damaged = plan;
            auto sink = std::find_if(damaged.accounts.begin(), damaged.accounts.end(),
                [](const auto &value) { return value.key.kind == economic_account_kind::sink; });
            assert(sink != damaged.accounts.end());
            sink->key.authority_id++;
            assert(actual_retained_fee(damaged, keys.seller_claim, result) == EILSEQ);
        }
        std::printf("PASS_ACTUAL_RETAINED_FEE fee_bps=%u exact_producer=1 bad_credit_refused=1 negative_credit_refused=1\n", fee);
        std::fflush(stdout);
    }
    std::puts("PASS_ORIGINAL_POSITIVE_FEE_AND_INVALID_SOURCE_BINDINGS retained_fee_scope_only=1 native_sql_qualification=0");
}
"""
    harness = harness.replace('@@ACTUAL_FEE_PREDICATE@@', literal)
    native_source = (root/'src/economy/auction_native_publication.c').read_text()
    native_begin = native_source.index('namespace\n{\nbool selected_ranges(')
    native_end = native_source.index('\n#include \"core/prototypes.h\"', native_begin)
    native_literal = native_source[native_begin:native_end]
    assert native_literal.count('bool auction_native_expected_player_forest(') == 1
    native_literal = '#include \"economy/auction_native_command_context.h\"\n#include <algorithm>\n#include <set>\n#include <utility>\n' + native_literal
    tree = ast.parse((root/'tests/async/test_auction_bid_accounting.py').read_text())
    call = next(n for n in ast.walk(tree) if isinstance(n, ast.Call)
                and isinstance(n.func, ast.Attribute) and n.func.attr == 'run'
                and n.args and isinstance(n.args[0], ast.List))
    command = []
    with tempfile.TemporaryDirectory(prefix='auction-retained-seller-fee-') as temp:
        temp = Path(temp);cpp = temp/'retained_fee.cpp';binary = temp/'retained_fee'
        cpp.write_text(harness)
        forest_cpp = temp/'actual_native_forest.cpp'
        forest_cpp.write_text(native_literal)
        for node in call.args[0].elts:
            if isinstance(node, ast.Constant):
                command.append(str(cpp) if node.value == 'tests/async/auction_bid_accounting_test.cpp' else node.value)
            elif isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute): command.append(os.environ.get('CXX', 'g++'))
            elif isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and node.func.id == 'str': command.append(str(binary))
            else: raise AssertionError(ast.dump(node))
        # Genuine native codec providers added since the older bid test list.
        # Preserve all original compile/sanitizer flags and link real project TUs.
        providers = [
            'src/economy/auction_native_command_context.c',
            'src/economy/native_quest_cost.c',
            'src/economy/native_quest_coin_give.c',
            'src/economy/shop_trade_recovery_manifest.c',
            'src/item/lockpick_retirement_continuation.c',
        ]
        command[command.index('-lcrypto'):command.index('-lcrypto')] = providers + [str(forest_cpp)]
        command.insert(command.index('-Isrc'), '-I'+str(root))
        subprocess.run(command, cwd=root, check=True, timeout=180)
        subprocess.run([str(binary)], cwd=root, check=True, timeout=30)

if __name__ == '__main__':
    main()
