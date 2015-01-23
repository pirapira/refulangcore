#ifndef LFR_TYPES_ELEMENTARY_H
#define LFR_TYPES_ELEMENTARY_H

#include <Definitions/inline.h>
#include <Utils/sanity.h>

#include <types/type_decls.h>

struct type_comparison_ctx;

bool type_elementary_equals(const struct type_elementary *t1,
                         const struct type_elementary *t2,
                         struct type_comparison_ctx *ctx);

/**
 * Given a built-in type value, returns the type itself
 */
const struct type *type_elementary_get_type(enum elementary_type etype);

/**
 * Given a built-in type value return the type string representation
 */
const struct RFstring *type_elementary_get_str(enum elementary_type etype);

/**
 * Check if @c id is an elementary identifier
 */
int type_elementary_identifier_p(const struct RFstring *id);

/**
 * Gets the elementary type of a type provided it is indeed an elementary type
 */
i_INLINE_DECL enum elementary_type type_elementary(struct type *t)
{
    RF_ASSERT(t->category == TYPE_CATEGORY_ELEMENTARY,
              "Non built-in type category detected");
    return t->elementary.etype;
}

#endif