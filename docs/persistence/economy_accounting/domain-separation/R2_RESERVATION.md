# R2 wallet-value and bank-payment reservation — 2026-10-07

Required inventory row C3/P1; code edits have not begun at this publication.
Current owned revision `cfa726957`, production preimage inherited unchanged from
`7dbc472123a729f2fedc345e5309a586ba8a02d8`. Fresh accounting upstream
`d91f59af06239a5736d10498b89091699c5e05c6` has the same currency source;
intervening coordinator updates are documentation. No owner reservation or
native publication work is transferred by this narrow calculation extraction.

## Exact preimage and interface

Production file `src/economy/currency_transaction.c` SHA-256: `4e505f698b8c266bac8b16f31e31a75e059e93a43fe1b40dbf9d23884f6152c7`.
Only the three helper bodies below and one local header include are reserved.
All callers, including identified reward, bank reward, wallet value submission,
identify preparation and bank payment submission, retain existing authority.

- `currency_vector canonical_value(int64_t value)` SHA-256 `20c9630cca28a71b355ebfd27914592bbe8a2f273b9651ace26bb58cbd628ada`.
- `bool wallet_value_delta(P_char character, int64_t value_delta, currency_vector *delta)` SHA-256 `a4b38e2c1e49cdee55edde728391d480b63115159c3c1582c0ed5939fadcf741`.
- `static bool bank_payment_deltas(P_char character, int64_t value, currency_vector *wallet,` SHA-256 `4ed222cf1edbb411d81154c3d9ebde695ee493b71640134d7db6fcb959411c15`.

New local `src/economy/currency_value_plan.h` will take
`std::array<int, CURRENCY_DENOMINATION_COUNT>` owned native denomination counts,
`int64_t` value, and return existing `currency_vector` outputs. The array matches
`points.cash[]` and bank `spare1..4` native int fields; it cannot admit an arbitrary
64-bit current balance. Header static bounds will make the existing 32-bit int
range assumption explicit. Summing four native counts times 1/10/100/1000 fits
int64_t even for signed native extremes; old upper-overflow guards are unreachable
for all defined native inputs. No wider input domain or saturation is introduced.
Do not add balance validity policy here: actual mutation authority already rejects
negative or >INT_MAX balances (`currency_prepare_mutation` returns EILSEQ).
Preserve defined helper arithmetic even for signed native counts; avoid recreating
the former `INT64_MAX - negative_total` undefined expression.

Canonical decomposition moves once into the owned header; the existing local
name delegates to it for unchanged reward callers. Wallet wrapper retains its
positive reward branch without character/balance validation, and captures only
negative-spend native balances after the existing actor checks. Zero and INT64_MIN
refuse without output changes. Negative spends canonicalize the remaining total
and subtract original denomination counts. Bank wrapper retains actor/positive
value checks, captures the four bank counts and delegates. Bank payment consumes
ascending denominations, rounds required coins upward and returns overpayment
as canonical wallet change. Only successful evaluation writes output vectors.
No mutation application or accounting-adapter formula is copied.

## Proof and scope

Add a focused executable sanitizer regression compiling actual extracted helper
bodies/wrappers with native-shaped fixture endpoints, and direct owned header
cases. Keep a retained original-source run using the same expectations. Cases:
positive reward without an actor; zero/INT64_MIN; absent/NPC spend; exact change;
noncanonical wallet input; insufficient funds; bank ascending order/overpayment;
INT_MAX denomination counts and INT64_MAX value; repeated owned evaluation;
unchanged sentinel outputs on refusal; native signed cases with defined original
arithmetic. Do not broaden accepted production state. Production-linked
`test_currency_completion_retention.py` already covers actual wallet reward/value,
bank payment and identify preparation, pending/replay/active refusal/publication;
run it unchanged for SQL and flat sanitizer scenarios. Run existing currency
contract, economic currency adapter and locker identify tests, maintained SQL/flat
builds, plus relevant real payment journey if available. Keep failures visible;
private link-provider correction does not qualify a shared manifest.

No submission, identity, revision, reason/site/deadline, active exclusions,
capability, admission, ACK/publication, SQL/native execution, receipt, recovery,
coin transfer, schema, configuration or paid outcome code changes are reserved.
R1 runtime qualification proceeds independently with the already built R1
binaries. Exact original helper bodies follow, to make this reservation reviewable.

```cpp
currency_vector canonical_value(int64_t value)
{
	currency_vector result = {};
	static constexpr std::array<int64_t, CURRENCY_DENOMINATION_COUNT> values = { 1, 10, 100,
										     1000 };
	for (size_t index = values.size(); index-- > 0;)
	{
		result.amount[index] = value / values[index];
		value %= values[index];
	}
	return result;
}
```

```cpp
bool wallet_value_delta(P_char character, int64_t value_delta, currency_vector *delta)
{
	if (!value_delta || value_delta == INT64_MIN)
		return false;
	currency_vector wallet_delta = {};
	if (value_delta > 0)
		wallet_delta = canonical_value(value_delta);
	else
	{
		if (!character || IS_NPC(character))
			return false;
		const std::array<int64_t, CURRENCY_DENOMINATION_COUNT> current = {
			GET_COPPER(character), GET_SILVER(character), GET_GOLD(character),
			GET_PLATINUM(character)
		};
		static constexpr std::array<int64_t, CURRENCY_DENOMINATION_COUNT> values = { 1, 10,
											     100,
											     1000 };
		int64_t total = 0;
		for (size_t index = 0; index < current.size(); ++index)
		{
			if (current[index] > (INT64_MAX - total) / values[index])
				return false;
			total += current[index] * values[index];
		}
		const int64_t spend = -value_delta;
		if (total < spend)
			return false;
		const currency_vector after = canonical_value(total - spend);
		for (size_t index = 0; index < current.size(); ++index)
			wallet_delta.amount[index] = after.amount[index] - current[index];
	}
	*delta = wallet_delta;
	return true;
}
```

```cpp
static bool bank_payment_deltas(P_char character, int64_t value, currency_vector *wallet,
				currency_vector *bank)
{
	if (!character || IS_NPC(character) || value <= 0)
		return false;
	const std::array<int64_t, CURRENCY_DENOMINATION_COUNT> current = {
		GET_BALANCE_COPPER(character), GET_BALANCE_SILVER(character),
		GET_BALANCE_GOLD(character), GET_BALANCE_PLATINUM(character)
	};
	static constexpr std::array<int64_t, CURRENCY_DENOMINATION_COUNT> values = { 1, 10, 100,
										     1000 };
	int64_t total = 0;
	for (size_t index = 0; index < current.size(); ++index)
	{
		if (current[index] > (INT64_MAX - total) / values[index])
			return false;
		total += current[index] * values[index];
	}
	if (total < value)
		return false;
	currency_vector bank_delta = {};
	int64_t remaining = value;
	for (size_t index = 0; index < current.size() && remaining > 0; ++index)
	{
		const int64_t needed = (remaining + values[index] - 1) / values[index];
		const int64_t used = std::min(current[index], needed);
		bank_delta.amount[index] = -used;
		remaining -= used * values[index];
	}
	currency_vector wallet_delta = {};
	if (remaining < 0)
		wallet_delta = canonical_value(-remaining);
	*wallet = wallet_delta;
	*bank = bank_delta;
	return true;
}
```
