/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright © Zend Technologies Ltd., a subsidiary company of          |
   |     Perforce Software, Inc., and Contributors.                       |
   +----------------------------------------------------------------------+
   | This source file is subject to the Modified BSD License that is      |
   | bundled with this package in the file LICENSE, and is available      |
   | through the World Wide Web at <https://www.php.net/license/>.        |
   |                                                                      |
   | SPDX-License-Identifier: BSD-3-Clause                                |
   +----------------------------------------------------------------------+
   | Authors: Ilija Tovilo <ilutov@php.net>                               |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_ENUM_H
#define ZEND_ENUM_H

#include "zend_char.h"
#include "zend_long.h"
#include "zend_portability.h" // for BEGIN_EXTERN_C
#include "zend_result.h"
#include "zend_types.h" // for struct _zend_object

#include <stdbool.h>

#include <stdint.h>

typedef struct _zend_class_entry zend_class_entry;
typedef struct _zend_function_entry zend_function_entry;
typedef struct _zend_object zend_object;
typedef struct _zend_object_handlers zend_object_handlers;
typedef struct _zend_string zend_string;
typedef struct _zval_struct zval;

BEGIN_EXTERN_C()

extern ZEND_API zend_class_entry *zend_ce_unit_enum;
extern ZEND_API zend_class_entry *zend_ce_backed_enum;
extern ZEND_API zend_object_handlers zend_enum_object_handlers;

typedef struct zend_enum_obj {
	int         case_id;
	zend_object std;
} zend_enum_obj;

ZEND_API zend_enum_obj *zend_enum_obj_from_obj(zend_object *zobj);

void zend_enum_startup(void);
void zend_register_enum_ce(void);
void zend_enum_add_interfaces(zend_class_entry *ce);
zend_result zend_enum_build_backed_enum_table(zend_class_entry *ce);
zend_object *zend_enum_new(zval *result, zend_class_entry *ce, int case_id, zend_string *case_name, zval *backing_value_zv);
void zend_verify_enum(const zend_class_entry *ce);
void zend_enum_register_funcs(zend_class_entry *ce);
void zend_enum_register_props(zend_class_entry *ce);
int zend_enum_next_case_id(zend_class_entry *enum_class);

ZEND_API zend_class_entry *zend_register_internal_enum(
	const char *name, uint8_t type, const zend_function_entry *functions);
ZEND_API void zend_enum_add_case(zend_class_entry *ce, zend_string *case_name, zval *value);
ZEND_API void zend_enum_add_case_cstr(zend_class_entry *ce, const char *name, zval *value);
ZEND_API zend_object *zend_enum_get_case(zend_class_entry *ce, zend_string *name);
ZEND_API zend_object *zend_enum_get_case_cstr(zend_class_entry *ce, const char *name);
ZEND_API zend_object *zend_enum_get_case_by_id(zend_class_entry *ce, int id);
ZEND_API zend_result zend_enum_get_case_by_value(zend_object **result, zend_class_entry *ce, zend_long long_key, zend_string *string_key, bool try_from);

ZEND_API int zend_enum_fetch_case_id(zend_object *zobj);
ZEND_API zval *zend_enum_fetch_case_name(zend_object *zobj);
ZEND_API zval *zend_enum_fetch_case_value(zend_object *zobj);

END_EXTERN_C()

#endif /* ZEND_ENUM_H */
