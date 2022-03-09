/*
 * author: Max Kellermann <mk@cm4all.com>
 */

#pragma once

#include "LittleEndian.hxx"

#include <stdint.h>

struct ZipFileHeader {
	uint8_t magic[4] = {'P', 'K', 0x03, 0x04};
	uint8_t extract_version = 20;
	uint8_t os = 0;
	LE16 flags = 0;
	LE16 compression_method = 8;
	LE16 time = 0, date = 0;
	LE32 crc;
	LE32 compressed_size, uncompressed_size;
	LE16 name_length;
	LE16 extra_length = 0;
};

static_assert(sizeof(ZipFileHeader) == 0x1e);

struct ZipDirectoryEntry {
	uint8_t magic[4] = {'P', 'K', 0x01, 0x02};
	LE16 version = 0x031e, version_needed = 0x0014;
	LE16 flags = 0;
	LE16 compression_method = 8;
	LE16 time = 0, date = 0;
	LE32 crc;
	LE32 compressed_size, uncompressed_size;
	LE16 name_length;
	LE16 extra_length = 0;
	LE16 comment_length = 0;
	LE16 disk = 0;
	LE16 internal_attributes = 0x0001; /* bit 0 = text file */
	LE32 external_attributes = 0644 << 16;
	LE32 local_header_offset = 0;

	ZipDirectoryEntry() = default;

	ZipDirectoryEntry(const ZipFileHeader &src)
		:crc(src.crc),
		 compression_method(src.compression_method),
		 compressed_size(src.compressed_size),
		 uncompressed_size(src.uncompressed_size),
		 name_length(src.name_length) {}
};

static_assert(sizeof(ZipDirectoryEntry) == 0x2e);

struct ZipDirectoryEnd {
	uint8_t magic[4] = {'P', 'K', 0x05, 0x06};
	LE16 disk = 0, disk_cd = 0;
	LE16 disk_entries, total_entries;
	LE32 directory_size;
	LE32 directory_offset;
	LE16 comment_length = 0;
};

static_assert(sizeof(ZipDirectoryEnd) == 0x16);
