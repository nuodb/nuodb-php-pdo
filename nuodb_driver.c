/****************************************************************************
 * Copyright (c) 2012 - 2013, NuoDB, Inc.
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

#include <stdio.h>
#include <stdarg.h>

#ifdef _MSC_VER  // Visual Studio specific
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utime.h>
#else
#include <sys/time.h>
#endif

#include "php.h"
#ifdef ZEND_ENGINE_2
# include "zend_exceptions.h"
#endif
#include "php_ini.h"
#include "ext/standard/info.h"
#include "pdo/php_pdo.h"
#include "pdo/php_pdo_driver.h"
#include "php_pdo_nuodb.h"

#define PdoNuoDbStatement void
#define PdoNuoDbHandle void
#include "php_pdo_nuodb_c_cpp_common.h"
#include "php_pdo_nuodb_int.h"

// Workaround DB-4112
struct sqlcode_to_sqlstate_t {
	int sqlcode;
	char *sqlstate;
};

ZEND_DECLARE_MODULE_GLOBALS(pdo_nuodb)

char const * const NUODB_OPT_DATABASE = "database";
char const * const NUODB_OPT_USER = "user";
char const * const NUODB_OPT_PASSWORD = "password";
char const * const NUODB_OPT_SCHEMA = "schema";
char const * const NUODB_OPT_IDENTIFIERS = "identifiers";

FILE *nuodb_log_fp = NULL;  
long nuodb_log_level = 1;  // The level of logging "detail" that the user wants to see in the log.
                           // The higher level numbers have more detail.
                           // The higher level numbers include lesser levels.
                           //
                           //   1 - errors/exceptions
                           //   2 - SQL statements
                           //   3 - API
                           //   4 - Functions
                           //   5 - Everything

static int nuodb_alloc_prepare_stmt(pdo_dbh_t *, pdo_stmt_t *, const char *, long, PdoNuoDbStatement ** nuodb_stmt TSRMLS_DC);

// Workaround DB-4112
static struct sqlcode_to_sqlstate_t sqlcode_to_sqlstate[] = {
	 	{-1, "42000"},
		{-2, "0A000"},
		{-3, "58000"},
		{-4, "42000"},
		{-5, "58000"},
		{-6, "08000"},
		{-7, "08000"},
		{-8, "22000"},
		{-9, "22000"},
		{-10, "08000"},
		{-11, "42000"},
		{-12, "58000"},
		{-13, "58000"},
		{-14, "58000"},
		{-15, "58000"},
		{-16, "58000"},
		{-17, "58000"},
		{-18, "58000"},
		{-19, "22000"},
		{-20, "22000"},
		{-21, "22000"},
		{-22, "58000"},
		{-23, "58000"},
		{-24, "40002"},
		{-25, "42000"},
		{-26, "58000"},
		{-27, "23000"},
		{-28, "58000"},
		{-29, "40001"},
		{-30, "58000"},
		{-31, "58000"},
		{-32, "58000"},
		{-33, "0X000"}, // filler -- does not exist in nuodb
		{-34, "0X000"}, // filler -- does not exist in nuodb
		{-35, "0x000"}, // filler -- does not exist in nuodb
		{-36, "58000"},
		{-37, "58000"},
		{-38, "58000"},
		{-39, "58000"},
		{-40, "58000"},
		{-41, "58000"},
		{-42, "01000"},
		{-43, "0A000"},
		{-44, "58000"},
		{-45, "23001"},
		{-46, "58000"},
		{-47, "58000"},
		{-48, "HY008"},
	    {-49, "58000"},
	    {-50, "58000"},
		{-51, "58000"},
		{-52, "58000"},
		{-53, "58000"}
};

/*static*/ int nuodb_handle_commit(pdo_dbh_t * dbh TSRMLS_DC);

// Workaround DB-4112
const char *nuodb_get_sqlstate(int sqlcode) {
	int index = abs(sqlcode) - 1;
	if ((sqlcode > -1) || (sqlcode < -53)) return NULL;
	return sqlcode_to_sqlstate[index].sqlstate;
}

