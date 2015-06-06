#include <check.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <String/rf_str_core.h>
#include <ast/ast.h>
#include <ast/function.h>

#include "testsupport_rir.h"

#include CLIB_TEST_HELPERS

START_TEST (test_finalized_function_arguments) {

    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "fn foo(a:u32) -> u64 {\n"
        "return 45 + a\n"
        "}"
        "fn boo(a:u32, b:string, c:f64) -> u64 {\n"
        "return 45 + a\n"
        "}"
        "fn goo() -> u64 {\n"
        "return 45\n"
        "}"
    );
    struct rir_testdriver *d = get_rir_testdriver();
    rir_testdriver_assign(d, &s);
    testsupport_rir_process(d);

    struct ast_node *fn1 = ast_node_get_child(d->front_driver->front.analyzer->root, 0);
    ck_assert_uint_eq(ast_fndecl_argsnum_get(ast_fnimpl_fndecl_get(fn1)), 1);
    struct ast_node *fn2 = ast_node_get_child(d->front_driver->front.analyzer->root, 1);
    ck_assert_uint_eq(ast_fndecl_argsnum_get(ast_fnimpl_fndecl_get(fn2)), 3);
    struct ast_node *fn3 = ast_node_get_child(d->front_driver->front.analyzer->root, 2);
    ck_assert_uint_eq(ast_fndecl_argsnum_get(ast_fnimpl_fndecl_get(fn3)), 0);

} END_TEST

Suite *rir_finalized_ast_suite_create(void)
{
    Suite *s = suite_create("rir_finalized_ast");

    TCase *finalized_ast = tcase_create("finalized_ast_nodes");
    tcase_add_checked_fixture(finalized_ast,
                              setup_rir_tests,
                              teardown_rir_tests);
    tcase_add_test(finalized_ast, test_finalized_function_arguments);

    suite_add_tcase(s, finalized_ast);

    return s;
}