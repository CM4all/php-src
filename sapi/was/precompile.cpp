/*
   +----------------------------------------------------------------------+
   | Copyright (c) CM4all GmbH                                            |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at the following url:        |
   | http://www.php.net/license/3_01.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Max Kellermann <max.kellermann@ionos.com>                    |
   +----------------------------------------------------------------------+
*/

#include "precompile.h"
#include "php.h"
#include "zend_system_id.h"
#include "zend_exceptions.h"
#include "../../ext/opcache/ZendAccelerator.h"
#include "../../ext/opcache/zend_accelerator_util_funcs.h"
#include "../../ext/opcache/zend_file_cache.h"
#include "../../ext/opcache/zend_shared_alloc.h"
#include "../../ext/opcache/zend_persist.h"
#include "../../ext/opcache/ZipFormat.hxx"
#include "ScopeExit.hxx"

#include <zlib.h> // for crc32()

#include <vector>

#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>

/**
 * Call opcache_compile_file(), catching bailouts.  This function must
 * not have objects with non-trivial destructors.
 */
static zend_persistent_script *
CompileFile(zend_file_handle *file_handle, zend_op_array **op_array_p) noexcept
{
	zend_persistent_script *persistent_script = nullptr;

	zend_try {
		persistent_script = opcache_compile_file(file_handle, ZEND_INCLUDE, op_array_p);
	} zend_end_try();

	return persistent_script;
}

static bool
PrecompileFile(zend_file_handle *file_handle, int output_fd)
{
	/* compile */

	zend_op_array *op_array;
	zend_persistent_script *persistent_script =
		CompileFile(file_handle, &op_array);
	if (persistent_script == nullptr) {
		if (EG(exception))
			zend_exception_error(EG(exception), E_ERROR);

		return false;
	}

	persistent_script->timestamp = 0x87654321;

	persistent_script->script.filename = file_handle->opened_path != nullptr
		? file_handle->opened_path
		: file_handle->filename;

	/* optimize */

	zend_optimize_script(&persistent_script->script, ZCG(accel_directives).optimization_level, ZCG(accel_directives).opt_debug_level);

	/* persist */

	zend_shared_alloc_init_xlat_table();

	constexpr bool for_shm = false;

	const size_t memory_used = zend_accel_script_persist_calc(persistent_script, for_shm);

	// TODO align?
	void *mem = ZCG(mem) = memalign(64, memory_used);
	AtScopeExit(mem) { free(mem); };

	zend_shared_alloc_clear_xlat_table();

	persistent_script->timestamp = 0x12345678;

	/* Copy into memory block */
	zend_persistent_script *new_persistent_script = zend_accel_script_persist(persistent_script, for_shm);

	/* disable timestamp validation and reduce differences between
	   opcaches if only the timestamps differ; this will allow
	   deduplication by reusing file extents */
	//new_persistent_script->timestamp = 0;
	new_persistent_script->timestamp = 0xdeadbeef;

	zend_shared_alloc_destroy_xlat_table();

	new_persistent_script->is_phar = false;

	/* Consistency check */
	assert((char*)new_persistent_script->mem + new_persistent_script->size == (char*)ZCG(mem));

	/* store */

	return zend_file_cache_script_store_fd(output_fd,
					       new_persistent_script,
					       for_shm);
}

static bool
PrecompileFile(const char *source_filename, int output_fd)
{
	zend_file_handle file_handle;
	zend_stream_init_filename(&file_handle, source_filename);
	AtScopeExit(&file_handle) { zend_destroy_file_handle(&file_handle); };

	return PrecompileFile(&file_handle, output_fd);
}

static bool
CommitFile(int tmp_fd, int directory_fd, const char *new_path)
{
	unlinkat(directory_fd, new_path, 0);

	char fd_path[64];
	snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", tmp_fd);
	if (linkat(AT_FDCWD, fd_path,
		   directory_fd, new_path,
		   AT_SYMLINK_FOLLOW) < 0) {
		perror("Failed to commit file");
		return false;
	}

	return true;
}

[[gnu::pure]]
static const char *
NormalizeFilename(const char *filename) noexcept
{
	if (strncmp(filename, "./", 2) == 0 && filename[2] != 0)
		filename += 2;

	return filename;
}

