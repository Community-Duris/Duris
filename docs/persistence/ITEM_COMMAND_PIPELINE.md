# Item command pipeline

Issue #347 makes item movement commands use one visible decision boundary. The
boundary is intentionally small:

~~~text
bounded GET parse
        |
        v
shared eligibility and live-source/destination policy
        |
        v
immutable item movement transaction
        |
        v
durable completion, then live publication
~~~

src/item/item_command_parser.h owns only the GET grammar. It turns the legacy
forms into a typed command kind while preserving the existing six behaviors:

| Form | Kind |
| --- | --- |
| get all or get all.<name> | floor bulk |
| get <object> | floor item |
| get all from <container> or get all.<name> from <container> | container bulk |
| get <object> from <container> | container item |
| get <object> from all | matching item from every eligible container |

The parser is bounded by MAX_INPUT_LENGTH, keeps all.<name> filtering separate
from the command kind, and does not retain the input buffer.

src/item/item_command_policy.h owns the shared command decisions:

- item_command_uses_durable_ownership() is the single durable/transient
  eligibility rule. Coins, PC corpse roots, uid-less objects, and unowned
  transient objects retain their existing synchronous or specialized paths.
- item_command_object_is_takeable() and item_command_container_is_valid() are
  shared GET eligibility checks.
- item_get_source_owner() validates the physical live topology against the
  runtime owner registry. It walks nested containment with a bounded cycle
  guard, accepts only the actor's room/custody, and preserves explicit virtual
  authorities (locker, auction, shopkeeper, and collector). NPC custody and
  nowhere placement have no durable source authority.
- item_command_resolve_put_destination() is the one put destination resolver
  for single and bulk puts. A locker chest is a virtual owner and therefore
  has no live parent target; an ordinary container must have an active runtime
  owner record.
- item_command_resolve_drop_destination() is the one room-versus-locker
  resolver for single and bulk drops.

The command layer may select objects and produce messages, but it does not
publish a durable move early. The movement transaction captures UIDs,
root/parent topology, expected item and owner revisions, source and target
identities, and batch membership. It revalidates actor, source, target, and
location when deferred work reaches the persistence boundary. Only the
completion callback publishes the live object change, so retryable, ambiguous,
failed, linkdead, reconnect, and shutdown paths cannot report a successful
move before the authoritative result exists. Operation identity, revisions,
ordering, transient creation grants, and terminal retention remain owned by
item_movement_transaction; this change does not create a second transaction
framework.

The command policy deliberately does not manufacture an owner for unsupported
NPC custody. The existing explicit refusal for a generic PC-to-NPC give remains
in place until a durable mobile-custody authority exists. That safety boundary
is preferable to moving the live object into a location that cannot be
reconstructed after save, reconnect, or recovery.

Flatfile and MariaDB adapters consume the same typed transfer payload and
revision/UID contract. Their repository and schema behavior is covered by the
existing item transfer, corpse, locker, auction, equipment, save-format, and
backend-specific tests; this command slice adds no persistence schema and does
not claim to make an unsupported backend operation valid.
