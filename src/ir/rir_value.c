#include <ir/rir_value.h>
#include <ir/rir.h>
#include <ir/rir_call.h>
#include <ir/rir_function.h>
#include <ir/rir_object.h>
#include <ir/rir_argument.h>
#include <ast/constants.h>
#include <String/rf_str_core.h>
#include <String/rf_str_manipulationx.h>
#include <Utils/memory.h>

bool rir_value_label_init_string(struct rir_value *v, struct rir_object *obj, const struct RFstring *s, struct rir_ctx *ctx)
{
    RF_ASSERT(obj->category == RIR_OBJ_BLOCK, "Expected rir block object");
    v->category = RIR_VALUE_LABEL;
    v->label_dst = &obj->block;
    if (!rf_string_copy_in(&v->id, s)) {
        return false;
    }
    return rir_map_addobj(ctx, &v->id, obj);
}

bool rir_value_constant_init(struct rir_value *v, const struct ast_constant *c)
{
    bool ret;
    v->category = RIR_VALUE_CONSTANT;
    v->constant = *c;
    switch (v->constant.type) {
    case CONSTANT_NUMBER_INTEGER:
        v->type = rir_ltype_elem_create(ELEMENTARY_TYPE_INT_64, false);
        ret = rf_string_initv(&v->id, "%"PRId64, v->constant.value.integer);
        break;
    case CONSTANT_NUMBER_FLOAT:
        v->type = rir_ltype_elem_create(ELEMENTARY_TYPE_FLOAT_64, false);
        ret = rf_string_initv(&v->id, "%f", v->constant.value.floating);
        break;
    case CONSTANT_BOOLEAN:
        v->type = rir_ltype_elem_create(ELEMENTARY_TYPE_BOOL, false);
        ret = rf_string_initv(&v->id, "%s", v->constant.value.boolean ? "true" : "false");
        break;
    }
    return ret;
}

bool rir_value_literal_init(struct rir_value *v, struct rir_object *obj, const struct RFstring *name, const struct RFstring *value)
{
    v->category = RIR_VALUE_LITERAL;
    v->obj = obj;
    v->type = rir_ltype_elem_create(ELEMENTARY_TYPE_STRING, false);
    if (!rf_string_copy_in(&v->id, name)) {
        return false;
    }
    return rf_string_copy_in(&v->literal, value);
}
bool rir_value_variable_init(struct rir_value *v, struct rir_object *obj, struct rir_ltype *type, struct rir_ctx *ctx)
{
    bool ret = false;
    v->category = RIR_VALUE_VARIABLE;
    v->obj = obj;
    if (!rf_string_initv(&v->id, "$%d", ctx->expression_idx++)) {
        return false;
    }
    if (obj->category == RIR_OBJ_EXPRESSION) {
        struct rir_expression *expr = &v->obj->expr;
        switch (v->obj->expr.type) {
        case RIR_EXPRESSION_CONVERT:
            v->type = rir_ltype_create_from_other(expr->convert.totype, false);
            break;
        case RIR_EXPRESSION_ALLOCA:
            v->type = rir_ltype_create_from_other(expr->alloca.type, true);
            break;
        case RIR_EXPRESSION_CMP:
            v->type = rir_ltype_elem_create(ELEMENTARY_TYPE_BOOL, false);
            break;
        case RIR_EXPRESSION_GETUNIONIDX:
            v->type = rir_ltype_elem_create(ELEMENTARY_TYPE_INT_64, false);
            break;
        case RIR_EXPRESSION_READ:
            if (!expr->read.memory->type->is_pointer) {
                RF_ERROR("Tried to rir read from a location not in memory");
                goto end;
            }
            v->type = rir_ltype_create_from_other(expr->alloca.type, false);
            break;
        case RIR_EXPRESSION_CALL:
            // figure out the type
            v->type = rir_ltype_create_from_other(rir_call_return_type(&expr->call, ctx), false);
            break;
        case RIR_EXPRESSION_ADD:
        case RIR_EXPRESSION_SUB:
        case RIR_EXPRESSION_MUL:
        case RIR_EXPRESSION_DIV:
            RF_ASSERT(rir_ltype_is_elementary(expr->binaryop.a->type), "Expected elementary type to be used in either part of rir binary op");
            v->type = rir_ltype_elem_create(expr->binaryop.a->type->etype, false);
            break;
        case RIR_EXPRESSION_OBJMEMBERAT:
            RF_ASSERT(rir_ltype_is_composite(expr->objmemberat.objmemory->type), "Expected composite type at objmemberat");
            v->type = rir_ltype_copy_from_other(
                rir_ltype_comp_member_type(
                    expr->objmemberat.objmemory->type,
                    expr->objmemberat.idx
                )
            );
            break;
        case RIR_EXPRESSION_UNIONMEMBERAT:
            // for now value type determining is the same as objmemberat.
            // the memberat index does not take into account the union index
            RF_ASSERT(rir_ltype_is_composite(expr->unionmemberat.unimemory->type), "Expected composite type at unionmemberat");
            v->type = rir_ltype_copy_from_other(
                rir_ltype_comp_member_type(
                    expr->unionmemberat.unimemory->type,
                    expr->unionmemberat.idx
                )
            );
            break;
        default:
            RF_ASSERT(false, "TODO: Unimplemented rir expression to value conversion");
            break;
        }
    } else if (obj->category == RIR_OBJ_ARGUMENT) {
        // argument constructor create the type, so just assign it here
        v->type = type;
    } else if (obj->category == RIR_OBJ_GLOBAL) {
        v->type = rir_ltype_copy_from_other(type);
    } else {
        RF_CRITICAL_FAIL("TODO ... should this even ever happen?");
    }
    // finally add it to the rir strmap
    ret = rir_map_addobj(ctx, &v->id, obj);

end:
    return ret;
}

