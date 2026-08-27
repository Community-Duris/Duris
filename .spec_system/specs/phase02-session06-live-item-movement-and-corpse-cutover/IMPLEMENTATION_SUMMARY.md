# Implementation Summary

Live durable item custody now publishes only after the operation-keyed ownership
transaction commits. A pointer-free runtime registry and movement adapter cover player
get, drop, put, give/trade, corpse loot, death transfer, restore, reconnect, and floor
recovery. Newly created items are adopted explicitly, and failures preserve prior live
custody.

The transfer payload now supports authoritative same-owner attach/detach topology as
well as cross-owner subtree movement. Corpse identity combines stable player and save
generation IDs; Redis hints and legacy item events no longer decide ownership.

Project version: `1.81.35`
