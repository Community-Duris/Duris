// migrate_objects.c
// binary object parsing for pfile migration tool

#include "migrate_common.h"

// helper to parse unique object fields
static void parse_unique_fields(char **buf, struct mig_obj *obj, unsigned long o_u_flag) {
    if (o_u_flag & O_U_KEYS) obj->name = mig_getString(buf);
    if (o_u_flag & O_U_DESC1) obj->description = mig_getString(buf);
    if (o_u_flag & O_U_DESC2) obj->short_descr = mig_getString(buf);
    if (o_u_flag & O_U_DESC3) obj->action_descr = mig_getString(buf);
    if (o_u_flag & O_U_EDESC) {
        int nDescs = mig_getShort(buf);
        while (nDescs--) {
            char *kw = mig_getString(buf);
            char *desc = mig_getString(buf);
            if (kw) free(kw);
            if (desc) free(desc);
        }
    }
    if (o_u_flag & O_U_VAL0) { obj->value[0] = mig_getInt(buf); obj->value_set |= (1 << 0); }
    if (o_u_flag & O_U_VAL1) { obj->value[1] = mig_getInt(buf); obj->value_set |= (1 << 1); }
    if (o_u_flag & O_U_VAL2) { obj->value[2] = mig_getInt(buf); obj->value_set |= (1 << 2); }
    if (o_u_flag & O_U_VAL3) { obj->value[3] = mig_getInt(buf); obj->value_set |= (1 << 3); }
    if (o_u_flag & O_U_VAL4) { obj->value[4] = mig_getInt(buf); obj->value_set |= (1 << 4); }
    if (o_u_flag & O_U_VAL5) { obj->value[5] = mig_getInt(buf); obj->value_set |= (1 << 5); }
    if (o_u_flag & O_U_VAL6) { obj->value[6] = mig_getInt(buf); obj->value_set |= (1 << 6); }
    if (o_u_flag & O_U_VAL7) { obj->value[7] = mig_getInt(buf); obj->value_set |= (1 << 7); }
    if (o_u_flag & O_U_TIMER) {
        obj->timer = mig_getInt(buf);
        mig_getInt(buf); mig_getInt(buf); mig_getInt(buf); // timer[1-3]
    }
    if (o_u_flag & O_U_TRAP) {
        mig_getShort(buf); mig_getShort(buf);
        mig_getShort(buf); mig_getShort(buf);
    }
    if (o_u_flag & O_U_TYPE) { obj->item_type = MIG_GET_BYTE(*buf); obj->item_type_set = 1; }
    if (o_u_flag & O_U_WEAR) { obj->wear_flags = mig_getInt(buf); obj->wear_flags_set = 1; }
    if (o_u_flag & O_U_EXTRA) { obj->extra_flags = mig_getInt(buf); obj->extra_flags_set = 1; }
    if (o_u_flag & O_U_ANTI) mig_getInt(buf);
    if (o_u_flag & O_U_ANTI2) mig_getInt(buf);
    if (o_u_flag & O_U_EXTRA2) mig_getInt(buf);
    if (o_u_flag & O_U_WEIGHT) obj->weight = mig_getInt(buf);
    if (o_u_flag & O_U_MATERIAL) MIG_GET_BYTE(*buf);
    if (o_u_flag & O_U_COST) obj->cost = mig_getInt(buf);
    if (o_u_flag & O_U_BV1) { obj->bitvector1 = mig_getLong(buf); obj->bitvector_set |= 1; }
    if (o_u_flag & O_U_BV2) { obj->bitvector2 = mig_getLong(buf); obj->bitvector_set |= 2; }
    if (o_u_flag & O_U_BV3) { obj->bitvector3 = mig_getLong(buf); obj->bitvector_set |= 4; }
    if (o_u_flag & O_U_BV4) { obj->bitvector4 = mig_getLong(buf); obj->bitvector_set |= 8; }
    if (o_u_flag & O_U_BV5) { obj->bitvector5 = mig_getLong(buf); obj->bitvector_set |= 16; }
    if (o_u_flag & O_U_AFFS) {
        for (int i = 0; i < MAX_OBJ_AFFECT; i++) {
            obj->affected[i].location = MIG_GET_BYTE(*buf);
            obj->affected[i].modifier = MIG_GET_BYTE(*buf);
        }
        obj->affected_set = 1;
    }
}

