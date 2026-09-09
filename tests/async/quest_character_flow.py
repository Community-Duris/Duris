#!/usr/bin/env python3
"""Character-creation flow for the real world-quest journey."""

from test_flatfile_combat_journey import MudClient


def create_quest_character(client: MudClient) -> None:
    """Create a Chaos level-56 character while accepting optional Hardcore UI."""
    entry, _ = client.expect_any(("term type", "account name"))
    if entry == "term type":
        client.send("9")
        client.expect("account name")
    client.send("Journeyacct")
    client.expect("is this correct?")
    client.send("y")
    client.expect("email address")
    client.send("journey@example.invalid")
    client.expect("is this correct?")
    client.send("y")
    client.expect("enter your password")
    client.send("Qz7!mN4@")
    client.expect("verify your password")
    client.send("Qz7!mN4@")
    client.expect("information correct?")
    client.send("y")
    client.expect("PRESS RETURN")
    client.send("")
    client.expect("Please select an option")
    client.send("2")
    client.expect("Enter your new name")
    client.send("Taverek")
    client.expect("Is this correct?")
    client.send("y")
    client.expect("meet these criteria?")
    client.send("y")
    client.expect("Your selection")
    client.send("h")
    client.expect("Male or Female")
    client.send("m")
    next_prompt, _ = client.expect_any(("Hardcore", "Class Selection"))
    if next_prompt == "Hardcore":
        client.send("n")
        client.expect("Class Selection")
    client.send("w")
    client.expect("Alignment only affects")
    client.send("g")
    client.expect("Your selection")
    client.send("p")
    client.expect("Press return to continue")
    client.send("")
    for label in ("first bonus", "second bonus", "third bonus", "fourth bonus"):
        client.expect(label)
        client.send("s")
    client.expect("swap stats")
    client.send("n")
    client.expect("keep this character")
    client.send("y")
    client.expect("PRESS RETURN to read Duris rules")
    client.send("")
    client.expect("official and legal response")
    client.send("y")
    client.expect("PRESS RETURN")
    client.send("")
    client.expect_any((
        "Your starter kit is ready",
        "Your Chaos Equipment has been prepared!!",
        "Your CHAOS equipment kit could not be prepared safely",
    ), timeout=60)