static bool
WriteFull(int fd, const void *data, size_t size) noexcept
{
	ssize_t nbytes = write(fd, data, size);
	if (nbytes < 0) {
		perror("Failed to write");
		return false;
	}

	if ((size_t)nbytes != size) {
		fprintf(stderr, "Short write\n");
		return false;
	}

	return true;
}

static bool
WritePreliminaryZipFileHeader(int fd, const char *filename, ZipFileHeader &h)
{
	const size_t filename_length = strlen(filename);

	h = {};
	h.compression_method = 0;
	h.name_length = filename_length;

	return WriteFull(fd, &h, sizeof(h)) &&
		WriteFull(fd, filename, filename_length);
}

[[gnu::pure]]
static uint32_t
CrcFromFile(int fd, off_t offset, size_t size) noexcept
{
	auto crc = crc32(0, nullptr, 0);

	while (size > 0) {
		Bytef buffer[16384];
		auto nbytes = pread(fd, buffer, std::min(sizeof(buffer), size),
				    offset);
		if (nbytes <= 0)
			abort();

		crc = crc32(crc, buffer, nbytes);
		offset += nbytes;
		size -= nbytes;
	}

	return crc;
}

bool
precompile(const char *const*files, size_t n_files)
{
	CG(compiler_options) |= ZEND_COMPILE_WITHOUT_EXECUTION|ZEND_COMPILE_WITH_FILE_CACHE;

	const int fd = open(".", O_TMPFILE|O_RDWR|O_CLOEXEC, 0644);
	if (fd < 0) {
		perror("Failed to create temporary file");
		return false;
	}

	AtScopeExit(fd) { close(fd); };

	std::vector<ZipDirectoryEntry> entries;
	entries.reserve(n_files);

	/* compile all sources specified on the command line and
	   append the opcode files into the ZIP file */
	for (size_t i = 0; i < n_files; ++i) {
		const char *const source_filename = NormalizeFilename(files[i]);

		/* remember the offset and write a preliminary
		   ZipFileHeader (which will be updated later) */
		const auto header_offset = lseek(fd, 0, SEEK_CUR);
		ZipFileHeader h;
		if (!WritePreliminaryZipFileHeader(fd, source_filename, h))
			return false;

		/* compile the file and write the opcode into the ZIP
		   file */
		const auto data_offset = lseek(fd, 0, SEEK_CUR);
		if (!PrecompileFile(source_filename, fd)) {
			fprintf(stderr, "Failed to compile %s\n", source_filename);
			return false;
		}

		/* finalize the ZipFileHeader, updating the size and
		   the CRC */

		const auto end_offset = lseek(fd, 0, SEEK_CUR);
		const size_t data_size = end_offset - data_offset;

		h.compressed_size = h.uncompressed_size = data_size;
		h.crc = CrcFromFile(fd, data_offset, data_size);

		if (pwrite(fd, &h, sizeof(h), header_offset) != (ssize_t)sizeof(h)) {
			perror("Failed to write");
			return false;
		}

		/* remember header data so we can later write central
		   directory */

		entries.emplace_back(h);
		entries.back().local_header_offset = header_offset;
	}

	/* write the central directory */

	ZipDirectoryEnd end{};
	end.disk_entries = end.total_entries = n_files;
	end.directory_offset = lseek(fd, 0, SEEK_CUR);

	size_t directory_size = 0;
	for (size_t i = 0; i < n_files; ++i) {
		const char *const name = NormalizeFilename(files[i]);
		const size_t name_length = strlen(name);

		const auto &entry = entries[i];
		if (!WriteFull(fd, &entry, sizeof(entry)) ||
		    !WriteFull(fd, name, name_length))
			return false;

		directory_size += sizeof(entry) + name_length;
	}

	end.directory_size = directory_size;

	if (!WriteFull(fd, &end, sizeof(end)))
		return false;

	/* truncate the ZIP file at the current position, just in case
	   we reverted a file which was already written */
	if (ftruncate(fd, lseek(fd, 0, SEEK_CUR)) < 0) {
		perror("Failed to truncate");
		return false;
	}

	/* commit the ZIP file, link it to its final filename */

	char zip_filename[64];
	sprintf(zip_filename, "opcache-%.32s.zip", zend_system_id);
	return CommitFile(fd, AT_FDCWD, zip_filename);
}
