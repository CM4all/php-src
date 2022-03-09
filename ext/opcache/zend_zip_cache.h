/*
   +----------------------------------------------------------------------+
   | Zend OPcache                                                         |
   +----------------------------------------------------------------------+
   | Copyright (c) CM4all GmbH                                            |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | https://www.php.net/license/3_01.txt                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Max Kellermann <mk@cm4all.com>                               |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_ZIP_CACHE_H
#define ZEND_ZIP_CACHE_H

#include "zend_API.h"

struct ZendZipCache;
typedef struct _zend_persistent_script zend_persistent_script;
typedef struct _zend_string zend_string;

BEGIN_EXTERN_C()

struct ZendZipCache *zend_zip_cache_new(void);
void zend_zip_cache_free(struct ZendZipCache *cache);

void zend_zip_cache_load(struct ZendZipCache *cache, const char *path, size_t path_length);

zend_persistent_script *zend_zip_cache_script_load(struct ZendZipCache *cache, zend_string *script_path);

END_EXTERN_C()

#endif /* ZEND_ZIP_CACHE_H */
