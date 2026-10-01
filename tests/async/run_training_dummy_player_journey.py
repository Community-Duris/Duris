#!/usr/bin/env python3
"""Exercise the real full-world training dummy through a new player's Telnet session.

Run against an isolated flat-file server with the complete world loaded. The
account and character names below must not be used against a shared server.
"""

import re
import random
import string
import sys
import time

from test_flatfile_combat_journey import MudClient, create_character, require


def command(client: MudClient, line: str, prompt: str = "Pos: standing >") -> str:
    client.send(line)
    return client.expect(prompt, timeout=20)


def main(port: int) -> None:
    client = MudClient(port)
    try:
        suffix = "".join(random.sample(string.ascii_lowercase, 6))
        create_character(client, expected_room=None, class_name="w",
                         account="Dum" + suffix, character="Dum" + suffix,
                         email="dummy-journey@example.invalid")
        client.expect("Pos: standing >")
        room = command(client, "look")
        require("training dummy" in room.lower(),
                f"spawn room has no training dummy:\n{room[-4000:]}")
        require("carved in the shape of" in room.lower(),
                f"room prose does not describe the dummy's race:\n{room[-4000:]}")

        description = command(client, "look dummy")
        require("effigy" in description.lower() and "tally rune" in description.lower(),
                "the dummy's detailed profile description is missing")

        report = command(client, "dummy report")
        require("Profile: level 56" in report and "Gear: mid" in report,
                "fixed dummy profile is not available to a player")
        require("Damage recorded:" in report, "dummy damage meter is unavailable")

        gift = command(client, "give sword dummy")
        require("refuses every item" in gift,
                f"dummy accepted a starter item:\n{gift[-2000:]}")

        follow = command(client, "follow dummy")
        require("cannot follow" in follow.lower(), "player can follow the dummy")

        client.send("dummy reset")
        client.expect("damage meter is reset", timeout=20)
        client.send("hit dummy")
        for _ in range(30):
            client.send("dummy report")
            report = client.expect("The dummy remains at full health", timeout=20)
            require("tranquility" not in report.lower(),
                    "safe room incorrectly rejected practice combat")
            require("No fighting permitted" not in report,
                    "safe room incorrectly blocked practice combat")
            match = re.search(r"Damage recorded: (\d+)", report)
            require(match is not None,
                    f"dummy damage report disappeared:\n{report[-2000:]}")
            if int(match.group(1)) > 0:
                break
            time.sleep(1)
        else:
            raise AssertionError("player landed no damage on the dummy after 30 attempts")

        require("The dummy remains at full health" in report,
                "dummy health/retaliation contract is not visible to players")
        client.send("dummy reset")
        reset = client.expect("damage meter is reset", timeout=20)
        require("damage meter is reset" in reset, "player could not reset the meter")
        client.send("dummy report")
        report = client.expect("The dummy remains at full health", timeout=20)
        require("Damage recorded: 0" in report, "reset did not clear the meter")

        print("full-world dummy player journey passed", flush=True)
        print("Spawn-room description:", room[-900:], flush=True)
        print("Detailed description:", description[-700:], flush=True)
        print("Profile:", report[-500:], flush=True)
    finally:
        client.close()


if __name__ == "__main__":
    main(int(sys.argv[1]))
