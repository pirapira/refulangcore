#include <lexer/lexer.h>
#include <inpfile.h>

#include <ast/constants.h>
#include <ast/string_literal.h>

#include <lexer/tokens.h>
#include "tokens_htable.h" /* include the gperf generated hash table */
#include "common.h"

static struct inplocation_mark i_file_start_loc_ = LOCMARK_INIT_ZERO();

static inline bool token_init(struct token *t,
                              enum token_type type,
                              struct inpfile *f,
                              char *sp, char *ep)
{
    t->type = type;
    if (!inplocation_init(&t->location, f, sp, ep)) {
        return false;
    }
    return true;
}

static inline bool token_init_identifier(struct token *t,
                                         struct inpfile *f,
                                         char *sp, char *ep)
{
    if (!token_init(t, TOKEN_IDENTIFIER, f, sp, ep)) {
        return false;
    }
    t->value.v = ast_identifier_create(&t->location);
    t->value.owned_by_lexer = true;
    if (!t->value.v) {
        return false;
    }
    return true;
}

static inline bool token_init_constant_int(struct token *t,
                                           struct inpfile *f,
                                           char *sp, char *ep,
                                           uint64_t value)
{
    if (!token_init(t, TOKEN_CONSTANT_INTEGER, f, sp, ep)) {
        return false;
    }
    t->value.v = ast_constant_create_integer(&t->location, value);
    t->value.owned_by_lexer = true;
    if (!t->value.v) {
        return false;
    }
    return true;
}

static inline bool token_init_constant_float(struct token *t,
                                             struct inpfile *f,
                                             char *sp, char *ep,
                                             double value)
{
    if (!token_init(t, TOKEN_CONSTANT_FLOAT, f, sp, ep)) {
        return false;
    }
    t->value.v = ast_constant_create_float(&t->location, value);
    t->value.owned_by_lexer = true;
    if (!t->value.v) {
        return false;
    }
    return true;
}

static inline bool token_init_string_literal(struct token *t,
                                             struct inpfile *f,
                                             char *sp, char *ep)
{
    if (!token_init(t, TOKEN_STRING_LITERAL, f, sp, ep)) {
        return false;
    }
    t->value.v = ast_string_literal_create(&t->location);
    t->value.owned_by_lexer = true;
    if (!t->value.v) {
        return false;
    }
    return true;
}

bool lexer_init(struct lexer *l, struct inpfile *f, struct info_ctx *info)
{
    darray_init(l->tokens);
    darray_init(l->indices);
    l->tok_index = 0;
    l->file = f;
    l->info = info;
    l->at_eof = false;
    return true;
}

struct lexer *lexer_create(struct inpfile *f, struct info_ctx *info)
{
    struct lexer *ret;
    RF_MALLOC(ret, sizeof(*ret), NULL);
    if (!lexer_init(ret, f, info)) {
        free(ret);
        return NULL;
    }
    return ret;
}

void lexer_deinit(struct lexer *l)
{
    struct token *tok;

    darray_foreach(tok, l->tokens) {
        if (token_has_value(tok) && tok->value.owned_by_lexer) {
            ast_node_destroy_from_lexer(tok->value.v);
        }
    }

    darray_free(l->tokens);
    darray_free(l->indices);
}

void lexer_destroy(struct lexer *l)
{
    lexer_deinit(l);
    free(l);
}

static const struct inplocation_mark *lexer_get_last_token_loc_start(struct lexer *l)
{
    if (darray_empty(l->tokens)) {
        return &i_file_start_loc_;
    }
    return token_get_start(&darray_top(l->tokens));
}

static bool lexer_add_token(struct lexer *l, enum token_type type,
                            char *sp, char* ep)
{
    darray_resize(l->tokens, l->tokens.size + 1);
    if (!token_init(&darray_top(l->tokens), type, l->file, sp, ep)) {
        return false;
    }

    return true;
}

static bool lexer_add_token_identifier(struct lexer *l,
                                       char *sp, char* ep)
{
    darray_resize(l->tokens, l->tokens.size + 1);
    if (!token_init_identifier(&darray_top(l->tokens), l->file, sp, ep)) {
        return false;
    }

    return true;
}

static bool lexer_add_token_constant_int(struct lexer *l,
                                         char *sp, char* ep,
                                         uint64_t v)
{
    darray_resize(l->tokens, l->tokens.size + 1);
    if (!token_init_constant_int(&darray_top(l->tokens), l->file, sp, ep, v)) {
        return false;
    }

    return true;
}

