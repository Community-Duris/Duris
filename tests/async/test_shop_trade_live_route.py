#!/usr/bin/env python3
"""Source obligations for shop custody, physical publication and notification.

Ordinary flat-file effects now run through a physical publisher before the
notification callback. Accounted/restored effects retain their separate native
owner. This contract checks the actual defined chain; it is not runtime proof.
"""

from _paths import SRC, extract_function
import contract_text as ct


FUNCTION_SPECS = {
 'buy':('shop.c','void shopping_buy('),
 'sell':('shop.c','void shopping_sell('),
 'purchase':('shop.c','P_obj get_purchase_obj('),
 'peruse':('shop.c','void shopping_peruse('),
 'listing':('shop.c','void shopping_list('),
 'physical':('shop.c','static bool shop_trade_publish_physical_impl('),
 'bridge':('shop.c','static bool shop_trade_publish_physical('),
 'source_match':('shop.c','static bool shop_trade_source_bytes_match('),
 'published_match':('shop.c','static bool shop_trade_published_bytes_match('),
 'accounted_physical':('shop.c','shop_trade_publish_accounted_physical('),
 'original_keeper':('shop.c','static P_char shop_trade_find_original_keeper('),
 'produced':('shop.c','static bool shop_trade_submit_produced_continuation('),
 'cleanup_submit':('shop.c','static bool shop_trade_submit_invalid_cleanup('),
 'cleanup_route':('shop.c','static bool shop_trade_route_invalid_cleanup('),
 'notification':('shop.c','static void shop_trade_completion_impl('),
 'notification_bridge':('shop.c','static void shop_trade_completion('),
 'accounted_notification':('shop.c','static void shop_trade_accounted_completion('),
 'payload':('shop_trade_runtime.c','build_payload(P_char player'),
 'matcher':('shop_trade_runtime.c','bool shop_trade_runtime_object_matches_payload('),
 'ownership':('shop_trade_transaction.c','bool publish_ownership('),
 'publish':('shop_trade_transaction.c','bool publish(decltype(pending)::iterator'),
 'submit':('shop_trade_transaction.c','bool shop_trade_transaction_submit_with_publication('),
 'authority':('shop.c','static bool refuse_unported_shop_mutation('),
}

def load_sources():
    # Skip forward declarations and inspect each actual owner/bridge definition.
    pieces = {key: extract_function(*spec) for key, spec in FUNCTION_SPECS.items()}
    pieces['shop'] = (SRC / 'shop.c').read_text(encoding='utf-8')
    pieces['utils'] = (SRC / 'core/utils.h').read_text(encoding='utf-8')
    return pieces


