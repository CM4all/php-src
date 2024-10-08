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

#ifndef ZEND_VALUE_H
#define ZEND_VALUE_H

#include "zend_endian.h"
#include "zend_long.h"
#include "zend_portability.h" // for zend_always_inline
#include "zend_rc_debug.h"
#include "zend_refcounted.h" // for GC_REFCOUNT()
#include "zend_type_code.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct _zend_refcounted zend_refcounted;
typedef struct _zend_string zend_string;
typedef struct _zend_array zend_array;
typedef struct _zend_object zend_object;
typedef struct _zend_resource zend_resource;
typedef struct _zend_reference zend_reference;
typedef struct _zend_ast_ref zend_ast_ref;
typedef struct _zend_class_entry zend_class_entry;
typedef union  _zend_function zend_function;
typedef struct _zval_struct zval;

typedef union _zend_value {
	zend_long         lval;				/* long value */
	double            dval;				/* double value */
	zend_refcounted  *counted;
	zend_string      *str;
	zend_array       *arr;
	zend_object      *obj;
	zend_resource    *res;
	zend_reference   *ref;
	zend_ast_ref     *ast;
	zval             *zv;
	void             *ptr;
	zend_class_entry *ce;
	zend_function    *func;
	struct {
		uint32_t w1;
		uint32_t w2;
	} ww;
} zend_value;

struct _zval_struct {
	zend_value        value;			/* value */
	union {
		uint32_t type_info;
		struct {
			ZEND_ENDIAN_LOHI_3(
				uint8_t    type,			/* active type */
				uint8_t    type_flags,
				union {
					uint16_t  extra;        /* not further specified */
				} u)
		} v;
	} u1;
	union {
		uint32_t     next;                 /* hash collision chain */
		uint32_t     cache_slot;           /* cache slot (for RECV_INIT) */
		uint32_t     opline_num;           /* opline number (for FAST_CALL) */
		uint32_t     lineno;               /* line number (for ast nodes) */
		uint32_t     num_args;             /* arguments number for EX(This) */
		uint32_t     fe_pos;               /* foreach position */
		uint32_t     fe_iter_idx;          /* foreach iterator index */
		uint32_t     guard;                /* recursion and single property guard */
		uint32_t     constant_flags;       /* constant flags */
		uint32_t     extra;                /* not further specified */
	} u2;
};

static zend_always_inline uint8_t zval_get_type(const zval* pz) {
	return pz->u1.v.type;
}

/* we should never set just Z_TYPE, we should set Z_TYPE_INFO */
#define Z_TYPE(zval)				zval_get_type(&(zval))
#define Z_TYPE_P(zval_p)			Z_TYPE(*(zval_p))

#define Z_TYPE_FLAGS(zval)			(zval).u1.v.type_flags
#define Z_TYPE_FLAGS_P(zval_p)		Z_TYPE_FLAGS(*(zval_p))

#define Z_TYPE_EXTRA(zval)			(zval).u1.v.u.extra
#define Z_TYPE_EXTRA_P(zval_p)		Z_TYPE_EXTRA(*(zval_p))

#define Z_TYPE_INFO(zval)			(zval).u1.type_info
#define Z_TYPE_INFO_P(zval_p)		Z_TYPE_INFO(*(zval_p))

#define Z_NEXT(zval)				(zval).u2.next
#define Z_NEXT_P(zval_p)			Z_NEXT(*(zval_p))

#define Z_CACHE_SLOT(zval)			(zval).u2.cache_slot
#define Z_CACHE_SLOT_P(zval_p)		Z_CACHE_SLOT(*(zval_p))

#define Z_LINENO(zval)				(zval).u2.lineno
#define Z_LINENO_P(zval_p)			Z_LINENO(*(zval_p))

#define Z_OPLINE_NUM(zval)			(zval).u2.opline_num
#define Z_OPLINE_NUM_P(zval_p)		Z_OPLINE_NUM(*(zval_p))

#define Z_FE_POS(zval)				(zval).u2.fe_pos
#define Z_FE_POS_P(zval_p)			Z_FE_POS(*(zval_p))

#define Z_FE_ITER(zval)				(zval).u2.fe_iter_idx
#define Z_FE_ITER_P(zval_p)			Z_FE_ITER(*(zval_p))

#define Z_GUARD(zval)				(zval).u2.guard
#define Z_GUARD_P(zval_p)			Z_GUARD(*(zval_p))

#define Z_CONSTANT_FLAGS(zval)		(zval).u2.constant_flags
#define Z_CONSTANT_FLAGS_P(zval_p)	Z_CONSTANT_FLAGS(*(zval_p))

#define Z_EXTRA(zval)				(zval).u2.extra
#define Z_EXTRA_P(zval_p)			Z_EXTRA(*(zval_p))

#define Z_COUNTED(zval)				(zval).value.counted
#define Z_COUNTED_P(zval_p)			Z_COUNTED(*(zval_p))

#define Z_TYPE_MASK					0xff
#define Z_TYPE_FLAGS_MASK			0xff00

#define Z_TYPE_FLAGS_SHIFT			8
#define Z_TYPE_INFO_EXTRA_SHIFT		16

#define Z_GC_TYPE(zval)				GC_TYPE(Z_COUNTED(zval))
#define Z_GC_TYPE_P(zval_p)			Z_GC_TYPE(*(zval_p))

