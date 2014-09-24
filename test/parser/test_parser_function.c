#include <check.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <String/rf_str_core.h>
#include <parser/parser.h>
#include "../../src/parser/recursive_descent/function.h"
#include <ast/function.h>
#include <ast/type.h>
#include <ast/generics.h>
#include <lexer/lexer.h>
#include <info/msg.h>

#include "../testsupport_front.h"
#include "testsupport_parser.h"

#include CLIB_TEST_HELPERS

START_TEST(test_acc_fndecl_1) {
    struct ast_node *n;
    struct inpfile *file;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "fn dosth(a:i32) -> i32");
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);
    file = &d->front.file;

    struct ast_node *name = testsupport_parser_identifier_create(file,
                                                                0, 3, 0, 7);
    struct ast_node *id1 = testsupport_parser_identifier_create(file,
                                                                0, 9, 0, 9);
    testsupport_parser_xidentifier_create_simple(id2, file, 0, 11, 0, 13);
    testsupport_parser_node_create(t1, typedesc, file, 0, 9, 0, 13, id1, id2);
    testsupport_parser_xidentifier_create_simple(id3, file, 0, 19, 0, 21);
    testsupport_parser_node_create(fn, fndecl, file, 0, 0, 0, 21,
                                   name,
                                   NULL,
                                   t1,
                                   id3
    );

    ck_test_parse_as(n, fndecl, d, "function", fn);

    ast_node_destroy(n);
    ast_node_destroy(fn);
}END_TEST

START_TEST(test_acc_fndecl_2) {
    struct ast_node *n;
    struct inpfile *file;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "fn dosth(a:i32, b:string) -> i32|nil");
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);
    file = &d->front.file;

    struct ast_node *name = testsupport_parser_identifier_create(file,
                                                                0, 3, 0, 7);
    struct ast_node *id1 = testsupport_parser_identifier_create(file,
                                                                0, 9, 0, 9);
    testsupport_parser_xidentifier_create_simple(id2, file, 0, 11, 0, 13);
    testsupport_parser_node_create(t1, typedesc, file, 0, 9, 0, 13, id1, id2);
    struct ast_node *id3 = testsupport_parser_identifier_create(file,
                                                                0, 16, 0, 16);
    testsupport_parser_xidentifier_create_simple(id4, file, 0, 18, 0, 23);
    testsupport_parser_node_create(t2, typedesc, file, 0, 16, 0, 23, id3, id4);
    testsupport_parser_node_create(op1, typeop, file, 0, 9, 0, 23,
                                   TYPEOP_PRODUCT, t1, t2);

    testsupport_parser_xidentifier_create_simple(id5, file, 0, 29, 0, 31);
    testsupport_parser_xidentifier_create_simple(id6, file, 0, 33, 0, 35);
    testsupport_parser_node_create(op2, typeop, file, 0, 29, 0, 35,
                                   TYPEOP_SUM, id5, id6);

    testsupport_parser_node_create(fn, fndecl, file, 0, 0, 0, 35,
                                   name,
                                   NULL,
                                   op1,
                                   op2
    );

    ck_test_parse_as(n, fndecl, d, "function", fn);

    ast_node_destroy(n);
    ast_node_destroy(fn);
}END_TEST

START_TEST(test_acc_fndecl_void) {
    struct ast_node *n;
    struct inpfile *file;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "fn dosth_no_args()");
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);
    file = &d->front.file;

    struct ast_node *name = testsupport_parser_identifier_create(file,
                                                                0, 3, 0, 15);

    testsupport_parser_node_create(fn, fndecl, file, 0, 0, 0, 17,
                                   name,
                                   NULL,
                                   NULL,
                                   NULL
    );

    ck_test_parse_as(n, fndecl, d, "function", fn);

    ast_node_destroy(n);
    ast_node_destroy(fn);
}END_TEST