// parse binary object data for corpses/saved items
struct mig_obj *parse_binary_objects(char **buf) {
    int obj_vers = MIG_GET_BYTE(*buf);
    if (obj_vers > SAV_ITEMVERS) {
        printf("  error: item version %d > %d\n", obj_vers, SAV_ITEMVERS);
        return NULL;
    }

    mig_getInt(buf); // total count - not needed
    struct mig_obj *root = NULL;
    struct mig_obj *last = NULL;
    struct mig_obj *container_stack[32];
    int stack_depth = 0;

    for (;;) {
        unsigned char o_f_flag = MIG_GET_BYTE(*buf);

        if (o_f_flag & O_F_EOL) {
            if (stack_depth > 0) {
                stack_depth--;
                continue;
            }
            break;
        }

        struct mig_obj *obj = (struct mig_obj *)malloc(sizeof(struct mig_obj));
        memset(obj, 0, sizeof(struct mig_obj));

        obj->vnum = mig_getInt(buf);
        mig_getShort(buf); // craftsmanship
        mig_getShort(buf); // condition

        if (o_f_flag & O_F_WORN)
            MIG_GET_BYTE(*buf); // wear location - skip

        if (o_f_flag & O_F_COUNT)
            mig_getShort(buf); // quantity - skip

        if (o_f_flag & O_F_AFFECTS) {
            int aff_count = MIG_GET_BYTE(*buf);
            while (aff_count--) {
                mig_getInt(buf);   // time
                mig_getShort(buf); // type
                mig_getShort(buf); // data
                mig_getInt(buf);   // extra2
            }
        }

        if (o_f_flag & O_F_UNIQUE) {
            unsigned long o_u_flag = mig_getInt(buf);
            parse_unique_fields(buf, obj, o_u_flag);
        }

        // O_F_SPELLBOOK - store spell bitfield for migration
        if (o_f_flag & O_F_SPELLBOOK) {
            int tmp = mig_getInt(buf);
            if (tmp > 0) {
                obj->spellbook_bits = (char *)malloc(tmp);
                obj->spellbook_size = tmp;
                for (int i = 0; i < tmp; i++)
                    obj->spellbook_bits[i] = MIG_GET_BYTE(*buf);
            }
        }

        // link to list or container
        if (stack_depth > 0) {
            struct mig_obj *parent = container_stack[stack_depth - 1];
            if (!parent->contains) {
                parent->contains = obj;
            } else {
                struct mig_obj *c = parent->contains;
                while (c->next) c = c->next;
                c->next = obj;
            }
        } else {
            if (!root)
                root = obj;
            if (last)
                last->next = obj;
            last = obj;
        }

        if (o_f_flag & O_F_CONTAINS) {
            container_stack[stack_depth++] = obj;
        }
    }

    return root;
}

// parse locker items (same format as binary objects)
struct mig_obj *parse_locker_items(char **buf) {
    int obj_vers = MIG_GET_BYTE(*buf);
    if (obj_vers > SAV_ITEMVERS) {
        printf("  error: locker item version %d > %d\n", obj_vers, SAV_ITEMVERS);
        return NULL;
    }

    mig_getInt(buf); // total count - not needed

    struct mig_obj *root = NULL;
    struct mig_obj *last = NULL;
    struct mig_obj *container_stack[32];
    int stack_depth = 0;

