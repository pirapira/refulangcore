#ifndef LFR_LEXER_H
#define LFR_LEXER_H

#include <stdbool.h>

/* #include <Data_Structures/darray.h> */
#include <ast/identifier.h>

#include <lexer/tokens.h>


struct inpfile;

struct lexer {
    struct {darray(struct token);} tokens;
    struct {darray(int);} indices;
    unsigned int tok_index;
    struct inpfile *file;
    struct info_ctx *info;
    //! Denotes that the lexer has reached the end of its input
    bool at_eof;
};


bool lexer_init(struct lexer *l, struct inpfile *f, struct info_ctx *info);
struct lexer *lexer_create(struct inpfile *f, struct info_ctx *info);
void lexer_deinit(struct lexer *l);
void lexer_destroy(struct lexer *l);

bool lexer_scan(struct lexer *l);

struct token *lexer_next_token(struct lexer *l);
struct token *lexer_lookahead(struct lexer *l, unsigned int num);
struct token *lexer_last_token_valid(struct lexer *l);

/**
 * Consumes and returns the next token iff it's a token of @c type
*/
i_INLINE_DECL struct token *lexer_expect_token(struct lexer *l, enum token_type type)
{
    struct token *tok = lexer_lookahead(l, 1);
    return (tok && tok->type == type && lexer_next_token(l)) ? tok : NULL;
}

i_INLINE_DECL struct inplocation *lexer_last_token_location(struct lexer *l)
{
    return &(lexer_last_token_valid(l))->location;
}

i_INLINE_DECL struct inplocation_mark *lexer_last_token_start(struct lexer *l)
{
    return &(lexer_last_token_valid(l))->location.start;
}

i_INLINE_DECL struct inplocation_mark *lexer_last_token_end(struct lexer *l)
{
    return &(lexer_last_token_valid(l))->location.end;
}

i_INLINE_DECL void lexer_inject_input_file(struct lexer *l, struct inpfile *f)
{
    l->file = f;
}

void lexer_push(struct lexer *l);
void lexer_pop(struct lexer *l);
void lexer_rollback(struct lexer *l);

#endif
