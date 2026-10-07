#!/usr/bin/env python3
"""Regression contract: enhancement policy belongs to the enhance module.

Follow the actual legacy/native skipped-item owners, not global symbol counts.
The five reload clears cover parsing an opened file; the separate missing-file
direct-reentry concern remains unqualified by this source-boundary contract.
"""

from _paths import SRC, extract_function
import contract_text as ct


SPECS = {
    'boot': ('enhance.c', 'void boot_enhancement_system(void)'),
    'death': ('enhance.c', 'void enhance_on_eligible_npc_death(P_char ch, P_char killer)'),
    'legacy_skip': ('enhance.c', 'void enhance_on_npc_item_reset_skipped(P_char mob, P_obj missing_item)'),
    'policy': ('enhance.c', 'bool original_npc_reset_material('),
    'selector': ('enhance.c', 'bool quest_mobile_native_reset_material_owner::select('),
    'reload': ('enhance.c', 'void load_enhance_config(void)'),
    'reset': ('db.c', 'void reset_zone(int zone, int force_item_repop)'),
    'die': ('fight.c', 'void die(P_char ch, P_char killer)'),
    'main': ('comm.c', 'int main(int argc, char **argv)'),
    'comm_boot': ('comm.c', 'int run_the_game(int port, int sslport)'),
    'native_skip': ('quest_mobile_native_birth.c', 'void quest_mobile_native_birth_owner::skipped_item('),
    'native_owns': ('quest_mobile_native_birth.c', 'bool quest_mobile_native_birth_owner::owns('),
    'native_prepare': ('quest_mobile_native_birth.c', 'P_obj quest_mobile_native_birth_owner::prepare_item('),
    'native_carry': ('quest_mobile_native_birth.c', 'bool quest_mobile_native_birth_owner::carry('),
    'local_carry': ('handler.c', 'bool quest_mobile_native_local_stock::carry('),
    'birth_publish': ('quest_mobile_native_birth.c', 'bool quest_mobile_native_birth_owner::publish('),
    'item_publish': ('db.c', 'P_obj quest_mobile_native_item_stage::publish()'),
    'recovery': ('world_recovery_npc_items.c', 'P_obj load_recovery_object('),
    'recovery_carry': ('world_recovery_npc_items.c', 'size_t rehydrate_carried_item('),
    'recovery_equip': ('world_recovery_npc_items.c', 'size_t rehydrate_equipped_item('),
}

def load_sources():
    p = {name: extract_function(*spec) for name, spec in SPECS.items()}
    for name, filename in [('header','enhance.h'),('enhance','enhance.c'),('fight','fight.c'),
                           ('db','db.c'),('comm','comm.c')]:
        p[name] = (SRC/filename).read_text(encoding='utf-8')
    p['G'] = ct.section(p['reset'], "case 'G':", "case 'E':")
    p['E'] = ct.section(p['reset'], "case 'E':", "case 'F':")
    p['G_skip'] = ct.section(p['G'], 'if (!ITEM_LOAD_CHECK', 'if (mob)')
    p['E_skip'] = ct.section(p['E'], 'if (!ITEM_LOAD_CHECK', 'if (mob && (ZCMD.arg3 > 0)')
    p['recovery_skip'] = ct.section(p['recovery'], 'if (!ITEM_LOAD_CHECK', 'return object;')
    p['native_kind_refusal'] = ct.section(p['native_skip'], 'if (rnum < 0)', 'P_obj material')
    return p

