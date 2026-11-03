# moriatst builder and owner questions

These source-backed questions record decisions needed for the standalone
shop component. Recommendations do not imply that the file is loaded or that
shop gameplay was tested.

| ID | Builder or owner question | Recommended resolution |
| --- | --- | --- |
| MST-01 | Is `moriatst.shp` an intentional test copy, an alternate source, or an abandoned file? | Keep the exact source and ask its editor or owner to identify its purpose before any archive or path decision. |
| MST-02 | Should the full `areas/AREA` shop aggregation include only active `moria.shp`? | Preserve the current manifest behavior until the owner confirms the intended source set. |
| MST-03 | Is the runtime fallback meant to load every `areas/shp/*.shp` file when `areas/world.shp` is absent? | Resolve and document the fallback contract; its current glob includes this unlisted file. |
| MST-04 | Can any supported boot path reach shop loading without first generating `areas/world.shp`? | Inventory launch, recovery, and deployment paths before treating the candidate as inert or movable. |
| MST-05 | If fallback remains, should it apply the same `areas/AREA` filter as the normal compiler? | Decide the source-selection policy before a separate code change; do not infer the policy from the manifest alone. |
| MST-06 | Are duplicate keeper-and-room shop records for #99035/#99306, #99036/#99312, and #99037/#99318 intentional under fallback loading? | Preserve both files pending review; the current reset-side selector treats repeated matching records as ambiguous. |
| MST-07 | Are candidate buy/sell multipliers 0.90/1.10, 1.00/1.10, and 1.00/1.00 intentional when active Moria uses 0.80/1.10 for all three records? | Keep the raw values documented; the loader clamps candidate buy values to 0.80 but preserves #99037's sell value at 1.00 versus active Moria's 1.10. Ask the owner whether that effective difference is intentional. |
| MST-08 | Should the three stock lists remain exact copies of active Moria objects #99063–#99070? | Keep stock tied to the active object definitions unless an owner identifies a distinct product set. |
| MST-09 | Are the three keeper mobiles #99035–#99037 and in-room assignments #99306/#99312/#99318 meant to remain identical to Moria? | Preserve the direct references; active Moria ZON resets already place each keeper in the same room, and specs.assign.c binds the mobiles to Moria-specific procedures. Ask the Moria/shop owner to confirm the shared identity and placement. |
| MST-10 | Are the terse, coarse transaction prompts intentional for the Moria merchants? | Review tone with the content owner before any separate wording edit; this audit leaves the text unchanged. |
| MST-11 | Do tracked histories or editor/deployment records identify this file as a successor or predecessor to `moria.shp`? | Record only owner-backed lineage; same-stem history and matching content do not establish replacement. |
| MST-12 | Is `moriatst.shp` safe to archive or move while the wildcard fallback can include it? | Retain it in place until all launch modes, generated streams, and deployment copies are closed and an owner approves a separate disposition. |

See the [moriatst source audit](MORIATST_AUDIT_REVIEW.md) for the source
records and loader references supporting these questions.
