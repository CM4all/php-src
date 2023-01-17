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
   | Authors: Dmitry Stogov <dmitry@zend.com>                             |
   |          Xinchen Hui <laruence@php.net>                              |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_GDB
#define ZEND_GDB

#ifdef PHP_WIN32
# include "zend_config.w32.h"
#else
# include "php_config.h" // for ZEND_API
#endif

#include <stdbool.h>
#include <stddef.h> // for size_t

ZEND_API bool zend_gdb_register_code(const void *object, size_t size);
ZEND_API void zend_gdb_unregister_all(void);
ZEND_API bool zend_gdb_present(void);

#endif
