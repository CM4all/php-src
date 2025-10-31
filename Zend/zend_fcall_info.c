/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) Zend Technologies Ltd. (http://www.zend.com)           |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@php.net>                                 |
   |          Zeev Suraski <zeev@php.net>                                 |
   |          Andrei Zmievski <andrei@php.net>                            |
   |          Dmitry Stogov <dmitry@php.net>                              |
   +----------------------------------------------------------------------+
*/

#include "zend_fcall_info.h"
#include "zend_API.h" // for zend_is_callable_ex()

ZEND_API void zend_release_fcall_info_cache(zend_fcall_info_cache *fcc) {
	if (fcc->function_handler &&
		(fcc->function_handler->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
		if (fcc->function_handler->common.function_name) {
			zend_string_release_ex(fcc->function_handler->common.function_name, 0);
		}
		zend_free_trampoline(fcc->function_handler);
		fcc->function_handler = NULL;
	}
}

ZEND_API zend_result zend_fcall_info_init(zval *callable, uint32_t check_flags, zend_fcall_info *fci, zend_fcall_info_cache *fcc, zend_string **callable_name, char **error) /* {{{ */
{
	if (!zend_is_callable_ex(callable, NULL, check_flags, callable_name, fcc, error)) {
		return FAILURE;
	}

	fci->size = sizeof(*fci);
	fci->object = fcc->object;
	ZVAL_COPY_VALUE(&fci->function_name, callable);
	fci->retval = NULL;
	fci->param_count = 0;
	fci->params = NULL;
	fci->named_params = NULL;

	return SUCCESS;
}
/* }}} */

ZEND_API void zend_fcall_info_args_clear(zend_fcall_info *fci, bool free_mem) /* {{{ */
{
	if (fci->params) {
		zval *p = fci->params;
		zval *end = p + fci->param_count;

		while (p != end) {
			i_zval_ptr_dtor(p);
			p++;
		}
		if (free_mem) {
			efree(fci->params);
			fci->params = NULL;
		}
	}
	fci->param_count = 0;
}
/* }}} */

ZEND_API void zend_fcall_info_args_save(zend_fcall_info *fci, uint32_t *param_count, zval **params) /* {{{ */
{
	*param_count = fci->param_count;
	*params = fci->params;
	fci->param_count = 0;
	fci->params = NULL;
}
/* }}} */

ZEND_API void zend_fcall_info_args_restore(zend_fcall_info *fci, uint32_t param_count, zval *params) /* {{{ */
{
	zend_fcall_info_args_clear(fci, 1);
	fci->param_count = param_count;
	fci->params = params;
}
/* }}} */

ZEND_API zend_result zend_fcall_info_args_ex(zend_fcall_info *fci, zend_function *func, zval *args) /* {{{ */
{
	zval *arg, *params;
	uint32_t n = 1;

	zend_fcall_info_args_clear(fci, !args);

	if (!args) {
		return SUCCESS;
	}

	if (Z_TYPE_P(args) != IS_ARRAY) {
		return FAILURE;
	}

	fci->param_count = zend_hash_num_elements(Z_ARRVAL_P(args));
	fci->params = params = (zval *) erealloc(fci->params, fci->param_count * sizeof(zval));

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(args), arg) {
		if (func && !Z_ISREF_P(arg) && ARG_SHOULD_BE_SENT_BY_REF(func, n)) {
			ZVAL_NEW_REF(params, arg);
			Z_TRY_ADDREF_P(arg);
		} else {
			ZVAL_COPY(params, arg);
		}
		params++;
		n++;
	} ZEND_HASH_FOREACH_END();

	return SUCCESS;
}
/* }}} */

ZEND_API zend_result zend_fcall_info_args(zend_fcall_info *fci, zval *args) /* {{{ */
{
	return zend_fcall_info_args_ex(fci, NULL, args);
}
/* }}} */

ZEND_API void zend_fcall_info_argp(zend_fcall_info *fci, uint32_t argc, zval *argv) /* {{{ */
{
	zend_fcall_info_args_clear(fci, !argc);

	if (argc) {
		fci->param_count = argc;
		fci->params = (zval *) erealloc(fci->params, fci->param_count * sizeof(zval));

		for (uint32_t i = 0; i < argc; ++i) {
			ZVAL_COPY(&fci->params[i], &argv[i]);
		}
	}
}
/* }}} */

ZEND_API void zend_fcall_info_argv(zend_fcall_info *fci, uint32_t argc, va_list *argv) /* {{{ */
{
	zend_fcall_info_args_clear(fci, !argc);

	if (argc) {
		zval *arg;
		fci->param_count = argc;
		fci->params = (zval *) erealloc(fci->params, fci->param_count * sizeof(zval));

		for (uint32_t i = 0; i < argc; ++i) {
			arg = va_arg(*argv, zval *);
			ZVAL_COPY(&fci->params[i], arg);
		}
	}
}
/* }}} */

ZEND_API void zend_fcall_info_argn(zend_fcall_info *fci, uint32_t argc, ...) /* {{{ */
{
	va_list argv;

	va_start(argv, argc);
	zend_fcall_info_argv(fci, argc, &argv);
	va_end(argv);
}
/* }}} */

ZEND_API zend_result zend_fcall_info_call(zend_fcall_info *fci, zend_fcall_info_cache *fcc, zval *retval_ptr, zval *args) /* {{{ */
{
	zval retval, *org_params = NULL;
	uint32_t org_count = 0;
	zend_result result;

	fci->retval = retval_ptr ? retval_ptr : &retval;
	if (args) {
		zend_fcall_info_args_save(fci, &org_count, &org_params);
		zend_fcall_info_args(fci, args);
	}
	result = zend_call_function(fci, fcc);

	if (!retval_ptr && Z_TYPE(retval) != IS_UNDEF) {
		zval_ptr_dtor(&retval);
	}
	if (args) {
		zend_fcall_info_args_restore(fci, org_count, org_params);
	}
	return result;
}
/* }}} */

ZEND_API void zend_get_callable_zval_from_fcc(const zend_fcall_info_cache *fcc, zval *callable)
{
	if (fcc->closure) {
		ZVAL_OBJ_COPY(callable, fcc->closure);
	} else if (fcc->function_handler->common.scope) {
		array_init(callable);
		if (fcc->object) {
			GC_ADDREF(fcc->object);
			add_next_index_object(callable, fcc->object);
		} else {
			add_next_index_str(callable, zend_string_copy(fcc->calling_scope->name));
		}
		add_next_index_str(callable, zend_string_copy(fcc->function_handler->common.function_name));
	} else {
		ZVAL_STR_COPY(callable, fcc->function_handler->common.function_name);
	}
}
