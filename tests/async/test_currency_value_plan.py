#!/usr/bin/env python3
"""Exercise actual native value/payment wrappers and the owned numeric rules."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
from _paths import ROOT, source


def function(text, signature):
    start = text.index(signature)
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


HARNESS = r"""
#include "core/utils.h"
#include "economy/currency_value_plan.h"
#include <cassert>
#include <climits>
#include <cstdio>
#include <limits>
@WRAPPERS@
using counts = std::array<int, CURRENCY_DENOMINATION_COUNT>;
using amounts = std::array<int64_t, CURRENCY_DENOMINATION_COUNT>;
void balance(char_data &actor, const counts &wallet, const counts &bank) {
    for (size_t i=0; i<wallet.size(); ++i) actor.points.cash[i]=wallet[i];
    GET_BALANCE_COPPER(&actor)=bank[0]; GET_BALANCE_SILVER(&actor)=bank[1];
    GET_BALANCE_GOLD(&actor)=bank[2]; GET_BALANCE_PLATINUM(&actor)=bank[3];
}
int main() {
    char_data actor{}; pc_only_data player{}; actor.only.pc=&player;
    const amounts sentinel={7,8,9,10};
    currency_vector wallet{sentinel}, bank{sentinel};
    assert(canonical_value(1234).amount==(amounts{4,3,2,1}));
    assert(wallet_value_delta(nullptr,1234,&wallet));
    assert(wallet.amount==(amounts{4,3,2,1}));
    assert(wallet_value_delta(nullptr,INT64_MAX,&wallet));
    assert(wallet.amount==(amounts{7,0,8,9223372036854775LL}));
    for (auto value : {int64_t{0},INT64_MIN,-int64_t{1}}) {
        wallet.amount=sentinel;
        assert(!wallet_value_delta(nullptr,value,&wallet) && wallet.amount==sentinel);
    }
    actor.specials.act=ACT_ISNPC;
    assert(wallet_value_delta(&actor,1,&wallet) && wallet.amount==(amounts{1,0,0,0}));
    wallet.amount=sentinel;
    assert(!wallet_value_delta(&actor,-1,&wallet) && wallet.amount==sentinel);
    assert(!bank_payment_deltas(&actor,1,&wallet,&bank));
    assert(wallet.amount==sentinel && bank.amount==sentinel);
    actor.specials.act=0;
    balance(actor,{15,2,1,0},{5,2,1,1});
    assert(wallet_value_delta(&actor,-36,&wallet));
    assert(wallet.amount==(amounts{-6,7,-1,0}));
    assert(wallet_value_delta(&actor,-135,&wallet));
    assert(wallet.amount==(amounts{-15,-2,-1,0}));
    wallet.amount=sentinel;
    assert(!wallet_value_delta(&actor,-136,&wallet) && wallet.amount==sentinel);
    assert(bank_payment_deltas(&actor,26,&wallet,&bank));
    assert(bank.amount==(amounts{-5,-2,-1,0}) && wallet.amount==(amounts{9,9,0,0}));
    assert(bank_payment_deltas(&actor,1125,&wallet,&bank));
    assert(bank.amount==(amounts{-5,-2,-1,-1}) && wallet.amount==(amounts{0,0,0,0}));
    for (auto value : {int64_t{0},-int64_t{1},INT64_MIN,INT64_MAX,int64_t{1126}}) {
        wallet.amount=bank.amount=sentinel;
        assert(!bank_payment_deltas(&actor,value,&wallet,&bank));
        assert(wallet.amount==sentinel && bank.amount==sentinel);
    }
    assert(!bank_payment_deltas(nullptr,1,&wallet,&bank));
    assert(wallet.amount==sentinel && bank.amount==sentinel);
    const counts maximum={INT_MAX,INT_MAX,INT_MAX,INT_MAX};
    const int64_t total=2385854331817LL;
    balance(actor,maximum,maximum);
    assert(wallet_value_delta(&actor,-total,&wallet));
    assert(wallet.amount==(amounts{-INT_MAX,-INT_MAX,-INT_MAX,-INT_MAX}));
    assert(bank_payment_deltas(&actor,total,&wallet,&bank));
    assert(bank.amount==(amounts{-INT_MAX,-INT_MAX,-INT_MAX,-INT_MAX}));
    assert(wallet.amount==(amounts{0,0,0,0}));
    // Signed native facts: original arithmetic is defined while running totals
    // stay nonnegative; balance validity remains the mutation owner's policy.
    balance(actor,{10,-1,2,0},{10,-1,2,0});
    assert(wallet_value_delta(&actor,-1,&wallet));
    assert(wallet.amount==(amounts{-1,10,-1,0}));
    assert(bank_payment_deltas(&actor,11,&wallet,&bank));
    assert(bank.amount==(amounts{-10,1,-1,0}) && wallet.amount==(amounts{9,8,0,0}));
    for (const counts current : {counts{15,2,1,0},maximum,counts{0,0,0,1}}) {
        balance(actor,current,current);
        for (int64_t spend : {int64_t{1},int64_t{26},int64_t{1000},INT64_MAX}) {
            currency_vector a{sentinel}, b{sentinel}, c{sentinel}, d{sentinel};
            const bool native_wallet=wallet_value_delta(&actor,-spend,&a);
            assert(currency_prepare_wallet_value_delta(current,-spend,&b)==native_wallet);
            assert(a.amount==b.amount);
            const bool native_bank=bank_payment_deltas(&actor,spend,&a,&c);
            assert(currency_prepare_bank_payment_deltas(current,spend,&b,&d)==native_bank);
            assert(a.amount==b.amount && c.amount==d.amount);
            currency_vector again{sentinel}, again_bank{sentinel};
            assert(currency_prepare_bank_payment_deltas(current,spend,&again,&again_bank)==native_bank);
            assert(again.amount==b.amount && again_bank.amount==d.amount);
        }
    }
    wallet.amount=bank.amount=sentinel;
    assert(!currency_prepare_wallet_value_delta({},0,&wallet));
    assert(!currency_prepare_wallet_value_delta({},INT64_MIN,&wallet));
    assert(!currency_prepare_wallet_value_delta({},1,nullptr));
    assert(!currency_prepare_bank_payment_deltas({},1,&wallet,&bank));
    assert(!currency_prepare_bank_payment_deltas(maximum,1,nullptr,&bank));
    assert(!currency_prepare_bank_payment_deltas(maximum,1,&wallet,nullptr));
    assert(wallet.amount==sentinel && bank.amount==sentinel);
    assert(currency_prepare_wallet_value_delta({},1234,&wallet));
    assert(wallet.amount==(amounts{4,3,2,1}));
    puts("Currency owned rules and native wrappers passed numeric/payment controls.");
}
"""


def main():
    path = Path(os.environ.get('DURIS_CURRENCY_VALUE_PREIMAGE', source('currency_transaction.c')))
    text = path.read_text(encoding='utf-8')
    wrappers = '\n'.join(function(text, signature) for signature in (
        'currency_vector canonical_value(', 'bool wallet_value_delta(',
        'static bool bank_payment_deltas('))
    artifacts = ROOT / 'bin/tests'
    artifacts.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='currency-value-', dir=artifacts) as directory:
        cpp = Path(directory) / 'value.cpp'
        binary = Path(directory) / 'value'
        cpp.write_text(HARNESS.replace('@WRAPPERS@', wrappers), encoding='utf-8')
        subprocess.run([*shlex.split(os.environ.get('CXX', 'g++')), '-std=c++20',
                        '-Wall', '-Wextra', '-Werror', '-g', '-O1',
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Isrc', str(cpp), '-o', str(binary)], cwd=ROOT, check=True,
                       timeout=120)
        subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=30)


if __name__ == '__main__':
    main()
