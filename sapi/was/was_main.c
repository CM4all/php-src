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
   | Author: Max Kellermann <max.kellermann@ionos.com>                    |
   +----------------------------------------------------------------------+
*/

#include "metrics.h"
#include "SAPI.h"
#include "php_main.h"
#include "php_variables.h"
#include "php_getopt.h"
#include "php_ini_builder.h"
#include "zend.h"
#include "zend_system_id.h"
#include "ext/standard/head.h"
#include "precompile.h"

#include <was/simple.h>
#include <was/multi.h>

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/uio.h>

static zend_module_entry was_module_entry;

static char *was_cookies = NULL;

static void
reset_was_metrics(struct php_was_metrics *m)
{
	memset(m, 0, sizeof(*m));
}

static void
submit_was_metrics(struct was_simple *w, const struct php_was_metrics *m)
{
}

static void
finish_was_metrics(struct was_simple *w)
{
	if (!want_was_metrics)
		return;

	want_was_metrics = false;
	submit_was_metrics(w, &was_metrics);
}

static bool
IsPipe(int fd)
{
    struct stat st;
    return fstat(fd, &st) == 0 && S_ISFIFO(st.st_mode);
}

static void init_sapi_from_env(sapi_module_struct *sapi_module)
{
	char *p = getenv("PHP_INI");
	if (p)
		sapi_module->php_ini_path_override = p;
}

static zend_result php_was_startup(sapi_module_struct *sapi_module)
{
	return php_module_startup(sapi_module, &was_module_entry);
}

static zend_result sapi_was_activate(void)
{
	return SUCCESS;
}

static zend_result sapi_was_deactivate(void)
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

	if (w == NULL) {
		/* special case for "--precompile" mode, for error
		   messages written by zend_exception_error() */
		write(STDERR_FILENO, str, str_length);
		return str_length;
	}

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

	if (w == NULL)
		/* special case for "--precompile" mode */
		return SAPI_HEADER_SENT_SUCCESSFULLY;

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

	if (w == NULL)
		/* special case for "--precompile" mode */
		return;

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

	const char *remote_host = was_simple_get_remote_host(w);
	if (remote_host != NULL)
		add_variable(PARSE_SERVER, "REMOTE_ADDR", remote_host, array);
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

static zend_result was_module_main(struct was_simple *w)
{
	if (php_request_startup() == FAILURE)
		return FAILURE;

	zend_file_handle file_handle;
	memset(&file_handle, 0, sizeof(file_handle));
	zend_stream_init_filename(&file_handle, SG(request_info).path_translated);
	php_execute_script(&file_handle);
	zend_destroy_file_handle(&file_handle);

	finish_was_metrics(w);

	zend_try {
		php_request_shutdown(NULL);
	} zend_end_try();

	return SUCCESS;
}

static bool was_process_request(struct was_simple *w, const char *request_uri)
{
	want_was_metrics = was_simple_want_metrics(w);
	if (want_was_metrics)
		reset_was_metrics(&was_metrics);

	bool success = true;

	zend_first_try {
		init_request_info(w, request_uri);

		if (was_module_main(w) != SUCCESS)
			success = false;
	} zend_end_try();

	return success;
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
	finish_was_metrics(w);

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
	       "  --precompile     Precompile PHP sources for opcache\n"
	       "  --system-id      Print the Zend system id\n"
	       "\n", prog);
}

struct CommandLine {
	struct php_ini_builder ini_builder;

	int optind;

	bool ini_ignore;

	bool precompile;

	bool print_system_id;
};

