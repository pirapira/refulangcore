#include <ir/rir_call.h>
#include <ir/rir.h>
#include <ir/rir_argument.h>
#include <ir/rir_block.h>
#include <ir/rir_binaryop.h>
#include <ir/rir_constant.h>
#include <ir/rir_convert.h>
#include <ir/rir_function.h>
#include <ir/rir_object.h>
#include <ir/rir_process.h>
#include <ast/function.h>
#include <types/type.h>
#include <types/type_operators.h>
#include <types/type_function.h>

/* -- code to process a constructor call -- */

struct args_to_val_ctx {
    struct rir_ctx *rirctx;
    //! left hand of assignment
    struct rir_value *lhs;
    //! index of the argument we are iterating
    unsigned index;
};

static void args_to_val_ctx_init(struct args_to_val_ctx *ctx, struct rir_value *lhs, struct rir_ctx *rirctx)
{
    ctx->rirctx = rirctx;
    ctx->lhs = lhs;
    ctx->index = 0;
}
static bool ctor_args_to_value_cb(const struct ast_node *n, struct args_to_val_ctx *ctx)
{
    // for each argument, process the ast node and create the rir arg expression
    const struct rir_value *argexprval = rir_process_ast_node_getreadval(n, ctx->rirctx);
    if (!argexprval) {
        RF_ERROR("Could not create rir expression from constructor argument");
        return false;
    }

    struct rir_expression *e;
    // find the target value to write to
    struct rir_value *targetval;
    if (ctx->lhs->type->category == RIR_TYPE_COMPOSITE) {
        // if lhs type is composite create expression to read its index from the lhs composite type
        if (!(e = rir_objmemberat_create(ctx->lhs, ctx->index, ctx->rirctx))) {
            RF_ERROR("Failed to create rir expression to get rir object's member memory");
            return false;
        }
        rirctx_block_add(ctx->rirctx, e);
        if (!(targetval = rir_getread_val(e, ctx->rirctx))) {
            RF_ERROR("Failed to create rir expression to read an object's value");
            return false;
        }
    } else {
        RF_ASSERT(ctx->index == 0, "Only at 0 index can a non composite type have been found.");
        targetval = ctx->lhs;
    }
    // write the arg expression to the position
    if (!(e = rir_write_create(targetval, argexprval, ctx->rirctx))) {
        RF_ERROR("Failed to create expression to write to an object's member");
        return false;
    }
    rirctx_block_add(ctx->rirctx, e);

    ++ctx->index;
    return true;
}

/**
 * Populate an object's memory from arguments of an ast function call
 * @param objmemory        The rir value of the object's memory to populate
 * @param ast_call         The ast function call to populate from
 * @param ctx              The rir ctx
 * @return                 True for success
 */
static bool rir_populate_from_astcall(struct rir_value *objmemory, const struct ast_node *ast_call, struct rir_ctx *ctx)
{
    struct args_to_val_ctx argsctx;
    if (type_is_sumtype(ast_fncall_type(ast_call))) {
        RF_ASSERT(rir_type_is_composite(objmemory->type), "Constructor should assign to a composite type");
        int union_idx = rir_type_union_matched_type_from_fncall(objmemory->type, ast_call, ctx);
        if (union_idx == -1) {
            RF_ERROR("RIR sum constructor not matching any part of the original type");
            return false;
        }
        // create code to set the  union's index with the matching type
        struct rir_value *rir_idx_const = rir_constantval_create_fromint32(union_idx, ctx->rir);
        struct rir_expression *e = rir_setunionidx_create(objmemory, rir_idx_const, ctx);
        if (!e) {
            return false;
        }
        rirctx_block_add(ctx, e);
        // create code to load the appropriate union subtype for reading
        struct rir_expression *ummbr_ptr = rir_unionmemberat_create(objmemory, union_idx, ctx);
        if (!ummbr_ptr) {
            return false;
        }
        rirctx_block_add(ctx, ummbr_ptr);
        if (!(objmemory = rir_getread_val(ummbr_ptr, ctx))) {
            return false;
        }
    }
    // now for whichever object (normal type, or union type sutype) is loaded as left hand side
    // assign from constructor's arguments
    args_to_val_ctx_init(&argsctx, objmemory, ctx);
    ast_fncall_for_each_arg(ast_call, (fncall_args_cb)ctor_args_to_value_cb, &argsctx);
    return true;
}

static bool rir_process_ctorcall(const struct ast_node *n, struct rir_ctx *ctx)
{
    struct rir_value *lhs = rir_ctx_lastassignval_get(ctx);
    if (!lhs) {
        RF_ERROR("RIR constructor call should have a valid left hand side in the assignment");
        return false;
    }
    return rir_populate_from_astcall(lhs, n, ctx);
}

/* -- code to process a normal function call -- */
struct fncall_args_toarr_ctx {
    struct rir_ctx *rirctx;
    const struct type *fndecl_type;
    struct value_arr *arr;
};

static void fncall_args_toarr_ctx_init(struct fncall_args_toarr_ctx *ctx,
                                       const struct ast_node *ast_call,
                                       struct rir_ctx *rirctx,
                                       struct value_arr *arr)
{
    ctx->rirctx = rirctx;
    ctx->arr = arr;
    darray_init(*arr);
    ctx->fndecl_type = ast_fncall_type(ast_call);
}