bool rir_value_label_init(struct rir_value *v, struct rir_object *obj, struct rir_ctx *ctx)
{
    RF_ASSERT(obj->category == RIR_OBJ_BLOCK, "Expected rir block object");
    v->category = RIR_VALUE_LABEL;
    v->label_dst = &obj->block;
    if (!rf_string_initv(&v->id, "%%label_%d", ctx->label_idx++)) {
        return false;
    }
    return rir_map_addobj(ctx, &v->id, obj);
}

void rir_value_nil_init(struct rir_value *v)
{
    RF_STRUCT_ZERO(v);
    v->category = RIR_VALUE_NIL;
}

void rir_value_deinit(struct rir_value *v)
{
    rf_string_deinit(&v->id);
    rir_ltype_destroy(v->type);
}

void rir_value_destroy(struct rir_value *v)
{
    rir_value_deinit(v);
    free(v);
}

bool rir_value_tostring(struct rir *r, const struct rir_value *v)
{
    switch (v->category) {
    case RIR_VALUE_CONSTANT:
    case RIR_VALUE_VARIABLE:
    case RIR_VALUE_LABEL:
    case RIR_VALUE_LITERAL:
        if (!rf_stringx_append(r->buff, &v->id)) {
            return false;
        }
    case RIR_VALUE_NIL:
        break;
    }
    return true;
}

const struct RFstring *rir_value_string(const struct rir_value *v)
{
    switch (v->category) {
    case RIR_VALUE_CONSTANT:
    case RIR_VALUE_VARIABLE:
    case RIR_VALUE_LABEL:
    case RIR_VALUE_LITERAL:
        return &v->id;
    case RIR_VALUE_NIL:
        break;
    }
    return rf_string_empty_get();
}

const struct RFstring *rir_value_actual_string(const struct rir_value *v)
{
    switch (v->category) {
    case RIR_VALUE_CONSTANT:
        return ast_constant_string(&v->constant);
    case RIR_VALUE_LITERAL:
        return RFS("\""RF_STR_PF_FMT"\"", RF_STR_PF_ARG(&v->literal));
    case RIR_VALUE_VARIABLE:
    case RIR_VALUE_LABEL:
    case RIR_VALUE_NIL:
        RF_CRITICAL_FAIL("Should never request actual value for such value type");
        break;
    }
    return NULL;
}

struct rir_block *rir_value_label_dst(const struct rir_value *v)
{
    RF_ASSERT(v->category == RIR_VALUE_LABEL, "Expected a label value");
    return v->label_dst;
}
  
int64_t rir_value_constant_int_get(const struct rir_value *v)
{
    RF_ASSERT(v->category == RIR_VALUE_CONSTANT, "Expected a constant value");
    RF_ASSERT(v->constant.type == CONSTANT_NUMBER_INTEGER, "Expected an integer constant");
    return v->constant.value.integer;
}

i_INLINE_INS bool rir_value_is_nil(const struct rir_value *v);