static bool lexer_add_token_constant_float(struct lexer *l,
                                         char *sp, char* ep,
                                         double v)
{
    darray_resize(l->tokens, l->tokens.size + 1);
    if (!token_init_constant_float(&darray_top(l->tokens), l->file, sp, ep, v)) {
        return false;
    }

    return true;
}

static bool lexer_add_token_string_literal(struct lexer *l,
                                           char *sp, char* ep)
{
    darray_resize(l->tokens, l->tokens.size + 1);
    if (!token_init_string_literal(&darray_top(l->tokens), l->file, sp, ep)) {
        return false;
    }

    return true;
}

static void lexer_get_dblslash_comment(struct lexer *l, char *p, char *lim, char **ret_p)
{
    // skip the first 2 '//'
    p += 2;
    // TODO: this assumes newline is always LF which depends on where we get our data from
    // If we use RFTextFile that is always gonna be true. If other ways we have to add
    // logic for different types of new line characters
    while (p < lim && *(p++) != '\n') {
    }
    *ret_p = p;
}

#define COND_IDENTIFIER_BEGIN(p_)               \
    (((p_) >= 'A' && (p_) <= 'Z') ||            \
     ((p_) >= 'a' && (p_) <= 'z') ||            \
     (p_) == '_')

#define COND_IDENTIFIER(p_)                     \
    (COND_IDENTIFIER_BEGIN(p_) ||               \
     ((p_) >= '0' && (p_) <= '9'))

static bool lexer_get_identifier(struct lexer *l, char *p,
                                 char *lim, char **ret_p)
{
    const struct internal_token *itoken;
    char *sp = p;
    while (p < lim) {
        if (COND_IDENTIFIER(*(p+1))) {
            p ++;
        } else {
            break;
        }
    }

    // check if it's a keyword
    itoken = lexer_lexeme_is_token(sp, p - sp + 1);
    if (itoken) {
        if (!lexer_add_token(l, itoken->type, sp, p)) {
            return false;
        }
    } else {
        // it's a normal identifier
        if (!lexer_add_token_identifier(l, sp, p)) {
            return false;
        }
    }

    if (p != lim) {
        *ret_p = p + 1;
    } else {
        *ret_p = p;
        l->at_eof = true;
    }

    return true;
}

#define COND_NUMERIC(p_)                        \
    ((p_) >= '0' && (p_) <= '9')

#define COND_WS(p_)                                               \
    ((p_) == ' ' || (p_) == '\t' || (p_) == '\n' || (p_) == '\r')
static bool lexer_get_constant_int_or_float(struct lexer *l, char *p,
                                            char *lim, bool negative,
                                            char **ret_p)
{
    struct RFstring tmps;
    double float_val;
    uint64_t int_val;
    size_t len;
    bool is_float = false;
    char *sp = negative ? p - 1 : p;

    while (p < lim) {
        if (COND_NUMERIC(*p) && *(p + 1) == '.') {
            is_float = true;
            p ++;
        } else if (COND_WS(*(p + 1))) {
            break;
        } else {
            p ++;
        }
    }

    RF_STRING_SHALLOW_INIT(&tmps, sp, p - sp + 1);
    if (is_float) {
        if (!rf_string_to_double(&tmps, &float_val, &len)) {
            return false;
        }
        p = sp + len;
        if (!lexer_add_token_constant_float(l, sp, p, float_val)) {
            return false;
        }
    } else {
        if (!rf_string_to_uint_dec(&tmps, &int_val, &len)) {
            return false;
        }
        p = sp + len;
        if (!lexer_add_token_constant_int(l, sp, p, int_val)) {
            return false;
        }
    }

    if (p != lim) {
        *ret_p = p + 1;
    } else {
        *ret_p = p;
        l->at_eof = true;
    }

    return true;
}

