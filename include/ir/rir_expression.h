#ifndef LFR_IR_RIR_EXPRESSION_H
#define LFR_IR_RIR_EXPRESSION_H

#include <RFintrusive_list.h>
#include <stdint.h>
#include <ast/constants_decls.h>
#include <ir/rir_value.h>
#include <ir/rir_argument.h>

struct ast_node;
struct rir;
struct rir_ctx;
struct rirtostr_ctx;

enum rir_expression_type {
    RIR_EXPRESSION_CALL,
    RIR_EXPRESSION_ALLOCA,
    RIR_EXPRESSION_RETURN,
    RIR_EXPRESSION_CONVERT,
    RIR_EXPRESSION_WRITE,
    RIR_EXPRESSION_READ,
    RIR_EXPRESSION_OBJMEMBERAT,
    RIR_EXPRESSION_SETUNIONIDX,
    RIR_EXPRESSION_GETUNIONIDX,
    RIR_EXPRESSION_UNIONMEMBERAT,
    RIR_EXPRESSION_CONSTANT,
    RIR_EXPRESSION_ADD,
    RIR_EXPRESSION_SUB,
    RIR_EXPRESSION_MUL,
    RIR_EXPRESSION_DIV,
    RIR_EXPRESSION_CMP,
    RIR_EXPRESSION_LOGIC_AND,
    RIR_EXPRESSION_LOGIC_OR,
    // PLACEHOLDER, should not make it into actual production
    RIR_EXPRESSION_PLACEHOLDER
};


/**
 *  RIR call specific members of an expression.
 *  @note: The return value which is also the value of the function call
 *  is part of the containing expression and not of this struct
 */
struct rir_call {
    //! Name of the function call
    struct RFstring name;
    //! An array of values that comprise this call's arguments
    struct value_arr args;
};

struct rir_alloca {
    const struct rir_ltype *type;
    uint64_t num;
};

struct rir_return {
    const struct rir_expression *val;
};

struct rir_convert {
    //! Type to convert to
    const struct rir_ltype *totype;
    //! Value to convert
    const struct rir_value *convval;
};

struct rir_binaryop {
    const struct rir_value *a;
    const struct rir_value *b;
};

struct rir_read {
    //! Memory value to read from
    const struct rir_value *memory;
};

struct rir_objmemberat {
    const struct rir_value *objmemory;
    uint32_t idx;
};

struct rir_setunionidx {
    const struct rir_value *unimemory;
    const struct rir_value *idx;
};

struct rir_getunionidx {
    const struct rir_value *unimemory;
};

struct rir_unionmemberat {
    const struct rir_value *unimemory;
    uint32_t idx;
};


struct rir_object *rir_alloca_create_obj(const struct rir_ltype *type,
                                         uint64_t num,
                                         struct rir_ctx *ctx);

struct rir_expression *rir_setunionidx_create(const struct rir_value *unimemory,
                                              const struct rir_value *idx,
                                              struct rir_ctx *ctx);
struct rir_expression *rir_getunionidx_create(const struct rir_value *unimemory,
                                              struct rir_ctx *ctx);

struct rir_expression *rir_unionmemberat_create(const struct rir_value *unimemory,
                                                uint32_t idx,
                                                struct rir_ctx *ctx);

struct rir_object *rir_objmemberat_create_obj(const struct rir_value *objmemory,
                                              uint32_t idx,
                                              struct rir_ctx *ctx);
struct rir_expression *rir_objmemberat_create(const struct rir_value *objmemory,
                                              uint32_t idx,
                                              struct rir_ctx *ctx);

struct rir_expression *rir_read_create(const struct rir_value *memory_to_read,
                                       struct rir_ctx *ctx);

struct rir_object *rir_return_create(const struct rir_expression *val, struct rir_ctx *ctx);
void rir_return_init(struct rir_expression *ret,
                     const struct rir_expression *val);

struct rir_expression *rir_conversion_create(const struct rir_ltype *totype,
                                             const struct rir_value *convval,
                                             struct rir_ctx *ctx);

struct rir_expression {
    enum rir_expression_type type;
    union {
        struct rir_convert convert;
        struct rir_call call;
        struct rir_alloca alloca;
        struct rir_setunionidx setunionidx;
        struct rir_getunionidx getunionidx;
        struct rir_unionmemberat unionmemberat;
        struct rir_objmemberat objmemberat;
        struct rir_binaryop binaryop;
        struct rir_read read;
        struct rir_return ret;
        // kind of ugly but it's exactly what we need
        struct ast_constant constant;
    };
    struct rir_value val;
    // Control to be added to expression list of a rir block
    struct RFilist_node ln;
};

bool rir_expression_init(struct rir_object *expr,
                         enum rir_expression_type type,
                         struct rir_ctx *ctx);
void rir_expression_destroy(struct rir_expression *expr);
bool rir_expression_tostring(struct rirtostr_ctx *ctx, const struct rir_expression *e);
#endif