#define Z_GC_FLAGS(zval)			GC_FLAGS(Z_COUNTED(zval))
#define Z_GC_FLAGS_P(zval_p)		Z_GC_FLAGS(*(zval_p))

#define Z_GC_INFO(zval)				GC_INFO(Z_COUNTED(zval))
#define Z_GC_INFO_P(zval_p)			Z_GC_INFO(*(zval_p))
#define Z_GC_TYPE_INFO(zval)		GC_TYPE_INFO(Z_COUNTED(zval))
#define Z_GC_TYPE_INFO_P(zval_p)	Z_GC_TYPE_INFO(*(zval_p))

/* zval.u1.v.type_flags */
#define IS_TYPE_REFCOUNTED			(1<<0)
#define IS_TYPE_COLLECTABLE			(1<<1)
/* Used for static variables to check if they have been initialized. We can't use IS_UNDEF because
 * we can't store IS_UNDEF zvals in the static_variables HashTable. This needs to live in type_info
 * so that the ZEND_ASSIGN overrides it but is moved to extra to avoid breaking the Z_REFCOUNTED()
 * optimization that only checks for Z_TYPE_FLAGS() without `& (IS_TYPE_COLLECTABLE|IS_TYPE_REFCOUNTED)`. */
#define IS_STATIC_VAR_UNINITIALIZED		(1<<0)

#if 1
/* This optimized version assumes that we have a single "type_flag" */
/* IS_TYPE_COLLECTABLE may be used only with IS_TYPE_REFCOUNTED */
# define Z_TYPE_INFO_REFCOUNTED(t)	(((t) & Z_TYPE_FLAGS_MASK) != 0)
#else
# define Z_TYPE_INFO_REFCOUNTED(t)	(((t) & (IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT)) != 0)
#endif

/* extended types */
#define IS_INTERNED_STRING_EX		IS_STRING

#define IS_STRING_EX				(IS_STRING         | (IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT))
#define IS_ARRAY_EX					(IS_ARRAY          | (IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT) | (IS_TYPE_COLLECTABLE << Z_TYPE_FLAGS_SHIFT))
#define IS_OBJECT_EX				(IS_OBJECT         | (IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT) | (IS_TYPE_COLLECTABLE << Z_TYPE_FLAGS_SHIFT))
#define IS_RESOURCE_EX				(IS_RESOURCE       | (IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT))
#define IS_REFERENCE_EX				(IS_REFERENCE      | (IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT))

#define IS_CONSTANT_AST_EX			(IS_CONSTANT_AST   | (IS_TYPE_REFCOUNTED << Z_TYPE_FLAGS_SHIFT))

/* array flags */
#define IS_ARRAY_IMMUTABLE			GC_IMMUTABLE
#define IS_ARRAY_PERSISTENT			GC_PERSISTENT

/* object flags (zval.value->gc.u.flags) */
#define IS_OBJ_WEAKLY_REFERENCED	GC_PERSISTENT
#define IS_OBJ_DESTRUCTOR_CALLED	(1<<8)
#define IS_OBJ_FREE_CALLED			(1<<9)

#define OBJ_FLAGS(obj)              GC_FLAGS(obj)

/* Fast class cache */
#define ZSTR_HAS_CE_CACHE(s)		(GC_FLAGS(s) & IS_STR_CLASS_NAME_MAP_PTR)
#define ZSTR_GET_CE_CACHE(s)		ZSTR_GET_CE_CACHE_EX(s, 1)
#define ZSTR_SET_CE_CACHE(s, ce)	ZSTR_SET_CE_CACHE_EX(s, ce, 1)

#define ZSTR_VALID_CE_CACHE(s)		EXPECTED((GC_REFCOUNT(s)-1)/sizeof(void *) < CG(map_ptr_last))

#define ZSTR_GET_CE_CACHE_EX(s, validate) \
	((!(validate) || ZSTR_VALID_CE_CACHE(s)) ? GET_CE_CACHE(GC_REFCOUNT(s)) : NULL)

#define ZSTR_SET_CE_CACHE_EX(s, ce, validate) do { \
		if (!(validate) || ZSTR_VALID_CE_CACHE(s)) { \
			ZEND_ASSERT((validate) || ZSTR_VALID_CE_CACHE(s)); \
			SET_CE_CACHE(GC_REFCOUNT(s), ce); \
		} \
	} while (0)

#define GET_CE_CACHE(ce_cache) \
	(*(zend_class_entry **)ZEND_MAP_PTR_OFFSET2PTR(ce_cache))

#define SET_CE_CACHE(ce_cache, ce) do { \
		*((zend_class_entry **)ZEND_MAP_PTR_OFFSET2PTR(ce_cache)) = ce; \
	} while (0)

/* Recursion protection macros must be used only for arrays and objects */
#define GC_IS_RECURSIVE(p) \
	(GC_FLAGS(p) & GC_PROTECTED)

#define GC_PROTECT_RECURSION(p) do { \
		GC_ADD_FLAGS(p, GC_PROTECTED); \
	} while (0)

#define GC_UNPROTECT_RECURSION(p) do { \
		GC_DEL_FLAGS(p, GC_PROTECTED); \
	} while (0)

