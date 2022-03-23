/*
   +----------------------------------------------------------------------+
   | Zend OPcache                                                         |
   +----------------------------------------------------------------------+
   | Copyright © The PHP Group and Contributors.                          |
   +----------------------------------------------------------------------+
   | This source file is subject to the Modified BSD License that is      |
   | bundled with this package in the file LICENSE, and is available      |
   | through the World Wide Web at <https://www.php.net/license/>.        |
   |                                                                      |
   | SPDX-License-Identifier: BSD-3-Clause                                |
   +----------------------------------------------------------------------+
   | Authors: Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_FILE_CACHE_H
#define ZEND_FILE_CACHE_H

int zend_file_cache_script_store(zend_persistent_script *script, bool in_shm);
zend_persistent_script *zend_file_cache_script_load(zend_file_handle *file_handle);
zend_persistent_script *zend_file_cache_script_load_ex(zend_file_handle *file_handle, bool validate_only);
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
