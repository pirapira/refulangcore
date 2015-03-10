#ifndef LFR_TESTSUPPORT_PARSER_H
#define LFR_TESTSUPPORT_PARSER_H

#include <stdbool.h>
#include <check.h>
#include <Preprocessor/rf_xmacro_argcount.h>

struct inpfile;

/**
 * A utility testing macro to generate an ast_node whose _create() accepts
 * a start and end location mark
 */
#define testsupport_parser_node_create(...)                             \
    RF_SELECT_FUNC_IF_NARGGT(i_testsupport_parser_node_create, 7, __VA_ARGS__) \

#define i_testsupport_parser_node_create1(node_, type_,                 \
                                          file_, sl_, sc_, el_, ec_, ...) \
    struct ast_node *node_;                                             \
    do {                                                                \
        struct inplocation temp_location_ = LOC_INIT(file_, sl_, sc_, el_, ec_); \
        node_ = ast_##type_##_create(&temp_location_.start,\
                                     &temp_location_.end, __VA_ARGS__);  \
        node_->owner = AST_OWNEDBY_PARSER;                              \
    } while(0)

#define i_testsupport_parser_node_create0(node_, type_,                 \
                                          file_, sl_, sc_, el_, ec_)    \
    struct ast_node *node_;                                             \
    do {                                                                \
        struct inplocation temp_location_ = LOC_INIT(file_, sl_, sc_, el_, ec_); \
        node_ = ast_##type_##_create(&temp_location_.start,             \
                                     &temp_location_.end);              \
        node_->owner = AST_OWNEDBY_PARSER;                              \
    } while(0)

/**
 * A utility testing macro to generate a constant number at a location
 */
#define testsupport_parser_constant_create(node_, file_,                \
                                           sl_, sc_, el_,               \
                                           ec_, type_, value_)          \
    struct ast_node *node_;                                             \
    do {                                                                \
        struct inplocation temp_location_ = LOC_INIT(file_, sl_, sc_, el_, ec_); \
        node_ = ast_constantnum_create_##type_(&temp_location_, value_); \
        node_->owner = AST_OWNEDBY_PARSER;                              \
    } while (0)

/**
 * A utility testing macro to generate a string literal at a location
 */
#define testsupport_parser_string_literal_create(node_, file_,          \
                                                 sl_, sc_, el_,         \
                                                 ec_)                   \
    struct ast_node *node_;                                             \
    do {                                                                \
        struct inplocation temp_location_ = LOC_INIT(file_, sl_, sc_, el_, ec_); \
        node_ = ast_string_literal_create(&temp_location_);             \
        node_->owner = AST_OWNEDBY_PARSER;                              \
    } while (0)

/**
 * A utility testing macro to generate a block at a location
 */
#define testsupport_parser_block_create(node_, file_,                   \
                                        sl_, sc_, el_,                  \
                                        ec_)                            \
        struct ast_node *node_;                                         \
        do {                                                            \
            struct inplocation temp_location_ = LOC_INIT(file_, sl_, sc_, el_, ec_); \
            node_ = ast_block_create();                                 \
            ast_node_set_start(node_, &temp_location_.start);           \
            ast_node_set_end(node_, &temp_location_.end);               \
            node_->owner = AST_OWNEDBY_PARSER;                          \
        } while (0)

/**
 * A utility testing function to generate an identifier at a location
 */
struct ast_node *testsupport_parser_identifier_create(struct inpfile *file,
                                                      unsigned int sline,
                                                      unsigned int scol,
                                                      unsigned int eline,
                                                      unsigned int ecol);

/**
 * A utility test macro to help create an xidentifier node wrapped over
 * a simple identifier
 */
#define testsupport_parser_xidentifier_create_simple(node_, file_,             \
                                              sl_, sc_, el_, ec_)       \
    testsupport_parser_node_create(node_, xidentifier, file_,           \
                                   sl_, sc_, el_, ec_,                  \
                                   testsupport_parser_identifier_create( \
                                       file_, sl_, sc_, el_, ec_),      \
                                   false, NULL)

#define testsupport_parser_prepare(driver_)                     \
    do {                                                        \
        ck_assert(lexer_scan((driver_)->front.lexer));          \
    } while (0)


/**
 * A utility testing macro used to test if the parser succesfully does an
 * accept.
 */
#define ck_test_parse_as(...) \
    RF_SELECT_FUNC_IF_NARGGT(i_ck_test_parse_as, 5, __VA_ARGS__)

#define i_ck_test_parse_as1(node_, type_, driver_,  node_name, target_, ...) \
    do {                                                                \
            testsupport_parser_prepare(driver_);                        \
            node_ = parser_acc_##type_((driver_)->front.parser, __VA_ARGS__); \
            ck_assert_parsed_node(node_, driver_, "Could not parse "node_name); \
            check_ast_match(n, target_, (driver_)->front.file);        \
            parser_finalize_parsing(node_);                            \
        } while (0)

#define i_ck_test_parse_as0(node_, type_, driver_,  node_name, target_)    \
        do {                                                            \
            testsupport_parser_prepare(driver_);                        \
            node_ = parser_acc_##type_((driver_)->front.parser);        \
            ck_assert_parsed_node(node_, driver_, "Could not parse "node_name); \
            check_ast_match(n, target_, (driver_)->front.file);        \
            parser_finalize_parsing(node_);                            \
        } while (0)



#define ck_assert_parsed_node(n_, d_, msg_)                             \
        do {                                                            \
            if (!(n_)) {                                                \
                struct RFstringx *tmp_ = front_testdriver_geterrors(d_); \
                if (tmp_) {                                             \
                    ck_abort_msg(msg_" -- with parser errors\n"RF_STR_PF_FMT, \
                                 RF_STR_PF_ARG(tmp_));                  \
                } else {                                                \
                    ck_abort_msg(msg_" -- with no parser errors");      \
                }                                                       \
            }                                                           \
        } while(0)

#define ck_assert_parser_errors(info_, expected_arr_)                   \
        ck_assert_parser_errors_impl(                                   \
            info_,                                                      \
            expected_arr_,                                              \
            sizeof(expected_arr_)/sizeof(struct info_msg),              \
            __FILE__, __LINE__)

struct info_msg;
struct info_ctx;
bool ck_assert_parser_errors_impl(struct info_ctx *info,
                                  struct info_msg *errors,
                                  unsigned num,
                                  const char *filename,
                                  unsigned int line);
#endif