START_TEST(test_acc_fndecl_with_generics) {
    struct ast_node *n;
    struct inpfile *file;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "fn do_generic<Type a, Type b>(a:b, x:string) -> (r1:i32,r2:i8)");
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);
    file = &d->front.file;

    struct ast_node *name = testsupport_parser_identifier_create(file,
                                                                0, 3, 0, 12);

    struct ast_node *id1 = testsupport_parser_identifier_create(file,
                                                                0, 14, 0, 17);
    struct ast_node *id2 = testsupport_parser_identifier_create(file,
                                                                0, 19, 0, 19);
    struct ast_node *gtype1 = ast_genrtype_create(id1, id2);
    struct ast_node *id3 = testsupport_parser_identifier_create(file,
                                                                0, 22, 0, 25);
    struct ast_node *id4 = testsupport_parser_identifier_create(file,
                                                                0, 27, 0, 27);
    struct ast_node *gtype2 = ast_genrtype_create(id3, id4);
    testsupport_parser_node_create(genr, genrdecl, file, 0, 13, 0, 28);
    ast_node_add_child(genr, gtype1);
    ast_node_add_child(genr, gtype2);


    struct ast_node *id5 = testsupport_parser_identifier_create(file,
                                                                0, 30, 0, 30);
    testsupport_parser_xidentifier_create_simple(id6, file, 0, 32, 0, 32);
    testsupport_parser_node_create(t1, typedesc, file, 0, 30, 0, 32, id5, id6);
    struct ast_node *id7 = testsupport_parser_identifier_create(file,
                                                                0, 35, 0, 35);
    testsupport_parser_xidentifier_create_simple(id8, file, 0, 37, 0, 42);
    testsupport_parser_node_create(t2, typedesc, file, 0, 35, 0, 42, id7, id8);
    testsupport_parser_node_create(op1, typeop, file, 0, 30, 0, 42,
                                   TYPEOP_PRODUCT, t1, t2);

    struct ast_node *id9 = testsupport_parser_identifier_create(file,
                                                                0, 49, 0, 50);
    testsupport_parser_xidentifier_create_simple(id10, file, 0, 52, 0, 54);
    testsupport_parser_node_create(t3, typedesc, file, 0, 49, 0, 54, id9, id10);
    struct ast_node *id11 = testsupport_parser_identifier_create(file,
                                                                0, 56, 0, 57);
    testsupport_parser_xidentifier_create_simple(id12, file, 0, 59, 0, 60);
    testsupport_parser_node_create(t4, typedesc, file, 0, 56, 0, 60, id11, id12);
    testsupport_parser_node_create(op2, typeop, file, 0, 49, 0, 60,
                                   TYPEOP_PRODUCT, t3, t4);

    testsupport_parser_node_create(fn, fndecl, file, 0, 0, 0, 60,
                                   name,
                                   genr,
                                   op1,
                                   op2
    );

    ck_test_parse_as(n, fndecl, d, "function", fn);

    ast_node_destroy(n);
    ast_node_destroy(fn);
}END_TEST

START_TEST(test_acc_fndecl_err1) {
    struct ast_node *n;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "fn ()");
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);

    ck_assert(lexer_scan(d->front.lexer));
    n = parser_acc_fndecl(d->front.parser);
    ck_assert_msg(n == NULL, "parsing function declaration should fail");
    ck_assert_msg(
        parser_has_syntax_error(d->front.parser),
        "a syntax error should have been reported");

    struct info_msg errors[] = {
        TESTPARSER_MSG_INIT_START(
            &d->front.file,
            "Expected an identifier for the function name after 'fn'",
            0, 3)
    };
    ck_assert_parser_errors(d->front.info, errors);
}END_TEST

START_TEST(test_acc_fndecl_err2) {
    struct ast_node *n;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "fn dosth)");
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);

    ck_assert(lexer_scan(d->front.lexer));
    n = parser_acc_fndecl(d->front.parser);
    ck_assert_msg(n == NULL, "parsing function declaration should fail");
    ck_assert_msg(
        parser_has_syntax_error(d->front.parser),
        "a syntax error should have been reported");

    struct info_msg errors[] = {
        TESTPARSER_MSG_INIT_START(
            &d->front.file,
            "Expected '(' at function declaration",
            0, 8)
    };
    ck_assert_parser_errors(d->front.info, errors);
}END_TEST