static bool lexer_get_constant_int(
    struct lexer *l, char *p,
    char *lim, char **ret_p,
    bool negative,
    bool (*conv_fun)(const struct RFstring*, uint64_t*, size_t *))
{
    struct RFstring tmps;
    uint64_t int_val;
    size_t len;
    char *sp = negative ? p - 1 : p;

    while (p < lim) {
        if (COND_WS(*(p+1))) {
            break;
        } else {
            p ++;
        }
    }

    RF_STRING_SHALLOW_INIT(&tmps, sp, p - sp + 1);
    if (!conv_fun(&tmps, &int_val, &len)) {
            return false;
    }
    p = sp + len;
    if (!lexer_add_token_constant_int(l, sp, p, int_val)) {
            return false;
    }

    if (p != lim) {
        *ret_p = p + 1;
    } else {
        *ret_p = p;
        l->at_eof = true;
    }

    return true;
}

static bool lexer_get_numeric(struct lexer *l, char *p,
                              char *lim, bool negative, char **ret_p)
{
    char *sp = p;

    if (sp < lim) {
        char *sp_1 = p + 1;
        if (*sp == '0' && *sp_1 != '.' && !COND_WS(*sp_1)) {
                if (*sp_1 == 'x') {
                    return lexer_get_constant_int(l, p, lim, ret_p, negative,
                                                  rf_string_to_uint_hex);
                } else if (*sp_1 == 'b') {
                    return lexer_get_constant_int(l, p, lim, ret_p, negative,
                                                   rf_string_to_uint_bin);
                }
                // else oct
                return lexer_get_constant_int(l, p, lim, ret_p, negative,
                                              rf_string_to_uint_oct);
        }
    }
    return lexer_get_constant_int_or_float(l, p, lim, negative, ret_p);
}

static bool lexer_get_string_literal(struct lexer *l, char *p,
                                     char *lim, char **ret_p)
{
    char *sp = p;
    struct RFstring tmps;
    struct RFstring_iterator it;
    uint32_t val;
    uint32_t pr_val = 0;
    bool end_found = false;
    RF_STRING_SHALLOW_INIT(&tmps, sp, lim - sp + 1);

    // iterate as an RFString since we may have Unicode characters inside
    rf_string_get_iter(&tmps, &it);

    while (rf_string_iterator_next(&it, &val)) {
        if (val == '"' && it.character_pos != 0 && pr_val != '\\') {
            end_found = true;
            break;
        }
        pr_val = val;
    }
    if (!end_found) {
        lexer_synerr(
            l, lexer_get_last_token_loc_start(l),
            NULL,
            "Unterminated string literal detected");
        return false;
    }
    p = it.p - 1;

    if (!lexer_add_token_string_literal(l, sp, p)) {
            return false;
    }

    if (p != lim) {
        *ret_p = p + 1;
    } else {
        *ret_p = p;
        l->at_eof = true;
    }

    return true;
}

#define COND_TOKEN_AMBIG1(p_)                     \
    ((p_) == '+' || (p_) == '-' || (p_) == '>' || \
     (p_) == '<' || (p_) == '|' || (p_) == '=' || \
     (p_) == '&')