#define GC_TRY_PROTECT_RECURSION(p) do { \
		if (!(GC_FLAGS(p) & GC_IMMUTABLE)) GC_PROTECT_RECURSION(p); \
	} while (0)

#define GC_TRY_UNPROTECT_RECURSION(p) do { \
		if (!(GC_FLAGS(p) & GC_IMMUTABLE)) GC_UNPROTECT_RECURSION(p); \
	} while (0)

#define Z_IS_RECURSIVE(zval)        GC_IS_RECURSIVE(Z_COUNTED(zval))
#define Z_PROTECT_RECURSION(zval)   GC_PROTECT_RECURSION(Z_COUNTED(zval))
#define Z_UNPROTECT_RECURSION(zval) GC_UNPROTECT_RECURSION(Z_COUNTED(zval))
#define Z_IS_RECURSIVE_P(zv)        Z_IS_RECURSIVE(*(zv))
#define Z_PROTECT_RECURSION_P(zv)   Z_PROTECT_RECURSION(*(zv))
#define Z_UNPROTECT_RECURSION_P(zv) Z_UNPROTECT_RECURSION(*(zv))

#define ZEND_GUARD_OR_GC_IS_RECURSIVE(pg, t, zobj) \
	(pg ? ZEND_GUARD_IS_RECURSIVE(pg, t) : GC_IS_RECURSIVE(zobj))

#define ZEND_GUARD_OR_GC_PROTECT_RECURSION(pg, t, zobj) do { \
		if (pg) { \
			ZEND_GUARD_PROTECT_RECURSION(pg, t); \
		} else { \
			GC_PROTECT_RECURSION(zobj); \
		} \
	} while(0)

#define ZEND_GUARD_OR_GC_UNPROTECT_RECURSION(pg, t, zobj) do { \
		if (pg) { \
			ZEND_GUARD_UNPROTECT_RECURSION(pg, t); \
		} else { \
			GC_UNPROTECT_RECURSION(zobj); \
		} \
	} while(0)

/* All data types < IS_STRING have their constructor/destructors skipped */
#define Z_CONSTANT(zval)			(Z_TYPE(zval) == IS_CONSTANT_AST)
#define Z_CONSTANT_P(zval_p)		Z_CONSTANT(*(zval_p))

#if 1
/* This optimized version assumes that we have a single "type_flag" */
/* IS_TYPE_COLLECTABLE may be used only with IS_TYPE_REFCOUNTED */
#define Z_REFCOUNTED(zval)			(Z_TYPE_FLAGS(zval) != 0)
#else
#define Z_REFCOUNTED(zval)			((Z_TYPE_FLAGS(zval) & IS_TYPE_REFCOUNTED) != 0)
#endif
#define Z_REFCOUNTED_P(zval_p)		Z_REFCOUNTED(*(zval_p))

#define Z_COLLECTABLE(zval)			((Z_TYPE_FLAGS(zval) & IS_TYPE_COLLECTABLE) != 0)
#define Z_COLLECTABLE_P(zval_p)		Z_COLLECTABLE(*(zval_p))

/* deprecated: (COPYABLE is the same as IS_ARRAY) */
#define Z_COPYABLE(zval)			(Z_TYPE(zval) == IS_ARRAY)
#define Z_COPYABLE_P(zval_p)		Z_COPYABLE(*(zval_p))

/* deprecated: (IMMUTABLE is the same as IS_ARRAY && !REFCOUNTED) */
#define Z_IMMUTABLE(zval)			(Z_TYPE_INFO(zval) == IS_ARRAY)
#define Z_IMMUTABLE_P(zval_p)		Z_IMMUTABLE(*(zval_p))
#define Z_OPT_IMMUTABLE(zval)		Z_IMMUTABLE(zval_p)
#define Z_OPT_IMMUTABLE_P(zval_p)	Z_IMMUTABLE(*(zval_p))

/* the following Z_OPT_* macros make better code when Z_TYPE_INFO accessed before */
#define Z_OPT_TYPE(zval)			(Z_TYPE_INFO(zval) & Z_TYPE_MASK)
#define Z_OPT_TYPE_P(zval_p)		Z_OPT_TYPE(*(zval_p))

#define Z_OPT_CONSTANT(zval)		(Z_OPT_TYPE(zval) == IS_CONSTANT_AST)
#define Z_OPT_CONSTANT_P(zval_p)	Z_OPT_CONSTANT(*(zval_p))

#define Z_OPT_REFCOUNTED(zval)		Z_TYPE_INFO_REFCOUNTED(Z_TYPE_INFO(zval))
#define Z_OPT_REFCOUNTED_P(zval_p)	Z_OPT_REFCOUNTED(*(zval_p))

/* deprecated: (COPYABLE is the same as IS_ARRAY) */
#define Z_OPT_COPYABLE(zval)		(Z_OPT_TYPE(zval) == IS_ARRAY)
#define Z_OPT_COPYABLE_P(zval_p)	Z_OPT_COPYABLE(*(zval_p))

#define Z_OPT_ISREF(zval)			(Z_OPT_TYPE(zval) == IS_REFERENCE)
#define Z_OPT_ISREF_P(zval_p)		Z_OPT_ISREF(*(zval_p))

