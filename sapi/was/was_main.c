/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
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

#include "SAPI.h"
#include "php_main.h"
#include "php_variables.h"
#include "php_getopt.h"
#include "zend.h"
#include "ext/standard/head.h"

#include <was/simple.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/uio.h>

static char *was_cookies = NULL;

static void init_sapi_from_env(sapi_module_struct *sapi_module)
{
	char *p = getenv("PHP_INI");
	if (p)
		sapi_module->php_ini_path_override = p;
}

static int php_was_startup(sapi_module_struct *sapi_module)
{
	if (php_module_startup(sapi_module, NULL, 0)==FAILURE) {
		return FAILURE;
	}
	return SUCCESS;
}

static int sapi_was_activate()
{
	return SUCCESS;
}

static int sapi_was_deactivate(void)
{
	free(was_cookies);
	was_cookies = NULL;

	if (SG(request_info).path_translated) {
		efree(SG(request_info).path_translated);
		SG(request_info).path_translated = NULL;
	}

	return SUCCESS;
}

static size_t sapi_was_ub_write(const char *str, size_t str_length)
{
	struct was_simple *const w = SG(server_context);
	if (!was_simple_write(w, str, str_length))
		php_handle_aborted_connection();
	return str_length;
}

static char *sapi_was_getenv(const char *name, size_t name_len)
{
	struct was_simple *const w = SG(server_context);
	return (char *)was_simple_get_parameter(w, name);
}

static bool send_was_header(struct was_simple *w,
			    const char *data, size_t length)
{
	const char *const end = data + length;
	const char *colon = memchr(data, ':', length);
	if (colon == NULL || colon == data)
		return true;

	const char *name = data;
	size_t name_length = colon - data;

	const char *value = colon + 1;
	while (value < end && *value == ' ')
		++value;

	const size_t value_length = end - value;

	return was_simple_set_header_n(w, name, name_length,
				       value, value_length);
}

static int sapi_was_send_headers(sapi_headers_struct *sapi_headers)
{
	struct was_simple *const w = SG(server_context);

	if (!was_simple_status(w, SG(sapi_headers).http_response_code))
		return SAPI_HEADER_SEND_FAILED;

	zend_llist_position pos;
	for (sapi_header_struct *h =
		     zend_llist_get_first_ex(&sapi_headers->headers, &pos);
	     h != NULL;
	     h = zend_llist_get_next_ex(&sapi_headers->headers, &pos)) {
		if (!send_was_header(w, h->header, h->header_len))
			return SAPI_HEADER_SEND_FAILED;
	}

	if (SG(sapi_headers).send_default_content_type) {
		char    *hd;

		hd = sapi_get_default_content_type();
		was_simple_set_header(w, "content-type", hd);
		efree(hd);
	}

	return SAPI_HEADER_SENT_SUCCESSFULLY;
}

static size_t sapi_was_read_post(char *buffer, size_t count_bytes)
{
	struct was_simple *const w = SG(server_context);

	/* PHP doesn't handle partial reads properly, so we loop until
	   either the requested number of bytes have been read, the
	   end of the request body is reached or an error occurs */

	size_t position = 0;
	while (position < count_bytes) {
		ssize_t nbytes = was_simple_read(w, buffer + position,
						 count_bytes - position);
		if (nbytes <= 0) {
			if (nbytes < 0)
				/* don't return any data if an error
				   has occurred */
				return 0;
			break;
		}

		position += (size_t)nbytes;
	}

	return position;
}

static char *concat_cookies(char *a, const char *b)
{
	if (a == NULL)
		return strdup(b);

	const size_t a_length = strlen(a);
	const size_t b_length = strlen(b);

	a = realloc(a, a_length + 2 + b_length + 1);
	if (a == NULL)
		return NULL;

	char *p = a + a_length;
	*p++ = ';';
	*p++ = ' ';
	memcpy(p, b, b_length + 1);

	return a;
}

static char *sapi_was_read_cookies(void)
{
	if (was_cookies != NULL)
		return was_cookies;

	struct was_simple *const w = SG(server_context);

	struct was_simple_iterator *i = was_simple_get_multi_header(w, "cookie");
	const struct was_simple_pair *pair;
	while ((pair = was_simple_iterator_next(i)) != NULL) {
		char *new_cookies = concat_cookies(was_cookies, pair->value);
		if (new_cookies == NULL)
			break;

		was_cookies = new_cookies;
	}

	was_simple_iterator_free(i);

	return was_cookies;
}