START_TEST(test_acc_fndecl_err3) {
    struct ast_node *n;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "fn dosth(a:int");
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);

    ck_assert(lexer_scan(d->front.lexer));
    n = parser_acc_fndecl(d->front.parser);
    ck_assert_msg(n == NULL, "parsing function declaration should fail");
    ck_assert_msg(
        parser_has_syntax_error(d->front.parser),
        "a syntax error should have been reported");

    struct info_msg errors[] = {
        TESTPARSER_MSG_INIT_START(
            &d->front.file,
            "Expected ')' at function declaration after type description",
            0, 13)
    };
    ck_assert_parser_errors(d->front.info, errors);
}END_TEST

START_TEST(test_acc_fndecl_err4) {
    struct ast_node *n;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "fn dosth(a:int, )");
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);

    ck_assert(lexer_scan(d->front.lexer));
    n = parser_acc_fndecl(d->front.parser);
    ck_assert_msg(n == NULL, "parsing function declaration should fail");
    ck_assert_msg(
        parser_has_syntax_error(d->front.parser),
        "a syntax error should have been reported");

    struct info_msg errors[] = {
        TESTPARSER_MSG_INIT_START(
            &d->front.file,
            "Expected a '(' or identifier after ','",
            0, 14),
        TESTPARSER_MSG_INIT_START(
            &d->front.file,
            "Expected either a type description for the function's arguments "
            "or ')' after '('",
            0, 8),
    };
    ck_assert_parser_errors(d->front.info, errors);
}END_TEST

START_TEST(test_acc_fndecl_err5) {
    struct ast_node *n;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "fn dosth(a:int) ->");
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);

    ck_assert(lexer_scan(d->front.lexer));
    n = parser_acc_fndecl(d->front.parser);
    ck_assert_msg(n == NULL, "parsing function declaration should fail");
    ck_assert_msg(
        parser_has_syntax_error(d->front.parser),
        "a syntax error should have been reported");

    struct info_msg errors[] = {
        TESTPARSER_MSG_INIT_START(
            &d->front.file,
            "Expected type description for the function's return type after"
            " '->'",
            0, 17)
    };
    ck_assert_parser_errors(d->front.info, errors);
}END_TEST

START_TEST(test_acc_fndecl_err6) {
    struct ast_node *n;
    static const struct RFstring s = RF_STRING_STATIC_INIT(
        "fn dosth(a:int) -> (a:int, ");
    struct front_testdriver *d = get_front_testdriver();
    front_testdriver_assign(d, &s);

    ck_assert(lexer_scan(d->front.lexer));
    n = parser_acc_fndecl(d->front.parser);
    ck_assert_msg(n == NULL, "parsing function declaration should fail");
    ck_assert_msg(
        parser_has_syntax_error(d->front.parser),
        "a syntax error should have been reported");

    struct info_msg errors[] = {
        TESTPARSER_MSG_INIT_START(
            &d->front.file,
            "Expected a '(' or identifier after ','",
            0, 25),
        TESTPARSER_MSG_INIT_START(
            &d->front.file,
            "Expected a type description after '('",
            0, 19),
        TESTPARSER_MSG_INIT_START(
            &d->front.file,
            "Expected type description for the function's return type after"
            " '->'",
            0, 17)
    };
    ck_assert_parser_errors(d->front.info, errors);
}END_TEST


Suite *parser_function_suite_create(void)
{
    Suite *s = suite_create("parser_function");

    TCase *fp = tcase_create("parser_functiondecl_parsing");
    tcase_add_checked_fixture(fp, setup_front_tests, teardown_front_tests);
    tcase_add_test(fp, test_acc_fndecl_1);
    tcase_add_test(fp, test_acc_fndecl_2);
    tcase_add_test(fp, test_acc_fndecl_void);
    tcase_add_test(fp, test_acc_fndecl_with_generics);

    TCase *fpf = tcase_create("parser_functiondecl_parsing_failures");
    tcase_add_checked_fixture(fpf, setup_front_tests, teardown_front_tests);
    tcase_add_test(fpf, test_acc_fndecl_err1);
    tcase_add_test(fpf, test_acc_fndecl_err2);
    tcase_add_test(fpf, test_acc_fndecl_err3);
    tcase_add_test(fpf, test_acc_fndecl_err4);
    tcase_add_test(fpf, test_acc_fndecl_err5);
    tcase_add_test(fpf, test_acc_fndecl_err6);

    suite_add_tcase(s, fp);
    suite_add_tcase(s, fpf);
    return s;
}
