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

#ifndef ZEND_RESOURCE_H
#define ZEND_RESOURCE_H

#include "zend_refcounted.h"
#include "zend_long.h"

typedef struct _zend_resource {
	zend_refcounted_h gc;
	zend_long         handle; // TODO: may be removed ???
	int               type;
	void             *ptr;
} zend_resource;

#endif /* ZEND_RESOURCE_H */
