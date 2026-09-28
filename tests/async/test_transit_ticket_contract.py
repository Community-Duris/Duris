#!/usr/bin/env python3
"""Contract and behavior tests for atomic transit ticket purchase cost sinks."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class TransitTicketContract(unittest.TestCase):
    def test_ferry_ticket_purchase_atomic_compound_ordering(self):
        source = (ROOT / "src/world/ferryact.c").read_text(encoding="utf-8")
        proc_idx = source.find("int ferry_automat_proc(P_obj obj, P_char ch, int cmd, char *arg)")
        self.assertGreater(proc_idx, 0, "ferry_automat_proc should be present")

        proc_body = source[proc_idx:proc_idx + 4000]

        # Verify money debit precedes ticket creation
        sub_money = proc_body.find("SUB_MONEY(ch, ticket_cost, 0)")
        read_obj = proc_body.find("read_object(FERRY_TICKET_VNUM, VIRTUAL)")
        obj_to_char = proc_body.find("obj_to_char(ticket, ch)")

        self.assertGreater(sub_money, 0, "SUB_MONEY must be present in buy block")
        self.assertGreater(read_obj, sub_money, "read_object must follow successful money payment")
        self.assertGreater(obj_to_char, read_obj, "delivery must follow ticket creation")

        # Verify rollback refund if ticket object creation fails
        self.assertIn("ADD_MONEY(ch, ticket_cost);", proc_body, "must refund money on ticket creation failure")

    def test_flight_path_ticket_purchase_atomic_compound_ordering(self):
        source = (ROOT / "src/world/transport.c").read_text(encoding="utf-8")
        buy_idx = source.find("bool flying_transport_cmd_buy(P_char ch, P_char victim, char *arg)")
        self.assertGreater(buy_idx, 0, "flying_transport_cmd_buy should be present")

        buy_body = source[buy_idx:buy_idx + 3500]

        # Verify money debit precedes ticket creation
        sub_money = buy_body.find("SUB_MONEY(ch, cost, 0)")
        read_obj = buy_body.find("read_object(VNUM_OBJ_FLIGHT_PATH_TICKET, VIRTUAL)")
        obj_to_char = buy_body.find("obj_to_char(ticket, ch)")

        self.assertGreater(sub_money, 0, "SUB_MONEY must be present in buy block")
        self.assertGreater(read_obj, sub_money, "read_object must follow successful money payment")
        self.assertGreater(obj_to_char, read_obj, "delivery must follow ticket creation")

        # Verify rollback refund if ticket object creation fails
        self.assertIn("ADD_MONEY(ch, cost);", buy_body, "must refund money on ticket creation failure")


if __name__ == "__main__":
    unittest.main()