int _record_error(pdo_dbh_t *dbh, pdo_stmt_t *stmt, const char *file, int line, const char *sql_state,  int error_code, const char *error_message)
{
	pdo_nuodb_db_handle *H = (pdo_nuodb_db_handle *)dbh->driver_data;
	pdo_error_type *pdo_err;
	pdo_nuodb_error_info *einfo;
	pdo_nuodb_stmt *S = NULL;

	PDO_DBG_LEVEL_FMT(1, "_record_error: sql_state=%s  error_code=%d  error_message=%s", sql_state, error_code, error_message);
	if (stmt) {
		S = (pdo_nuodb_stmt*)stmt->driver_data;
		PDO_DBG_INF_FMT("sql=%s", S->sql);
		pdo_err = &stmt->error_code;
		einfo   = &S->einfo;
	} else {
		pdo_err = &dbh->error_code;
		einfo   = &H->einfo;
	}

	einfo->errcode = error_code;
	einfo->file = (char *)file;
	einfo->line = line;
	if (einfo->errmsg) {
		pefree(einfo->errmsg, dbh->is_persistent);
		einfo->errmsg = NULL;
	}

	if (!einfo->errcode) { /* no error */
		strcpy(*pdo_err, PDO_ERR_NONE);
		PDO_DBG_RETURN(0);
	}

	einfo->errmsg = pestrdup(error_message, dbh->is_persistent);
	strncpy(*pdo_err, sql_state, 6);


	if (!dbh->methods) {
		TSRMLS_FETCH();
		PDO_DBG_INF("Throwing exception");
		zend_throw_exception_ex(php_pdo_get_exception(), einfo->errcode TSRMLS_CC, "SQLSTATE[%s] [%d] %s",
				*pdo_err, einfo->errcode, einfo->errmsg);
	}

	PDO_DBG_RETURN(einfo->errcode);

}

int _record_error_formatted(pdo_dbh_t *dbh, pdo_stmt_t *stmt, const char *file, int line, const char *sql_state,  int error_code, const char *format, ...)
{
  va_list arg;
  char 	*error_message;
  int ret = 1;

  va_start(arg, format);
  vspprintf(&error_message, 0, format, arg);
  va_end(arg);

  ret = _record_error(dbh, stmt, file, line, sql_state, error_code, strdup(error_message));
  efree((char *)error_message);
  return ret;
}

int _pdo_nuodb_error(pdo_dbh_t *dbh, pdo_stmt_t *stmt, const char *file, int line/* TSRMLS_DC*/)
{
	int ret = 1;
	pdo_nuodb_db_handle *H = (pdo_nuodb_db_handle *)dbh->driver_data;
	pdo_nuodb_stmt *S = NULL;
	const char *sql_state = NULL;
	int error_code = 0;
	const char *error_message = NULL;

	PDO_DBG_ENTER("_pdo_nuodb_error");
	if (stmt) {
		S = (pdo_nuodb_stmt*)stmt->driver_data;
		sql_state = *(pdo_nuodb_stmt_sqlstate(S));
		error_code = pdo_nuodb_stmt_errno(S);
		error_message = pdo_nuodb_stmt_errmsg(S);
	} else {
		sql_state = *(pdo_nuodb_db_handle_sqlstate(H));
		error_code =  pdo_nuodb_db_handle_errno(H);
		error_message = pdo_nuodb_db_handle_errmsg(H);
	}
	ret = _record_error(dbh, stmt, file, line, sql_state,  error_code, error_message);

	PDO_DBG_RETURN(ret);
}



/* called by PDO to close a db handle */
static int nuodb_handle_closer(pdo_dbh_t * dbh TSRMLS_DC) /* {{{ */
{
    pdo_nuodb_db_handle * H = (pdo_nuodb_db_handle *)dbh->driver_data;
    PDO_DBG_ENTER("nuodb_handle_closer");
    PDO_DBG_INF_FMT("dbh=%p", dbh);

    if (H == NULL) {
    	PDO_DBG_RETURN(0);
    }
    if (H->in_nuodb_implicit_txn == 1) {
    	nuodb_handle_commit(dbh TSRMLS_CC);
    }
    pdo_nuodb_db_handle_close_connection(H); //H->db->closeConnection();
    pdo_nuodb_db_handle_delete(H); //delete H->db;
	if (H->einfo.errmsg) {
		pefree(H->einfo.errmsg, dbh->is_persistent);
		H->einfo.errmsg = NULL;
	}

    pefree(H, dbh->is_persistent);
    PDO_DBG_RETURN(1);
}
/* }}} */