static int add_variable(int filter_arg, const char *name, const char *_value,
			zval *array)
{
	char *value = (char *)_value;
	size_t value_length = strlen(value);

	if (sapi_module.input_filter(filter_arg, (char *)name, &value,
				     value_length, &value_length))
		php_register_variable_safe((char *)name, value, value_length,
					   array);

	return 1;
}

static size_t get_host_length(const char *s)
{
	if (!isalnum(*s))
		return 0;

	size_t i = 1;
	for (; s[i] != 0 && s[i] != ':'; ++i) {
		if (!isalnum(*s) && *s != '-' && *s != '_')
			return 0;
	}

	return i;
}

static void add_header_variable(const char *name, const char *value,
				zval *array)
{
	char buffer[5 + strlen(name) + 1];

	char *p = stpcpy(buffer, "HTTP_");
	for (; *name != 0; ++name) {
		if (islower(*name))
			*p++ = *name - 'a' + 'A';
		else if (isupper(*name) || isdigit(*name))
			*p++ = *name;
		else
			*p++ = '_';
	}

	*p = 0;

	add_variable(PARSE_SERVER, buffer, value, array);
}

static void sapi_was_register_variables(zval *array)
{
	struct was_simple *const w = SG(server_context);
	struct was_simple_iterator *i = was_simple_get_parameter_iterator(w);
	if (i != NULL) {
		for (const struct was_simple_pair *p;
		     (p = was_simple_iterator_next(i)) != NULL;) {
			add_variable(PARSE_SERVER, p->name, p->value, array);
		}

		was_simple_iterator_free(i);
	}

	i = was_simple_get_header_iterator(w);
	if (i != NULL) {
		for (const struct was_simple_pair *p;
		     (p = was_simple_iterator_next(i)) != NULL;) {
			add_header_variable(p->name, p->value, array);
		}

		was_simple_iterator_free(i);
	}

	const char *host = was_simple_get_header(w, "host");
	if (host != NULL) {
		size_t host_length = get_host_length(host);
		if (host_length > 0) {
			char *host2 = strndup(host, host_length);
			if (host2 != NULL) {
				add_variable(PARSE_SERVER, "SERVER_NAME",
					     host2, array);
				free(host2);
			}
		}
	}

	add_variable(PARSE_SERVER, "REQUEST_METHOD",
		     http_method_to_string(was_simple_get_method(w)), array);

	add_variable(PARSE_SERVER, "REQUEST_URI",
		     SG(request_info).request_uri, array);

	const char *script_name = was_simple_get_script_name(w);
	if (script_name == NULL)
		script_name = SG(request_info).request_uri;

	add_variable(PARSE_SERVER, "SCRIPT_NAME", script_name, array);
	add_variable(PARSE_SERVER, "PHP_SELF", script_name, array);

	const char *path_info = was_simple_get_path_info(w);
	if (path_info != NULL)
		add_variable(PARSE_SERVER, "PATH_INFO", path_info, array);

	const char *query_string = was_simple_get_query_string(w);
	if (query_string != NULL)
		add_variable(PARSE_SERVER, "QUERY_STRING", query_string, array);

	const char *script_filename =
		was_simple_get_parameter(w, "PHP_SCRIPT");
	if (script_filename != NULL)
		add_variable(PARSE_SERVER, "SCRIPT_FILENAME", script_filename, array);

	const char *https = was_simple_get_header(w, "x-cm4all-https");
	if (https != NULL && strcmp(https, "on") == 0)
		add_variable(PARSE_SERVER, "HTTPS", "on", array);
}

static void sapi_was_log_message(const char *message, int syslog_type_int)
{
	char buf[8192];
	int len = strlen( message );
	if (*(message + len - 1 ) != '\n') {
		snprintf( buf, 8191, "%s\n", message );
		message = buf;
		if (len > 8191)
			len = 8191;
		++len;
	}

	write(STDERR_FILENO, message, len);
}

