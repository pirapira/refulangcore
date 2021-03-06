#ifndef LFR_IR_TYPE_H
#define LFR_IR_TYPE_H

#include <stdbool.h>
#include <Definitions/inline.h>
#include <types/type_decls.h>

struct rir_ctx;
struct rir_value;

enum rir_type_category {
    RIR_TYPE_ELEMENTARY,
    RIR_TYPE_COMPOSITE,
};

//! Represents a leaf type in the IR. Essentially a much simpler form of type
struct rir_type {
    enum rir_type_category category;
    bool is_pointer;
    union {
        //! Elementary type
        enum elementary_type etype;
        //! Composite type
        const struct rir_typedef *tdef;
    };
};

i_INLINE_DECL bool rir_type_is_elementary(const struct rir_type *t)
{
    return t->category == RIR_TYPE_ELEMENTARY;
}

i_INLINE_DECL bool rir_type_is_specific_elementary(const struct rir_type *t,
                                                   enum elementary_type etype)
{
    return t->category == RIR_TYPE_ELEMENTARY && t->etype == etype;
}

i_INLINE_DECL bool rir_type_is_composite(const struct rir_type *t)
{
    return t->category == RIR_TYPE_COMPOSITE;
}
bool rir_type_is_union(const struct rir_type *t);

void rir_type_elem_init(struct rir_type *t, enum elementary_type etype);
struct rir_type *rir_type_elem_create(enum elementary_type etype, bool is_pointer);
struct rir_type *rir_type_elem_create_from_string(const struct RFstring *name, bool is_pointer);
struct rir_type *rir_type_comp_create(const struct rir_typedef *def, bool is_pointer);
struct rir_type *rir_type_create_from_type(const struct type *t, struct rir_ctx *ctx);
void rir_type_comp_init(struct rir_type *t, const struct rir_typedef *def, bool is_pointer);

struct rir_type *rir_type_copy_from_other(const struct rir_type *other);
struct rir_type *rir_type_create_from_other(const struct rir_type *other, bool is_pointer);

/**
 * @return true if two types are equal. Does not take pointers into account
 */
bool rir_type_equal(const struct rir_type *a, const struct rir_type *b);
/**
 * @return true if two types are equal and are also both pointers or not
 */
bool rir_type_identical(const struct rir_type *a, const struct rir_type *b);
size_t rir_type_bytesize(const struct rir_type *a);

/**
 * Create a temporary string representation of the type
 *
 * @note This call needs to be enclosed in RFS_PUSH/RFS_POP
 *
 * @param t         The type whose representation to get
 * @return          A string representing @a t
 */
const struct RFstring *rir_type_string(const struct rir_type *t);

const struct rir_type *rir_type_comp_member_type(const struct rir_type *t, uint32_t idx);
int rir_type_union_matched_type_from_fncall(const struct rir_type *t, const struct ast_node *n, struct rir_ctx *ctx);

void rir_type_destroy(struct rir_type *t);
#endif