/* called by PDO to prepare an SQL query */
static int nuodb_handle_preparer(pdo_dbh_t * dbh, const char * sql, long sql_len, /* {{{ */
                                 pdo_stmt_t * stmt, zval * driver_options TSRMLS_DC)
{
  int ret = 0;
  pdo_nuodb_db_handle * H = (pdo_nuodb_db_handle *)dbh->driver_data;
  pdo_nuodb_stmt * S = NULL;
  PdoNuoDbStatement * nuodb_stmt;
  int map_size = 0;
  int count_input_params = 0;
  int index = 0;
  char rewritten = 0;
  HashPosition pos;
  int keytype;
  char *strindex;
  int strindexlen;
  long intindex = -1;
  long max_index = 0;
  char *nsql = NULL;
  int nsql_len = 0;
  char *pData;

  PDO_DBG_ENTER("nuodb_handle_preparer");
  PDO_DBG_INF_FMT("dbh=%p", dbh);
  PDO_DBG_INF_FMT("sql=%.*s", sql_len, sql);

  stmt->supports_placeholders = PDO_PLACEHOLDER_NAMED;
  stmt->named_rewrite_template = ":pdo%d";
  ret = pdo_parse_params(stmt, (char*)sql, sql_len, &nsql, &nsql_len TSRMLS_CC);
  if (ret == 1) /* the SQL query was re-written */
  {
    rewritten = 1;
    sql = nsql;
    sql_len = nsql_len;
  }
  else if (ret == -1) /* could not understand it! */
  {
    strcpy(dbh->error_code, stmt->error_code);
    PDO_DBG_RETURN(0);
  }

  S = (pdo_nuodb_stmt *) ecalloc(1, sizeof(*S));
  S->H = H;
  S->stmt = NULL;
  S->einfo.errcode = 0;
  S->einfo.errmsg = NULL;
  S->sql = strdup(sql);
  S->qty_input_params = 0;
  S->implicit_txn = 0;
  S->commit_on_close = 0;
  S->in_params = NULL;
  S->out_params = NULL;

  stmt->driver_data = S;
  stmt->methods = &nuodb_stmt_methods;

  map_size = 0;
  if (stmt->bound_param_map)
  {
    map_size = zend_hash_num_elements(stmt->bound_param_map);
    if ((map_size > 0) && (S->in_params == NULL))
    {
      index = 0;
      S->in_params = (nuo_params *) ecalloc(1, NUO_PARAMS_LENGTH(map_size));
      S->in_params->num_alloc = S->in_params->num_params = map_size;
      for (index = 0; index < map_size; index++)
      {
        // zend_hash_index_find(stmt->bound_param_map, index, (void **)&pData);
        S->in_params->params[index].col_name[0] = '\0';
        S->in_params->params[index].col_name_length = 0;
        S->in_params->params[index].len = 0;
        S->in_params->params[index].data = NULL;
      }
    }
  }

  if (stmt->bound_param_map != NULL) /* find the largest int key */
  {
    zend_hash_internal_pointer_reset_ex(stmt->bound_param_map, &pos);
    while (HASH_KEY_NON_EXISTANT != (keytype = zend_hash_get_current_key_ex(
             stmt->bound_param_map, &strindex, &strindexlen, &intindex, 0, &pos)))
    {
      if (HASH_KEY_IS_LONG == keytype) {
        if (intindex > max_index) {
          max_index = intindex;
        }
      }
      zend_hash_move_forward_ex(stmt->bound_param_map, &pos);
    }
    S->qty_input_params = max_index + 1;
  }

  /* allocate and prepare statement */
  if (!nuodb_alloc_prepare_stmt(dbh, stmt, sql, sql_len, &nuodb_stmt TSRMLS_CC)) {
	  // There was an error preparing the statement
	  if (rewritten == 1) efree((void *)sql);
	  PDO_DBG_RETURN(0);
  }
  S->stmt = nuodb_stmt;
  index = 0;
  stmt->driver_data = S;
  stmt->methods = &nuodb_stmt_methods;
  stmt->supports_placeholders = PDO_PLACEHOLDER_POSITIONAL;
  if (rewritten == 1) efree((void *)sql);
  PDO_DBG_RETURN(1);
}
/* }}} */


