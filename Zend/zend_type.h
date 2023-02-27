/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) Zend Technologies Ltd. (http://www.zend.com)           |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_TYPE_H
#define ZEND_TYPE_H

#include "zend_type_info.h" // for MAY_BE_*

#include <stdint.h>

/*
 * zend_type - is an abstraction layer to represent information about type hint.
 * It shouldn't be used directly. Only through ZEND_TYPE_* macros.
 *
 * ZEND_TYPE_IS_SET()        - checks if there is a type-hint
 * ZEND_TYPE_IS_ONLY_MASK()  - checks if type-hint refer to standard type only
 * ZEND_TYPE_IS_COMPLEX()    - checks if type is a type_list, or contains a class either as a CE or as a name
 * ZEND_TYPE_HAS_NAME()      - checks if type-hint contains some class as zend_string *
 * ZEND_TYPE_IS_INTERSECTION() - checks if the type_list represents an intersection type list
 * ZEND_TYPE_IS_UNION()      - checks if the type_list represents a union type list
 *
 * ZEND_TYPE_NAME()       - returns referenced class name
 * ZEND_TYPE_PURE_MASK()  - returns MAY_BE_* type mask
 * ZEND_TYPE_FULL_MASK()  - returns MAY_BE_* type mask together with other flags
 *
 * ZEND_TYPE_ALLOW_NULL() - checks if NULL is allowed
 *
 * ZEND_TYPE_INIT_*() should be used for construction.
 */

typedef struct {
	/* Not using a union here, because there's no good way to initialize them
	 * in a way that is supported in both C and C++ (designated initializers
	 * are only supported since C++20). */
	void *ptr;
	uint32_t type_mask;
	/* TODO: We could use the extra 32-bit of padding on 64-bit systems. */
} zend_type;

typedef struct {
	uint32_t num_types;
	zend_type types[1];
} zend_type_list;

#define _ZEND_TYPE_EXTRA_FLAGS_SHIFT 25
#define _ZEND_TYPE_MASK ((1u << 25) - 1)
/* Only one of these bits may be set. */
#define _ZEND_TYPE_NAME_BIT (1u << 24)
#define _ZEND_TYPE_LIST_BIT (1u << 22)
#define _ZEND_TYPE_KIND_MASK (_ZEND_TYPE_LIST_BIT|_ZEND_TYPE_NAME_BIT)
/* For BC behaviour with iterable type */
#define _ZEND_TYPE_ITERABLE_BIT (1u << 21)
/* Whether the type list is arena allocated */
#define _ZEND_TYPE_ARENA_BIT (1u << 20)
/* Whether the type list is an intersection type */
#define _ZEND_TYPE_INTERSECTION_BIT (1u << 19)
/* Whether the type is a union type */
#define _ZEND_TYPE_UNION_BIT (1u << 18)
/* Type mask excluding the flags above. */
#define _ZEND_TYPE_MAY_BE_MASK ((1u << 18) - 1)
/* Must have same value as MAY_BE_NULL */
#define _ZEND_TYPE_NULLABLE_BIT 0x2u

#define ZEND_TYPE_IS_SET(t) \
	(((t).type_mask & _ZEND_TYPE_MASK) != 0)

/* If a type is complex it means it's either a list with a union or intersection,
 * or the void pointer is a class name */
#define ZEND_TYPE_IS_COMPLEX(t) \
	((((t).type_mask) & _ZEND_TYPE_KIND_MASK) != 0)

#define ZEND_TYPE_HAS_NAME(t) \
	((((t).type_mask) & _ZEND_TYPE_NAME_BIT) != 0)

#define ZEND_TYPE_HAS_LIST(t) \
	((((t).type_mask) & _ZEND_TYPE_LIST_BIT) != 0)

#define ZEND_TYPE_IS_ITERABLE_FALLBACK(t) \
	((((t).type_mask) & _ZEND_TYPE_ITERABLE_BIT) != 0)

#define ZEND_TYPE_IS_INTERSECTION(t) \
	((((t).type_mask) & _ZEND_TYPE_INTERSECTION_BIT) != 0)

#define ZEND_TYPE_IS_UNION(t) \
	((((t).type_mask) & _ZEND_TYPE_UNION_BIT) != 0)

#define ZEND_TYPE_USES_ARENA(t) \
	((((t).type_mask) & _ZEND_TYPE_ARENA_BIT) != 0)

#define ZEND_TYPE_IS_ONLY_MASK(t) \
	(ZEND_TYPE_IS_SET(t) && (t).ptr == NULL)

#define ZEND_TYPE_NAME(t) \
	((zend_string *) (t).ptr)

#define ZEND_TYPE_LITERAL_NAME(t) \
	((const char *) (t).ptr)

#define ZEND_TYPE_LIST(t) \
	((zend_type_list *) (t).ptr)

#define ZEND_TYPE_LIST_SIZE(num_types) \
	(sizeof(zend_type_list) + ((num_types) - 1) * sizeof(zend_type))

/* This iterates over a zend_type_list. */
#define ZEND_TYPE_LIST_FOREACH(list, type_ptr) do { \
	zend_type *_list = (list)->types; \
	zend_type *_end = _list + (list)->num_types; \
	for (; _list < _end; _list++) { \
		type_ptr = _list;

#define ZEND_TYPE_LIST_FOREACH_END() \
	} \
} while (0)

