#include <lexer/tokens.h>

#include <String/rf_str_core.h>
#include <Utils/build_assert.h>

static struct RFstring strings_[] = {
    RF_STRING_STATIC_INIT("identifier"),
    RF_STRING_STATIC_INIT("numeric"),

    /* keywords */
    RF_STRING_STATIC_INIT("const"),
    RF_STRING_STATIC_INIT("type"),
    RF_STRING_STATIC_INIT("fn"),
    RF_STRING_STATIC_INIT("if"),
    RF_STRING_STATIC_INIT("elif"),
    RF_STRING_STATIC_INIT("else"),

    /* symbols */
    RF_STRING_STATIC_INIT(":"),
    RF_STRING_STATIC_INIT("{"),
    RF_STRING_STATIC_INIT("}"),
    RF_STRING_STATIC_INIT("("),
    RF_STRING_STATIC_INIT(")"),

    /* operators */
    RF_STRING_STATIC_INIT("+"),
    RF_STRING_STATIC_INIT("-"),
    RF_STRING_STATIC_INIT("*"),
    RF_STRING_STATIC_INIT("/"),

    RF_STRING_STATIC_INIT("++"),
    RF_STRING_STATIC_INIT("--"),
    RF_STRING_STATIC_INIT("="),

    RF_STRING_STATIC_INIT("=="),
    RF_STRING_STATIC_INIT("!="),
    RF_STRING_STATIC_INIT(">"),
    RF_STRING_STATIC_INIT(">="),
    RF_STRING_STATIC_INIT("<"),
    RF_STRING_STATIC_INIT("<="),

    RF_STRING_STATIC_INIT("|"),
    RF_STRING_STATIC_INIT(","),
    RF_STRING_STATIC_INIT("->"),

    RF_STRING_STATIC_INIT("&&"),
    RF_STRING_STATIC_INIT("||"),
};

const struct RFstring *tokentype_to_str(enum token_type type)
{
    BUILD_ASSERT(sizeof(strings_) / sizeof(struct RFstring) == TOKENS_MAX);
    
    return &strings_[type];
}