static void pdo_dbh_t_set_in_txn(void *dbh_opaque, unsigned in_txn) /* {{{ */
{
        pdo_dbh_t *dbh = (pdo_dbh_t *)dbh_opaque;
        dbh->in_txn = in_txn;
}
/* }}} */

/* called by PDO to execute a statement that doesn't produce a result set */
static long nuodb_handle_doer(pdo_dbh_t * dbh, const char * sql, long sql_len TSRMLS_DC) /* {{{ */
{
    pdo_nuodb_db_handle * H = NULL;
    long res;

    PDO_DBG_ENTER("nuodb_handle_doer");
    PDO_DBG_INF_FMT("dbh=%p", dbh);
    PDO_DBG_INF_FMT("sql=%.*s", sql_len, sql);

    H = (pdo_nuodb_db_handle *)dbh->driver_data;


    res = pdo_nuodb_db_handle_doer(H, dbh, sql, (unsigned)dbh->in_txn, (unsigned)dbh->auto_commit, &pdo_dbh_t_set_in_txn);

    // do we have a dbh error code?
    if (strncmp(dbh->error_code, PDO_ERR_NONE, 6)) {
        PDO_DBG_RETURN(-1);
    }

    PDO_DBG_RETURN(res);
}
/* }}} */

/* called by the PDO SQL parser to add quotes to values that are copied into SQL */
static int nuodb_handle_quoter(pdo_dbh_t * dbh, const char * unquoted, int unquotedlen, /* {{{ */
                               char ** quoted, int * quotedlen, enum pdo_param_type paramtype TSRMLS_DC)
{
    int qcount = 0;
    char const * co, *l, *r;
    char * c;
    PDO_DBG_ENTER("nuodb_handle_quoter");
    PDO_DBG_INF_FMT("unquoted=%.*s", unquotedlen, unquoted);

    if (!unquotedlen)
    {
        *quotedlen = 2;
        *quoted = (char *) emalloc(*quotedlen+1);
        strcpy(*quoted, "''");
        PDO_DBG_RETURN(1);
    }

    /* only require single quotes to be doubled if string lengths are used */
    /* count the number of ' characters */
    for (co = unquoted; (co = strchr(co,'\'')); qcount++, co++);

    *quotedlen = unquotedlen + qcount + 2;
    *quoted = c = (char *) emalloc(*quotedlen+1);
    *c++ = '\'';

    /* foreach (chunk that ends in a quote) */
    for (l = unquoted; (r = strchr(l,'\'')); l = r+1)
    {
        strncpy(c, l, r-l+1);
        c += (r-l+1);
        /* add the second quote */
        *c++ = '\'';
    }

    /* copy the remainder */
    strncpy(c, l, *quotedlen-(c-*quoted)-1);
    (*quoted)[*quotedlen-1] = '\'';
    (*quoted)[*quotedlen]   = '\0';

    PDO_DBG_RETURN(1);
}
/* }}} */

/* called by PDO to start a transaction */
static int nuodb_handle_begin(pdo_dbh_t * dbh TSRMLS_DC) /* {{{ */
{
    pdo_nuodb_db_handle * H = NULL;

    PDO_DBG_ENTER("nuodb_handle_begin");
    PDO_DBG_INF_FMT("dbh=%p", dbh);
    PDO_DBG_RETURN(1);
    H = (pdo_nuodb_db_handle *)dbh->driver_data;
    H->in_nuodb_implicit_txn = 0;
    H->in_nuodb_explicit_txn = 1;
}
/* }}} */