#define Z_ISREF(zval)				(Z_TYPE(zval) == IS_REFERENCE)
#define Z_ISREF_P(zval_p)			Z_ISREF(*(zval_p))

#define Z_ISUNDEF(zval)				(Z_TYPE(zval) == IS_UNDEF)
#define Z_ISUNDEF_P(zval_p)			Z_ISUNDEF(*(zval_p))

#define Z_ISNULL(zval)				(Z_TYPE(zval) == IS_NULL)
#define Z_ISNULL_P(zval_p)			Z_ISNULL(*(zval_p))

#define Z_ISERROR(zval)				(Z_TYPE(zval) == _IS_ERROR)
#define Z_ISERROR_P(zval_p)			Z_ISERROR(*(zval_p))

#define Z_LVAL(zval)				(zval).value.lval
#define Z_LVAL_P(zval_p)			Z_LVAL(*(zval_p))

#define Z_DVAL(zval)				(zval).value.dval
#define Z_DVAL_P(zval_p)			Z_DVAL(*(zval_p))

#define Z_STR(zval)					(zval).value.str
#define Z_STR_P(zval_p)				Z_STR(*(zval_p))

#define Z_STRVAL(zval)				ZSTR_VAL(Z_STR(zval))
#define Z_STRVAL_P(zval_p)			Z_STRVAL(*(zval_p))

#define Z_STRLEN(zval)				ZSTR_LEN(Z_STR(zval))
#define Z_STRLEN_P(zval_p)			Z_STRLEN(*(zval_p))

#define Z_STRHASH(zval)				ZSTR_HASH(Z_STR(zval))
#define Z_STRHASH_P(zval_p)			Z_STRHASH(*(zval_p))

#define Z_ARR(zval)					(zval).value.arr
#define Z_ARR_P(zval_p)				Z_ARR(*(zval_p))

#define Z_ARRVAL(zval)				Z_ARR(zval)
#define Z_ARRVAL_P(zval_p)			Z_ARRVAL(*(zval_p))

#define Z_OBJ(zval)					(zval).value.obj
#define Z_OBJ_P(zval_p)				Z_OBJ(*(zval_p))

#define Z_OBJ_HT(zval)				Z_OBJ(zval)->handlers
#define Z_OBJ_HT_P(zval_p)			Z_OBJ_HT(*(zval_p))

#define Z_OBJ_HANDLER(zval, hf)		Z_OBJ_HT((zval))->hf
#define Z_OBJ_HANDLER_P(zv_p, hf)	Z_OBJ_HANDLER(*(zv_p), hf)

#define Z_OBJ_HANDLE(zval)          (Z_OBJ((zval)))->handle
#define Z_OBJ_HANDLE_P(zval_p)      Z_OBJ_HANDLE(*(zval_p))

#define Z_OBJCE(zval)				(Z_OBJ(zval)->ce)
#define Z_OBJCE_P(zval_p)			Z_OBJCE(*(zval_p))

#define Z_OBJPROP(zval)				Z_OBJ_HT((zval))->get_properties(Z_OBJ(zval))
#define Z_OBJPROP_P(zval_p)			Z_OBJPROP(*(zval_p))

#define Z_RES(zval)					(zval).value.res
#define Z_RES_P(zval_p)				Z_RES(*zval_p)

#define Z_RES_HANDLE(zval)			Z_RES(zval)->handle
#define Z_RES_HANDLE_P(zval_p)		Z_RES_HANDLE(*zval_p)

#define Z_RES_TYPE(zval)			Z_RES(zval)->type
#define Z_RES_TYPE_P(zval_p)		Z_RES_TYPE(*zval_p)

#define Z_RES_VAL(zval)				Z_RES(zval)->ptr
#define Z_RES_VAL_P(zval_p)			Z_RES_VAL(*zval_p)

#define Z_REF(zval)					(zval).value.ref
#define Z_REF_P(zval_p)				Z_REF(*(zval_p))

#define Z_REFVAL(zval)				&Z_REF(zval)->val
#define Z_REFVAL_P(zval_p)			Z_REFVAL(*(zval_p))

#define Z_AST(zval)					(zval).value.ast
#define Z_AST_P(zval_p)				Z_AST(*(zval_p))

#define GC_AST(p)					((zend_ast*)(((char*)p) + sizeof(zend_ast_ref)))

#define Z_ASTVAL(zval)				GC_AST(Z_AST(zval))
#define Z_ASTVAL_P(zval_p)			Z_ASTVAL(*(zval_p))

#define Z_INDIRECT(zval)			(zval).value.zv
#define Z_INDIRECT_P(zval_p)		Z_INDIRECT(*(zval_p))

#define Z_CE(zval)					(zval).value.ce
#define Z_CE_P(zval_p)				Z_CE(*(zval_p))

#define Z_FUNC(zval)				(zval).value.func
#define Z_FUNC_P(zval_p)			Z_FUNC(*(zval_p))

#define Z_PTR(zval)					(zval).value.ptr
#define Z_PTR_P(zval_p)				Z_PTR(*(zval_p))

#define ZVAL_UNDEF(z) do {				\
		Z_TYPE_INFO_P(z) = IS_UNDEF;	\
	} while (0)

#define ZVAL_NULL(z) do {				\
		Z_TYPE_INFO_P(z) = IS_NULL;		\
	} while (0)