    for (;;) {
        unsigned char o_f_flag = MIG_GET_BYTE(*buf);

        if (o_f_flag & O_F_EOL) {
            if (stack_depth > 0) {
                stack_depth--;
                continue;
            }
            break;
        }

        struct mig_obj *obj = (struct mig_obj *)malloc(sizeof(struct mig_obj));
        memset(obj, 0, sizeof(struct mig_obj));

        obj->vnum = mig_getInt(buf);
        mig_getShort(buf); // craftsmanship
        mig_getShort(buf); // condition

        if (o_f_flag & O_F_WORN)
            MIG_GET_BYTE(*buf); // wear location - skip for lockers

        if (o_f_flag & O_F_COUNT)
            mig_getShort(buf); // quantity

        if (o_f_flag & O_F_AFFECTS) {
            int aff_count = MIG_GET_BYTE(*buf);
            while (aff_count--) {
                mig_getInt(buf);
                mig_getShort(buf);
                mig_getShort(buf);
                mig_getInt(buf);
            }
        }

        if (o_f_flag & O_F_UNIQUE) {
            unsigned long o_u_flag = mig_getInt(buf);
            parse_unique_fields(buf, obj, o_u_flag);
        }

        // O_F_SPELLBOOK - store spell bitfield for migration
        if (o_f_flag & O_F_SPELLBOOK) {
            int tmp = mig_getInt(buf);
            if (tmp > 0) {
                obj->spellbook_bits = (char *)malloc(tmp);
                obj->spellbook_size = tmp;
                for (int i = 0; i < tmp; i++)
                    obj->spellbook_bits[i] = MIG_GET_BYTE(*buf);
            }
        }

        // link to list or container
        if (stack_depth > 0) {
            struct mig_obj *parent = container_stack[stack_depth - 1];
            if (!parent->contains) {
                parent->contains = obj;
            } else {
                struct mig_obj *c = parent->contains;
                while (c->next) c = c->next;
                c->next = obj;
            }
        } else {
            if (!root) root = obj;
            if (last) last->next = obj;
            last = obj;
        }

        if (o_f_flag & O_F_CONTAINS) {
            container_stack[stack_depth++] = obj;
        }
    }

    return root;
}

// parse player items section
int parse_player_items(char **buf, struct mig_player *p) {
    int obj_vers = MIG_GET_BYTE(*buf);
    if (obj_vers > SAV_ITEMVERS) return 0;

    mig_getInt(buf); // total count
    struct mig_obj *container_stack[32];
    int stack_depth = 0;
    struct mig_obj *last_inventory = NULL;

    for (;;) {
        unsigned char o_f_flag = MIG_GET_BYTE(*buf);

        if (o_f_flag & O_F_EOL) {
            if (stack_depth > 0) {
                stack_depth--;
                continue;
            }
            break;
        }

        struct mig_obj *obj = (struct mig_obj *)malloc(sizeof(struct mig_obj));
        memset(obj, 0, sizeof(struct mig_obj));

        obj->vnum = mig_getInt(buf);
        mig_getShort(buf); // craftsmanship
        mig_getShort(buf); // condition

        int wear_slot = -1;
        if (o_f_flag & O_F_WORN)
            wear_slot = MIG_GET_BYTE(*buf);

        if (o_f_flag & O_F_COUNT)
            mig_getShort(buf); // quantity

        if (o_f_flag & O_F_AFFECTS) {
            int aff_count = MIG_GET_BYTE(*buf);
            while (aff_count--) {
                mig_getInt(buf);
                mig_getShort(buf);
                mig_getShort(buf);
                mig_getInt(buf);
            }
        }

        if (o_f_flag & O_F_UNIQUE) {
            unsigned long o_u_flag = mig_getInt(buf);
            parse_unique_fields(buf, obj, o_u_flag);
        }

        // O_F_SPELLBOOK - store spell bitfield for migration
        if (o_f_flag & O_F_SPELLBOOK) {
            int tmp = mig_getInt(buf);
            if (tmp > 0) {
                obj->spellbook_bits = (char *)malloc(tmp);
                obj->spellbook_size = tmp;
                for (int i = 0; i < tmp; i++)
                    obj->spellbook_bits[i] = MIG_GET_BYTE(*buf);
            }
        }

        // link to appropriate place
        if (stack_depth > 0) {
            struct mig_obj *parent = container_stack[stack_depth - 1];
            if (!parent->contains) {
                parent->contains = obj;
            } else {
                struct mig_obj *c = parent->contains;
                while (c->next) c = c->next;
                c->next = obj;
            }
        } else if (wear_slot > 0 && wear_slot <= MIG_MAX_WEAR) {
            p->equipment[wear_slot - 1] = obj;
        } else {
            if (!p->inventory)
                p->inventory = obj;
            if (last_inventory)
                last_inventory->next = obj;
            last_inventory = obj;
        }

        if (o_f_flag & O_F_CONTAINS) {
            container_stack[stack_depth++] = obj;
        }
    }

    return 1;
}
