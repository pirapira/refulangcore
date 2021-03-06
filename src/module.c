#include <module.h>

#include <Utils/fixed_memory_pool.h>

#include <utils/common_strings.h>
#include <compiler.h>
#include <utils/common_strings.h>
#include <front_ctx.h>
#include <ast/ast.h>
#include <ast/module.h>
#include <ast/type.h>
#include <ast/ast_utils.h>
#include <types/type.h>
#include <types/type_comparisons.h>
#include <analyzer/analyzer.h>
#include <analyzer/analyzer_pass1.h>
#include <analyzer/typecheck.h>
#include <ir/rir.h>


static bool module_init(struct module *m, struct ast_node *n, struct front_ctx *front)
{
    // initialize
    RF_STRUCT_ZERO(m);
    m->node = n;
    m->front = front;
    darray_init(m->dependencies);
    darray_init(m->foreignfn_arr);
    // add to the compiler's modules
    darray_append(compiler_instance_get()->modules, m);

    // initialize analysis related members
    m->symbol_table_records_pool = rf_fixed_memorypool_create(sizeof(struct symbol_table_record),
                                                              RECORDS_TABLE_POOL_CHUNK_SIZE);
    if (!m->symbol_table_records_pool) {
        RF_ERROR("Failed to initialize a fixed memory pool for symbol records");
        return false;
    }
    m->types_pool = rf_fixed_memorypool_create(sizeof(struct type),
                                               TYPES_POOL_CHUNK_SIZE);
    if (!m->types_pool) {
        RF_ERROR("Failed to initialize a fixed memory pool for types");
        return false;
    }

    RF_MALLOC(m->types_set, sizeof(*m->types_set), return false);
    rf_objset_init(m->types_set, type);

    rf_objset_init(&m->identifiers_set, string);
    rf_objset_init(&m->string_literals_set, string);

    return true;
}

struct module *module_create(struct ast_node *n, struct front_ctx *front)
{
    struct module *ret;
    RF_ASSERT(n->type == AST_MODULE || n->type == AST_ROOT,
              "Unexpected ast node type");
    RF_MALLOC(ret, sizeof(*ret), return NULL);
    if (!module_init(ret, n, front)) {
        free(ret);
        ret = NULL;
    }
    return ret;
}

static void module_deinit(struct module *m)
{
   if (m->symbol_table_records_pool) {
        rf_fixed_memorypool_destroy(m->symbol_table_records_pool);
    }
    rf_objset_clear(&m->identifiers_set);
    rf_objset_clear(&m->string_literals_set);

    if (m->types_set) {
        type_objset_destroy(m->types_set, m->types_pool);
    }
    if (m->types_pool) {
        rf_fixed_memorypool_destroy(m->types_pool);
    }

    if (m->rir) {
        rir_destroy(m->rir);
    }

    darray_free(m->foreignfn_arr);
    darray_free(m->dependencies);
}

void module_destroy(struct module* m)
{
    module_deinit(m);
    free(m);
}

void module_add_foreign_import(struct module *m, struct ast_node *import)
{
    RF_ASSERT(ast_node_is_foreign_import(import), "Expected a foreign import node");
    struct ast_node *child;
    rf_ilist_for_each(&import->children, child, lh) {
        // for now foreign import should only import function decls
        AST_NODE_ASSERT_TYPE(child, AST_FUNCTION_DECLARATION);
        darray_append(m->foreignfn_arr, child);
    }
}

bool module_add_import(struct module *m, struct ast_node *import)
{
    AST_NODE_ASSERT_TYPE(import, AST_IMPORT);
    struct ast_node *c;
    struct module *other_mod;
    rf_ilist_for_each(&import->children, c, lh) {

        other_mod = compiler_module_get(ast_identifier_str(c));
        if (!other_mod) {
            // requested import module not found
            i_info_ctx_add_msg(m->front->info,   
                               MESSAGE_SEMANTIC_ERROR,
                               ast_node_startmark(import),
                               ast_node_endmark(import),
                               "Requested module \""RF_STR_PF_FMT"\" not found for importing.",
                               RF_STR_PF_ARG(ast_identifier_str(c)));
            return false;
        }
        darray_append(m->dependencies, other_mod);
    }
    return true;
}

