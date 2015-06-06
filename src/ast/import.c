#include <ast/ast.h>
#include <ast/import.h>

struct ast_node *ast_import_create(const struct inplocation_mark *start,
                                   const struct inplocation_mark *end,
                                   bool foreign)
{
    struct ast_node *ret;
    ret = ast_node_create_marks(AST_IMPORT, start, end);
    if (!ret) {
        return NULL;
    }
    ret->import.foreign = foreign;
    return ret;
}

i_INLINE_INS bool ast_import_is_foreign(const struct ast_node *n);

static const struct ast_node i_foreign_fncall = {
    .type = AST_FUNCTION_CALL,
    .state = AST_NODE_STATE_CREATED
};
const struct ast_node *ast_foreign_fncall()
{
    return &i_foreign_fncall;
}