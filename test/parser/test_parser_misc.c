/**
 * Tests for parsing of ast node not having their own test file
 */

#include <check.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <String/rf_str_core.h>
#include <parser/parser.h>
#include "../../src/parser/recursive_descent/block.h"
#include <ast/function.h>
#include <ast/type.h>
#include <ast/typeclass.h>
#include <ast/generics.h>
#include <ast/block.h>
#include <ast/operators.h>
#include <ast/constants.h>
#include <ast/module.h>
#include <ast/string_literal.h>
#include <lexer/lexer.h>
#include <info/msg.h>

#include "../testsupport_front.h"
#include "testsupport_parser.h"

#include CLIB_TEST_HELPERS

START_TEST (test_acc_string_literals) {
    struct ast_node *n;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "{\n"
        "a = \"a_string_literal\"\n"
        "}");
    front_testdriver_new_main_source(&s);

    struct ast_node *id_a = testsupport_parser_identifier_create(1, 0, 1, 0);
    testsupport_parser_string_literal_create(lit_1, 1, 4, 1, 21);
    testsupport_parser_node_create(bop1, binaryop, 1, 0, 1, 21,
                                   BINARYOP_ASSIGN, id_a, lit_1);


    testsupport_parser_block_create(bnode, 0, 0, 2, 0);
    ast_node_add_child(bnode, bop1);

    ck_test_parse_as(n, block, "block with literals", bnode, true);

    // check that the string literal gets parsed without the \"\"
    struct ast_node *c = ast_node_get_child(n, 0);
    ck_assert_rf_str_eq_cstr(ast_string_literal_get_str(ast_binaryop_right(c)),
                             "a_string_literal");

    ast_node_destroy(n);
    ast_node_destroy(bnode);
} END_TEST

START_TEST (test_acc_boolean_constants) {
    struct ast_node *n;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "{\n"
        "a = true\n"
        "b = false\n"
        "}");
    front_testdriver_new_main_source(&s);

    struct ast_node *id_a = testsupport_parser_identifier_create(1, 0, 1, 0);
    struct ast_node *id_b = testsupport_parser_identifier_create(2, 0, 2, 0);
    testsupport_parser_constant_create(t_node, 1, 4, 1, 7, boolean, true);
    testsupport_parser_constant_create(f_node, 2, 4, 2, 8, boolean, false);
    testsupport_parser_node_create(bop1, binaryop, 1, 0, 1, 7,
                                   BINARYOP_ASSIGN, id_a, t_node);
    testsupport_parser_node_create(bop2, binaryop, 2, 0, 2, 8,
                                   BINARYOP_ASSIGN, id_b, f_node);

    testsupport_parser_block_create(bnode, 0, 0, 3, 0);
    ast_node_add_child(bnode, bop1);
    ast_node_add_child(bnode, bop2);

    ck_test_parse_as(n, block,  "block with boolean constants", bnode, true);

    ast_node_destroy(n);
    ast_node_destroy(bnode);
} END_TEST

START_TEST (test_acc_import_statements) {
    struct ast_node *n;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "import mod1, mod2\n"
        "import mod3\n"
        "foreign_import a_function(u64)\n"
    );
    front_testdriver_new_main_source(&s);

    struct ast_node *id_mod1 = testsupport_parser_identifier_create(0, 7, 0, 10);
    struct ast_node *id_mod2 = testsupport_parser_identifier_create(0, 13, 0, 16);
    struct ast_node *id_mod3 = testsupport_parser_identifier_create(1, 7, 1, 10);
    // TODO: finish, left in the middle
    struct ast_node *id_fn_name = testsupport_parser_identifier_create(2, 15, 2, 24);
    testsupport_parser_xidentifier_create_simple(arg_id, 2, 26, 2, 28);
    testsupport_parser_node_create(fn, fndecl, 2, 15, 2, 29,
                                   FNDECL_PARTOF_FOREIGN_IMPORT,
                                   id_fn_name,
                                   NULL,
                                   arg_id,
                                   NULL
    );
    testsupport_parser_node_create(imp1, import, 0, 0, 0, 16, false);
    ast_node_add_child(imp1, id_mod1);
    ast_node_add_child(imp1, id_mod2);
    testsupport_parser_node_create(imp2, import, 1, 0, 1, 10, false);
    ast_node_add_child(imp2, id_mod3);
    testsupport_parser_node_create(imp3, import, 2, 0, 2, 29, true);
    ast_node_add_child(imp3, fn);

    struct ast_node *expected_root = ast_root_create(front_testdriver_file());
    ast_node_add_child(expected_root, imp1);
    ast_node_add_child(expected_root, imp2);
    ast_node_add_child(expected_root, imp3);

    ck_test_parse_root(n, expected_root);

    ast_node_destroy(expected_root);
} END_TEST

START_TEST (test_acc_import_statements_fail1) {
    static const struct RFstring s = RF_STRING_STATIC_INIT("import ");
    front_testdriver_new_main_source(&s);

    ck_test_fail_parse_file();
    struct info_msg errors[] = {
        TESTSUPPORT_INFOMSG_INIT_START(
            MESSAGE_SYNTAX_ERROR,
            "Expected an identifier at import statement",
            0, 5),
        TESTSUPPORT_INFOMSG_INIT_START(
            MESSAGE_SYNTAX_ERROR,
            "Expected an outermost statement",
            0, 0),
    };
    ck_assert_parser_errors(errors);

} END_TEST

START_TEST (test_acc_import_statements_fail2) {
    static const struct RFstring s = RF_STRING_STATIC_INIT("foreign_import func1,");
    front_testdriver_new_main_source(&s);

    ck_test_fail_parse_file();
    struct info_msg errors[] = {
        TESTSUPPORT_INFOMSG_INIT_START(
            MESSAGE_SYNTAX_ERROR,
            "Expected an identifier or a function declaration at foreign_import statement",
            0, 20),
        TESTSUPPORT_INFOMSG_INIT_START(
            MESSAGE_SYNTAX_ERROR,
            "Expected an outermost statement",
            0, 15),
    };
    ck_assert_parser_errors(errors);

} END_TEST

Suite *parser_misc_suite_create(void)
{
    Suite *s = suite_create("parser_misc");

    TCase *tc1 = tcase_create("parser_misc_nodes");
    tcase_add_checked_fixture(tc1, setup_front_tests, teardown_front_tests);
    tcase_add_test(tc1, test_acc_string_literals);
    tcase_add_test(tc1, test_acc_boolean_constants);
    tcase_add_test(tc1, test_acc_import_statements);

    tcase_add_test(tc1, test_acc_import_statements_fail1);
    tcase_add_test(tc1, test_acc_import_statements_fail2);

    suite_add_tcase(s, tc1);
    return s;
}
