"""Real SQL dispatch dependencies shared by narrow native integration fixtures.

The dispatch function retains every supported command branch, including its
accounting owner. Link those owners instead of stubbing their behavior or letting
individual fixture lists drift as the production dispatcher grows.
"""

SQL_DISPATCH_SOURCES = (
    'item/item_transfer_command.c',
    'item/craft_pouch_mutation.c',
    'combat/chaos_pouch_ledger.c',
    'item/item_transfer_repository.c',
    'persistence/sql_room_item_payload.c',
    'economy/auction_command.c',
    'economy/auction_repository.c',
    'combat/combat_outcome_repository.c',
    'guild/artifact_guild_command.c',
    'guild/artifact_guild_repository.c',
    'economy/boon_reward_command.c',
    'economy/boon_reward_repository.c',
    'world/zone_touch_command.c',
    'world/zone_touch_repository.c',
    'account/session_audit_command.c',
    'account/session_audit_repository.c',
    'economy/coin_transfer_command.c',
    'economy/collector_command.c',
    'economy/collector_codec.c',
    'economy/collector_policy.c',
    'economy/collector_repository.c',
    'persistence/corpse_lifecycle_command.c',
    'persistence/corpse_lifecycle_repository.c',
    'persistence/player_death_restitution_command.c',
    'persistence/player_death_restitution_repository.c',
    'player/player_snapshot_codec.c',
    'player/player_load_repository.c',
    'player/player_save_journal.c',
    'persistence/quest_reward_obligation_repository.c',
    'player/player_death_recovery_query.c',
    'player/player_death_conflict_repository.c',
    'player/player_load_topology.c',
    'persistence/persistence_observability.c',
    'persistence/economic_accounting_repository.c',
    'persistence/economic_sql_bank_transaction.c',
    'economy/economic_currency_adapter.c',
    'economy/economic_accounting_types.c',
    'economy/economic_accounting_plan.c', 'economy/economic_source_event.c',
    'economy/economic_accounting_intent.c',
    'persistence/economic_sql_lifecycle_guard.c',
    'persistence/critical_command_repository.c',
    'item/economic_accounting_item_reference.c',
    'economy/coin_transfer_accounting.c',
    'economy/item_transfer_accounting.c',
    'economy/collector_accounting.c',
    'economy/shop_trade_command.c', 'economy/shop_trade_recovery_manifest.c',
    'economy/shop_trade_accounting.c',
    'persistence/economic_sql_item_transfer_transaction.c',
    'persistence/economic_sql_collector_transaction.c',
    'persistence/economic_sql_shop_trade_transaction.c', 'persistence/shop_item_runtime_payload.c', 'economy/shop_trade_recovery_image.c',
    'economy/economic_command_admission.c',
)


if __name__ == '__main__':
    # Shell fixtures consume the same reviewed dependency set as Python owners.
    print(' '.join('src/' + path for path in SQL_DISPATCH_SOURCES))
