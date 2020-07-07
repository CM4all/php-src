AC_MSG_CHECKING(for WAS support)

PHP_ARG_ENABLE([was],,
  [AS_HELP_STRING([--enable-was],
    [Build PHP as WAS application])],
  [no])

if test "$PHP_WAS" != "no"; then
  PKG_CHECK_MODULES([LIBWAS], [libcm4all-was-simple >= 1.17],, [AC_MSG_ERROR([libwas not found])])
  PHP_EVAL_LIBLINE($LIBWAS_LIBS)
  PHP_EVAL_INCLINE($LIBWAS_CFLAGS)

  PHP_ADD_MAKEFILE_FRAGMENT($abs_srcdir/sapi/was/Makefile.frag,$abs_srcdir/sapi/was,sapi/was)
  SAPI_WAS_PATH=sapi/was/php
  PHP_SELECT_SAPI(was, program, was_main.c, "", '$(SAPI_WAS_PATH)')
  case $host_alias in
  *darwin*)
    BUILD_WAS="\$(CC) \$(CFLAGS_CLEAN) \$(EXTRA_CFLAGS) \$(EXTRA_LDFLAGS_PROGRAM) \$(LDFLAGS) \$(NATIVE_RPATHS) \$(PHP_GLOBAL_OBJS:.lo=.o) \$(PHP_BINARY_OBJS:.lo=.o) \$(PHP_WAS_OBJS:.lo=.o) \$(PHP_FRAMEWORKS) \$(EXTRA_LIBS) \$(ZEND_EXTRA_LIBS) -o \$(SAPI_WAS_PATH)"
    ;;
  *cygwin*)
    SAPI_WAS_PATH=sapi/was/php.exe
    BUILD_WAS="\$(LIBTOOL) --mode=link \$(CC) -export-dynamic \$(CFLAGS_CLEAN) \$(EXTRA_CFLAGS) \$(EXTRA_LDFLAGS_PROGRAM) \$(LDFLAGS) \$(PHP_RPATHS) \$(PHP_GLOBAL_OBJS) \$(PHP_BINARY_OBJS) \$(PHP_WAS_OBJS) \$(EXTRA_LIBS) \$(ZEND_EXTRA_LIBS) -o \$(SAPI_WAS_PATH)"
    ;;
  *)
    BUILD_WAS="\$(LIBTOOL) --mode=link \$(CC) -export-dynamic \$(CFLAGS_CLEAN) \$(EXTRA_CFLAGS) \$(EXTRA_LDFLAGS_PROGRAM) \$(LDFLAGS) \$(PHP_RPATHS) \$(PHP_GLOBAL_OBJS) \$(PHP_BINARY_OBJS) \$(PHP_WAS_OBJS) \$(EXTRA_LIBS) \$(ZEND_EXTRA_LIBS) -o \$(SAPI_WAS_PATH)"
    ;;
  esac

  PHP_SUBST(SAPI_WAS_PATH)
  PHP_SUBST(BUILD_WAS)
fi

AC_MSG_RESULT($PHP_WAS)
