/*
   +----------------------------------------------------------------------+
   | Zend OPcache                                                         |
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | https://www.php.net/license/3_01.txt                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_FILE_CACHE_H
#define ZEND_FILE_CACHE_H

#include "ZendAccelerator.h" // for accel_time_t

#include <stddef.h>

int zend_file_cache_script_store(zend_persistent_script *script, bool in_shm);
zend_persistent_script *zend_file_cache_script_load(zend_file_handle *file_handle);
void zend_file_cache_invalidate(zend_string *full_path);

typedef struct _zend_file_cache_metainfo {
	char         magic[8];
	char         system_id[32];
	size_t       mem_size;
	size_t       str_size;
	size_t       script_offset;
	accel_time_t timestamp;
	uint32_t     checksum;
} zend_file_cache_metainfo;

BEGIN_EXTERN_C()

/**
 * Helper function for zend_file_cache_script_store().
 *
 * @return true on success, false on error
 */
bool zend_file_cache_script_store_fd(const int fd, zend_persistent_script *script, const bool in_shm);

void zend_file_cache_unserialize(zend_persistent_script  *script,
                                 void                    *buf);

END_EXTERN_C()

#endif /* ZEND_FILE_CACHE_H */