def verify(p):
    checked = []
    def require(ok, label):
        if not ok:
            raise AssertionError(label)
        checked.append(label)
    def has(part, needle, label):
        require(ct.contains(p[part], needle), label)
    def ordered(part, needles, label):
        positions = [ct.index(p[part], needle) for needle in needles]
        require(positions == sorted(positions) and len(set(positions)) == len(positions), label)

    # Every original declaration/definition predicate is preserved and scoped.
    for signature, part in [
            ('void boot_enhancement_system(void)', 'boot'),
            ('void enhance_on_eligible_npc_death(P_char ch, P_char killer)', 'death'),
            ('void enhance_on_npc_item_reset_skipped(P_char mob, P_obj missing_item)', 'legacy_skip')]:
        has('header', signature+';', 'original narrow API '+part)
        has(part, signature, 'actual module definition '+part)
    require('enhancematload(' not in p['fight'], 'original no death policy in fight')
    has('die', 'enhance_on_eligible_npc_death(ch, killer);', 'actual death caller')
    has('death', 'if (!ch || !IS_NPC(ch) || IS_PC_PET(ch) || GET_EXP(ch) <= 0) return;',
        'death eligibility remains module policy')
    has('death', 'enhance_load_essence_drop(ch, killer);', 'death forwards module essence selection')
    for forbidden in ('load_npc_missing_item_material', 'enhance_stat_npc_material_fallback_enabled',
                      'get_matstart(missing_item)'):
        require(forbidden not in p['db'], 'original no reset policy in db '+forbidden)
    for forbidden in ('load_enhance_config();', 'load_enhance_index();'):
        require(not ct.contains(p['comm'], forbidden), 'original boot hides '+forbidden)
    has('main', 'run_the_game(port, sslport)', 'main reaches actual game boot owner')
    has('comm_boot', 'boot_enhancement_system();', 'actual comm boot API')
    has('boot', 'if (enhancement_system_ready) return;', 'module owns boot once latch')
    ordered('boot', ['load_enhance_config();', 'load_enhance_index();', 'enhancement_system_ready = true;'],
            'module owns boot configuration before index and ready')
    # Existing test proves these assignments exist. Strengthen to precede parsing
    # an opened file, while retaining the missing-file early-return gap separately.
    for mask in ('enhance_allow_mask', 'enhance_allow_mask2', 'enhance_allow_mask3',
                 'enhance_allow_mask4', 'enhance_allow_mask5'):
        has('reload', mask+' = 0;', 'original reload clear '+mask)
        ordered('reload', [mask+' = 0;', 'while (fgets(line, sizeof(line), fp))'],
                'opened-file reset before parse '+mask)
    require(ct.count(p['db'], 'enhance_on_npc_item_reset_skipped(mob, obj);') == 2,
            'original exact two db legacy hook sites ignoring code whitespace')
    for part in ('G', 'E'):
        require(ct.count(p[part], 'enhance_on_npc_item_reset_skipped(mob, obj);') == 1,
                'actual legacy skipped hook in '+part)
        has(part+'_skip', 'if (native_sql_reset) quest_mobile_native_birth_owner::skipped_item(mob, obj); '
                  'else enhance_on_npc_item_reset_skipped(mob, obj);',
            'actual native/legacy dispatch in '+part)
        ordered(part+'_skip', ['!ITEM_LOAD_CHECK(obj, ival, ZCMD.arg4)',
                       'quest_mobile_native_birth_owner::skipped_item(mob, obj);',
                       'enhance_on_npc_item_reset_skipped(mob, obj);',
                       'quest_mobile_native_birth_owner::discard_item(obj);'],
                'skip notification before discarded candidate in '+part)
    has('G', '(!mob || !IS_SHOPKEEPER(mob))', 'original G shopkeeper exception')
    has('reset', 'const bool native_sql_reset = economic_gameplay_authority::active_regular_sql();',
        'actual SQL-native reset selection')
    has('reset', "(ZCMD.command == 'G' || ZCMD.command == 'E' || ZCMD.command == 'P') && "
                 'quest_mobile_native_birth_owner::owns(mob)', 'native item route requires same birth owner')
    has('native_skip', 'if (!owns(mob) || !quest_mobile_native_reset_material_owner::select(mob, missing, &vnum)) return;',
        'same born mob and module selector before native fallback')
    ordered('native_skip', ['quest_mobile_native_reset_material_owner::select(mob, missing, &vnum)',
                            'const int rnum = real_object(vnum);', 'if (rnum < 0)', 'P_obj material = prepare_item(rnum);',
                            'if (material) carry(material, mob);'], 'selected material follows native staging')
    has('native_kind_refusal', 'return;', 'missing material kind refuses before native staging')
    require(not ct.contains(p['native_skip'], 'read_object(') and
            not ct.contains(p['native_skip'], 'enhance_on_npc_item_reset_skipped('),
            'native skip does not use legacy constructor/grant')
    has('selector', 'return original_npc_reset_material(mob, missing_item, vnum);',
        'native material policy forwards to enhance module')
    has('legacy_skip', 'if (!original_npc_reset_material(mob, missing_item, &high_vnum)) return;',
        'legacy material policy uses same original selector')
    ordered('legacy_skip', ['original_npc_reset_material(mob, missing_item, &high_vnum)',
                            'read_object(high_vnum, VIRTUAL)', 'obj_to_char(material, mob);'],
            'legacy construct/grant remains within enhance')
    has('policy', 'if (!vnum || !enhance_stat_enabled || !enhance_stat_npc_material_fallback_enabled || '
                  '!mob || !IS_NPC(mob) || !missing_item) return false;', 'shared original opt-in/NPC/input gates')
    has('policy', '*vnum = get_matstart(missing_item) + 4;', 'shared exact material policy')
    has('native_owns', 'births[current_birth]->character == mob && !births[current_birth]->mobile_consumed',
        'native stock belongs to unconsumed original mob')
    ordered('native_prepare', ['item.uid = item_uid_allocator_next();',
                               'quest_mobile_native_item_stage::prepare(rnum, REAL, item.uid, item.stage.get())',
                               'b.stock.push_back(std::move(item));'], 'native constructor retained in original stock')
    has('native_carry', 'const bool placed = quest_mobile_native_local_stock::carry(obj, mob);',
        'native staging uses local stock owner')
    has('native_carry', 'if (!placed) births[current_birth]->blocked = true;', 'failed local carry blocks original birth')
    has('local_carry', 'if (!native_birth_detached(ch) || !object || !object->obj_uid || !OBJ_NOWHERE(object) || '
                       'object->next || object->prev || object->affects) return false;',
        'local stock refuses live/missing/linked candidate')
    ordered('local_carry', ['native_birth_group(object, ch->carrying);', 'object->loc_p = LOC_CARRIED;',
                            'object->loc.carrying = ch;'], 'local staged carried projection')
    has('birth_publish', 'if (!b.completed || b.blocked || !b.coordinator_generation || !b.submitted || '
                         'b.completion.disposition != critical_completion_disposition::execution || '
                         '(b.completion.outcome != critical_apply_outcome::applied && '
                         'b.completion.outcome != critical_apply_outcome::already_applied)) return false;',
        'native publication requires original successful completion')
    ordered('birth_publish', ['economic_sql_native_mobile_birth_lock_publication(',
                              'item.stage->retain_admitted();', 'prepare_action(index, ITEM_PUBLICATION, row, 0)',
                              'item.stage->publish()', 'item.published = published;'],
            'actual native source/custody cut before retained item publication')
    has('item_publish', '!s.admitted || s.published || !s.object || s.object->obj_uid != s.uid',
        'staged item publish requires admitted exact original UID')
    ordered('item_publish', ['s.object = nullptr;', 's.published = true;', 'object_list = object;'],
            'publication consumes original stage before live object linkage')
    ordered('recovery_skip', ['!ITEM_LOAD_CHECK(object, itemvalue(object), command.arg4) && !IS_SHOPKEEPER(mob)',
                         'enhance_on_npc_item_reset_skipped(mob, object);', 'extract_obj(object);'],
            'independent recovery missing-item hook before skipped-object extraction')
    for part in ('recovery_carry', 'recovery_equip'):
        has(part, 'load_recovery_object(mob, command, artifact_respawn);', 'actual recovery loader '+part)
    return checked


if __name__ == '__main__':
    try:
        checks = verify(load_sources())
    except (AssertionError, ValueError) as error:
        raise SystemExit(f'enhancement module boundary contract failed: {error}') from error
    print(f'enhancement module boundary contract passed ({len(checks)} source checks)')