static int
ParseCommandLine(int argc, char *argv[], struct CommandLine *command_line)
{
	static const opt_struct OPTIONS[] = {
		{'d', 1, "define"},
		{'n', 0, "no-php-ini"},
		{'h', 0, "help"},
		{'p', 0, "precompile"},
		{'y', 0, "system-id"},
		{'?', 0, "usage"},/* help alias (both '?' and 'usage') */
		{'-', 0, NULL} /* end of args */
	};

	int php_optind = 1;

	while (true) {
		char *php_optarg = NULL;
		int c = php_getopt(argc, argv, OPTIONS,
				   &php_optarg, &php_optind,
				   1, 2);
		if (c == -1)
			break;

		switch (c) {
		case 'd':
			/* define ini entries on command line */
			php_ini_builder_define(&command_line->ini_builder, php_optarg);
			break;

		case 'n':
			command_line->ini_ignore = true;
			break;

		case 'h': /* help & quit */
		case '?':
			php_was_usage(argv[0]);
			return EXIT_SUCCESS;

		case 'p':
			command_line->precompile = true;
			command_line->ini_ignore = true;

			/* no HTML output, please */
			php_ini_builder_define(&command_line->ini_builder, "html_errors=0");

			/* we need the opcache extension */
			php_ini_builder_define(&command_line->ini_builder, "zend_extension=opcache.so");
			php_ini_builder_define(&command_line->ini_builder, "opcache.enable=1");

			/* this is an arbitrary path (which must
			   exist); with out this option, opcache
			   refuses to enable "file_cache_only" */
			php_ini_builder_define(&command_line->ini_builder, "opcache.file_cache=/tmp");

			/* file_cache_only is necessary because for
			   precompilation, we need to disable string
			   interning (see accel_new_interned_string())
			   or else zend_accel_script_persist() doesn't
			   work properly */
			php_ini_builder_define(&command_line->ini_builder, "opcache.file_cache_only=1");

#ifdef HAVE_JIT
			/* JIT must be disabled when storing to a file
			   because the JIT will redirect opcode
			   handlers to itself */
			php_ini_builder_define(&command_line->ini_builder, "opcache.jit=disable");
#endif
			break;

		case 'y':
			command_line->print_system_id = true;
			break;

		case PHP_GETOPT_INVALID_ARG: /* print usage on bad options, exit 1 */
			php_was_usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	command_line->optind = php_optind;

	return -1;
}

static bool
RunWas(struct was_simple *w)
{
	const char *request_uri;
	while ((request_uri = was_simple_accept(w)) != NULL) {
		if (!was_process_request(w, request_uri))
			return false;
	}

	return true;
}

static bool
RunMultiWas(struct was_multi *m)
{
	const struct sigaction new_sigchld = {
		.sa_handler = SIG_IGN,
		.sa_flags = SA_RESTART,
	};
	struct sigaction old_sigchld;
	sigaction(SIGCHLD, &new_sigchld, &old_sigchld);

	const int dev_null = open("/dev/null", O_RDONLY|O_NOCTTY|O_CLOEXEC);

	struct was_simple *w;
	while ((w = was_multi_accept_simple(m)) != NULL) {
		// TODO use threads instead of processes
		pid_t pid = fork();
		if (pid == 0) {
			sigaction(SIGCHLD, &old_sigchld, NULL);
			if (dev_null > 0) {
				dup2(dev_null, STDIN_FILENO);
				close(dev_null);
			}

			return RunWas(w);
		}

		if (pid < 0)
			perror("fork() failed");

		// TODO no SIGCHLD=SIG_IGN, waitpid() instead
		// TODO forward SIGTERM to child processes

		was_simple_free(w);
	}

	sigaction(SIGCHLD, &old_sigchld, NULL);
	return true;
}

int main(int argc, char *argv[])
{
	signal(SIGPIPE, SIG_IGN);

	zend_signal_startup();

	struct CommandLine command_line = { .ini_ignore = false };
	php_ini_builder_init(&command_line.ini_builder);

	int ret = ParseCommandLine(argc, argv, &command_line);
	if (ret != -1)
		return ret;

	sapi_startup(&was_sapi_module);

	was_sapi_module.ini_entries = php_ini_builder_finish(&command_line.ini_builder);
	was_sapi_module.php_ini_ignore = command_line.ini_ignore;

	was_sapi_module.executable_location = argv[0];

	init_sapi_from_env(&was_sapi_module);

	if (was_sapi_module.startup(&was_sapi_module) == FAILURE) {
		php_module_shutdown();
		php_ini_builder_deinit(&command_line.ini_builder);
		return EXIT_FAILURE;
	}

	if (command_line.print_system_id) {
		printf("%.32s\n", zend_system_id);
		php_module_shutdown();
		php_ini_builder_deinit(&command_line.ini_builder);
		return EXIT_SUCCESS;
	}

	ret = EXIT_SUCCESS;

	if (command_line.precompile) {
		if (php_request_startup() == FAILURE)
			return -1;

		if (!precompile((const char *const*)argv + command_line.optind,
				argc - command_line.optind))
			ret = EXIT_FAILURE;

		zend_try {
			php_request_shutdown(NULL);
		} zend_end_try();

		php_module_shutdown();
		php_ini_builder_deinit(&command_line.ini_builder);
		return ret;
	}

	if (IsPipe(0)) {
		/* Single-WAS (classic) mode */
		struct was_simple *w = was_simple_new();
		if (!RunWas(w))
			ret = EXIT_FAILURE;
		was_simple_free(w);
	} else {
		/* Multi-WAS mode */
		struct was_multi *m = was_multi_new();
		if (!RunMultiWas(m))
			ret = EXIT_FAILURE;
		was_multi_free(m);
	}

	php_module_shutdown();
	php_ini_builder_deinit(&command_line.ini_builder);
	return ret;
}
