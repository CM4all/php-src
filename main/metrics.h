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

#include <stdbool.h>
#include <time.h>

/**
 * Metrics to be submitted to the WAS client at the end of a request.
 */
struct php_was_metrics {
	unsigned mysql_connect_count, mysql_connect_errors;
	float mysql_connect_wait;

	unsigned mysql_query_count, mysql_query_errors;
	float mysql_query_wait;
};

extern bool want_was_metrics;

extern struct php_was_metrics was_metrics;
