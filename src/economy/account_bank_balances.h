#ifndef DURIS_ACCOUNT_BANK_BALANCES_H_INCLUDED
#define DURIS_ACCOUNT_BANK_BALANCES_H_INCLUDED

#include <cstdint>

struct AccountBankBalances
{
	int copper;
	int silver;
	int gold;
	int platinum;

	constexpr int64_t total_copper() const noexcept
	{
		return static_cast<int64_t>(copper) * 1 + static_cast<int64_t>(silver) * 10 +
		       static_cast<int64_t>(gold) * 100 + static_cast<int64_t>(platinum) * 1000;
	}

	constexpr bool is_valid() const noexcept
	{
		return copper >= 0 && silver >= 0 && gold >= 0 && platinum >= 0;
	}

	constexpr bool operator==(const AccountBankBalances &) const noexcept = default;
};

#endif // DURIS_ACCOUNT_BANK_BALANCES_H_INCLUDED
