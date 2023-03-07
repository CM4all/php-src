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
   |          Xinchen Hui <laruence@php.net>                              |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_TYPES_H
#define ZEND_TYPES_H

#ifdef __SSE2__
# include <emmintrin.h>
#endif
#if defined(__AVX2__)
# include <immintrin.h>
#endif
#if defined(__aarch64__) || defined(_M_ARM64)
# include <arm_neon.h>
#endif

/* This constant is deprecated, use SIZE_MAX instead */
#define ZEND_SIZE_MAX SIZE_MAX

#ifdef ZTS
#define ZEND_TLS static TSRM_TLS
#define ZEND_EXT_TLS TSRM_TLS
#else
#define ZEND_TLS static
#define ZEND_EXT_TLS
#endif

#endif /* ZEND_TYPES_H */
