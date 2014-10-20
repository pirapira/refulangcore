#include <ast/block.h>

#include <ast/ast.h>

struct ast_node *ast_block_create()
{
    struct ast_node *ret;
    ret = ast_node_create(AST_BLOCK);
    if (!ret) {
        RF_ERRNOMEM();
        return NULL;
    }

    return ret;
}

i_INLINE_INS bool ast_block_symbol_table_init(struct ast_node *n,
                                              struct analyzer *a);
i_INLINE_INS struct symbol_table* ast_block_symbol_table_get(struct ast_node *n);
