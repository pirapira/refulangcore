#ifndef LFR_IR_TYPE_H
#define LFR_IR_TYPE_H

#include <Data_Structures/darray.h>
#include <Utils/struct_utils.h>
#include <types/type_decls.h>

struct type;
struct RFstring;
struct ast_node;
struct symbol_table;

//! Keep synced with @ref elementary_types at type_decls.h
enum rir_type_category {
    ELEMENTARY_RIR_TYPE_INT = 0,
    ELEMENTARY_RIR_TYPE_UINT,
    ELEMENTARY_RIR_TYPE_INT_8,
    ELEMENTARY_RIR_TYPE_UINT_8,
    ELEMENTARY_RIR_TYPE_INT_16,
    ELEMENTARY_RIR_TYPE_UINT_16,
    ELEMENTARY_RIR_TYPE_INT_32,
    ELEMENTARY_RIR_TYPE_UINT_32,
    ELEMENTARY_RIR_TYPE_INT_64,
    ELEMENTARY_RIR_TYPE_UINT_64,
    ELEMENTARY_RIR_TYPE_FLOAT_32,
    ELEMENTARY_RIR_TYPE_FLOAT_64,
    ELEMENTARY_RIR_TYPE_STRING,
    ELEMENTARY_RIR_TYPE_BOOL,
    ELEMENTARY_RIR_TYPE_NIL,

    COMPOSITE_PRODUCT_RIR_TYPE,
    COMPOSITE_SUM_RIR_TYPE,
    COMPOSITE_IMPLICATION_RIR_TYPE,
    COMPOSITE_RIR_DEFINED,

    RIR_TYPE_CATEGORY_COUNT
};

/**
 * Representation of a type for the Refu IR
 *
 * It is much like @see struct type but with some constraints.
 * It is represented by an array of subtypes and not by a tree. A single
 * rir_type can only contain subtypes that are connected by the same type
 * operation.
 *
 * TODO: In order to achieve this all possible composite types need to be defined
 * somewhere. Figure out where and do it.
 */
struct rir_type {
    enum rir_type_category category;
    //! Array of types that may constitute this type. e.g: i64, u32, string
    struct {darray(struct rir_type*);} subtypes;
    //! Name of the variable the type describes. TODO: Maybe move this somwhere
    //! else. Separate the notion of type from parameter?
    const struct RFstring *name;
    //! Control to input into the rir types list
    struct RFilist_node ln;
    //! Denotes if the type is indexed in the rir types list (or some other list)
    //! that takes care of destruction. If so then it's not destroyed when it's found
    //! as a child of another type. TODO: This is kind of ugly .. if possible fix
    bool indexed;
};

/**
 * Allocates a new rir_type equivalent of @c input.
 *
 * @param input                The type from which to allocate the rir_type
 * @return                     The allocated type
 */
struct rir_type *rir_type_alloc();
/**
 * Create a new allocated rir_type
 */
struct rir_type *rir_type_create(const struct type *input,
                                 const struct RFstring *name,
                                 struct rir_type **newly_created_type);
/**
 * Initialize a rir_type
 */
bool rir_type_init(struct rir_type *type, const struct type *input,
                   const struct RFstring *name,
                   struct rir_type **newly_created_type);
/**
 * Initialize a rir_type without proceeding to initialize its children
 *
 * @return If we need to iterate the children of @c input the function will
 *         return true. Else it will return false
 */
bool rir_type_init_before_iteration(struct rir_type *type,
                                    const struct type *input,
                                    const struct RFstring *name);

void rir_type_dealloc(struct rir_type *t);
void rir_type_destroy(struct rir_type *t);
void rir_type_deinit(struct rir_type *t);

/**
 * Returns the type_category of the rir type if it's an operator.
 * @param t    The rir type in questions
 * @return     The type category of the rir type if it's an operator.
 *             If it's not an operator it returns @c RIR_TYPE_CATEGORY_COUNT
 */
enum rir_type_category rir_type_op_from_rir_type(const struct rir_type *t);
/**
 * Returns the rir type op equivalent of the given normal type
 * @param t     A normal type whose rir type operator equivalent to get.
 *              Must belong to category @c TYPE_CATEGORY_OPERATOR
 * @return      The rir type op equivalent of the given normal type.
 */
enum rir_type_category rir_type_op_from_type(const struct type *t);

/**
 * Equality comparison for two rir types
 */
bool rir_type_equals(struct rir_type *a, struct rir_type *b);

/**
 * Equality comparison for a rir and a normal type and name combinarion
 *
 * @param a          The rir type to compare
 * @param b          The normal type to compare to the rir type
 * @param name       An optional name to check for along with the type @c b. Can be NULL
 * @return           True if they are equal and false otherwise
 */
bool rir_type_equals_type(struct rir_type *a, struct type *b, const struct RFstring *name);

/**
 * Checks if @c t is a subtype of @c other. Only checks for pointer equality
 */
bool rir_type_is_subtype_of_other(struct rir_type *t,
                                  struct rir_type *other);

/**
 * A form of @ref rir_type_equals_type for a specific index
 */
bool rir_type_with_index_equals_type(struct rir_type *r_type, unsigned int *index, struct type *n_type);

//! @return the name of the nth parameter of the type
const struct RFstring *rir_type_get_nth_name(struct rir_type *t, unsigned n);
//! @return the type of the nth parameter of the type
const struct rir_type *rir_type_get_nth_type(struct rir_type *t, unsigned n);

/**
 * Gets a string representation of the rir_type
 *
 * Before this function you need to execute use @ref RFS_buffer_push() in order
 * to remember the temporary string buffer position and after it you need to
 * pop it with @ref RFS_buffer_pop().
 *
 * @param t                 The type whose string representation to get
 * @return                  Returns a pointer to the the string representation.
 *                          If there is an error returns NULL.
 */
const struct RFstring *rir_type_str(const struct rir_type *t);


i_INLINE_DECL bool rir_type_is_elementary(const struct rir_type *t)
{
    return t->category < COMPOSITE_PRODUCT_RIR_TYPE;
}
#endif
