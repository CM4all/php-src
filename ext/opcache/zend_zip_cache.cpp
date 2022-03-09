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

#include "zend_zip_cache.h"
#include "zend_system_id.h"
#include "zend_shared_alloc.h"
#include "zend_file_cache.h"
#include "zend_accelerator_util_funcs.h"
#include "ZipFormat.hxx"
#include "MemoryMapping.hxx"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#define PREFIX "opcache-"
#define SUFFIX ".zip"

/**
 * Pointer to the contents of a file inside a ZIP file.
 */
class ZendArchiveFile {
	const void *data;
	std::size_t size;

public:
	constexpr ZendArchiveFile(const void *_data, std::size_t _size) noexcept
		:data(_data), size(_size) {}

	/**
	 * Unserialize the bytecode into a #zend_persistent_script in
	 * shared memory and add it to ZCSG(hash).
	 */
	zend_persistent_script *LoadScript(zend_string *script_path) const noexcept;
};

inline zend_persistent_script *
ZendArchiveFile::LoadScript(zend_string *script_path) const noexcept
{
	if (ZCSG(restart_in_progress) ||
	    ZSMMG(memory_exhausted) ||
	    accelerator_shm_read_lock() != SUCCESS)
		return nullptr;

	const auto &info = *(const zend_file_cache_metainfo *)data;
	std::byte *p = (std::byte *)(&info + 1);

	ZCG(mem) = p + info.mem_size;

	zend_shared_alloc_lock();

	/* copy the bytecode file to shared memory */

	std::byte *buf = (std::byte *)zend_shared_alloc_aligned(info.mem_size);
	if (!buf) {
		zend_accel_schedule_restart_if_necessary(ACCEL_RESTART_OOM);
		zend_shared_alloc_unlock();
		return nullptr;
	}

	memcpy(buf, p, info.mem_size);
	zend_map_ptr_extend(ZCSG(map_ptr_last));

	/* unserialize the embedded zend_persistent_script instance */

	auto *script = (zend_persistent_script *)(buf + info.script_offset);
	script->corrupted = 0;

	zend_try {
		zend_file_cache_unserialize(script, buf);
	} zend_catch {
		zend_shared_alloc_unlock();
		return nullptr;
	} zend_end_try();

	script->corrupted = 0;

	ZCSG(map_ptr_last) = CG(map_ptr_last);
	script->dynamic_members.checksum = zend_accel_script_checksum(script);
	script->dynamic_members.last_used = ZCG(request_time);

	/* store the unserialized zend_persistent_script in ZCSG(hash)
	   to be reused for the next execution */

	zend_accel_hash_update(&ZCSG(hash), script->script.filename, 0, script);

	zend_shared_alloc_unlock();

	return script;
}

/**
 * A memory-mapped ZIP file containing PHP bytecode files.
 */
class ZendArchive {
	MemoryMapping mapping;

	std::unordered_map<std::string_view, ZendArchiveFile> files;

public:
	ZendArchive(void *_data, std::size_t _size) noexcept;

	zend_persistent_script *LoadScript(zend_string *script_path, std::string_view filename) const noexcept {
		if (auto i = files.find(filename); i != files.end())
			return i->second.LoadScript(script_path);

		return nullptr;
	}
};

inline ZendArchive::ZendArchive(void *_data, std::size_t _size) noexcept
	:mapping(_data, _size)
{
	const std::byte *i = (const std::byte *)mapping.data();
	const auto end = i + mapping.size();

	/* iterate over all files in the ZIP archive and add them to
	   the "files" map */
	while (true) {
		const auto &h = *(const ZipFileHeader *)i;
		i = (const std::byte *)(&h + 1);
		if (i > end)
			break;

		if (h.magic[0] != 0x50 || h.magic[1] != 0x4b ||
		    h.magic[2] != 0x03 || h.magic[3] != 0x04 ||
		    h.uncompressed_size == 0xffffffff)
			break;

		std::string_view filename{(const char *)i, h.name_length};
		i += h.name_length;
		i += h.extra_length;

		if (i > end)
			break;

		const void *data = i;
		const std::size_t data_size = h.uncompressed_size;

		i += data_size;
		if (i > end)
			break;

		if (h.compression_method == 0) {
			if (filename.size() > 4 && memcmp(filename.data() + filename.size() - 4, ".bin", 4) == 0)
				filename.remove_suffix(4);

			files.try_emplace(filename, data, data_size);
		}
	}
}

struct ZendZipCache {
private:
	std::map<std::string, ZendArchive, std::less<>> archives;

public:
	void LoadArchive(std::string_view path) noexcept;

	zend_persistent_script *LoadScript(zend_string *script_path, std::string_view path) const noexcept;
};

/**
 * Build the path of the ZIP file in the specified directory.  Free
 * with efree().
 */
static char *get_zend_zip_path(std::string_view path) noexcept
{
	const size_t length = path.size() + sizeof(PREFIX) - 1 + 1 + 32 + sizeof(SUFFIX);
	char *filename = (char *)emalloc(length), *p = filename;

	p = std::copy(path.begin(), path.end(), p);
	*p++ = '/';
	p = (char *)mempcpy(p, PREFIX, sizeof(PREFIX) - 1);
	p = (char *)mempcpy(p, zend_system_id, 32);
	memcpy(p, SUFFIX, sizeof(SUFFIX));

	return filename;
}

static int open_zend_zip(std::string_view path) noexcept
{
	char *zip_path = get_zend_zip_path(path);
	int fd = open(zip_path, O_RDONLY|O_CLOEXEC);
	efree(zip_path);
	return fd;
}

static std::optional<ZendArchive> zend_archive_open(std::string_view path)
{
	int fd = open_zend_zip(path);
	if (fd < 0)
		return std::nullopt;

	struct stat st;
	if (fstat(fd, &st) < 0 || !S_ISREG(st.st_mode) ||
	    st.st_size <= 0 || st.st_size > 1024 * 1024 * 1024) {
		close(fd);
		return std::nullopt;
	}

	void *mapping = mmap(nullptr, st.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	close(fd);
	if (mapping == MAP_FAILED)
		return std::nullopt;

	return ZendArchive(mapping, st.st_size);
}

struct ZendZipCache *zend_zip_cache_new()
{
	return new ZendZipCache();
}

void zend_zip_cache_close(struct ZendZipCache *cache)
{
	delete cache;
}

inline void ZendZipCache::LoadArchive(std::string_view path) noexcept
{
	if (auto archive = zend_archive_open(path))
		archives.try_emplace(std::string{path}, std::move(*archive));
}

void zend_zip_cache_load(struct ZendZipCache *cache, const char *path, size_t path_length)
{
	cache->LoadArchive({path, path_length});
}

inline zend_persistent_script *ZendZipCache::LoadScript(zend_string *script_path, std::string_view path) const noexcept
{
	for (auto i = archives.lower_bound(path); i != archives.begin();) {
		--i;

		const std::string_view i_path = i->first;
		if (i_path.size() >= path.size() ||
		    memcmp(path.data(), i_path.data(), i_path.size()) != 0)
			return nullptr;

		if (path[i_path.size()] == '/') {
			std::string_view filename{path};
			filename.remove_prefix(i_path.size() + 1);
			return i->second.LoadScript(script_path, filename);
		}
	}

	return nullptr;
}

zend_persistent_script *zend_zip_cache_script_load(struct ZendZipCache *cache, zend_string *script_path)
{
	assert(cache != nullptr);
	assert(script_path != nullptr);

	const std::string_view path{ZSTR_VAL(script_path), ZSTR_LEN(script_path)};
	return cache->LoadScript(script_path, path);
}