const struct RFstring *module_name(const struct module *m)
{
    return m->node->type == AST_ROOT ? &g_str_main : ast_module_name(m->node);
}

struct symbol_table *module_symbol_table(const struct module *m)
{
    return ast_node_symbol_table_get(m->node);
}

bool module_symbol_table_init(struct module *m)
{
    RF_ASSERT(m->node->type == AST_ROOT || m->node->type == AST_MODULE,
              "Illegal ast node detected");
        return m->node->type == AST_ROOT
            ? true  // the root symbol table should have already been initialized
            : symbol_table_init(&m->node->module.st, m);
}

struct inpfile *module_get_file(const struct module *m)
{
    return m->front->info->file;
}

bool module_is_main(const struct module *m)
{
    return rf_string_equal(module_name(m), &g_str_main);
}

bool module_add_stdlib(struct module *m)
{
    struct module *other_mod = compiler_module_get(&g_str_stdlib);
    if (!other_mod) {
        RF_ERROR("stdlib was requested but could not be found in the parsed compiler modules");
        return false;
    }
    darray_append(m->dependencies, other_mod);
    return true;
}


bool module_types_set_add(struct module *m, struct type *new_type)
{
    return rf_objset_add(m->types_set, type, new_type);
}

struct type *module_get_or_create_type(struct module *mod,
                                       const struct ast_node *desc,
                                       struct symbol_table *st,
                                       struct ast_node *genrdecl)
{
    struct type *t;
    if (desc->type == AST_TYPE_LEAF) {
        desc = ast_typeleaf_right(desc);
    }
    if (desc->type == AST_XIDENTIFIER) {
        return type_lookup_xidentifier(desc, mod, st, genrdecl);
    }
    struct rf_objset_iter it;
    rf_objset_foreach(mod->types_set, &it, t) {
        if (type_equals_ast_node(t, desc, mod, st, genrdecl, TYPECMP_IDENTICAL)) {
            return t;
        }
    }

    // else we have to create a new type
    t = type_create_from_node(desc, mod, st, genrdecl);
    if (!t) {
        return NULL;
    }

    // TODO: Should it not have been added already by the proper creation function?
    // add it to the list
    module_types_set_add(mod, t);
    return t;
}


static bool module_determine_dependencies_do(struct ast_node *n, void *user_arg)
{
    struct module *mod = user_arg;
    switch (n->type) {
    case AST_IMPORT:
        if (!ast_import_is_foreign(n)) {
            return module_add_import(mod, n);
        }
    default:
        break;
    }
    return true;
}

bool module_determine_dependencies(struct module *m, bool use_stdlib)
{
    // initialize module symbol table here instead of analyzer_first_pass
    // since we need it beforehand to get symbols from import
    if (!module_symbol_table_init(m)) {
        RF_ERROR("Could not initialize symbol table for root node");
        return false;
    }

    // read the imports and add dependencies
    if (!ast_pre_traverse_tree(m->node, module_determine_dependencies_do, m)) {
        return false;
    }

    // TODO: This can't be the best way to achieve this. Rethink when possible
    // if this is the main module add the stdlib as dependency,
    // unless a program without the stdlib was requested
    if (use_stdlib && module_is_main(m)) {
        return module_add_stdlib(m);
    }
    return true;
}

bool module_analyze(struct module *m)
{
    bool ret = false;
    // since analyze pass is always going to be one per thread initializing
    // thread local type creation context here should be okay
    type_creation_ctx_init();
    // create symbol tables and change ast nodes ownership
    if (!analyzer_first_pass(m)) {
        if (!module_have_errors(m)) {
            RF_ERROR("Failure at module analysis first pass");
        }
        goto end;
    }

    if (!analyzer_typecheck(m, m->node)) {
        if (!module_have_errors(m)) {
            RF_ERROR("Failure at module's typechecking");
        }
        goto end;
    }

    if (!analyzer_finalize(m)) {
        RF_ERROR("Failure at module's finalization");
        goto end;
    }

    // success
    ret = true;
end:
    type_creation_ctx_deinit();
    return ret;
}

bool module_have_errors(const struct module *m)
{
    return info_ctx_has(m->front->info, MESSAGE_SEMANTIC_ERROR | MESSAGE_SYNTAX_ERROR);
}
