"""Actual do_split UI / paired SQL owner controls, authored and UNRUN.

Root calls run(args, shared, starter) only after authenticating the unchanged
maintained Case, chargen/starter helpers, current server and observer, and this
module in its fresh input binding. args is the existing Case argument object.
This module starts no container or compiler. The Case owns each fresh schema,
runtime and server, and stops its game before dropping its schema.

The optional fault is commissioned OFFLINE on that fresh schema. A BEFORE
INSERT trigger delays a genuine schema-1 coin root, then advances the sender's
real SQL wallet revision before the real endpoint comparison. It inserts no
receipt, completion, ledger, command or synthetic identity. The unchanged SQL
repository produces ESTALE, rolls back both endpoints, and persists its own
rejection. No post-bootstrap SQL mutation or fake provider/callback is used.
"""
import errno
import re
import time


NAMES = ("Splitsender", "Splitone", "Splittwo")
ACCOUNTS = ("Splitsender", "Splitone", "Splittwo")
DENOMINATIONS = ("copper", "silver", "gold", "platinum")
# Actual pinned critical_command_type::coin_transfer and currency_reason_type.
COIN_COMMAND = 17
COIN_REASON = 16


def case_class(shared, starter):
    journey = shared.journey

    class SplitCommitOrderCase(shared.Case):
        def __init__(self, args, reject_after):
            label = "normal" if reject_after is None else "reject-after-" + str(reject_after)
            super().__init__(args, "split-order-" + label)
            self.reject_after = reject_after
            self.clients = []
            self.pids = []

        def bootstrap(self):
            super().bootstrap()
            self.record("genuine_starter_template",
                        facts=starter.add_selected_starter_template(self.runtime, self.args.source_root))

        def observe(self, label):
            native = super().observe(label)
            assert native["admission"]["pointer"] == 0
            assert native["mode"]["selected_integer"] == 1
            assert native["outcome_core"]["status"] == "observed_real_typed_off_mode_journal_only"
            return native

        def scalar(self, query):
            result = self.sql(query)
            assert re.fullmatch(r"[0-9]+", result), result
            return int(result)

        def command_as(self, index, line, expected, timeout=30):
            client = self.clients[index]
            client.send(line)
            return client.expect(expected, timeout)

        def close_all(self, polite):
            errors = []
            for index, client in enumerate(self.clients):
                try:
                    if polite:
                        client.send("quit")
                        client.expect("ACCOUNT MENU", 30)
                        client.send("0")
                except BaseException as error:
                    errors.append(error)
                finally:
                    try:
                        (self.evidence / ("client-" + NAMES[index] + "-" + str(self.sequence) + ".transcript")) \
                            .write_bytes(bytes(client.transcript))
                    except BaseException as error:
                        errors.append(error)
                    finally:
                        try:
                            client.close()
                        except BaseException as error:
                            errors.append(error)
            self.clients = []
            self.client = None
            if errors:
                raise errors[0]

        def setup_players(self):
            self.bootstrap()
            self.boot("genuine-chargen")
            for index, (name, account) in enumerate(zip(NAMES, ACCOUNTS)):
                client = journey.MudClient(self.port, source_host="127.0.0." + str(index + 2))
                self.clients.append(client)
                journey.create_character(client, class_name="w", account=account,
                                         character=name, email=account + "@example.invalid")
                client.send("save")
                client.expect("Save complete for " + name + ".", 30)
                pid = self.scalar("SELECT pid FROM player_data WHERE name='" + name + "'")
                assert pid > 0 and pid not in self.pids
                self.pids.append(pid)
            self.pid = self.pids[0]
            self.close_all(polite=True)
            self.stop()
            assert self.process is None and not self.clients
            assert re.fullmatch(r"consumables_off_test_[0-9a-f]{12}", self.database)
            for index, pid in enumerate(self.pids):
                cash = 8 if index == 0 else 1
                assert self.scalar(f"SELECT COUNT(*) FROM currency_ledger WHERE pid={pid}") == 0
                assert self.sql(f"SELECT account_name FROM player_data WHERE pid={pid}") == ACCOUNTS[index]
                # Declared starting inputs, before any test transfer and while stopped.
                self.sql(f"UPDATE player_data SET copper={cash},silver={cash},gold={cash},"
                         f"platinum={cash},wallet_revision=wallet_revision+1 WHERE pid={pid}", bootstrap=True)
                racewar = self.scalar(f"SELECT racewar FROM player_data WHERE pid={pid}")
                self.sql("INSERT IGNORE INTO account_banks(account_name,racewar) "
                         f"VALUES('{ACCOUNTS[index]}',{racewar})", bootstrap=True)
                assert self.sql("SELECT bank_copper,bank_silver,bank_gold,bank_platinum "
                                f"FROM account_banks WHERE account_name='{ACCOUNTS[index]}' AND racewar={racewar}") \
                    == "0\t0\t0\t0"
                # Commission the declared offline wallet and bank opening snapshots.
                self.sql(
                    "UPDATE currency_wallet_baseline AS baseline "
                    "JOIN player_data AS player ON player.pid=baseline.pid "
                    "SET baseline.opening_copper=player.copper,"
                    "baseline.opening_silver=player.silver,"
                    "baseline.opening_gold=player.gold,"
                    "baseline.opening_platinum=player.platinum,"
                    "baseline.opening_revision=player.wallet_revision "
                    f"WHERE player.pid={pid}", bootstrap=True)
                assert self.scalar(
                    f"SELECT COUNT(*) FROM currency_wallet_baseline WHERE pid={pid}") == 1
                assert (
                    self.sql("SELECT opening_copper,opening_silver,opening_gold,opening_platinum,"
                             f"opening_revision FROM currency_wallet_baseline WHERE pid={pid}")
                    == self.sql(f"SELECT copper,silver,gold,platinum,wallet_revision "
                                f"FROM player_data WHERE pid={pid}"))
                self.sql(
                    "INSERT IGNORE INTO currency_bank_baseline(bank_id,opening_copper,"
                    "opening_silver,opening_gold,opening_platinum,opening_revision) "
                    "SELECT id,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision "
                    "FROM account_banks "
                    f"WHERE account_name='{ACCOUNTS[index]}' AND racewar={racewar}",
                    bootstrap=True)
                assert self.scalar(
                    "SELECT COUNT(*) FROM account_banks AS bank "
                    "JOIN currency_bank_baseline AS baseline ON baseline.bank_id=bank.id "
                    f"WHERE bank.account_name='{ACCOUNTS[index]}' AND bank.racewar={racewar}") == 1
                assert (
                    self.sql("SELECT baseline.opening_copper,baseline.opening_silver,"
                             "baseline.opening_gold,baseline.opening_platinum,"
                             "baseline.opening_revision FROM account_banks AS bank "
                             "JOIN currency_bank_baseline AS baseline ON baseline.bank_id=bank.id "
                             f"WHERE bank.account_name='{ACCOUNTS[index]}' AND bank.racewar={racewar}")
                    == self.sql("SELECT bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision "
                                "FROM account_banks "
                                f"WHERE account_name='{ACCOUNTS[index]}' AND racewar={racewar}"))
            self.commission_fault()
            self.boot("split-controls")
            for name, account in zip(NAMES, ACCOUNTS):
                self.clients.append(journey.reconnect_character(self.port, account=account, character=name))
            native = self.observe("genuine-nonstaff-participants")
            for pid in self.pids:
                matches = [row for row in native["characters"] if not row["npc"] and row["pid"] == pid]
                assert len(matches) == 1 and matches[0]["level"] == 1 and matches[0]["runtime_id"] > 0
            for index in (1, 2):
                self.command_as(index, "consent " + NAMES[0], "You attempt to give consent")
                self.command_as(0, "group " + NAMES[index], NAMES[index] + " is now a member of your group.")
            self.command_as(0, "group", "Your group consists of ( 3/")
            self.record("actual_group_ready", names=NAMES, pids=self.pids,
                        accounts=ACCOUNTS, reject_after=self.reject_after)

        def commission_fault(self):
            assert self.process is None and self.client is None and not self.clients
            condition = "FALSE" if self.reject_after is None else (
                f"(SELECT COUNT(*) FROM currency_ledger WHERE pid={self.pid} "
                f"AND reason_type={COIN_REASON}) >= {self.reject_after}")
            # The root inbox insert precedes SAVEPOINT coin_endpoints. Its real
            # revision advance survives that savepoint's rollback, so the emitted
            # stale authority still matches committed SQL after rejection.
            trigger = f"""CREATE TRIGGER split_order_revision_race BEFORE INSERT ON critical_operation_inbox
FOR EACH ROW BEGIN
    IF NEW.command_type={COIN_COMMAND} AND NEW.schema_version=1 THEN
        DO SLEEP(6);
        IF {condition} THEN
            UPDATE player_data SET wallet_revision=wallet_revision+1 WHERE pid={self.pid};
        END IF;
    END IF;
END"""
            self.sql("DELIMITER //\n" + trigger + "//\nDELIMITER ;\n", bootstrap=True)
            self.record("offline_sql_contention_fixture", trigger=trigger,
                        receipts_or_outcomes_inserted=False)

        def wallets(self):
            ids = ",".join(map(str, self.pids))
            rows = self.sql("SELECT pid,copper,silver,gold,platinum,wallet_revision "
                            f"FROM player_data WHERE pid IN ({ids}) ORDER BY pid")
            result = {int(parts[0]): list(map(int, parts[1:]))
                      for line in rows.splitlines() for parts in [line.split("\t")]}
            assert set(result) == set(self.pids) and all(len(row) == 5 for row in result.values())
            return result

        def banks(self):
            ids = ",".join(map(str, self.pids))
            return self.sql("SELECT p.pid,b.bank_copper,b.bank_silver,b.bank_gold,b.bank_platinum "
                            "FROM player_data p JOIN account_banks b ON b.account_name=p.account_name "
                            f"AND b.racewar=p.racewar WHERE p.pid IN ({ids}) ORDER BY p.pid")

        def roots(self):
            text = self.sql("SELECT HEX(operation_id),schema_version,status,result_code "
                            f"FROM critical_operation_inbox WHERE command_type={COIN_COMMAND} ORDER BY operation_id")
            return {parts[0]: tuple(map(int, parts[1:]))
                    for line in text.splitlines() for parts in [line.split("\t")]}

        def ledger(self):
            ids = ",".join(map(str, self.pids))
            fields = ",".join("wallet_delta_" + name for name in DENOMINATIONS)
            banks = ",".join("bank_delta_" + name for name in DENOMINATIONS)
            text = self.sql(f"SELECT HEX(operation_id),pid,{fields},{banks} FROM currency_ledger "
                            f"WHERE pid IN ({ids}) AND reason_type={COIN_REASON} ORDER BY operation_id")
            return {parts[0]: tuple(map(int, parts[1:]))
                    for line in text.splitlines() for parts in [line.split("\t")]}

        def drain(self):
            for client in self.clients:
                client._receive()

        def no_early_success(self, offsets):
            for index, client in enumerate(self.clients):
                text = bytes(client.transcript[offsets[index]:]).decode(errors="replace")
                assert "splits some money" not in text and "You give " not in text
                assert "gives you your share:" not in text and "You receive " not in text

        def split(self, denomination):
            self.command_as(0, "look", "The Regression Arena")
            self.drain()
            before = self.wallets()
            banks_before = self.banks()
            roots_before, ledger_before = self.roots(), self.ledger()
            offsets = [len(client.transcript) for client in self.clients]
            self.clients[0].send("split 7 " + DENOMINATIONS[denomination])
            # A later independent input proves the actual command was dispatched.
            self.command_as(0, "look", "The Regression Arena")
            deadline = time.monotonic() + 3
            while not self.scalar("SELECT COUNT(*) FROM information_schema.PROCESSLIST "
                                  "WHERE DB=DATABASE() AND STATE='User sleep'"):
                assert time.monotonic() < deadline, "real delayed SQL root was not reached"
                self.drain()
            self.drain()
            assert self.wallets() == before, "wallet changed before the first real SQL commit"
            assert self.ledger() == ledger_before and self.banks() == banks_before
            self.no_early_success(offsets)
            completed = 2 if self.reject_after is None else self.reject_after
            keep = 7 - 2 * completed
            self.clients[0].expect(f"You keep {keep} {DENOMINATIONS[denomination]} coins for yourself.", 35)
            self.drain()
            after = self.wallets()
            expected = {pid: list(row) for pid, row in before.items()}
            expected[self.pid][denomination] -= 2 * completed
            expected[self.pid][4] += completed + (self.reject_after is not None)
            for index in range(completed):
                expected[self.pids[index + 1]][denomination] += 2
                expected[self.pids[index + 1]][4] += 1
            assert after == expected, (before, after, expected)
            assert self.banks() == banks_before, "split changed bank funds"
            for column in range(4):
                assert sum(row[column] for row in after.values()) == sum(row[column] for row in before.values())
            roots_after, ledger_after = self.roots(), self.ledger()
            new_roots = {key: row for key, row in roots_after.items() if key not in roots_before}
            new_ledger = {key: row for key, row in ledger_after.items() if key not in ledger_before}
            assert len(new_roots) == completed + (self.reject_after is not None)
            assert sum(row == (1, 1, 0) for row in new_roots.values()) == completed
            if self.reject_after is not None:
                assert sum(row == (1, 1, errno.ESTALE) for row in new_roots.values()) == 1
            assert len(new_ledger) == 2 * completed
            for column in range(1, 5):
                assert sum(row[column] for row in new_ledger.values()) == 0
            assert all(row[5:] == (0, 0, 0, 0) for row in new_ledger.values())
            owned_children = set()
            for operation, result in new_roots.items():
                assert re.fullmatch(r"[0-9A-F]{32}", operation) and operation != "0" * 32
                if result[2]:
                    continue
                receipt = self.sql("SELECT HEX(payload) FROM critical_outbox "
                                   f"WHERE operation_id=UNHEX('{operation}') AND event_index=0")
                assert re.fullmatch(r"[0-9A-F]{64}", receipt), receipt
                source_id, destination_id = receipt[:32], receipt[32:]
                assert source_id in new_ledger and destination_id in new_ledger
                assert source_id not in owned_children and destination_id not in owned_children
                owned_children.update((source_id, destination_id))
                source, destination = new_ledger[source_id], new_ledger[destination_id]
                debit, credit = [0] * 4, [0] * 4
                debit[denomination], credit[denomination] = -2, 2
                assert source[0] == self.pid and source[1:5] == tuple(debit)
                assert destination[0] in self.pids[1:completed + 1] and destination[1:5] == tuple(credit)
            assert owned_children == set(new_ledger)
            text = bytes(self.clients[0].transcript[offsets[0]:]).decode(errors="replace")
            assert text.count("You give ") == completed
            assert text.count("The split stopped; completed shares remain transferred.") == (self.reject_after is not None)
            for index in (1, 2):
                text = bytes(self.clients[index].transcript[offsets[index]:]).decode(errors="replace")
                assert text.count("gives you your share:  2 " + DENOMINATIONS[denomination] + " coins.") == (index <= completed)
            self.record("genuine_split_commit_conservation", denomination=DENOMINATIONS[denomination],
                        before=before, after=after, roots=new_roots, endpoint_ledgers=new_ledger,
                        completed_shares=completed, keep=keep)

        def run(self):
            self.setup_players()
            columns = range(4) if self.reject_after is None else (0,)
            for column in columns:
                self.split(column)
            self.close_all(polite=True)
            self.stop()
            durable = self.wallets()
            self.boot("saved-and-restarted")
            for name, account in zip(NAMES, ACCOUNTS):
                self.clients.append(journey.reconnect_character(self.port, account=account, character=name))
            assert self.wallets() == durable
            self.observe("saved-and-restarted")
            self.close_all(polite=True)

        def cleanup(self):
            try:
                self.close_all(polite=False)
            finally:
                super().cleanup()

    return SplitCommitOrderCase


def run(args, shared, starter):
    """Called only by Root's authenticated existing Case/observer input recipe."""
    results = []
    selected = case_class(shared, starter)
    for reject_after in (None, 0, 1):
        case = selected(args, reject_after)
        receipt = {"status": "RUNNING", "reject_after": reject_after}
        try:
            case.run()
            receipt["status"] = "PASS"
        except BaseException as error:
            receipt.update(status="FAIL", error_type=type(error).__name__, error=str(error))
            raise
        finally:
            try:
                case.cleanup()
            except BaseException as error:
                receipt.update(status="FAIL", cleanup_error=type(error).__name__ + ": " + str(error))
                raise
            finally:
                receipt["game_released"] = case.process is None or case.process.poll() is not None
                receipt["schema_released"] = not case.created
                case.record("split_order_result", **receipt)
        assert receipt["game_released"] and receipt["schema_released"]
        results.append(receipt)
    return results
