#include <info/info.h>

#include <Utils/bits.h>

#include <info/msg.h>
#include <compiler_args.h>
#include <inplocation.h>
#include <inpfile.h>


#define INFO_CTX_BUFF_SIZE 512 //TODO: move somewhere else

struct info_ctx *info_ctx_create(struct inpfile *f)
{
    struct info_ctx *ctx;

    RF_MALLOC(ctx, sizeof(*ctx), return NULL);
    rf_ilist_head_init(&ctx->msg_list);
    if (!rf_stringx_init_buff(&ctx->buff, INFO_CTX_BUFF_SIZE, "")) {
        free(ctx);
        return NULL;
    }
    ctx->syntax_error = false;
    ctx->file = f;

    return ctx;
}

void info_ctx_destroy(struct info_ctx *ctx)
{
    struct info_msg *m;
    struct info_msg *tmp;

    rf_ilist_for_each_safe(&ctx->msg_list, m, tmp, ln) {
        info_msg_destroy(m);
    }
    rf_stringx_deinit(&ctx->buff);
    free(ctx);
}

void info_print_cond(int vlevel, const char *fmt, ...)
{
    struct compiler_args *cargs = compiler_args_get();
    if (cargs->verbose_level >= vlevel) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
}


bool i_info_ctx_add_msg(struct info_ctx *ctx,
                        enum info_msg_type type,
                        struct inplocation *loc,
                        const char *fmt,
                        ...)
{
    va_list args;
    struct info_msg *msg;

    va_start(args, fmt);
    msg = info_msg_create(type, loc, fmt, args);
    va_end(args);

    if (!msg) {
        return false;
    }

    rf_ilist_add_tail(&ctx->msg_list, &msg->ln);
    return true;
}

bool info_ctx_has(struct info_ctx *ctx, enum info_msg_type type)
{
    struct info_msg *m;
    if (type == MESSAGE_ANY) {
        return !rf_ilist_is_empty(&ctx->msg_list);
    }

    rf_ilist_for_each(&ctx->msg_list, m, ln) {
        if (RF_BITFLAG_ON(type, m->type)) {
            return true;
        }
    }
    return false;
}

void info_ctx_flush(struct info_ctx *ctx, FILE *f, int type)
{
    struct info_msg *m;
    struct info_msg *tmp;

    rf_ilist_for_each_safe(&ctx->msg_list, m, tmp, ln) {
        if (RF_BITFLAG_ON(type, MESSAGE_ANY) ||
            RF_BITFLAG_ON(type, m->type)) {

            info_msg_print(m, f, ctx->file);
            rf_ilist_delete_from(&ctx->msg_list, &m->ln);
            info_msg_destroy(m);
        }
    }
}

bool info_ctx_get_messages_fmt(struct info_ctx *ctx,
                               enum info_msg_type type,
                               struct RFstringx *str)
{
    struct info_msg *m;
    if (rf_ilist_is_empty(&ctx->msg_list)) {
        return false;
    }
    rf_ilist_for_each(&ctx->msg_list, m, ln) {
        if (RF_BITFLAG_ON(type, MESSAGE_ANY) ||
            RF_BITFLAG_ON(type, m->type)) {
            if (!info_msg_get_formatted(m, str, ctx->file)) {
                return false;
            }
            rf_stringx_move_end(str);
        }
    }
    rf_stringx_reset(str);
    return true;
}


void info_ctx_get_iter(struct info_ctx *ctx,
                       enum info_msg_type types,
                       struct info_ctx_msg_iterator *iter)
{
    iter->msg_types = types;
    iter->start = &ctx->msg_list.n;
    iter->next = ctx->msg_list.n.next;
}

struct info_msg *info_ctx_msg_iterator_next(struct info_ctx_msg_iterator *it)
{
    struct info_msg *msg;
    while (it->next != it->start) {
        msg = rf_ilist_entry(it->next, struct info_msg, ln);
        it->next = it->next->next;
        if (msg->type & it->msg_types) {
            return msg;
        }
    }
    return NULL;
}
