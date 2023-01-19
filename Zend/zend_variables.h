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
   | Authors: Andi Gutmans <andi@php.net>                                 |
   |          Zeev Suraski <zeev@php.net>                                 |
   |          Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_VARIABLES_H
#define ZEND_VARIABLES_H

#include "zend_portability.h" // for BEGIN_EXTERN_C

typedef struct _zend_refcounted zend_refcounted;
typedef struct _zval_struct zval;

BEGIN_EXTERN_C()

ZEND_API void ZEND_FASTCALL rc_dtor_func(zend_refcounted *p);
ZEND_API void ZEND_FASTCALL zval_copy_ctor_func(zval *zvalue);

ZEND_API void zval_ptr_dtor_nogc(zval *zval_ptr);

ZEND_API void i_zval_ptr_dtor(zval *zval_ptr);

ZEND_API void zval_copy_ctor(zval *zvalue);

ZEND_API void zval_opt_copy_ctor(zval *zvalue);

ZEND_API void zval_ptr_dtor_str(zval *zval_ptr);

ZEND_API void zval_ptr_dtor(zval *zval_ptr);
ZEND_API void zval_ptr_safe_dtor(zval *zval_ptr);
ZEND_API void zval_internal_ptr_dtor(zval *zvalue);

ZEND_API void zval_add_ref(zval *p);

END_EXTERN_C()

#define ZVAL_PTR_DTOR zval_ptr_dtor
#define ZVAL_INTERNAL_PTR_DTOR zval_internal_ptr_dtor

#endif
