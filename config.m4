dnl $Id$
dnl config.m4 extension for pdo_nuodb
dnl

PHP_ARG_WITH(pdo-nuodb,for NuoDB support for PDO,
[  --with-pdo-nuodb[=DIR] PDO: NuoDB support.  DIR is the NuoDB base
                            install directory [/opt/nuodb]])

if test "$PHP_PDO_NUODB" != "no"; then

  PHP_REQUIRE_CXX()
  AC_LANG_CPLUSPLUS()
  PHP_CHECK_PDO_INCLUDES()

  if test "$PHP_PDO" = "no" && test "$ext_shared" = "no"; then
    AC_MSG_ERROR([PDO is not enabled! Add --enable-pdo to your configure line.])
  fi

  if test "$PHP_PDO_NUODB" = "yes"; then
    NUODB_INCDIR=/opt/nuodb/include
    NUODB_LIBDIR=/opt/nuodb/lib64
    NUODB_LIBDIR_FLAG=-L$NUODB_LIBDIR
  else
    NUODB_INCDIR=$PHP_PDO_NUODB/include
    NUODB_LIBDIR=$PHP_PDO_NUODB/lib64
    NUODB_LIBDIR_FLAG=-L$NUODB_LIBDIR
  fi

  NUODB_LIBNAME=nuoclient
  PHP_ADD_LIBRARY_WITH_PATH($NUODB_LIBNAME, $NUODB_LIBDIR, 
  			    PDO_NUODB_SHARED_LIBADD)
  PHP_ADD_INCLUDE($NUODB_INCDIR)
  AC_DEFINE(HAVE_PDO_NUODB, 1, [ ])
  PHP_SUBST(PDO_NUODB_SHARED_LIBADD)
  PHP_ADD_LIBRARY(stdc++, 1, PDO_NUODB_SHARED_LIBADD)
  PHP_ADD_EXTENSION_DEP(pdo_nuodb, pdo)
  PHP_NEW_EXTENSION(pdo_nuodb, pdo_nuodb.c nuodb_driver.c nuodb_statement.c php_pdo_nuodb_cpp_int.cpp, $ext_shared,,-I$pdo_inc_path -I$NUODB_INCDIR)

fi
