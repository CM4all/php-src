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
   | Author: Max Kellermann <mk@cm4all.com>                               |
   +----------------------------------------------------------------------+
*/

#pragma once

#include <time.h>

struct wallclock {
	struct timespec start;
};

static inline void
wallclock_start(struct wallclock *w)
{
	clock_gettime(CLOCK_MONOTONIC, &w->start);
}

static inline float
timespec_diff_float(const struct timespec *start,
		    const struct timespec *end)
{
	return (float)(end->tv_sec - start->tv_sec) +
		(float)((end->tv_nsec - start->tv_nsec) * 1e-9f);
}

static inline float
wallclock_end(const struct wallclock *w)
{
	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC, &end);

	return timespec_diff_float(&w->start, &end);
}
