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

#ifndef ZEND_ENDIAN_H
#define ZEND_ENDIAN_H

#ifdef PHP_WIN32
#include "config.w32.h"
#else
#include "zend_config.h"
#endif

#ifdef WORDS_BIGENDIAN
# define ZEND_ENDIAN_LOHI(lo, hi)          hi; lo;
# define ZEND_ENDIAN_LOHI_3(lo, mi, hi)    hi; mi; lo;
# define ZEND_ENDIAN_LOHI_4(a, b, c, d)    d; c; b; a;
# define ZEND_ENDIAN_LOHI_C(lo, hi)        hi, lo
# define ZEND_ENDIAN_LOHI_C_3(lo, mi, hi)  hi, mi, lo,
# define ZEND_ENDIAN_LOHI_C_4(a, b, c, d)  d, c, b, a
#else
# define ZEND_ENDIAN_LOHI(lo, hi)          lo; hi;
# define ZEND_ENDIAN_LOHI_3(lo, mi, hi)    lo; mi; hi;
# define ZEND_ENDIAN_LOHI_4(a, b, c, d)    a; b; c; d;
# define ZEND_ENDIAN_LOHI_C(lo, hi)        lo, hi
# define ZEND_ENDIAN_LOHI_C_3(lo, mi, hi)  lo, mi, hi,
# define ZEND_ENDIAN_LOHI_C_4(a, b, c, d)  a, b, c, d
#endif

#endif /* ZEND_ENDIAN_H */