static bool ast_fncall_args_toarr_cb(const struct ast_node *n, struct fncall_args_toarr_ctx *ctx)
{
    const struct rir_value *argexprval = rir_process_ast_node_getreadval(n, ctx->rirctx);
    if (!argexprval) {
        RF_ERROR("Could not create rir expression from fncall argument");
        return false;
    }
    const struct type *argtype = ctx->fndecl_type->category == TYPE_CATEGORY_OPERATOR
        ? type_get_subtype(ctx->fndecl_type, darray_size(*ctx->arr))
        : ctx->fndecl_type;
    if (!(argexprval = rir_maybe_convert(argexprval, rir_type_create_from_type(argtype, ctx->rirctx), ctx->rirctx))) {
        RF_ERROR("Could not create conversion for rir call argument");
        return false;
    }
    darray_append(*ctx->arr, (struct rir_value*)argexprval);
    return true;
}

struct rir_object *rir_call_create_obj_from_ast(const struct ast_node *n, struct rir_ctx *ctx)
{
    struct rir_object *ret = rir_object_create(RIR_OBJ_EXPRESSION, ctx->rir);
    if (!ret) {
        return NULL;
    }

    // copy the name in
    if (!rf_string_copy_in(&ret->expr.call.name, ast_fncall_name(n))) {
        goto fail;
    }

    if (ast_fncall_is_sum(n)) {
        // if it's a call to a function with a sum type, get the type the call matched
        struct rir_type *sumtype = rir_type_create_from_type(ast_fncall_type(n), ctx);
        if (!sumtype) {
            RF_ERROR("Could not get the rir type of a sum function call");
        }
        // create an alloca for that type
        struct rir_expression *e = rir_alloca_create(sumtype, 0, ctx);
        if (!e) {
            RF_ERROR("Failed to create a rir alloca instruction");
            goto fail;
        }
        rirctx_block_add(ctx, e);
        // populate the memory of the sumtype
        if (!rir_populate_from_astcall(&e->val, n, ctx)) {
            RF_ERROR("Failed to rir union type's memory");
        }
        // then pass that as the only argument to the call
        darray_init(ret->expr.call.args);
        darray_append(ret->expr.call.args, &e->val);
    } else {
        // turn the function call args into a rir value array
        struct fncall_args_toarr_ctx fncarg_ctx;
        fncall_args_toarr_ctx_init(&fncarg_ctx, n, ctx, &ret->expr.call.args);
        if (!ast_fncall_for_each_arg(n, (fncall_args_cb)ast_fncall_args_toarr_cb, &fncarg_ctx)) {
            goto fail;
        }
    }

    // now initialize the rir expression part of the struct
    if (!rir_object_expression_init(ret, RIR_EXPRESSION_CALL, ctx)) {
        goto fail;
    }

    return ret;
fail:
    free(ret);
    return NULL;
}

bool rir_process_fncall(const struct ast_node *n, struct rir_ctx *ctx)
{
    const struct RFstring *fn_name = ast_fncall_name(n);
    const struct type *fn_type;
    // if we are in a block start check from there. If not simply search in the module
    fn_type = type_lookup_identifier_string(fn_name, rir_ctx_curr_st(ctx));
    if (!fn_type) {
        RF_ERROR("No function call of a given name could be found");
        return false;
    }

    if (fn_type->category == TYPE_CATEGORY_DEFINED) { // a constructor
        return rir_process_ctorcall(n, ctx);
    } else if (n->fncall.is_explicit_conversion) {
        return rir_process_convertcall(n, ctx);
    }
    // else normal function call
    struct rir_object *cobj = rir_call_create_obj_from_ast(n, ctx);
    if (!cobj) {
        RF_ERROR("Could not create a rir function call instruction");
        return false;
    }
    rirctx_block_add(ctx, &cobj->expr);
    RIRCTX_RETURN_EXPR(ctx, true, cobj);
}

bool rir_call_tostring(struct rirtostr_ctx *ctx, const struct rir_expression *cexpr)
{
    bool ret = false;
    RFS_PUSH();
    if (rir_value_is_nil(&cexpr->val)) {
        if (!rf_stringx_append(
                ctx->rir->buff,
                RFS(RIRTOSTR_INDENT"call("RF_STR_PF_FMT, RF_STR_PF_ARG(&cexpr->call.name)))) {
            goto end;
        }
    } else {
        if (!rf_stringx_append(
                ctx->rir->buff,
                RFS(RIRTOSTR_INDENT RF_STR_PF_FMT" = call("RF_STR_PF_FMT,
                    RF_STR_PF_ARG(rir_value_string(&cexpr->val)),
                    RF_STR_PF_ARG(&cexpr->call.name)))) {
            goto end;
        }
    }

    struct rir_value **arg_val;
    darray_foreach(arg_val, cexpr->call.args) {
        if (!rf_stringx_append(ctx->rir->buff, RFS(", "RF_STR_PF_FMT, RF_STR_PF_ARG(rir_value_string(*arg_val))))) {
            goto end;
        }
    }
    if (!rf_stringx_append_cstr(ctx->rir->buff, ")\n")) {
        goto end;
    }
    // success
    ret = true;

end:
    RFS_POP();
    return ret;
}

struct rir_type *rir_call_return_type(struct rir_call *c, struct rir_ctx *ctx)
{
    struct rir_fndecl *decl = rir_fndecl_byname(ctx->rir, &c->name);
    RF_ASSERT(decl, "At this point in the RIR the function declaration should have been found");
    return decl->return_type;
}

void rir_call_deinit(struct rir_call *c)
{
    rf_string_deinit(&c->name);
    darray_free(c->args);
    // the args are rir objects themselves so will be freed from global list
}