/* called by PDO to commit a transaction */
/*static*/ int nuodb_handle_commit(pdo_dbh_t * dbh TSRMLS_DC) /* {{{ */
{
    pdo_nuodb_db_handle * H = NULL;

    PDO_DBG_ENTER("nuodb_handle_commit");
    PDO_DBG_INF_FMT("dbh=%p", dbh);
    H = (pdo_nuodb_db_handle *)dbh->driver_data;
    if (pdo_nuodb_db_handle_commit(H) == 0)
    {
    	H->in_nuodb_implicit_txn = 0;
    	H->in_nuodb_explicit_txn = 0;
        //RECORD_ERROR(dbh);
        PDO_DBG_RETURN(0);
    }
	H->in_nuodb_implicit_txn = 0;
	H->in_nuodb_explicit_txn = 0;
    PDO_DBG_RETURN(1);
}
/* }}} */

/* called by PDO to rollback a transaction */
static int nuodb_handle_rollback(pdo_dbh_t * dbh TSRMLS_DC) /* {{{ */
{
    pdo_nuodb_db_handle * H = NULL;

    PDO_DBG_ENTER("nuodb_handle_rollback");
    PDO_DBG_INF_FMT("dbh=%p", dbh);


    H = (pdo_nuodb_db_handle *)dbh->driver_data;
    if (pdo_nuodb_db_handle_rollback(H) == 0)
    {
    	H->in_nuodb_implicit_txn = 0;
    	H->in_nuodb_explicit_txn = 0;
        //RECORD_ERROR(dbh);
        PDO_DBG_RETURN(0);
    }
	H->in_nuodb_implicit_txn = 0;
	H->in_nuodb_explicit_txn = 0;
    PDO_DBG_RETURN(1);
}
/* }}} */

/* used by prepare and exec to allocate a statement handle and prepare the SQL */
static int nuodb_alloc_prepare_stmt(pdo_dbh_t * dbh, pdo_stmt_t * pdo_stmt, const char * sql, long sql_len, /* {{{ */
                                    PdoNuoDbStatement ** nuodb_stmt TSRMLS_DC)
{
    pdo_nuodb_db_handle * H = NULL;
    char * c, *new_sql, in_quote, in_param, pname[64], *ppname;
    long l, pindex = -1;

    PDO_DBG_ENTER("nuodb_alloc_prepare_stmt");
    PDO_DBG_INF_FMT("dbh=%p", dbh);
    PDO_DBG_LEVEL_FMT(2, "sql=%.*s", sql_len, sql);

    H = (pdo_nuodb_db_handle *)dbh->driver_data;
    *nuodb_stmt = NULL;

    /* There is no max sql statement length in NuoDB - but use 1Mib for now */
    if (sql_len > 0x100000)
    {
        strcpy(dbh->error_code, "01004");
        PDO_DBG_RETURN(0);
    }

    /* in order to support named params,
    we need to replace :foo by ?, and store the name we just replaced */
    new_sql = c = (char *) emalloc(sql_len+1);

    for (l = in_quote = in_param = 0; l <= sql_len; ++l)
    {
        if ( !(in_quote ^= (sql[l] == '\'')))
        {
            if (!in_param)
            {
                switch (sql[l])
                {
                case ':':
                    in_param = 1;
                    ppname = pname;
                    *ppname++ = sql[l];
                case '?':
                    *c++ = '?';
                    ++pindex;
                    continue;
                }
            }
            else
            {
                if ((in_param &= ((sql[l] >= 'A' && sql[l] <= 'Z') || (sql[l] >= 'a' && sql[l] <= 'z')
                                  || (sql[l] >= '0' && sql[l] <= '9') || sql[l] == '_' || sql[l] == '-')))
                {


                    *ppname++ = sql[l];
                    continue;
                }
                else
                {
                    *ppname++ = 0;
                }
            }
        }
        *c++ = sql[l];
    }

    /* prepare the statement */
    *nuodb_stmt = pdo_nuodb_db_handle_create_statement(H, pdo_stmt, new_sql);
    PDO_DBG_INF_FMT("nuodb_alloc_prepare_stmt: dbh=%p S=%p sql=%.*s", dbh, pdo_stmt->driver_data, sql_len, sql);
    //PDO_DBG_INF_FMT("sql=%.*s", sql_len, sql);
    //PDO_DBG_INF_FMT("S=%p", pdo_stmt->driver_data);
	efree(new_sql);

	if (*nuodb_stmt == NULL) {
        PDO_DBG_RETURN(0);
    }

    // do we have a statement error code?
    if ((pdo_stmt->error_code[0] != '\0') && strncmp(pdo_stmt->error_code, PDO_ERR_NONE, 6)) {
        PDO_DBG_RETURN(0);
    }

    // do we have a dbh error code?
    if (strncmp(dbh->error_code, PDO_ERR_NONE, 6)) {
        PDO_DBG_RETURN(0);
    }

    PDO_DBG_RETURN(1);
}
/* }}} */