/* This iterates over any zend_type. If it's a type list, all list elements will
 * be visited. If it's a single type, only the single type is visited. */
#define ZEND_TYPE_FOREACH(type, type_ptr) do { \
	zend_type *_cur, *_end; \
	if (ZEND_TYPE_HAS_LIST(type)) { \
		zend_type_list *_list = ZEND_TYPE_LIST(type); \
		_cur = _list->types; \
		_end = _cur + _list->num_types; \
	} else { \
		_cur = &(type); \
		_end = _cur + 1; \
	} \
	do { \
		type_ptr = _cur;

#define ZEND_TYPE_FOREACH_END() \
	} while (++_cur < _end); \
} while (0)

#define ZEND_TYPE_SET_PTR(t, _ptr) \
	((t).ptr = (_ptr))

#define ZEND_TYPE_SET_PTR_AND_KIND(t, _ptr, kind_bit) do { \
	(t).ptr = (_ptr); \
	(t).type_mask &= ~_ZEND_TYPE_KIND_MASK; \
	(t).type_mask |= (kind_bit); \
} while (0)

#define ZEND_TYPE_SET_LIST(t, list) \
	ZEND_TYPE_SET_PTR_AND_KIND(t, list, _ZEND_TYPE_LIST_BIT)

/* FULL_MASK() includes the MAY_BE_* type mask, as well as additional metadata bits.
 * The PURE_MASK() only includes the MAY_BE_* type mask. */
#define ZEND_TYPE_FULL_MASK(t) \
	((t).type_mask)

#define ZEND_TYPE_PURE_MASK(t) \
	((t).type_mask & _ZEND_TYPE_MAY_BE_MASK)

#define ZEND_TYPE_FULL_MASK_WITHOUT_NULL(t) \
	((t).type_mask & ~_ZEND_TYPE_NULLABLE_BIT)

#define ZEND_TYPE_PURE_MASK_WITHOUT_NULL(t) \
	((t).type_mask & _ZEND_TYPE_MAY_BE_MASK & ~_ZEND_TYPE_NULLABLE_BIT)

#define ZEND_TYPE_CONTAINS_CODE(t, code) \
	(((t).type_mask & (1u << (code))) != 0)

#define ZEND_TYPE_ALLOW_NULL(t) \
	(((t).type_mask & _ZEND_TYPE_NULLABLE_BIT) != 0)

#define ZEND_TYPE_INIT_NONE(extra_flags) \
	{ NULL, (extra_flags) }

#define ZEND_TYPE_INIT_MASK(_type_mask) \
	{ NULL, (_type_mask) }

#define ZEND_TYPE_INIT_CODE(code, allow_null, extra_flags) \
	ZEND_TYPE_INIT_MASK(((code) == _IS_BOOL ? MAY_BE_BOOL : ( (code) == IS_ITERABLE ? _ZEND_TYPE_ITERABLE_BIT : ((code) == IS_MIXED ? MAY_BE_ANY : (1 << (code))))) \
		| ((allow_null) ? _ZEND_TYPE_NULLABLE_BIT : 0) | (extra_flags))

#define ZEND_TYPE_INIT_PTR(ptr, type_kind, allow_null, extra_flags) \
	{ (void *) (ptr), \
		(type_kind) | ((allow_null) ? _ZEND_TYPE_NULLABLE_BIT : 0) | (extra_flags) }

#define ZEND_TYPE_INIT_PTR_MASK(ptr, type_mask) \
	{ (void *) (ptr), (type_mask) }

#define ZEND_TYPE_INIT_UNION(ptr, extra_flags) \
	{ (void *) (ptr), (_ZEND_TYPE_LIST_BIT|_ZEND_TYPE_UNION_BIT) | (extra_flags) }

#define ZEND_TYPE_INIT_INTERSECTION(ptr, extra_flags) \
	{ (void *) (ptr), (_ZEND_TYPE_LIST_BIT|_ZEND_TYPE_INTERSECTION_BIT) | (extra_flags) }

#define ZEND_TYPE_INIT_CLASS(class_name, allow_null, extra_flags) \
	ZEND_TYPE_INIT_PTR(class_name, _ZEND_TYPE_NAME_BIT, allow_null, extra_flags)

#define ZEND_TYPE_INIT_CLASS_CONST(class_name, allow_null, extra_flags) \
	ZEND_TYPE_INIT_PTR(class_name, _ZEND_TYPE_NAME_BIT, allow_null, extra_flags)

#define ZEND_TYPE_INIT_CLASS_CONST_MASK(class_name, type_mask) \
	ZEND_TYPE_INIT_PTR_MASK(class_name, _ZEND_TYPE_NAME_BIT | (type_mask))

#endif /* ZEND_TYPE_H */
