/****************************************************************************
 * Copyright (c) 2012 - 2014, NuoDB, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of NuoDB, Inc. nor the names of its contributors may
 *       be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NUODB, INC. BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************/

#ifndef PHP_PDO_NUODB_H
#define PHP_PDO_NUODB_H

extern zend_module_entry pdo_nuodb_module_entry;
#define phpext_pdo_nuodb_ptr &pdo_nuodb_module_entry

#ifdef PHP_WIN32
#	define PHP_PDO_NUODB_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_PDO_NUODB_API __attribute__ ((visibility("default")))
#else
#	define PHP_PDO_NUODB_API
#endif

#ifdef ZTS
#    include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(pdo_nuodb);
PHP_MSHUTDOWN_FUNCTION(pdo_nuodb);
PHP_RINIT_FUNCTION(pdo_nuodb);
PHP_RSHUTDOWN_FUNCTION(pdo_nuodb);
PHP_MINFO_FUNCTION(pdo_nuodb);

/*
  Declare any global variables you may need between the BEGIN
  and END macros here:
*/
ZEND_BEGIN_MODULE_GLOBALS(pdo_nuodb)
FILE *log_fp;
long enable_log;
long log_level;
char *logfile_path;
ZEND_END_MODULE_GLOBALS(pdo_nuodb)


/* In every utility function you add that needs to use variables
   in php_pdo_nuodb_globals, call TSRMLS_FETCH(); after declaring other
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as PDO_NUODB_G(variable).  You are
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#    define PDO_NUODB_G(v) TSRMG(pdo_nuodb_globals_id, zend_pdo_nuodb_globals *, v)
#else
#    define PDO_NUODB_G(v) (pdo_nuodb_globals.v)
#endif

#endif	/* PHP_PDO_NUODB_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
