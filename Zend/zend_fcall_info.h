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

#ifndef ZEND_ZEND_FCALL_INFO_H
#define ZEND_ZEND_FCALL_INFO_H

#include "zend_compile.h" // for union _zend_function
#include "zend_globals.h"
#include "zend_globals_macros.h" // for EG()
#include "zend_objects_API.h" // for OBJ_RELEASE()
#include "zend_portability.h" // for BEGIN_EXTERN_C
#include "zend_value.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct _zend_array HashTable;

BEGIN_EXTERN_C()

typedef struct _zend_fcall_info {
	size_t size;
	zval function_name;
	zval *retval;
	zval *params;
	zend_object *object;
	uint32_t param_count;
	/* This hashtable can also contain positional arguments (with integer keys),
	 * which will be appended to the normal params[]. This makes it easier to
	 * integrate APIs like call_user_func_array(). The usual restriction that
	 * there may not be position arguments after named arguments applies. */
	HashTable *named_params;
} zend_fcall_info;

typedef struct _zend_fcall_info_cache {
	zend_function *function_handler;
	zend_class_entry *calling_scope;
	zend_class_entry *called_scope;
	zend_object *object; /* Instance of object for method calls */
	zend_object *closure; /* Closure reference, only if the callable *is* the object */
} zend_fcall_info_cache;

#define ZEND_FCI_INITIALIZED(fci) ((fci).size != 0)
#define ZEND_FCC_INITIALIZED(fcc) ((fcc).function_handler != NULL)

ZEND_API void zend_release_fcall_info_cache(zend_fcall_info_cache *fcc);

#ifndef __cplusplus
# define empty_fcall_info (zend_fcall_info) {0}
# define empty_fcall_info_cache (zend_fcall_info_cache) {0}
#else
# define empty_fcall_info zend_fcall_info {}
# define empty_fcall_info_cache zend_fcall_info_cache {}
#endif

/** Build zend_call_info/cache from a zval*
 *
 * Caller is responsible to provide a return value (fci->retval), otherwise the we will crash.
 * In order to pass parameters the following members need to be set:
 * fci->param_count = 0;
 * fci->params = NULL;
 * The callable_name argument may be NULL.
 */
ZEND_API zend_result zend_fcall_info_init(zval *callable, uint32_t check_flags, zend_fcall_info *fci, zend_fcall_info_cache *fcc, zend_string **callable_name, char **error);

/** Clear arguments connected with zend_fcall_info *fci
 * If free_mem is not zero then the params array gets free'd as well
 */
ZEND_API void zend_fcall_info_args_clear(zend_fcall_info *fci, bool free_mem);

/** Save current arguments from zend_fcall_info *fci
 * params array will be set to NULL
 */
ZEND_API void zend_fcall_info_args_save(zend_fcall_info *fci, uint32_t *param_count, zval **params);

/** Free arguments connected with zend_fcall_info *fci and set back saved ones.
 */
ZEND_API void zend_fcall_info_args_restore(zend_fcall_info *fci, uint32_t param_count, zval *params);

/** Set or clear the arguments in the zend_call_info struct taking care of
 * refcount. If args is NULL and arguments are set then those are cleared.
 */
ZEND_API zend_result zend_fcall_info_args(zend_fcall_info *fci, zval *args);
ZEND_API zend_result zend_fcall_info_args_ex(zend_fcall_info *fci, zend_function *func, zval *args);

/** Set arguments in the zend_fcall_info struct taking care of refcount.
 * If argc is 0 the arguments which are set will be cleared, else pass
 * a variable amount of zval** arguments.
 */
ZEND_API void zend_fcall_info_argp(zend_fcall_info *fci, uint32_t argc, zval *argv);

/** Set arguments in the zend_fcall_info struct taking care of refcount.
 * If argc is 0 the arguments which are set will be cleared, else pass
 * a variable amount of zval** arguments.
 */
ZEND_API void zend_fcall_info_argv(zend_fcall_info *fci, uint32_t argc, va_list *argv);

/** Set arguments in the zend_fcall_info struct taking care of refcount.
 * If argc is 0 the arguments which are set will be cleared, else pass
 * a variable amount of zval** arguments.
 */
ZEND_API void zend_fcall_info_argn(zend_fcall_info *fci, uint32_t argc, ...);

/** Call a function using information created by zend_fcall_info_init()/args().
 * If args is given then those replace the argument info in fci is temporarily.
 */
ZEND_API zend_result zend_fcall_info_call(zend_fcall_info *fci, zend_fcall_info_cache *fcc, zval *retval, zval *args);

/* Zend FCC API to store and handle PHP userland functions */
static zend_always_inline bool zend_fcc_equals(const zend_fcall_info_cache* a, const zend_fcall_info_cache* b)
{
	if (UNEXPECTED((a->function_handler->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE) &&
		(b->function_handler->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE))) {
		return a->object == b->object
			&& a->calling_scope == b->calling_scope
			&& a->closure == b->closure
			&& zend_string_equals(a->function_handler->common.function_name, b->function_handler->common.function_name)
		;
	}
	return a->function_handler == b->function_handler
		&& a->object == b->object
		&& a->calling_scope == b->calling_scope
		&& a->closure == b->closure
	;
}

static zend_always_inline void zend_fcc_addref(zend_fcall_info_cache *fcc)
{
	ZEND_ASSERT(ZEND_FCC_INITIALIZED(*fcc) && "FCC Not initialized, possibly refetch trampoline freed by ZPP?");
	/* If the cached trampoline is set, free it */
	if (UNEXPECTED(fcc->function_handler == &EG(trampoline))) {
		zend_function *copy = (zend_function*)emalloc(sizeof(zend_function));

		memcpy(copy, fcc->function_handler, sizeof(zend_function));
		fcc->function_handler->common.function_name = NULL;
		fcc->function_handler = copy;
	}
	if (fcc->object) {
		GC_ADDREF(fcc->object);
	}
	if (fcc->closure) {
		GC_ADDREF(fcc->closure);
	}
}

static zend_always_inline void zend_fcc_dup(/* restrict */ zend_fcall_info_cache *dest, const zend_fcall_info_cache *src)
{
	memcpy(dest, src, sizeof(zend_fcall_info_cache));
	zend_fcc_addref(dest);
}

static zend_always_inline void zend_fcc_dtor(zend_fcall_info_cache *fcc)
{
	ZEND_ASSERT(fcc->function_handler);
	if (fcc->object) {
		OBJ_RELEASE(fcc->object);
	}
	/* Need to free potential trampoline (__call/__callStatic) copied function handler before releasing the closure */
	zend_release_fcall_info_cache(fcc);
	if (fcc->closure) {
		OBJ_RELEASE(fcc->closure);
	}
	*fcc = empty_fcall_info_cache;
}

ZEND_API void zend_get_callable_zval_from_fcc(const zend_fcall_info_cache *fcc, zval *callable);

END_EXTERN_C()

#endif /* ZEND_FCALL_INFO_H */
