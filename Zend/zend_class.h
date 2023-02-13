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

#ifndef ZEND_CLASS_H
#define ZEND_CLASS_H

#include "zend_hash.h"
#include "zend_map_ptr.h" // for ZEND_MAP_PTR_DEF()

#include <stdint.h>

typedef struct _zend_class_arrayaccess_funcs zend_class_arrayaccess_funcs;
typedef struct _zend_class_entry zend_class_entry;
typedef struct _zend_class_iterator_funcs zend_class_iterator_funcs;
typedef union  _zend_function zend_function;
typedef struct _zend_object_handlers zend_object_handlers;
typedef struct _zend_object_iterator zend_object_iterator;
typedef struct _zend_serialize_data zend_serialize_data;
typedef struct _zend_string zend_string;
typedef struct _zval_struct zval;
typedef struct _zend_unserialize_data zend_unserialize_data;
typedef struct _zend_array HashTable;

typedef struct _zend_class_name {
	zend_string *name;
	zend_string *lc_name;
} zend_class_name;

typedef struct _zend_trait_method_reference {
	zend_string *method_name;
	zend_string *class_name;
} zend_trait_method_reference;

typedef struct _zend_trait_precedence {
	zend_trait_method_reference trait_method;
	uint32_t num_excludes;
	zend_string *exclude_class_names[1];
} zend_trait_precedence;

typedef struct _zend_trait_alias {
	zend_trait_method_reference trait_method;

	/**
	* name for method to be added
	*/
	zend_string *alias;

	/**
	* modifiers to be set on trait method
	*/
	uint32_t modifiers;
} zend_trait_alias;

typedef struct _zend_class_mutable_data {
	zval      *default_properties_table;
	HashTable *constants_table;
	uint32_t   ce_flags;
	HashTable *backed_enum_table;
} zend_class_mutable_data;

typedef struct _zend_class_dependency {
	zend_string      *name;
	zend_class_entry *ce;
} zend_class_dependency;

typedef struct _zend_inheritance_cache_entry zend_inheritance_cache_entry;

typedef struct _zend_error_info {
	int type;
	uint32_t lineno;
	zend_string *filename;
	zend_string *message;
} zend_error_info;

struct _zend_inheritance_cache_entry {
	zend_inheritance_cache_entry *next;
	zend_class_entry             *ce;
	zend_class_entry             *parent;
	zend_class_dependency        *dependencies;
	uint32_t                      dependencies_count;
	uint32_t                      num_warnings;
	zend_error_info             **warnings;
	zend_class_entry             *traits_and_interfaces[1];
};

struct _zend_class_entry {
	char type;
	zend_string *name;
	/* class_entry or string depending on ZEND_ACC_LINKED */
	union {
		zend_class_entry *parent;
		zend_string *parent_name;
	};
	int refcount;
	uint32_t ce_flags;

	int default_properties_count;
	int default_static_members_count;
	zval *default_properties_table;
	zval *default_static_members_table;
	ZEND_MAP_PTR_DEF(zval *, static_members_table);
	HashTable function_table;
	HashTable properties_info;
	HashTable constants_table;

	ZEND_MAP_PTR_DEF(zend_class_mutable_data*, mutable_data);
	zend_inheritance_cache_entry *inheritance_cache;

	struct _zend_property_info **properties_info_table;

	zend_function *constructor;
	zend_function *destructor;
	zend_function *clone;
	zend_function *__get;
	zend_function *__set;
	zend_function *__unset;
	zend_function *__isset;
	zend_function *__call;
	zend_function *__callstatic;
	zend_function *__tostring;
	zend_function *__debugInfo;
	zend_function *__serialize;
	zend_function *__unserialize;

	const zend_object_handlers *default_object_handlers;

	/* allocated only if class implements Iterator or IteratorAggregate interface */
	zend_class_iterator_funcs *iterator_funcs_ptr;
	/* allocated only if class implements ArrayAccess interface */
	zend_class_arrayaccess_funcs *arrayaccess_funcs_ptr;

	/* handlers */
	union {
		zend_object* (*create_object)(zend_class_entry *class_type);
		int (*interface_gets_implemented)(zend_class_entry *iface, zend_class_entry *class_type); /* a class implements this interface */
	};
	zend_object_iterator *(*get_iterator)(zend_class_entry *ce, zval *object, int by_ref);
	zend_function *(*get_static_method)(zend_class_entry *ce, zend_string* method);

	/* serializer callbacks */
	int (*serialize)(zval *object, unsigned char **buffer, size_t *buf_len, zend_serialize_data *data);
	int (*unserialize)(zval *object, zend_class_entry *ce, const unsigned char *buf, size_t buf_len, zend_unserialize_data *data);

	uint32_t num_interfaces;
	uint32_t num_traits;

	/* class_entry or string(s) depending on ZEND_ACC_LINKED */
	union {
		zend_class_entry **interfaces;
		zend_class_name *interface_names;
	};

	zend_class_name *trait_names;
	zend_trait_alias **trait_aliases;
	zend_trait_precedence **trait_precedences;
	HashTable *attributes;

	uint32_t enum_backing_type;
	HashTable *backed_enum_table;

	union {
		struct {
			zend_string *filename;
			uint32_t line_start;
			uint32_t line_end;
			zend_string *doc_comment;
		} user;
		struct {
			const struct _zend_function_entry *builtin_functions;
			struct _zend_module_entry *module;
		} internal;
	} info;
};

#endif /* ZEND_CLASS_H */