#define ZVAL_FALSE(z) do {				\
		Z_TYPE_INFO_P(z) = IS_FALSE;	\
	} while (0)

#define ZVAL_TRUE(z) do {				\
		Z_TYPE_INFO_P(z) = IS_TRUE;		\
	} while (0)

#define ZVAL_BOOL(z, b) do {			\
		Z_TYPE_INFO_P(z) =				\
			(b) ? IS_TRUE : IS_FALSE;	\
	} while (0)

#define ZVAL_LONG(z, l) do {			\
		zval *__z = (z);				\
		Z_LVAL_P(__z) = l;				\
		Z_TYPE_INFO_P(__z) = IS_LONG;	\
	} while (0)

#define ZVAL_DOUBLE(z, d) do {			\
		zval *__z = (z);				\
		Z_DVAL_P(__z) = d;				\
		Z_TYPE_INFO_P(__z) = IS_DOUBLE;	\
	} while (0)

#define ZVAL_STR(z, s) do {						\
		zval *__z = (z);						\
		zend_string *__s = (s);					\
		Z_STR_P(__z) = __s;						\
		/* interned strings support */			\
		Z_TYPE_INFO_P(__z) = ZSTR_IS_INTERNED(__s) ? \
			IS_INTERNED_STRING_EX : 			\
			IS_STRING_EX;						\
	} while (0)

#define ZVAL_INTERNED_STR(z, s) do {				\
		zval *__z = (z);							\
		zend_string *__s = (s);						\
		Z_STR_P(__z) = __s;							\
		Z_TYPE_INFO_P(__z) = IS_INTERNED_STRING_EX;	\
	} while (0)

#define ZVAL_NEW_STR(z, s) do {					\
		zval *__z = (z);						\
		zend_string *__s = (s);					\
		Z_STR_P(__z) = __s;						\
		Z_TYPE_INFO_P(__z) = IS_STRING_EX;		\
	} while (0)

#define ZVAL_STR_COPY(z, s) do {						\
		zval *__z = (z);								\
		zend_string *__s = (s);							\
		Z_STR_P(__z) = __s;								\
		/* interned strings support */					\
		if (ZSTR_IS_INTERNED(__s)) {					\
			Z_TYPE_INFO_P(__z) = IS_INTERNED_STRING_EX;	\
		} else {										\
			GC_ADDREF(__s);								\
			Z_TYPE_INFO_P(__z) = IS_STRING_EX;			\
		}												\
	} while (0)

#define ZVAL_ARR(z, a) do {						\
		zend_array *__arr = (a);				\
		zval *__z = (z);						\
		Z_ARR_P(__z) = __arr;					\
		Z_TYPE_INFO_P(__z) = IS_ARRAY_EX;		\
	} while (0)

#define ZVAL_NEW_PERSISTENT_ARR(z) do {							\
		zval *__z = (z);										\
		zend_array *_arr =										\
		(zend_array *) malloc(sizeof(zend_array));				\
		Z_ARR_P(__z) = _arr;									\
		Z_TYPE_INFO_P(__z) = IS_ARRAY_EX;						\
	} while (0)

#define ZVAL_OBJ(z, o) do {						\
		zval *__z = (z);						\
		Z_OBJ_P(__z) = (o);						\
		Z_TYPE_INFO_P(__z) = IS_OBJECT_EX;		\
	} while (0)

#define ZVAL_OBJ_COPY(z, o) do {				\
		zval *__z = (z);						\
		zend_object *__o = (o);					\
		GC_ADDREF(__o);							\
		Z_OBJ_P(__z) = __o;						\
		Z_TYPE_INFO_P(__z) = IS_OBJECT_EX;		\
	} while (0)

#define ZVAL_RES(z, r) do {						\
		zval *__z = (z);						\
		Z_RES_P(__z) = (r);						\
		Z_TYPE_INFO_P(__z) = IS_RESOURCE_EX;	\
	} while (0)

#define ZVAL_NEW_RES(z, h, p, t) do {							\
		zend_resource *_res =									\
		(zend_resource *) emalloc(sizeof(zend_resource));		\
		zval *__z;												\
		GC_SET_REFCOUNT(_res, 1);								\
		GC_TYPE_INFO(_res) = GC_RESOURCE;						\
		_res->handle = (h);										\
		_res->type = (t);										\
		_res->ptr = (p);										\
		__z = (z);												\
		Z_RES_P(__z) = _res;									\
		Z_TYPE_INFO_P(__z) = IS_RESOURCE_EX;					\
	} while (0)

#define ZVAL_NEW_PERSISTENT_RES(z, h, p, t) do {				\
		zend_resource *_res =									\
		(zend_resource *) malloc(sizeof(zend_resource));		\
		zval *__z;												\
		GC_SET_REFCOUNT(_res, 1);								\
		GC_TYPE_INFO(_res) = GC_RESOURCE |						\
			(GC_PERSISTENT << GC_FLAGS_SHIFT);					\
		_res->handle = (h);										\
		_res->type = (t);										\
		_res->ptr = (p);										\
		__z = (z);												\
		Z_RES_P(__z) = _res;									\
		Z_TYPE_INFO_P(__z) = IS_RESOURCE_EX;					\
	} while (0)

