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
   | Authors: Andi Gutmans <andi@php.net>                                 |
   |          Zeev Suraski <zeev@php.net>                                 |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_OBJECTS_API_H
#define ZEND_OBJECTS_API_H

#include "zend_portability.h" // for BEGIN_EXTERN_C

#include <stdbool.h>
#include <stdint.h>

typedef struct _zend_class_entry zend_class_entry;
typedef struct _zend_object zend_object;
typedef struct _zend_property_info zend_property_info;
typedef struct _zval_struct zval;

#define OBJ_BUCKET_INVALID			(1<<0)

#define IS_OBJ_VALID(o)				(!(((uintptr_t)(o)) & OBJ_BUCKET_INVALID))

#define SET_OBJ_INVALID(o)			((zend_object*)((((uintptr_t)(o)) | OBJ_BUCKET_INVALID)))

#define GET_OBJ_BUCKET_NUMBER(o)	(((intptr_t)(o)) >> 1)

#define SET_OBJ_BUCKET_NUMBER(o, n)	do { \
		(o) = (zend_object*)((((uintptr_t)(n)) << 1) | OBJ_BUCKET_INVALID); \
	} while (0)

#define ZEND_OBJECTS_STORE_ADD_TO_FREE_LIST(h) do { \
		SET_OBJ_BUCKET_NUMBER(EG(objects_store).object_buckets[(h)], EG(objects_store).free_list_head); \
		EG(objects_store).free_list_head = (h); \
	} while (0)

#define OBJ_RELEASE(obj) zend_object_release(obj)

typedef struct _zend_objects_store {
	zend_object **object_buckets;
	uint32_t top;
	uint32_t size;
	int free_list_head;
} zend_objects_store;

/* Global store handling functions */
BEGIN_EXTERN_C()
ZEND_API void ZEND_FASTCALL zend_objects_store_init(zend_objects_store *objects, uint32_t init_size);
ZEND_API void ZEND_FASTCALL zend_objects_store_call_destructors(zend_objects_store *objects);
ZEND_API void ZEND_FASTCALL zend_objects_store_mark_destructed(zend_objects_store *objects);
ZEND_API void ZEND_FASTCALL zend_objects_store_free_object_storage(zend_objects_store *objects, bool fast_shutdown);
ZEND_API void ZEND_FASTCALL zend_objects_store_destroy(zend_objects_store *objects);

/* Store API functions */
ZEND_API void ZEND_FASTCALL zend_objects_store_put(zend_object *object);
ZEND_API void ZEND_FASTCALL zend_objects_store_del(zend_object *object);

/* Called when the ctor was terminated by an exception */
ZEND_API void zend_object_store_ctor_failed(zend_object *obj);

ZEND_API void zend_object_release(zend_object *obj);

ZEND_API size_t zend_object_properties_size(zend_class_entry *ce);

/* Allocates object type and zeros it, but not the standard zend_object and properties.
 * Standard object MUST be initialized using zend_object_std_init().
 * Properties MUST be initialized using object_properties_init(). */
ZEND_API void *zend_object_alloc(size_t obj_size, zend_class_entry *ce);

ZEND_API ZEND_ATTRIBUTE_PURE zend_property_info *zend_get_property_info_for_slot(zend_object *obj, zval *slot);

/* Helper for cases where we're only interested in property info of typed properties. */
ZEND_API ZEND_ATTRIBUTE_PURE zend_property_info *zend_get_typed_property_info_for_slot(zend_object *obj, zval *slot);

END_EXTERN_C()

#endif /* ZEND_OBJECTS_H */