/* called by PDO to get the last insert id */
static char *nuodb_handle_last_id(pdo_dbh_t *dbh, const char *name, unsigned int *len TSRMLS_DC)
{
    int c = 0;
    pdo_nuodb_db_handle * H = (pdo_nuodb_db_handle *)dbh->driver_data;
    char *id_str = NULL;
    int id = pdo_nuodb_db_handle_last_id(H, name);
    if (id == 0) return NULL;
    *len = snprintf(NULL, 0, "%lu", id);
    id_str = (char *) emalloc((*len)+1);
    c = snprintf(id_str, (*len)+1, "%lu", id);
    return id_str;
}


/* called by PDO to set a driver-specific dbh attribute */
static int nuodb_handle_set_attribute(pdo_dbh_t * dbh, long attr, zval * val TSRMLS_DC) /* {{{ */
{
    pdo_nuodb_db_handle * H = NULL;
    PDO_DBG_ENTER("nuodb_handle_set_attribute");
    H = (pdo_nuodb_db_handle *)dbh->driver_data;

    switch (attr)
    {
    case PDO_ATTR_AUTOCOMMIT:
        convert_to_boolean(val);

        /* ignore if the new value equals the old one */
        if (dbh->auto_commit ^ Z_BVAL_P(val))
        {
            if (dbh->in_txn)
            {
                if (Z_BVAL_P(val))
                {
                    /* turning on auto_commit with an open transaction is illegal, because
                    we won't know what to do with it */
                	_record_error_formatted(dbh, NULL, __FILE__, __LINE__, "HY011", -17, "Cannot enable auto-commit while a transaction is already open");
                    PDO_DBG_RETURN(0);
                }
                else
                {
                    /* close the transaction */
                    if (!nuodb_handle_commit(dbh TSRMLS_CC))
                    {
                        break;
                    }
                    dbh->in_txn = 0;
                }
            }
            dbh->auto_commit = Z_BVAL_P(val);
            pdo_nuodb_db_handle_set_auto_commit(H, dbh->auto_commit);

        }
        PDO_DBG_RETURN(1);

    case PDO_ATTR_FETCH_TABLE_NAMES:
        convert_to_boolean(val);
        H->fetch_table_names = Z_BVAL_P(val);
        PDO_DBG_RETURN(1);
    default:
        PDO_DBG_ERR_FMT("unknown/unsupported attribute: %d", attr);
        break;
    }
    PDO_DBG_RETURN(0);
}
/* }}} */

/* called by PDO to get a driver-specific dbh attribute */
static int nuodb_handle_get_attribute(pdo_dbh_t * dbh, long attr, zval * val TSRMLS_DC) /* {{{ */
{
    pdo_nuodb_db_handle * H = (pdo_nuodb_db_handle *)dbh->driver_data;

    switch (attr)
    {
    case PDO_ATTR_AUTOCOMMIT:
        ZVAL_LONG(val,dbh->auto_commit);
        return 1;

    case PDO_ATTR_CONNECTION_STATUS:
        return 1;

    case PDO_ATTR_CLIENT_VERSION:
        ZVAL_STRING(val,"NuoDB 2.0.4",1);
        return 1;

    case PDO_ATTR_SERVER_VERSION:
    case PDO_ATTR_SERVER_INFO:
    {
        char *info = NULL;
        const char *server_name = pdo_nuodb_db_handle_get_nuodb_product_name(H);
        const char *server_version = pdo_nuodb_db_handle_get_nuodb_product_version(H);
        if ((server_name == NULL) || (server_version == NULL)) return 1;
        info = malloc(strlen(server_name) + strlen(server_version) + 4);
        sprintf(info, "%s %s", server_name, server_version);
        ZVAL_STRING(val, info, 1);
        free(info);
        return 1;
    }

    case PDO_ATTR_FETCH_TABLE_NAMES:
        ZVAL_BOOL(val, H->fetch_table_names);
        return 1;
    }
    return 0;
}
/* }}} */