#define ZVAL_REF(z, r) do {										\
		zval *__z = (z);										\
		Z_REF_P(__z) = (r);										\
		Z_TYPE_INFO_P(__z) = IS_REFERENCE_EX;					\
	} while (0)

#define ZVAL_NEW_EMPTY_REF(z) do {								\
		zend_reference *_ref =									\
		(zend_reference *) emalloc(sizeof(zend_reference));		\
		GC_SET_REFCOUNT(_ref, 1);								\
		GC_TYPE_INFO(_ref) = GC_REFERENCE;						\
		_ref->sources.ptr = NULL;									\
		Z_REF_P(z) = _ref;										\
		Z_TYPE_INFO_P(z) = IS_REFERENCE_EX;						\
	} while (0)

#define ZVAL_NEW_REF(z, r) do {									\
		zend_reference *_ref =									\
		(zend_reference *) emalloc(sizeof(zend_reference));		\
		GC_SET_REFCOUNT(_ref, 1);								\
		GC_TYPE_INFO(_ref) = GC_REFERENCE;						\
		ZVAL_COPY_VALUE(&_ref->val, r);							\
		_ref->sources.ptr = NULL;									\
		Z_REF_P(z) = _ref;										\
		Z_TYPE_INFO_P(z) = IS_REFERENCE_EX;						\
	} while (0)

#define ZVAL_MAKE_REF_EX(z, refcount) do {						\
		zval *_z = (z);											\
		zend_reference *_ref =									\
			(zend_reference *) emalloc(sizeof(zend_reference));	\
		GC_SET_REFCOUNT(_ref, (refcount));						\
		GC_TYPE_INFO(_ref) = GC_REFERENCE;						\
		ZVAL_COPY_VALUE(&_ref->val, _z);						\
		_ref->sources.ptr = NULL;									\
		Z_REF_P(_z) = _ref;										\
		Z_TYPE_INFO_P(_z) = IS_REFERENCE_EX;					\
	} while (0)

#define ZVAL_NEW_PERSISTENT_REF(z, r) do {						\
		zend_reference *_ref =									\
		(zend_reference *) malloc(sizeof(zend_reference));		\
		GC_SET_REFCOUNT(_ref, 1);								\
		GC_TYPE_INFO(_ref) = GC_REFERENCE |						\
			(GC_PERSISTENT << GC_FLAGS_SHIFT);					\
		ZVAL_COPY_VALUE(&_ref->val, r);							\
		_ref->sources.ptr = NULL;									\
		Z_REF_P(z) = _ref;										\
		Z_TYPE_INFO_P(z) = IS_REFERENCE_EX;						\
	} while (0)

#define ZVAL_AST(z, ast) do {									\
		zval *__z = (z);										\
		Z_AST_P(__z) = ast;										\
		Z_TYPE_INFO_P(__z) = IS_CONSTANT_AST_EX;				\
	} while (0)

#define ZVAL_INDIRECT(z, v) do {								\
		Z_INDIRECT_P(z) = (v);									\
		Z_TYPE_INFO_P(z) = IS_INDIRECT;							\
	} while (0)

#define ZVAL_PTR(z, p) do {										\
		Z_PTR_P(z) = (p);										\
		Z_TYPE_INFO_P(z) = IS_PTR;								\
	} while (0)

#define ZVAL_FUNC(z, f) do {									\
		Z_FUNC_P(z) = (f);										\
		Z_TYPE_INFO_P(z) = IS_PTR;								\
	} while (0)

#define ZVAL_CE(z, c) do {										\
		Z_CE_P(z) = (c);										\
		Z_TYPE_INFO_P(z) = IS_PTR;								\
	} while (0)

#define ZVAL_ALIAS_PTR(z, p) do {								\
		Z_PTR_P(z) = (p);										\
		Z_TYPE_INFO_P(z) = IS_ALIAS_PTR;						\
	} while (0)

#define ZVAL_ERROR(z) do {				\
		Z_TYPE_INFO_P(z) = _IS_ERROR;	\
	} while (0)

#define Z_REFCOUNT_P(pz)			zval_refcount_p(pz)
#define Z_SET_REFCOUNT_P(pz, rc)	zval_set_refcount_p(pz, rc)
#define Z_ADDREF_P(pz)				zval_addref_p(pz)
#define Z_DELREF_P(pz)				zval_delref_p(pz)

#define Z_REFCOUNT(z)				Z_REFCOUNT_P(&(z))
#define Z_SET_REFCOUNT(z, rc)		Z_SET_REFCOUNT_P(&(z), rc)
#define Z_ADDREF(z)					Z_ADDREF_P(&(z))
#define Z_DELREF(z)					Z_DELREF_P(&(z))

#define Z_TRY_ADDREF_P(pz) do {		\
	if (Z_REFCOUNTED_P((pz))) {		\
		Z_ADDREF_P((pz));			\
	}								\
} while (0)

#define Z_TRY_DELREF_P(pz) do {		\
	if (Z_REFCOUNTED_P((pz))) {		\
		Z_DELREF_P((pz));			\
	}								\
} while (0)

#define Z_TRY_ADDREF(z)				Z_TRY_ADDREF_P(&(z))
#define Z_TRY_DELREF(z)				Z_TRY_DELREF_P(&(z))

