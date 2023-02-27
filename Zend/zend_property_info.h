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

#ifndef ZEND_PROPERTY_INFO_H
#define ZEND_PROPERTY_INFO_H

#include "zend_type.h"

#include <stddef.h>
#include <stdint.h>

typedef struct _zend_class_entry zend_class_entry;
typedef struct _zend_string zend_string;
typedef struct _zend_array HashTable;

typedef struct _zend_property_info {
	uint32_t offset; /* property offset for object properties or
	                      property index for static properties */
	uint32_t flags;
	zend_string *name;
	zend_string *doc_comment;
	HashTable *attributes;
	zend_class_entry *ce;
	zend_type type;
} zend_property_info;

typedef struct {
	size_t num;
	size_t num_allocated;
	zend_property_info *ptr[1];
} zend_property_info_list;

typedef union {
	zend_property_info *ptr;
	uintptr_t list;
} zend_property_info_source_list;

#define ZEND_PROPERTY_INFO_SOURCE_FROM_LIST(list) (0x1 | (uintptr_t) (list))
#define ZEND_PROPERTY_INFO_SOURCE_TO_LIST(list) ((zend_property_info_list *) ((list) & ~0x1))
#define ZEND_PROPERTY_INFO_SOURCE_IS_LIST(list) ((list) & 0x1)

#endif /* ZEND_PROPERTY_INFO_H */
