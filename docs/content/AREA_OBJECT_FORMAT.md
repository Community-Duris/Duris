# Area object format extensions

The historical `.obj` format stores four permanent affect-bitvector masks as
unlabelled unsigned integers immediately after an object's condition value.
Duris runtime objects also have a fifth bank (`obj_data::bitvector5`) for
`AFF5_*` flags. To keep every existing four-bank area file compatible, the
fifth bank uses an explicit optional marker instead of another positional
number.

## Fifth affect bitvector

Place `B5 <mask>` after the fourth affect mask and before the first extra
description (`E`), object modifier (`A`), trap (`T`), or next-record marker:

```text
0
0
0
0
B5 8192
E
...
```

`<mask>` is the unsigned numeric combination of the desired `AFF5_*` values
from `src/core/defines.h`. The marker and value may be on separate lines because
the loader treats whitespace uniformly.

Objects without a `B5` marker retain a zero fifth mask and parse exactly as
before. Do not add a bare fifth number: legacy records use the token after the
fourth mask as the next section marker, so an unlabeled value is ambiguous.

Persistent object save/load already preserves `bitvector5`; this extension is
only for static prototypes loaded from `areas/obj/*.obj` through
`areas/world.obj`.
