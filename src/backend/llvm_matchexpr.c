#include "llvm_matchexpr.h"

#include <llvm-c/Core.h>

#include "llvm_ast.h"
#include "llvm_utils.h"
#include "llvm_operators.h"

#include <ast/matchexpr.h>
#include <types/type.h>
#include <ir/rir.h>
#include <ir/rir_type.h>

static void bllvm_add_matchcase(struct ast_node *matchcase,
                                struct rir_type *matching_type,
                                LLVMValueRef llvm_ret_alloca,
                                const struct type *ret_type,
                                LLVMValueRef llvm_typedecl_val,
                                LLVMValueRef llvm_switch,
                                LLVMBasicBlockRef match_end,
                                struct llvm_traversal_ctx *ctx)
{
    const struct type *case_matched_type = ast_matchcase_matched_type(matchcase);
    const struct rir_type *rcase_matched_type = rir_types_list_get_type(
        &ctx->rir->rir_types_list,
        case_matched_type,
        NULL
    );
    int index = rir_type_childof_type(rcase_matched_type, matching_type);
    RF_ASSERT(index != -1, "Case type not found in matching type");
    LLVMBasicBlockRef case_branch = LLVMInsertBasicBlock(match_end, "");
    bllvm_enter_block(ctx, case_branch);
    // create backend handles (BuildAlloca) for the symbols of the symbol table
    symbol_table_iterate(ctx->current_st, (htable_iter_cb)llvm_symbols_iterate_cb, ctx);
    // compile match case's expression and assign to type
    LLVMValueRef gep_to_main_contents = bllvm_gep_to_struct(llvm_typedecl_val, 0, ctx);
    bllvm_compile_assign_llvm(gep_to_main_contents, llvm_ret_alloca, ret_type, BLLVM_ASSIGN_MATCH_CASE, ctx);
    LLVMBuildBr(ctx->builder, match_end);
    LLVMAddCase(llvm_switch, LLVMConstInt(LLVMInt32Type(), index, 0), case_branch);
}

struct LLVMOpaqueValue *bllvm_compile_matchexpr(struct ast_node *n,
                                                struct llvm_traversal_ctx *ctx)
{
    struct ast_matchexpr_it it;
    struct ast_node *mcase;
    // find the type of the matched identifier
    struct symbol_table_record *rec;
    struct ast_node *id = ast_matchexpr_identifier(n);
    const struct RFstring *s = ast_identifier_str(id);
    rec = symbol_table_lookup_record(ctx->current_st, s, NULL);
    RF_ASSERT(rec && rec->rir_data, "Identifier's type was not determined");
    RF_ASSERT(rec->rir_data->category == COMPOSITE_RIR_DEFINED &&
              rir_type_is_sumtype(rec->rir_data),
              "Identifier's type was not a defined type containing sums");
    RFS_PUSH();
    LLVMTypeRef llvm_type = LLVMGetTypeByName(
        ctx->mod,
        rf_string_cstr_from_buff_or_die(type_defined_get_name(rec->data))
    );
    RFS_POP();
    RF_ASSERT(llvm_type, "Could not get llvm type for matching type");
    RF_ASSERT(rec->backend_handle, "Could not get llvm Value ref for the object");
    
    // allocate a return value
    LLVMTypeRef ret_llvm_type = bllvm_type_from_normal(n->expression_type, ctx);
    LLVMValueRef ret_alloc = LLVMBuildAlloca(ctx->builder, ret_llvm_type, "");

    LLVMBasicBlockRef match_end = bllvm_add_block_before_funcend(ctx);
    LLVMBasicBlockRef fatal_err_block = bllvm_add_fatal_block_before(match_end, 1, ctx);

    LLVMValueRef gep_to_selector = bllvm_gep_to_struct(rec->backend_handle, 1, ctx);
    LLVMValueRef selector_val = LLVMBuildLoad(ctx->builder, gep_to_selector, "");
    LLVMValueRef llvm_switch = LLVMBuildSwitch(
        ctx->builder,
        selector_val,
        fatal_err_block,
        ast_matchexpr_cases_num(n)
    );

    struct symbol_table *encasing_block_st = ctx->current_st;
    ast_matchexpr_foreach(n, &it, mcase) {
        // switch to the match case symbol table
        ctx->current_st = ast_matchcase_symbol_table_get(mcase);
        bllvm_add_matchcase(mcase, darray_item(rec->rir_data->subtypes, 0), ret_alloc, n->expression_type, rec->backend_handle, llvm_switch, match_end, ctx);
    }

    ctx->current_st = encasing_block_st;
    bllvm_enter_block(ctx, match_end);
    return ret_alloc;
}