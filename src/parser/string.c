#include <parser/string.h>

#include <Utils/sanity.h>

bool parser_string_init(struct parser_string *s,
                        struct RFstringx *input_str,
                        struct RFarray *arr,
                        unsigned int lines_num)
{
    RF_STRINGX_SHALLOW_COPY(&s->str, input_str);
    s->lines_num = lines_num;
    RF_MALLOC(s->lines, sizeof(uint32_t) * lines_num, false);
    memcpy(s->lines, arr->buff, sizeof(uint32_t) * lines_num);
    return true;
}

void parser_string_deinit(struct parser_string *s)
{
    rf_stringx_deinit(&s->str);
    free(s->lines);
}

bool parser_string_ptr_to_linecol(struct parser_string *s,
                                  char *p, unsigned int *line,
                                  unsigned int *col)
{
    uint32_t i;
    struct RFstring tmp;
    char *sp;
    bool found = false;
    char *sbeg = parser_string_beg(s);
    uint32_t off = p - sbeg;

    for (i = 0; i < s->lines_num - 1; i++) {
        if (off >= s->lines[i] && off < s->lines[i + 1]) {
        /* if (s->lines[i] >= off) { */
            found = true;
            break;
        }
    }
    if (!found) {
        if (off >= s->lines[s->lines_num - 1] &&
            off <= parser_string_len_from_beg(s)) {
            /* last line */
            i = s->lines_num - 1;
        } else {
            return false;
        }
    }

    /* *line = i + 1; */
    *line = i;
    sp = sbeg + s->lines[*line];
    RF_ASSERT(p - sp >= 0);

    RF_STRING_SHALLOW_INIT(&tmp, sp, p - sp);
    *col = rf_string_length(&tmp);

    return true;
}

i_INLINE_INS struct RFstringx *parser_string_str(struct parser_string *s);
i_INLINE_INS char *parser_string_data(struct parser_string *s);
i_INLINE_INS char *parser_string_beg(struct parser_string *s);
i_INLINE_INS uint32_t parser_string_len_from_beg(struct parser_string *s);
