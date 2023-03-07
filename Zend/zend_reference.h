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

#ifndef ZEND_REFERENCE_H
#define ZEND_REFERENCE_H

#include "zend_property_info.h"
#include "zend_refcounted.h"
#include "zend_value.h"

struct _zend_reference {
	zend_refcounted_h              gc;
	zval                           val;
	zend_property_info_source_list sources;
};

#endif /* ZEND_REFERENCE_H */
