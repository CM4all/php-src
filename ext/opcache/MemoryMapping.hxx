/*
 * Copyright 2022 CM4all GmbH
 * All rights reserved.
 *
 * author: Max Kellermann <mk@cm4all.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstddef>
#include <utility>

#include <sys/mman.h>

/**
 * C++ wrapper for a memory-mapped file.
 */
class MemoryMapping {
	void *data_ = MAP_FAILED;
	std::size_t size_;

public:
	MemoryMapping(void *_data, std::size_t _size) noexcept
		:data_(_data), size_(_size) {}

	MemoryMapping(MemoryMapping &&src) noexcept
		:data_(std::exchange(src.data_, MAP_FAILED)),
		 size_(src.size_) {}

	~MemoryMapping() noexcept {
		if (data_ != MAP_FAILED)
			munmap(data_, size_);
	}

	MemoryMapping &operator=(MemoryMapping &&src) noexcept {
		using std::swap;
		swap(data_, src.data_);
		swap(size_, src.size_);
		return *this;
	}

	void *data() const noexcept {
		return data_;
	}

	std::size_t size() const noexcept {
		return size_;
	}
};
