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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _MSC_VER  /* Visual Studio specific */
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utime.h>
#else
#include <sys/time.h>
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/pdo/php_pdo.h"
#include "ext/pdo/php_pdo_driver.h"
#include "php_pdo_nuodb.h"

#include "php_pdo_nuodb_c_cpp_common.h"
#include "php_pdo_nuodb_int.h"


/* If you declare any globals in php_pdo_nuodb.h uncomment this: */
ZEND_DECLARE_MODULE_GLOBALS(pdo_nuodb)


/* True global resources - no need for thread safety here */
static int le_pdo_nuodb;


/* {{{ pdo_nuodb_functions[]
 *
 * Every user visible function must have an entry in pdo_nuodb_functions[].
 */
const zend_function_entry pdo_nuodb_functions[] =
{
    PHP_FE_END  /* Must be the last line in pdo_nuodb_functions[] */
};
/* }}} */


/* {{{ pdo_nuodb_deps
 */
#if ZEND_MODULE_API_NO >= 20050922
static const zend_module_dep pdo_nuodb_deps[] =
{
    ZEND_MOD_REQUIRED("pdo")
    ZEND_MOD_END
};
#endif
/* }}} */


/* {{{ pdo_nuodb_module_entry
 */
zend_module_entry pdo_nuodb_module_entry =
{
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER_EX, NULL,
    pdo_nuodb_deps,
#endif
    "pdo_nuodb",
    pdo_nuodb_functions,
    PHP_MINIT(pdo_nuodb),
    PHP_MSHUTDOWN(pdo_nuodb),
    NULL, // RINIT
    NULL, // RSHUTDOWN
    PHP_MINFO(pdo_nuodb),
#if ZEND_MODULE_API_NO >= 20010901
    "0.1", /* Replace with version number for your extension */
#endif
    STANDARD_MODULE_PROPERTIES
};
/* }}} */


#ifdef COMPILE_DL_PDO_NUODB
ZEND_GET_MODULE(pdo_nuodb)
#endif


/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
  STD_PHP_INI_ENTRY("pdo_nuodb.enable_log",      "0", PHP_INI_ALL, OnUpdateLong, enable_log, zend_pdo_nuodb_globals, pdo_nuodb_globals)
  STD_PHP_INI_ENTRY("pdo_nuodb.log_level",      "1", PHP_INI_ALL, OnUpdateLong, log_level, zend_pdo_nuodb_globals, pdo_nuodb_globals)
  STD_PHP_INI_ENTRY("pdo_nuodb.logfile_path", "nuodb_pdo.log", PHP_INI_ALL, OnUpdateString, logfile_path, zend_pdo_nuodb_globals, pdo_nuodb_globals)
  STD_PHP_INI_ENTRY("pdo_nuodb.default_txn_isolation", "ConsistentRead", PHP_INI_ALL, OnUpdateString, default_txn_isolation, zend_pdo_nuodb_globals, pdo_nuodb_globals)
PHP_INI_END()
/* }}} */


/* {{{ php_pdo_nuodb_init_globals
*/
/*
** The log_level is the level of logging "detail" that the user wants
** to see in the log.  The higher level numbers have more detail.
** The higher level numbers include lesser levels.
**
**  1 - errors/exceptions
**  2 - SQL statements
**  3 - API
**  4 - Functions
**  5 - Everything
**
** The default_txn_isolation which is set each time a NuoDB connection
** is opened.  It must be one of:
**
**   None - 0 Transactions are not supported.
**   ReadUncommitted - 1 Dirty reads, non-repeatable reads and
**                     phantom reads can occur.
**   ReadCommitted - 2 Dirty reads are prevented; non-repeatable reads
**                   and phantom reads can occur.
**   RepeatableRead - 4 Dirty reads are prevented; non-repeatable
**                    reads happen after writes; phantom reads can occur.
**   Serializable - 8 Dirty reads, non-repeatable reads and phantom
**                  reads are prevented.
**   WriteCommitted - 5 Dirty reads are prevented; non-repeatable
**                    reads happen after writes; phantom reads can occur.
**   ConsistentRead - 7 NuoDB native mode
**
*/
static void php_pdo_nuodb_init_globals(zend_pdo_nuodb_globals *pdo_nuodb_globals)
{
  pdo_nuodb_globals->log_fp = NULL;
  pdo_nuodb_globals->enable_log = 0;
  pdo_nuodb_globals->log_level = PDO_NUODB_LOG_ERRORS;
  pdo_nuodb_globals->logfile_path = NULL;
  pdo_nuodb_globals->default_txn_isolation = NULL;
}
/* }}} */


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(pdo_nuodb)
{
    ZEND_INIT_MODULE_GLOBALS(pdo_nuodb, php_pdo_nuodb_init_globals, NULL);

    REGISTER_INI_ENTRIES();

    if (PDO_NUODB_G(enable_log) != 0) {
        PDO_NUODB_G(log_fp) = fopen(PDO_NUODB_G(logfile_path),"a");
    }

    php_pdo_register_driver(&pdo_nuodb_driver);
    return SUCCESS;
}
/* }}} */


/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(pdo_nuodb)
{
    FILE *fp = PDO_NUODB_G(log_fp);
    if (fp != NULL) fclose(PDO_NUODB_G(log_fp));

    UNREGISTER_INI_ENTRIES();

    php_pdo_unregister_driver(&pdo_nuodb_driver);
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(pdo_nuodb)
{
    php_info_print_table_start();
    /* TODO: we should display version of the driver and version of
       NuoDB */
    php_info_print_table_header(2, "PDO Driver for NuoDB", "enabled");
    php_info_print_table_end();

    /* Remove comments if you have entries in php.ini */
    DISPLAY_INI_ENTRIES();
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