#if ZEND_RC_DEBUG
# define GC_MAKE_PERSISTENT_LOCAL(p) do { \
		GC_ADD_FLAGS(p, GC_PERSISTENT_LOCAL); \
	} while (0)
#else
# define GC_MAKE_PERSISTENT_LOCAL(p) \
	do { } while (0)
#endif

static zend_always_inline uint32_t zval_refcount_p(const zval* pz) {
#if ZEND_DEBUG
	ZEND_ASSERT(Z_REFCOUNTED_P(pz) || Z_TYPE_P(pz) == IS_ARRAY);
#endif
	return GC_REFCOUNT(Z_COUNTED_P(pz));
}

static zend_always_inline uint32_t zval_set_refcount_p(zval* pz, uint32_t rc) {
	ZEND_ASSERT(Z_REFCOUNTED_P(pz));
	return GC_SET_REFCOUNT(Z_COUNTED_P(pz), rc);
}

static zend_always_inline uint32_t zval_addref_p(zval* pz) {
	ZEND_ASSERT(Z_REFCOUNTED_P(pz));
	return GC_ADDREF(Z_COUNTED_P(pz));
}

static zend_always_inline uint32_t zval_delref_p(zval* pz) {
	ZEND_ASSERT(Z_REFCOUNTED_P(pz));
	return GC_DELREF(Z_COUNTED_P(pz));
}

#if SIZEOF_SIZE_T == 4
# define ZVAL_COPY_VALUE_EX(z, v, gc, t)				\
	do {												\
		uint32_t _w2 = v->value.ww.w2;					\
		Z_COUNTED_P(z) = gc;							\
		z->value.ww.w2 = _w2;							\
		Z_TYPE_INFO_P(z) = t;							\
	} while (0)
#elif SIZEOF_SIZE_T == 8
# define ZVAL_COPY_VALUE_EX(z, v, gc, t)				\
	do {												\
		Z_COUNTED_P(z) = gc;							\
		Z_TYPE_INFO_P(z) = t;							\
	} while (0)
#else
# error "Unknown SIZEOF_SIZE_T"
#endif

#define ZVAL_COPY_VALUE(z, v)							\
	do {												\
		zval *_z1 = (z);								\
		const zval *_z2 = (v);							\
		zend_refcounted *_gc = Z_COUNTED_P(_z2);		\
		uint32_t _t = Z_TYPE_INFO_P(_z2);				\
		ZVAL_COPY_VALUE_EX(_z1, _z2, _gc, _t);			\
	} while (0)

#define ZVAL_COPY(z, v)									\
	do {												\
		zval *_z1 = (z);								\
		const zval *_z2 = (v);							\
		zend_refcounted *_gc = Z_COUNTED_P(_z2);		\
		uint32_t _t = Z_TYPE_INFO_P(_z2);				\
		ZVAL_COPY_VALUE_EX(_z1, _z2, _gc, _t);			\
		if (Z_TYPE_INFO_REFCOUNTED(_t)) {				\
			GC_ADDREF(_gc);								\
		}												\
	} while (0)

#define ZVAL_DUP(z, v)									\
	do {												\
		zval *_z1 = (z);								\
		const zval *_z2 = (v);							\
		zend_refcounted *_gc = Z_COUNTED_P(_z2);		\
		uint32_t _t = Z_TYPE_INFO_P(_z2);				\
		if ((_t & Z_TYPE_MASK) == IS_ARRAY) {			\
			ZVAL_ARR(_z1, zend_array_dup((zend_array*)_gc));\
		} else {										\
			if (Z_TYPE_INFO_REFCOUNTED(_t)) {			\
				GC_ADDREF(_gc);							\
			}											\
			ZVAL_COPY_VALUE_EX(_z1, _z2, _gc, _t);		\
		}												\
	} while (0)


/* ZVAL_COPY_OR_DUP() should be used instead of ZVAL_COPY() and ZVAL_DUP()
 * in all places where the source may be a persistent zval.
 */
#define ZVAL_COPY_OR_DUP(z, v)											\
	do {																\
		zval *_z1 = (z);												\
		const zval *_z2 = (v);											\
		zend_refcounted *_gc = Z_COUNTED_P(_z2);						\
		uint32_t _t = Z_TYPE_INFO_P(_z2);								\
		ZVAL_COPY_VALUE_EX(_z1, _z2, _gc, _t);							\
		if (Z_TYPE_INFO_REFCOUNTED(_t)) {								\
			/* Objects reuse PERSISTENT as WEAKLY_REFERENCED */			\
			if (EXPECTED(!(GC_FLAGS(_gc) & GC_PERSISTENT)				\
					|| GC_TYPE(_gc) == IS_OBJECT)) {					\
				GC_ADDREF(_gc);											\
			} else {													\
				zval_copy_ctor_func(_z1);								\
			}															\
		}																\
	} while (0)

#define ZVAL_DEREF(z) do {								\
		if (UNEXPECTED(Z_ISREF_P(z))) {					\
			(z) = Z_REFVAL_P(z);						\
		}												\
	} while (0)

#define ZVAL_DEINDIRECT(z) do {							\
		if (Z_TYPE_P(z) == IS_INDIRECT) {				\
			(z) = Z_INDIRECT_P(z);						\
		}												\
	} while (0)