/* called by PDO to retrieve driver-specific information about an error that has occurred */
static int pdo_nuodb_fetch_error_func(pdo_dbh_t * dbh, pdo_stmt_t * stmt, zval * info TSRMLS_DC) /* {{{ */
{
    pdo_nuodb_db_handle * H = (pdo_nuodb_db_handle *)dbh->driver_data;
 	pdo_nuodb_error_info *einfo = &H->einfo;
	PDO_DBG_ENTER("pdo_nuodb_fetch_error_func");
	PDO_DBG_INF_FMT("dbh=%p stmt=%p", dbh, stmt);
	if (stmt) {
		pdo_nuodb_stmt *S = (pdo_nuodb_stmt *)stmt->driver_data;
		einfo = &S->einfo;
	} else {
		einfo = &H->einfo;
	}
	if (einfo->errcode) {
		add_next_index_long(info, einfo->errcode);
		add_next_index_string(info, einfo->errmsg, 1);
	}
	PDO_DBG_RETURN(1);
}
/* }}} */

static struct pdo_dbh_methods nuodb_methods =   /* {{{ */
{
    nuodb_handle_closer,
    nuodb_handle_preparer,
    nuodb_handle_doer,
    nuodb_handle_quoter,
    nuodb_handle_begin,
    nuodb_handle_commit,
    nuodb_handle_rollback,
    nuodb_handle_set_attribute,
    nuodb_handle_last_id,
    pdo_nuodb_fetch_error_func,
    nuodb_handle_get_attribute,
    NULL /* check_liveness */
};
/* }}} */