static sapi_module_struct was_sapi_module = {
	"was",
	"Web Application Socket",

	php_was_startup, /* startup */
	php_module_shutdown_wrapper, /* shutdown */

	sapi_was_activate, /* activate */
	sapi_was_deactivate, /* deactivate */

	sapi_was_ub_write, /* unbuffered write */
	NULL, /* flush */
	NULL, /* get uid */
	sapi_was_getenv, /* getenv */

	php_error, /* error handler */

	NULL, /* header handler */
	sapi_was_send_headers, /* send headers handler */
	NULL, /* send header handler */

	sapi_was_read_post, /* read POST data */
	sapi_was_read_cookies, /* read Cookies */

	sapi_was_register_variables, /* register server variables */
	sapi_was_log_message, /* log message */
	NULL, /* get request time */
	NULL, /* child terminate */

	STANDARD_SAPI_MODULE_PROPERTIES
};

static void init_request_info(struct was_simple *w, const char *request_uri)
{
	SG(server_context) = w;
	SG(request_info).request_method = http_method_to_string(was_simple_get_method(w));
	SG(request_info).query_string = (char *)was_simple_get_query_string(w);
	SG(request_info).content_length = was_simple_input_remaining(w); // TODO: what if negative?
	SG(request_info).path_translated = estrdup(was_simple_get_parameter(w, "PHP_SCRIPT"));
	SG(request_info).request_uri = (char *)request_uri;

	const char *content_type = was_simple_get_header(w, "content-type");
	SG(request_info).content_type = content_type ? content_type : "";

	SG(sapi_headers).http_response_code = 200;
}

static int was_module_main()
{
	if (php_request_startup() == FAILURE)
		return -1;

	zend_file_handle file_handle;
	memset(&file_handle, 0, sizeof(file_handle));
	zend_stream_init_filename(&file_handle, SG(request_info).path_translated);
	php_execute_script(&file_handle);

	zend_try {
		php_request_shutdown(NULL);
	} zend_end_try();

	return 0;
}

static bool was_process_request(struct was_simple *w, const char *request_uri)
{
	int ret = EXIT_SUCCESS;

	zend_first_try {
		init_request_info(w, request_uri);

		if (was_module_main() == -1)
			ret = -1;
	} zend_end_try();

	return ret;
}

ZEND_BEGIN_ARG_INFO(arginfo_was__void, 0)
ZEND_END_ARG_INFO()

PHP_FUNCTION(was_request_headers)
{
	if (zend_parse_parameters_none()) {
		return;
	}

	array_init(return_value);

	struct was_simple *const w = SG(server_context);
	struct was_simple_iterator *i = was_simple_get_header_iterator(w);
	if (i == NULL)
		return;

	for (const struct was_simple_pair *p;
	     (p = was_simple_iterator_next(i)) != NULL;) {
		add_assoc_string_ex(return_value,
				    p->name, strlen(p->name),
				    p->value);
	}
}

PHP_FUNCTION(was_response_headers)
{
	if (zend_parse_parameters_none()) {
		return;
	}

	if (!&SG(sapi_headers).headers) {
		RETURN_FALSE;
	}

	array_init(return_value);

	zend_llist_position pos;
	for (sapi_header_struct *h = zend_llist_get_first_ex(&SG(sapi_headers).headers, &pos);
	     h != NULL;
	     h = zend_llist_get_next_ex(&SG(sapi_headers).headers, &pos)) {
		const char *const data = h->header;
		const size_t length = h->header_len;

		const char *colon = memchr(data, ':', length);
		if (colon == NULL)
			continue;

		const char *name = data;
		size_t name_length = colon - name;
		while (name_length > 0 && data[name_length - 1] == ' ')
			--name_length;

		if (name_length == 0)
			continue;

		const char *value = colon + 1;
		while (*value == ' ')
			++value;

		add_assoc_string_ex(return_value, name, name_length, value);
	}
}

PHP_FUNCTION(was_finish_request)
{
	if (zend_parse_parameters_none()) {
		return;
	}

	php_output_end_all();
	php_header();

	struct was_simple *const w = SG(server_context);
	if (!was_simple_end(w)) {
		RETURN_FALSE;
	}

	RETURN_TRUE;
}

PHP_MINFO_FUNCTION(was);

static const zend_function_entry was_functions[] = {
	PHP_FE(was_request_headers, arginfo_was__void)
	PHP_FE(was_response_headers, arginfo_was__void)
	PHP_FE(was_finish_request, arginfo_was__void)
	PHP_FALIAS(getallheaders, was_request_headers, arginfo_was__void)
	PHP_FALIAS(apache_request_headers, was_request_headers, arginfo_was__void)
	PHP_FALIAS(apache_response_headers, was_response_headers, arginfo_was__void)
	PHP_FE_END
};

