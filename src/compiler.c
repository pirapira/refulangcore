#include "compiler.h"

#include <refu.h>
#include <Utils/memory.h>

#include <info/info.h>
#include <compiler_args.h>
#include <front_ctx.h>
#include <ir/rir.h>
#include <backend/llvm.h>

struct rir_module;

bool compiler_init(struct compiler *c)
{
    RF_STRUCT_ZERO(c);

    // initialize Refu library
    rf_init(LOG_TARGET_STDOUT, NULL, LOG_WARNING);

    // initialize an error buffer string
    if (!rf_stringx_init_buff(&c->err_buff, 1024, "")) {
        return false;
    }

    if (!(c->args = compiler_args_create())) {
        return false;
    }

    return true;
}

void compiler_deinit(struct compiler *c)
{
    if (c->front) {
        front_ctx_destroy(c->front);
    }

    if (c->ir) {
        rir_destroy(c->ir);
    }

    compiler_args_destroy(c->args);
    rf_stringx_deinit(&c->err_buff);
    rf_deinit();
}

bool compiler_pass_args(struct compiler *c, int argc, char **argv)
{
    if (!compiler_args_parse(c->args, argc, argv)) {
        RF_ERROR("Failed to parse command line arguments");
        return false;
    }

    // do not proceed any further if we got request for help
    if (c->args->help_requested != HELP_NONE) {
        return true;
    }

    if (!(c->front = front_ctx_create(c->args))) {
        RF_ERROR("Failure at frontend context initialization");
        return false;
    }

    return true;
}

bool compiler_init_with_args(struct compiler *c, int argc, char **argv)
{
    if (!compiler_init(c)) {
        return false;
    }

    return compiler_pass_args(c, argc, argv);
}

bool compiler_process(struct compiler *c)
{
    struct analyzer *analyzer = front_ctx_process(c->front);
    if (!analyzer) {
        RF_ERROR("Failure to parse and analyze the input");

        // for now temporarily just dump all messages in the info context
        // TODO: fix
        if (!info_ctx_get_messages_fmt(c->front->info, MESSAGE_ANY, &c->err_buff)) {
            RF_ERROR("Could not retrieve messages from the info context");
            return false;
        }
        printf(RF_STR_PF_FMT, RF_STR_PF_ARG(&c->err_buff));
        return false;
    }

    // create the intermediate representation from the analyzer and free analyzer
    c->ir = rir_create(analyzer);
    if (!c->ir) {
        RF_ERROR("Could not initialize the intermediate representation");
        return false;
    }

    struct rir_module *rir_mod = rir_process(c->ir);
    if (!rir_mod) {
        RF_ERROR("Failed to create the Refu IR");
        return false;
    }

    // TODO -- also think when should the AST be serialized to a file (?)
    // some argument maybe which would signify we need to transfer the processed
    // program to another computer
    /* if (!serializer_serialize_file(c->serializer, c->ir)) { */
    /*     return NULL; */
    /* } */

    if (!backend_llvm_generate(rir_mod, c->args)) {
        RF_ERROR("Failed to create the LLVM IR from the Refu IR");
        return false;
    }

    return true;
}

bool compiler_help_requested(struct compiler *c)
{
    switch(c->args->help_requested) {
    case HELP_ARGS:
        compiler_args_print_help();
        return true;
    case HELP_VERSION:
        compiler_args_print_version();
        return true;
    default:
        return false;
    }
}