def verify(p):
    checked=[]
    def require(ok,label):
        if not ok:
            raise AssertionError(label)
        checked.append(label)
    def has(part,needle,label):
        require(ct.contains(p[part],needle),label)
    def ordered(part,needles,label):
        positions=[ct.index(p[part],x) for x in needles]
        require(positions==sorted(positions) and len(set(positions))==len(positions),label)
    submit='shop_trade_transaction_submit_with_publication(ch, payload, shop_trade_publish_physical, shop_trade_completion)'
    # The sixteen original nonobsolete global predicates remain mandatory.
    for token in ('shop_trade_find_object(payload.selected_item_uid)',
        'shop_trade_find_keeper(payload.shop_id)',
        'shop_trade_runtime_object_matches_payload(object, payload)',
        'shop_trade_action::buy_existing','shop_trade_action::buy_produced',
        'shop_trade_action::sell_store','shop_trade_action::sell_destroy',
        'shop_trade_action::discard_invalid',
        'committed shop trade could not publish live object','extract_obj(selected, FALSE)',
        'produced_purchase_sequences','shop_trade_container_accepts',
        'shop_trade_submit_produced_continuation','shop_trade_submit_invalid_cleanup',
        'shop_trade_route_invalid_cleanup','obj_to_obj(object, destination)'):
        has('shop',token,'original global '+token)
    require('put(ch, object, destination' not in p['shop'],'original anti-fallible-put')
    require(p['shop'].count('shop_trade_route_invalid_cleanup(')>=9,'original cleanup route census')
    require('That purchase is not available through flat-file persistence yet' not in p['buy'],
            'original trusted availability')
    has('buy','const int64_t transaction_price = IS_TRUSTED(ch) ? 0 : sale;', 'original trusted zero price')
    require(p['buy'].count('transaction_price')>=3,'original trusted continuation price')
    ordered('buy',[submit,'transact(ch, gem, keeper, sale)','writeShopKeeper('],'original buy before legacy mutations')
    ordered('sell',[submit,'sql_shop_sell(ch, temp1, sale)','ADD_MONEY(ch, sale)'],'original sell before legacy mutations')
    flat=ct.index(p['buy'],'if (persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)',
                  ct.index(p['buy'],'IS_CARRYING_N(ch)'))
    clone=ct.index(p['buy'],'selected = read_object(temp1->R_num, REAL)',flat)
    require(flat<clone<ct.index(p['buy'],submit),'original clone staging inside flat branch')
    for part in ('buy','sell','produced','cleanup_submit'):
        has(part,submit,'nonnull physical publisher binding '+part)
    has('submit','entry.publication = publication;', 'submit preserves physical publisher')
    ordered('submit',['entry.publication = publication','pending.emplace(',
                      'critical_command_coordinator_submit('],'submission before mutation/admission ownership')
    has('utils','#define OBJ_CARRIED(o) (((o) != NULL) && ((o)->loc_p & LOC_CARRIED))','nonnull carried-location primitive')
    has('utils','#define OBJ_CARRIED_BY(o, c) (OBJ_CARRIED(o) && ((o)->loc.carrying == (c)))','typed carried-by identity primitive')
    has('physical','((produced && OBJ_NOWHERE(object)) || ((buying || cleanup) && OBJ_CARRIED_BY(object, keeper)) || (!buying && !cleanup && OBJ_CARRIED_BY(object, ch)))','actual pre-effect custody branch')
    has('physical','if (!keeper || GET_VNUM(keeper) != payload.keeper_vnum','keeper existence/VNUM before custody')
    has('physical','!OBJ_CARRIED_BY(destination, ch)','destination player custody')
    has('physical','!source || !shop_trade_source_bytes_match(object, payload)','source identity and literal guard')
    has('source_match','return shop_trade_runtime_object_matches_payload(object, payload);','legacy snapshot delegation')
    has('source_match','return shop_trade_runtime_object_matches_accounted_payload(object, payload);','separate accounted literal delegation')
    has('published_match','accounted ? player_item_snapshot_tree_capture_literal(object, &actual, nullptr) : player_item_snapshot_tree_capture(object, &actual, nullptr)','separate literal/ordinary final captures')
    has('published_match','expected_bytes == actual_bytes','published complete literal equality')
    for needle in ('selected->obj_uid != payload.selected_item_uid',
                   'player_item_snapshot_tree_capture(selected, &snapshots, nullptr)',
                   'encoded.size() == payload.item_blob_size',
                   'std::equal(encoded.begin(), encoded.end(), payload.item_blob.begin())'):
        has('matcher',needle,'actual exact snapshot '+needle)
    ordered('physical',['!source || !shop_trade_source_bytes_match(object, payload)',
                        'stages |= source_verified','obj_from_char(object)'],'snapshot before detach')
    ordered('physical',['!source || !shop_trade_source_bytes_match(object, payload)',
                        'stages |= source_verified','else if (destroying)',
                        'extract_obj(object, TRUE)'],'cleanup snapshot before destruction')
    has('physical','if (!shop_trade_source_bytes_match(object, payload)) return false;','destruction fresh source guard')
    has('physical','if (placement != obj_to_char_result::placed) return false;','placement failure refuses')
    has('physical','if (!object || !OBJ_CARRIED_BY(object, buying ? ch : keeper)) return false;','placed custody readback')
    has('physical','!obj_can_nest(object, destination)','native nest preflight')
    has('physical','if (!nested) return false;','nest failure refuses')
    for started,returned in (('placement_started','placement_returned'),
                            ('nesting_started','nesting_returned'),
                            ('destruction_started','destruction_returned'),
                            ('room_notice_started','room_notice_returned'),
                            ('detachment_started','detachment_returned')):
        has('physical',f'((stages & {started}) && !(stages & {returned}))',
            'uncertain native effect refuses '+started)
    has('physical','if (stages & cash_started) return false;','uncertain cash effect refuses')
    has('physical','if ((stages & audit_started) && !(stages & audited)) return false;','uncertain audit refuses')
    ordered('physical',['((stages & placement_started) && !(stages & placement_returned))',
                        'P_char keeper = find_keeper()','stages |= source_verified'],
            'uncertain tails refuse before new source/effect proof')
    has('physical','!shop_trade_published_bytes_match(ch, object, payload, buying)','published exact literal before complete/retry')
    has('physical','if (shop_trade_find_object(payload.items[index].item_uid)) return false;','all destroyed UIDs absent')
    has('bridge','return shop_trade_publish_physical_impl(ch, result, payload, stages, 0);','legacy publisher delegates real implementation')
    has('accounted_physical','!keeper->runtime_id || !payload.expected_player_save_revision','accounted publisher requires original identity/save')
    has('accounted_physical','return shop_trade_publish_physical_impl(ch, result, payload, stages, original_keeper_id);','accounted publisher binds original lifetime')
    has('original_keeper','candidate->runtime_id != runtime_id','accounted keeper lookup uses frozen runtime ID')
    has('original_keeper','if (found || !IS_NPC(candidate) || GET_RNUM(candidate) != shop_index[shop_id].keeper) return nullptr;','duplicate/wrong original keeper refuses')
    has('notification','else if (produced && object && OBJ_NOWHERE(object) && shop_trade_source_bytes_match(object, payload)) extract_obj(object, FALSE);','original unpublished clone exact cleanup')
    ordered('notification',['if (!committed)','if (durable_commit)','else if (produced && object && OBJ_NOWHERE(object)','if (cleanup)'],'staged cleanup only under no-commit notification')
    require(not ct.contains(p['notification'],'obj_from_char(object)') and
            not ct.contains(p['notification'],'extract_obj(object, TRUE)'), 'notification does not redo committed physical effects')
    has('notification_bridge','shop_trade_completion_impl(ch, committed, result, error_code, payload, shop_trade_find_keeper(payload.shop_id), false);','actual legacy completion definition delegates notification only')
    has('accounted_notification','shop_trade_find_original_keeper(payload.shop_id, keeper_id)','accounted notification preserves original keeper')
    has('accounted_notification','shop_trade_completion_impl(ch, committed, result, error_code, payload, keeper, true);','accounted notification stays separately classified')
    for part,minimum in (('purchase',2),('buy',3),('peruse',2),('listing',1)):
        require(ct.count(p[part],'shop_trade_route_invalid_cleanup(')>=minimum,'per-caller authority cleanup '+part)
    has('cleanup_route','if (refuse_unported_shop_mutation(ch)) return true;','cleanup authority refusal is handled')
    ordered('cleanup_route',['refuse_unported_shop_mutation(ch)','persistence_mode_get()',
                             'shop_trade_submit_invalid_cleanup('],'authority before invalid-stock submission')
    has('cleanup_submit','if (!ch || !keeper || !object || !OBJ_CARRIED_BY(object, keeper)) return false;','cleanup submit freezes keeper custody')
    for part in ('buy','sell'):
        has(part,'accounted && (!shop_trade_preparation_owner::production_available()','accounted unavailable guard '+part)
        ordered(part,['refuse_unported_shop_mutation(ch)',submit],'authority guard before ordinary flat submission '+part)
    for needle in ('!item_owner_identity_equal(runtime.owner, expected_owner)',
                   'runtime.root_item_uid != selected->obj_uid',
                   'runtime.parent_item_uid != parent_uid','runtime.state != item_custody_state::active',
                   'creates ? ITEM_TRANSFER_ABSENT_REVISION : runtime.item_revision',
                   'built.expected_wallet_revision = player->only.pc->wallet_revision',
                   'built.expected_bank_revision = player->only.pc->bank_revision',
                   'built.expected_shop_revision = shop_revision'):
        has('payload',needle,'captured custody/revision '+needle)
    for needle in ('transfer.target_root_item_uid = payload.target_root_item_uid ? payload.target_root_item_uid : payload.selected_item_uid;',
                   'transfer.target_parent_item_uid = payload.target_parent_item_uid;',
                   'transfer.expected_target_parent_revision = payload.expected_target_parent_revision;',
                   'payload.items[index].expected_item_revision','return item_ownership_runtime_apply(transfer, transfer_result);'):
        has('ownership',needle,'original target/revision publication '+needle)
    ordered('publish',['committed_result_matches(entry, result)','shop_trade_runtime_can_advance(',
                      'currency_transaction_publish_balances(','publish_ownership(entry.payload, result)',
                      'shop_trade_runtime_advance(','!entry.publication(character, result, entry.payload, entry.physical_stages)',
                      'entry.physical_published = true','auto node = pending.extract(found)',
                      'finished.completion(character, committed, result'],'authoritative balance/custody/revision/physical publication before callback')
    has('publish','if (entry.publication && !entry.publication(character, result, entry.payload, entry.physical_stages)) { entry.publishing = false; return false; }','false physical publisher retains pending original')
    has('publish','current->runtime_id != entry.balance_runtime_id','original player runtime before physical publication')
    has('publish','catch (...) { entry.publishing = false; return false; }','throwing projection retains original')
    has('publish','const bool legacy_committed = committed && !entry.preparation;','ordinary projection excludes accounted preparation')
    for needle in ('if (legacy_committed && !entry.custody_published)',
                   'if (legacy_committed && !entry.revision_published)',
                   'if (legacy_committed && !entry.physical_published)'):
        has('publish',needle,'ordinary-only projection '+needle)
    has('publish','!same_native_receipt(original, entry.completed)','native original receipt guard before notification')
    ordered('publish',['shop_trade_native_publication_owner::publish_retained(',
                      'entry.native_acknowledged = true','finished.completion(character, committed, result'],
            'guarded native owner remains separate before accounted notification')
    return checked


if __name__ == '__main__':
    try:
        checks = verify(load_sources())
    except (AssertionError, ValueError) as error:
        raise SystemExit(f'live shop trade route contract failed: {error}') from error
    print(f'flat-file live shop trade route contract passed ({len(checks)} source checks)')