#define ZVAL_OPT_DEREF(z) do {							\
		if (UNEXPECTED(Z_OPT_ISREF_P(z))) {				\
			(z) = Z_REFVAL_P(z);						\
		}												\
	} while (0)

#define ZVAL_MAKE_REF(zv) do {							\
		zval *__zv = (zv);								\
		if (!Z_ISREF_P(__zv)) {							\
			ZVAL_NEW_REF(__zv, __zv);					\
		}												\
	} while (0)

#define ZVAL_UNREF(z) do {								\
		zval *_z = (z);									\
		zend_reference *ref;							\
		ZEND_ASSERT(Z_ISREF_P(_z));						\
		ref = Z_REF_P(_z);								\
		ZVAL_COPY_VALUE(_z, &ref->val);					\
		efree_size(ref, sizeof(zend_reference));		\
	} while (0)

#define ZVAL_COPY_DEREF(z, v) do {						\
		zval *_z3 = (v);								\
		if (Z_OPT_REFCOUNTED_P(_z3)) {					\
			if (UNEXPECTED(Z_OPT_ISREF_P(_z3))) {		\
				_z3 = Z_REFVAL_P(_z3);					\
				if (Z_OPT_REFCOUNTED_P(_z3)) {			\
					Z_ADDREF_P(_z3);					\
				}										\
			} else {									\
				Z_ADDREF_P(_z3);						\
			}											\
		}												\
		ZVAL_COPY_VALUE(z, _z3);						\
	} while (0)


#define SEPARATE_STRING(zv) do {						\
		zval *_zv = (zv);								\
		if (Z_REFCOUNT_P(_zv) > 1) {					\
			zend_string *_str = Z_STR_P(_zv);			\
			ZEND_ASSERT(Z_REFCOUNTED_P(_zv));			\
			ZEND_ASSERT(!ZSTR_IS_INTERNED(_str));		\
			ZVAL_NEW_STR(_zv, zend_string_init(			\
				ZSTR_VAL(_str),	ZSTR_LEN(_str), 0));	\
			GC_DELREF(_str);							\
		}												\
	} while (0)

#define SEPARATE_ARRAY(zv) do {							\
		zval *__zv = (zv);								\
		zend_array *_arr = Z_ARR_P(__zv);				\
		if (UNEXPECTED(GC_REFCOUNT(_arr) > 1)) {		\
			ZVAL_ARR(__zv, zend_array_dup(_arr));		\
			GC_TRY_DELREF(_arr);						\
		}												\
	} while (0)

#define SEPARATE_ZVAL_NOREF(zv) do {					\
		zval *_zv = (zv);								\
		ZEND_ASSERT(Z_TYPE_P(_zv) != IS_REFERENCE);		\
		if (Z_TYPE_P(_zv) == IS_ARRAY) {				\
			SEPARATE_ARRAY(_zv);						\
		}												\
	} while (0)

#define SEPARATE_ZVAL(zv) do {							\
		zval *_zv = (zv);								\
		if (Z_ISREF_P(_zv)) {							\
			zend_reference *_r = Z_REF_P(_zv);			\
			ZVAL_COPY_VALUE(_zv, &_r->val);				\
			if (GC_DELREF(_r) == 0) {					\
				efree_size(_r, sizeof(zend_reference));	\
			} else if (Z_OPT_TYPE_P(_zv) == IS_ARRAY) {	\
				ZVAL_ARR(_zv, zend_array_dup(Z_ARR_P(_zv)));\
				break;									\
			} else if (Z_OPT_REFCOUNTED_P(_zv)) {		\
				Z_ADDREF_P(_zv);						\
				break;									\
			}											\
		}												\
		if (Z_TYPE_P(_zv) == IS_ARRAY) {				\
			SEPARATE_ARRAY(_zv);						\
		}												\
	} while (0)

/* Properties store a flag distinguishing unset and uninitialized properties
 * (both use IS_UNDEF type) in the Z_EXTRA space. As such we also need to copy
 * the Z_EXTRA space when copying property default values etc. We define separate
 * macros for this purpose, so this workaround is easier to remove in the future. */
#define IS_PROP_UNINIT (1<<0)
#define IS_PROP_REINITABLE (1<<1)  /* It has impact only on readonly properties */
#define Z_PROP_FLAG_P(z) Z_EXTRA_P(z)
#define ZVAL_COPY_VALUE_PROP(z, v) \
	do { *(z) = *(v); } while (0)
#define ZVAL_COPY_PROP(z, v) \
	do { ZVAL_COPY(z, v); Z_PROP_FLAG_P(z) = Z_PROP_FLAG_P(v); } while (0)
#define ZVAL_COPY_OR_DUP_PROP(z, v) \
	do { ZVAL_COPY_OR_DUP(z, v); Z_PROP_FLAG_P(z) = Z_PROP_FLAG_P(v); } while (0)


static zend_always_inline bool zend_may_modify_arg_in_place(const zval *arg)
{
	return Z_REFCOUNTED_P(arg) && !(GC_FLAGS(Z_COUNTED_P(arg)) & (GC_IMMUTABLE | GC_PERSISTENT)) && Z_REFCOUNT_P(arg) == 1;
}

#endif /* ZEND_VALUE_H */
