#ifndef LFR_AST_FUNCTIONS_H
#define LFR_AST_FUNCTIONS_H

#include <ast/ast.h>
#include <Utils/sanity.h>
#include <ast/identifier.h>

#include <analyzer/symbol_table.h>

/* -- function declaration functions -- */

struct ast_node *ast_fndecl_create(struct inplocation_mark *start,
                                   struct inplocation_mark *end,
                                   enum fndecl_position pos,
                                   struct ast_node *name,
                                   struct ast_node *genr,
                                   struct ast_node *args,
                                   struct ast_node *ret);


i_INLINE_DECL const struct RFstring *ast_fndecl_name_str(const struct ast_node *n)
{
    AST_NODE_ASSERT_TYPE(n, AST_FUNCTION_DECLARATION);
    return ast_identifier_str(n->fndecl.name);
}

i_INLINE_DECL bool ast_fndecl_symbol_table_init(struct ast_node *n,
                                                struct analyzer *a)
{
    AST_NODE_ASSERT_TYPE(n, AST_FUNCTION_DECLARATION);
    return symbol_table_init(&n->fndecl.st, a);
}

i_INLINE_DECL struct symbol_table *ast_fndecl_symbol_table_get(struct ast_node *n)
{
    AST_NODE_ASSERT_TYPE(n, AST_FUNCTION_DECLARATION);
    return &n->fndecl.st;
}

i_INLINE_DECL struct ast_node *ast_fndecl_genrdecl_get(struct ast_node *n)
{
    AST_NODE_ASSERT_TYPE(n, AST_FUNCTION_DECLARATION);
    return n->fndecl.genr;
}

i_INLINE_DECL struct ast_node *ast_fndecl_args_get(struct ast_node *n)
{
    AST_NODE_ASSERT_TYPE(n, AST_FUNCTION_DECLARATION);
    return n->fndecl.args;
}

i_INLINE_DECL struct ast_node *ast_fndecl_return_get(struct ast_node *n)
{
    AST_NODE_ASSERT_TYPE(n, AST_FUNCTION_DECLARATION);
    return n->fndecl.ret;
}

i_INLINE_DECL enum fndecl_position ast_fndecl_position_get(struct ast_node *n)
{
    AST_NODE_ASSERT_TYPE(n, AST_FUNCTION_DECLARATION);
    return n->fndecl.position;
}

struct RFstring *ast_fndecl_ret_str(struct ast_node *n);

/* -- function implementation functions -- */

struct ast_node *ast_fnimpl_create(struct inplocation_mark *start,
                                   struct inplocation_mark *end,
                                   struct ast_node *decl,
                                   struct ast_node *body);

i_INLINE_DECL struct ast_node *ast_fnimpl_fndecl_get(struct ast_node *n)
{
    AST_NODE_ASSERT_TYPE(n, AST_FUNCTION_IMPLEMENTATION);
    return n->fnimpl.decl;
}

i_INLINE_DECL struct ast_node *ast_fnimpl_body_get(struct ast_node *n)
{
    AST_NODE_ASSERT_TYPE(n, AST_FUNCTION_IMPLEMENTATION);
    return n->fnimpl.body;
}

i_INLINE_DECL void ast_fnimpl_symbol_table_set(struct ast_node *n,
                                               struct symbol_table *st)
{
    AST_NODE_ASSERT_TYPE(n, AST_FUNCTION_IMPLEMENTATION);
    n->fnimpl.st = st;
}

i_INLINE_DECL struct symbol_table *ast_fnimpl_symbol_table_get(struct ast_node *n)
{
    AST_NODE_ASSERT_TYPE(n, AST_FUNCTION_IMPLEMENTATION);
    return n->fnimpl.st;
}

/* -- function call functions -- */

struct ast_node *ast_fncall_create(struct inplocation_mark *start,
                                   struct inplocation_mark *end,
                                   struct ast_node *name,
                                   struct ast_node *genr);

i_INLINE_DECL const struct RFstring* ast_fncall_name(struct ast_node *n)
{
    AST_NODE_ASSERT_TYPE(n, AST_FUNCTION_CALL);
    return ast_identifier_str(n->fncall.name);
}

#endif
