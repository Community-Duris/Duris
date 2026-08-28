# todo.md

TODO of tasks NOT done yet:

DONE - `drop all`, `drop all.<name>`, `put all`, and `put all.<name>` now use the
same serialized durable chain as `get all` (`start_bulk_drop` / `start_bulk_put`
in `src/actobj.c`). The blanket "Durable items must be dropped/put away one at a
time." rejections are gone. Regression test:
`tests/async/test_bulk_drop_put_durable_chain.py`.

 - Thorough testing of all the systems we are touching

---

- the lockers were still deleting items.. Wich xander-l gave up trouble shooting
- in-game copyover wipes all gear?
