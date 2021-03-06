#ifndef LFR_BACKEND_LLVM_OPERATORS_H
#define LFR_BACKEND_LLVM_OPERATORS_H

struct LLVMOpaqueValue;
struct rir_expression;
struct llvm_traversal_ctx;

struct LLVMOpaqueValue *bllvm_compile_comparison(const struct rir_expression *expr,
                                                 struct llvm_traversal_ctx *ctx);
struct LLVMOpaqueValue *bllvm_compile_rirbop(const struct rir_expression *expr,
                                             struct llvm_traversal_ctx *ctx);

#endif