bool lexer_scan(struct lexer *l)
{
    char *lim;
    char *sp;
    char *p;

    sp = p = inpfile_sp(l->file);
    lim = sp + rf_string_length_bytes(inpfile_str(l->file)) - 1;

   /* TODO: combine inpfile_at_eof with lexer eof check */
    while (!l->at_eof && p <= lim) {
        inpfile_acc_ws(l->file);
        if (inpfile_at_eof(l->file)) {
            break;
        }
        p = inpfile_p(l->file);
        sp = p;

        if (COND_IDENTIFIER_BEGIN(*p)) {
            if (!lexer_get_identifier(l, p, lim, &p)) {
                lexer_synerr(
                    l, lexer_get_last_token_loc_start(l),
                    NULL,
                    "Failed to scan identifier");
                return false;
            }
        } else if (COND_NUMERIC(*p)) {
            if (!lexer_get_numeric(l, p, lim, false, &p)) {
                lexer_synerr(
                    l, lexer_get_last_token_loc_start(l),
                    NULL,
                    "Failed to scan numeric literal");
                return false;
            }
         // if it's the start of a comment
        } else if (p + 1 <= lim && *p == '/' && *(p + 1) == '/') {
            lexer_get_dblslash_comment(l, p, lim, &p);                        
        } else { // see if it's a token
            unsigned int len = 1;
            const struct internal_token *itoken;
            const struct internal_token *itoken2;
            bool got_token = false;
            char *toksp = p;

            while (len <= MAX_WORD_LENGTH) {
                itoken = lexer_lexeme_is_token(p, len);
                if (itoken) {                    
                    if (itoken->type == TOKEN_SM_DBLQUOTE) {
                        // if it's the start of a string literal
                        if (!lexer_get_string_literal(l, p, lim, &p)) {
                            lexer_synerr(
                                l, lexer_get_last_token_loc_start(l),
                                NULL,
                                "Failed to scan string literal");
                            return false;
                        }
                        got_token = true;
                        break;
                    } else if (itoken->type == TOKEN_OP_MINUS &&
                               p + 1 <= lim &&
                               COND_NUMERIC(*(p + 1))) {
                        // if it's a negative numeric literal
                        if (!lexer_get_numeric(l, p + 1, lim, true, &p)) {
                            lexer_synerr(
                                l, lexer_get_last_token_loc_start(l),
                                NULL,
                                "Failed to scan numeric literal");
                            return false;
                        }
                        got_token = true;
                        break;
                    } else if (p + 1 <= lim && COND_TOKEN_AMBIG1(*toksp)) {
                        // if more than 1 tokens may start with that character
                        len = 2;
                        itoken2 = lexer_lexeme_is_token(p, len);
                        if (itoken2) {
                            itoken = itoken2;
                        } else {
                            len = 1;
                        }
                    }
                    if (!lexer_add_token(l, itoken->type, toksp, p + len - 1)) {
                        RF_ERROR("Failed to add a new token");
                        return false;
                    }
                    p += len;
                    got_token=true;
                    break;
                }
                len ++;
            }
            if (!got_token) {
                // error unknown token
                lexer_synerr(l, lexer_get_last_token_loc_start(l), NULL,
                             "Unknown token encountered");
                return false;
            }
        }
        inpfile_move(l->file, p - sp, p - sp);
    }
    return true;
}

// convenience macro to check if a token index is out of bounds for the lexer
#define LEXER_IND_OOB(lex_, ind_)               \
    (ind_ >= darray_size((lex_)->tokens))

struct token *lexer_next_token(struct lexer *l)
{
    struct token *tok;
    if (LEXER_IND_OOB(l, l->tok_index)) {
        return NULL;
    }
    tok = &darray_item(l->tokens, l->tok_index);
    l->tok_index ++;
    return tok;
}

struct token *lexer_lookahead(struct lexer *l, unsigned int num)
{
    struct token *tok;
    unsigned int index = l->tok_index + num - 1;
    if (LEXER_IND_OOB(l, index)) {
        return NULL;
    }
    tok = &darray_item(l->tokens, index);
    return tok;
}

struct token *lexer_last_token_valid(struct lexer *l)
{
    if (LEXER_IND_OOB(l, l->tok_index)) {
        return &darray_item(l->tokens, l->tok_index - 1);
    }
    return &darray_item(l->tokens, l->tok_index);
}

i_INLINE_INS struct token *lexer_expect_token(struct lexer *l, enum token_type type);
i_INLINE_INS struct inplocation *lexer_last_token_location(struct lexer *l);
i_INLINE_INS struct inplocation_mark *lexer_last_token_start(struct lexer *l);
i_INLINE_INS struct inplocation_mark *lexer_last_token_end(struct lexer *l);
i_INLINE_INS void lexer_inject_input_file(struct lexer *l, struct inpfile *f);

void lexer_push(struct lexer *l)
{
    darray_append(l->indices, l->tok_index);
}

void lexer_pop(struct lexer *l)
{
    RF_ASSERT(!darray_empty(l->indices), "lexer_pop called with empty array");
    (void)darray_pop(l->indices);
}

void lexer_rollback(struct lexer *l)
{
    unsigned int idx;
    unsigned int i;
    struct token *tok;
    RF_ASSERT(!darray_empty(l->indices),
              "lexer_rollback called with empty array");
    idx = darray_pop(l->indices);
    RF_ASSERT(l->tok_index >= idx, "asked to rollback to a token ahead of us?");
    // make sure that all value tokens in between now and rollback belong to the lexer
    i = darray_size(l->tokens) - 1;
    darray_foreach_reverse(tok, l->tokens) {
        if (i <= l->tok_index && i >= idx && token_has_value(tok)) {
            tok->value.owned_by_lexer = true;
            tok->value.v->state = AST_NODE_STATE_CREATED;
        }
        --i;
    }
    // set new token index
    l->tok_index = idx;
}