/* the driver-specific PDO handle constructor */
static int pdo_nuodb_handle_factory(pdo_dbh_t * dbh, zval * driver_options TSRMLS_DC) /* {{{ */
{
    struct pdo_data_src_parser vars[] =
    {
        { NUODB_OPT_DATABASE, NULL, 0 }, /* "database" */
        { NUODB_OPT_SCHEMA,  NULL,	0 }  /* "schema" */
    };

    pdo_nuodb_db_handle * H = NULL;
    int i;
    int	ret = 0;
    int status;
    short buf_len = 256;
    short dpb_len;
    SqlOption options[4];
    SqlOptionArray optionsArray;
    char *errMessage = NULL;

    if (PDO_NUODB_G(enable_log) != 0) {
      nuodb_log_fp = fopen(PDO_NUODB_G(logfile_path),"a");
      nuodb_log_level = PDO_NUODB_G(log_level);
    }

    PDO_DBG_ENTER("pdo_nuodb_handle_factory");
    dbh->driver_data = pecalloc(1, sizeof(*H), dbh->is_persistent);
    H = (pdo_nuodb_db_handle *) dbh->driver_data;
    H->pdo_dbh = dbh;
	H->einfo.errcode = 0;
	H->einfo.errmsg = NULL;

    php_pdo_parse_data_source(dbh->data_source, dbh->data_source_len, vars, 2);

    options[0].option = "database";
    options[0].extra = (void *) vars[0].optval;
    if (options[0].extra == NULL) {
    	H->einfo.errcode = -10;  // NuoDB SqlCode.CONNECTION_ERROR
    	H->einfo.file = __FILE__;
    	H->einfo.line = __LINE__;
        _record_error(H->pdo_dbh, NULL, H->einfo.file, H->einfo.line, "HY000", H->einfo.errcode, "DSN is missing 'database' parameter");
    	if (H->einfo.errmsg) {
    		pefree(H->einfo.errmsg, dbh->is_persistent);
    		H->einfo.errmsg = NULL;
    	}
        pefree(dbh->driver_data, dbh->is_persistent);
        dbh->driver_data = NULL;
        for (i = 0; i < sizeof(vars)/sizeof(vars[0]); ++i)
        {
            if (vars[i].freeme)
            {
                efree(vars[i].optval);
            }
        }
        PDO_DBG_RETURN(0);
    }
    options[1].option = "user";
    options[1].extra = (void *) dbh->username;
    if (options[1].extra == NULL) {
    	H->einfo.errcode = -10;  // NuoDB SqlCode.CONNECTION_ERROR
    	H->einfo.file = __FILE__;
    	H->einfo.line = __LINE__;
        _record_error(H->pdo_dbh, NULL, H->einfo.file, H->einfo.line, "HY000", H->einfo.errcode, "DSN is missing 'user' parameter");
    	if (H->einfo.errmsg) {
    		pefree(H->einfo.errmsg, dbh->is_persistent);
    		H->einfo.errmsg = NULL;
    	}
        pefree(dbh->driver_data, dbh->is_persistent);
        dbh->driver_data = NULL;
        for (i = 0; i < sizeof(vars)/sizeof(vars[0]); ++i)
        {
            if (vars[i].freeme)
            {
                efree(vars[i].optval);
            }
        }
        PDO_DBG_RETURN(0);
    }
    options[2].option = "password";
    options[2].extra = (void *) dbh->password;
    if (options[2].extra == NULL) {
    	H->einfo.errcode = -10;  // NuoDB SqlCode.CONNECTION_ERROR
    	H->einfo.file = __FILE__;
    	H->einfo.line = __LINE__;
        _record_error(H->pdo_dbh, NULL, H->einfo.file, H->einfo.line, "HY000", H->einfo.errcode, "DSN is missing 'password' parameter");
    	if (H->einfo.errmsg) {
    		pefree(H->einfo.errmsg, dbh->is_persistent);
    		H->einfo.errmsg = NULL;
    	}
        pefree(dbh->driver_data, dbh->is_persistent);
        dbh->driver_data = NULL;
        for (i = 0; i < sizeof(vars)/sizeof(vars[0]); ++i)
        {
            if (vars[i].freeme)
            {
                efree(vars[i].optval);
            }
        }
        PDO_DBG_RETURN(0);
    }
    options[3].option = "schema";
    options[3].extra = (vars[1].optval == NULL) ? "USER" : vars[1].optval;

    optionsArray.count = 4;
    optionsArray.array = options;

    PDO_DBG_INF_FMT("\ndatabase=%s\nuser=%s\nschema=%s",
                    options[0].extra, options[1].extra, options[3].extra);

    status = pdo_nuodb_db_handle_factory(H, &optionsArray, &errMessage);
    if (status != 0) {
    	ret = 1;
    }

    dbh->methods = &nuodb_methods;
    dbh->native_case = PDO_CASE_UPPER;  // TODO: the value should reflect how the database returns the names of the columns in result sets. If the name matches the case that was used in the query, set it to PDO_CASE_NATURAL (this is actually the default). If the column names are always returned in upper case, set it to PDO_CASE_UPPER. If the column names are always returned in lower case, set it to PDO_CASE_LOWER. The value you set is used to determine if PDO should perform case folding when the user sets the PDO_ATTR_CASE attribute.  Maybe switch to PDO_CASE_NATURAL when DB-1239 is fixed.
    dbh->alloc_own_columns = 1;  // if true, the driver requires that memory be allocated explicitly for the columns that are returned
    dbh->auto_commit = 1;  // NuoDB always starts in Auto Commit mode.

    for (i = 0; i < sizeof(vars)/sizeof(vars[0]); ++i)
    {
        if (vars[i].freeme)
        {
            efree(vars[i].optval);
        }
    }

    PDO_DBG_RETURN(ret);
}
/* }}} */


pdo_driver_t pdo_nuodb_driver =   /* {{{ */
{
    PDO_DRIVER_HEADER(nuodb),
    pdo_nuodb_handle_factory
};
/* }}} */
