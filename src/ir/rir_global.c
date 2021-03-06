#include <ir/rir_global.h>
#include <ir/rir.h>
#include <ir/rir_object.h>

#include <Utils/hash.h>
#include <String/rf_str_core.h>
#include <String/rf_str_common.h>
#include <String/rf_str_manipulationx.h>

static bool rir_global_init(struct rir_object *obj,
                            struct rir_type *type,
                            const struct RFstring *name,
                            const void *value)
{
    // only elementary types for now
    if (!rir_type_is_elementary(type)) {
        RF_ERROR("Tried to create a global non elementary type");
        return false;
    }

    struct rir_global *global = &obj->global;
    if (type->etype != ELEMENTARY_TYPE_STRING) {
        RF_ERROR("We only support string literal globals for now");
        return false;
    }
    return rir_value_literal_init(&global->val, obj, name, value);
}

struct rir_object *rir_global_create(struct rir_type *type,
                                     const struct RFstring *name,
                                     const void *value,
                                     struct rir_ctx *ctx)
{
    struct rir_object *ret = rir_object_create(RIR_OBJ_GLOBAL, ctx->rir);
    if (!ret) {
        return NULL;
    }
    if (!rir_global_init(ret, type, name, value)) {
        free(ret);
        ret = NULL;
    }
    return ret;
}


void rir_global_deinit(struct rir_global *global)
{
    rir_value_deinit(&global->val);
}

bool rir_global_tostring(struct rirtostr_ctx *ctx, const struct rir_global *g)
{
    bool ret;
    RFS_PUSH();
    ret = rf_stringx_append(
        ctx->rir->buff,
        RFS("global("RF_STR_PF_FMT", "RF_STR_PF_FMT", \""RF_STR_PF_FMT"\")\n",
            RF_STR_PF_ARG(rir_global_name(g)),
            RF_STR_PF_ARG(rir_type_string(rir_global_type(g))),
            RF_STR_PF_ARG(rir_value_actual_string(&g->val)))
    );
    RFS_POP();
    return ret;
}


i_INLINE_INS const struct RFstring *rir_global_name(const struct rir_global *g);
i_INLINE_INS struct rir_type *rir_global_type(const struct rir_global *g);


struct rir_object *rir_global_addorget_string(struct rir_ctx *ctx, const struct RFstring *s)
{
    struct rir_object *gstring = strmap_get(&ctx->rir->global_literals, s);
    if (!gstring) {
        RFS_PUSH();
        gstring = rir_global_create(
            rir_type_elem_create(ELEMENTARY_TYPE_STRING, false),
            RFS("gstr_%u", rf_hash_str_stable(s, 0)),
            s,
            ctx
        );
        if (gstring) {
            // here we must make sure to not use @a s since it can be a temporary string
            // and strmap_add does not copy the key string, but just points to it
            if (!strmap_add(&ctx->rir->global_literals, &rir_object_value(gstring)->literal, gstring)) {
                RF_ERROR("Failed to add a string literal to the global string map");
                gstring = NULL;
            }
        }
        RFS_POP();
    }
    return gstring;
}