static zend_module_entry was_module_entry = {
	STANDARD_MODULE_HEADER,
	"was",
	was_functions,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	PHP_VERSION,
	STANDARD_MODULE_PROPERTIES
};

static void php_was_usage(char *argv0)
{
	const char *prog = strrchr(argv0, '/');
	if (prog) {
		prog++;
	} else {
		prog = "php";
	}

	printf("Usage: %s [options]\n"
	       "  -d foo[=bar]     Define INI entry foo with value 'bar'\n"
	       "  -h               This help\n"
	       "\n", prog);
}

struct CommandLine {
	char *ini_entries;
	size_t ini_entries_len;
};

static int
ParseCommandLine(int argc, char *argv[], struct CommandLine *command_line)
{
	static const opt_struct OPTIONS[] = {
		{'d', 1, "define"},
		{'h', 0, "help"},
		{'?', 0, "usage"},/* help alias (both '?' and 'usage') */
	};

	int php_optind = 1;

	char *ini_entries = NULL;
	size_t ini_entries_len = 0;

	while (true) {
		char *php_optarg = NULL;
		int c = php_getopt(argc, argv, OPTIONS,
				   &php_optarg, &php_optind,
				   1, 2);
		if (c == -1)
			break;

		switch (c) {
		case 'd': {
			/* define ini entries on command line */
			size_t len = strlen(php_optarg);
			char *val;

			if ((val = strchr(php_optarg, '='))) {
				val++;
				if (!isalnum(*val) && *val != '"' && *val != '\'' && *val != '\0') {
					ini_entries = realloc(ini_entries, ini_entries_len + len + sizeof("\"\"\n\0"));
					memcpy(ini_entries + ini_entries_len, php_optarg, (val - php_optarg));
					ini_entries_len += (val - php_optarg);
					memcpy(ini_entries + ini_entries_len, "\"", 1);
					ini_entries_len++;
					memcpy(ini_entries + ini_entries_len, val, len - (val - php_optarg));
					ini_entries_len += len - (val - php_optarg);
					memcpy(ini_entries + ini_entries_len, "\"\n\0", sizeof("\"\n\0"));
					ini_entries_len += sizeof("\n\0\"") - 2;
				} else {
					ini_entries = realloc(ini_entries, ini_entries_len + len + sizeof("\n\0"));
					memcpy(ini_entries + ini_entries_len, php_optarg, len);
					memcpy(ini_entries + ini_entries_len + len, "\n\0", sizeof("\n\0"));
					ini_entries_len += len + sizeof("\n\0") - 2;
				}
			} else {
				ini_entries = realloc(ini_entries, ini_entries_len + len + sizeof("=1\n\0"));
				memcpy(ini_entries + ini_entries_len, php_optarg, len);
				memcpy(ini_entries + ini_entries_len + len, "=1\n\0", sizeof("=1\n\0"));
				ini_entries_len += len + sizeof("=1\n\0") - 2;
			}
			break;
		}

		case 'h': /* help & quit */
		case '?':
			php_was_usage(argv[0]);
			return EXIT_SUCCESS;

		case PHP_GETOPT_INVALID_ARG: /* print usage on bad options, exit 1 */
			php_was_usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	command_line->ini_entries = ini_entries;
	command_line->ini_entries_len = ini_entries_len;

	return -1;
}

int main(int argc, char *argv[])
{
	zend_signal_startup();

	struct CommandLine command_line = {NULL};

	int ret = ParseCommandLine(argc, argv, &command_line);
	if (ret != -1)
		return ret;

	sapi_startup(&was_sapi_module);

	was_sapi_module.ini_entries = command_line.ini_entries;

	was_sapi_module.executable_location = argv[0];

	init_sapi_from_env(&was_sapi_module);

	if (php_module_startup(&was_sapi_module, &was_module_entry, 1) == FAILURE) {
		return FAILURE;
	}

	ret = EXIT_SUCCESS;

	struct was_simple *w = was_simple_new();
	const char *request_uri;
	while ((request_uri = was_simple_accept(w)) != NULL) {
		ret = was_process_request(w, request_uri);
		if (ret)
			break;
	}

	was_simple_free(w);

	php_module_shutdown();
	return ret;
}
