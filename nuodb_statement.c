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

#include "php_pdo_nuodb_c_cpp_common.h"
#include "php_pdo_nuodb_int.h"

#include <time.h>

#define RECORD_ERROR(stmt) _nuodb_error(NULL, stmt,  __FILE__, __LINE__ TSRMLS_CC)
#define CHAR_BUF_LEN 24

static void _release_PdoNuoDbStatement(pdo_nuodb_stmt * S)
{
	pdo_nuodb_stmt_delete(S);
}

/* called by PDO to clean up a statement handle */
/* static */ int nuodb_stmt_dtor(pdo_stmt_t * stmt TSRMLS_DC) /* {{{ */
{
    int result = 1;
    int i;
    pdo_nuodb_stmt * S = (pdo_nuodb_stmt *)stmt->driver_data;
    PDO_DBG_ENTER("nuodb_stmt_dtor");
    PDO_DBG_INF_FMT("S=%ld", S);
    _release_PdoNuoDbStatement(S); /* release the statement */

	if (S->sql) {
        free(S->sql);
        S->sql = NULL;
    }

    if (S->error_msg) {
        free(S->error_msg);
        S->error_msg = NULL;
    }

    /* clean up input params */
    if (S->in_params != NULL)
    {
        efree(S->in_params);
    }

    efree(S);
    PDO_DBG_RETURN(result);
}
/* }}} */

/* called by PDO to execute a prepared query */
static int nuodb_stmt_execute(pdo_stmt_t * stmt TSRMLS_DC) /* {{{ */
{
    int status;
    pdo_nuodb_stmt * S = (pdo_nuodb_stmt *)stmt->driver_data;

    PDO_DBG_ENTER("nuodb_stmt_execute");
    PDO_DBG_INF_FMT("S=%ld", S);
    if (!S) {
        PDO_DBG_RETURN(0);
    }

    status = pdo_nuodb_stmt_execute(S, &stmt->column_count, &stmt->row_count);
    if (status == 0) {
        RECORD_ERROR(stmt);
        PDO_DBG_RETURN(0);
    }

    PDO_DBG_RETURN(1);
}
/* }}} */

/* called by PDO to fetch the next row from a statement */
static int nuodb_stmt_fetch(pdo_stmt_t * stmt, /* {{{ */
                            enum pdo_fetch_orientation ori, long offset TSRMLS_DC)
{
    pdo_nuodb_stmt * S = (pdo_nuodb_stmt *)stmt->driver_data;
    PDO_DBG_ENTER("nuodb_stmt_fetch");
    PDO_DBG_INF_FMT("S=%ld", S);
    if (!stmt->executed)
    {
        strcpy(stmt->error_code, "HY000");
        pdo_nuodb_db_handle_set_last_app_error(S->H, "Cannot fetch from a closed cursor");
	PDO_DBG_RETURN(0);
    }
    PDO_DBG_RETURN(pdo_nuodb_stmt_fetch(S, &stmt->row_count));
}
/* }}} */

/* called by PDO to retrieve information about the fields being returned */
static int nuodb_stmt_describe(pdo_stmt_t * stmt, int colno TSRMLS_DC) /* {{{ */
{
    pdo_nuodb_stmt *S = (pdo_nuodb_stmt *)stmt->driver_data;
    struct pdo_column_data *col = &stmt->columns[colno];
    char *cp;
	char const *column_name;
	int colname_len;
	int sqlTypeNumber;

	PDO_DBG_ENTER("nuodb_stmt_describe");
    PDO_DBG_INF_FMT("S=%ld", S);

    col->precision = 0;
    col->maxlen = 0;

    column_name = pdo_nuodb_stmt_get_column_name(S, colno);
    if (column_name == NULL)
    {
        PDO_DBG_RETURN(0);
    }
    colname_len = strlen(column_name);
    col->namelen = colname_len;
    col->name = cp = (char *) emalloc(colname_len + 1);
    memmove(cp, column_name, colname_len);
    *(cp+colname_len) = '\0';
    sqlTypeNumber = pdo_nuodb_stmt_get_sql_type(S, colno);
    switch (sqlTypeNumber)
    {
        case PDO_NUODB_SQLTYPE_BOOLEAN:
        {
            col->param_type = PDO_PARAM_BOOL;
            break;
        }
        case PDO_NUODB_SQLTYPE_INTEGER:
        {
            col->maxlen = 24;
            col->param_type = PDO_PARAM_STR;
            break;
        }
        case PDO_NUODB_SQLTYPE_BIGINT:
        {
            col->maxlen = 24;
            col->param_type = PDO_PARAM_STR;
            break;
        }
        case PDO_NUODB_SQLTYPE_DOUBLE:
        {
            col->param_type = PDO_PARAM_STR;
            break;
        }
        case PDO_NUODB_SQLTYPE_STRING:
        {
            col->param_type = PDO_PARAM_STR;
            break;
        }
        case PDO_NUODB_SQLTYPE_DATE:
        {
            col->param_type = PDO_PARAM_INT;
            break;
        }
        case PDO_NUODB_SQLTYPE_TIME:
        {
            col->param_type = PDO_PARAM_INT;
            break;
        }
        case PDO_NUODB_SQLTYPE_TIMESTAMP:
        {
            col->param_type = PDO_PARAM_INT;
            break;
        }
        case PDO_NUODB_SQLTYPE_BLOB:
        {
            col->param_type = PDO_PARAM_STR;
            break;
        }
        case PDO_NUODB_SQLTYPE_CLOB:
        {
            col->param_type = PDO_PARAM_STR;
            break;
        }
        default:
        {
	    PDO_DBG_ERR_FMT("unknown/unsupported type: %d", sqlTypeNumber);
	    PDO_DBG_RETURN(0);
        }
    }

    PDO_DBG_RETURN(1);

}
/* }}} */

static int nuodb_stmt_get_col(pdo_stmt_t * stmt, int colno, char ** ptr, /* {{{ */
                              unsigned long * len, int * caller_frees TSRMLS_DC)
{
    static void * (*ereallocPtr)(void *ptr, size_t size, int, char *, unsigned int, char *, unsigned int) = &_erealloc;

    pdo_nuodb_stmt * S = (pdo_nuodb_stmt *)stmt->driver_data;
    int sqlTypeNumber = 0;
    
    PDO_DBG_ENTER("nuodb_stmt_get_col");
    PDO_DBG_INF_FMT("S=%ld", S);
    
    sqlTypeNumber = pdo_nuodb_stmt_get_sql_type(S, colno);

    *len = 0;
    *caller_frees = 1;
    if (*ptr != NULL) efree(*ptr);
    *ptr = NULL;
    switch (sqlTypeNumber)
    {
        case PDO_NUODB_SQLTYPE_BOOLEAN:
        {
        	char val = pdo_nuodb_stmt_get_boolean(S, colno);
        	*ptr = (char *)emalloc(CHAR_BUF_LEN);
        	//*len = slprintf(*ptr, CHAR_BUF_LEN, "%d", val);
            (*ptr)[0] = val;
            *len = 1;
            break;
        }
        case PDO_NUODB_SQLTYPE_INTEGER:
        {
	        int val = pdo_nuodb_stmt_get_integer(S, colno);
            *ptr = (char *)emalloc(CHAR_BUF_LEN);
            *len = slprintf(*ptr, CHAR_BUF_LEN, "%d", val);
            break;
        }
        case PDO_NUODB_SQLTYPE_BIGINT:
        {
	    int64_t val = pdo_nuodb_stmt_get_long(S, colno);
            *ptr = (char *)emalloc(CHAR_BUF_LEN);
            *len = slprintf(*ptr, CHAR_BUF_LEN, "%d", val);
            break;
        }
        case PDO_NUODB_SQLTYPE_DOUBLE:
        {
            break;
        }
        case PDO_NUODB_SQLTYPE_STRING:
        {
	    int str_len;
            const char * str = pdo_nuodb_stmt_get_string(S, colno);
            if (str == NULL)
            {
                break;
            }
            str_len = strlen(str);
            *ptr = (char *) emalloc(str_len+1);
            memmove(*ptr, str, str_len);
            *((*ptr)+str_len)= 0;
            *len = str_len;
            break;
        }
        case PDO_NUODB_SQLTYPE_DATE:
        {
            long d = pdo_nuodb_stmt_get_date(S, colno);
            *len = sizeof(long);
            *ptr = (char *)emalloc(*len);
            memmove(*ptr, &d, *len);
            break;
        }
        case PDO_NUODB_SQLTYPE_TIME:
        {
            unsigned long t = pdo_nuodb_stmt_get_time(S, colno);
            *len = sizeof(long);
            *ptr = (char *)emalloc(*len);
            memmove(*ptr, &t, *len);
            break;
        }
        case PDO_NUODB_SQLTYPE_TIMESTAMP:
        {
            unsigned long ts = pdo_nuodb_stmt_get_timestamp(S, colno);
            *len = sizeof(long);
            *ptr = (char *)emalloc(*len);
            memmove(*ptr, &ts, *len);
            break;
        }
        case PDO_NUODB_SQLTYPE_BLOB:
        {
	    pdo_nuodb_stmt_get_blob(S, colno, ptr, len, ereallocPtr);
            break;
        }
        case PDO_NUODB_SQLTYPE_CLOB:
        {
	    pdo_nuodb_stmt_get_clob(S, colno, ptr, len, ereallocPtr);
            break;
        }
        default:
        {
  	    PDO_DBG_ERR_FMT("unknown/unsupported type: %d", sqlTypeNumber);
	    PDO_DBG_RETURN(0);
            break;
        }
    }

    PDO_DBG_RETURN(1);
}
/* }}} */

static int 
nuodb_stmt_param_hook(pdo_stmt_t * stmt, struct pdo_bound_param_data * param, /* {{{ */
                                 enum pdo_param_event event_type TSRMLS_DC)
{
    pdo_nuodb_stmt * S = (pdo_nuodb_stmt *)stmt->driver_data;
    nuo_params * nuodb_params = NULL;
    nuo_param * nuodb_param = NULL;

    PDO_DBG_ENTER("nuodb_stmt_param_hook");
    PDO_DBG_INF_FMT("S=%ld", S);

    nuodb_params = param->is_param ? S->in_params : S->out_params;
    if (param->is_param) 
    {
	switch (event_type) 
	{
	    case PDO_PARAM_EVT_FREE:
	    case PDO_PARAM_EVT_ALLOC:
	    case PDO_PARAM_EVT_EXEC_POST:
	    case PDO_PARAM_EVT_FETCH_PRE:
	    case PDO_PARAM_EVT_FETCH_POST:
	        PDO_DBG_RETURN(1);

	    case PDO_PARAM_EVT_NORMALIZE:
	        /* decode name from :pdo1, :pdo2 into 0, 1 etc. */
	        if (param->name) 
	        {
		    if (!strncmp(param->name, ":pdo", 4)) 
		    {
		        param->paramno = atoi(param->name + 4);
		    } 
		    else 
		    { 
		        /* resolve parameter name to rewritten name */
		        char *nameptr;
		        if (stmt->bound_param_map && 
	  		    SUCCESS == zend_hash_find(stmt->bound_param_map,
					      param->name, 
					      param->namelen + 1, 
					      (void**)&nameptr)) 
		        {
		            param->paramno = atoi(nameptr + 4) - 1;
		        } 
		        else 
		        {
			    strcpy(stmt->error_code, "HY093");
			    pdo_nuodb_db_handle_set_last_app_error(S->H, "Cannot resolve parameter name"); // TODO: put param->name in the message
			    PDO_DBG_RETURN(0);
		        }
		    }
	        }

		if (nuodb_params == NULL) {
		    strcpy(stmt->error_code, "HY105");
		    pdo_nuodb_db_handle_set_last_app_error(S->H, "Error processing parameters");
		    PDO_DBG_RETURN(0);
		}

		nuodb_param = &nuodb_params->params[param->paramno];
		if (nuodb_param != NULL) 
		{
		    switch(param->param_type) 
		    {
                        case PDO_PARAM_INT:
		        {
                            nuodb_param->sqltype = PDO_NUODB_SQLTYPE_INTEGER;
			    break;
			}
                        case PDO_PARAM_STR:
		        {
                            nuodb_param->sqltype = PDO_NUODB_SQLTYPE_STRING;
			    break;
			}
                        case PDO_PARAM_LOB:
		        {
                            nuodb_param->sqltype = PDO_NUODB_SQLTYPE_BLOB;
                            break;
		        }
		    }
		}
	        break;


	    case PDO_PARAM_EVT_EXEC_PRE: 
	    {
	        int num_input_params = 0;
      		if (!stmt->bound_param_map) {
		      PDO_DBG_RETURN(0);
	        }

		if (param->paramno >= 0) 
		{
		    if (param->paramno >= S->qty_input_params) {
		        strcpy(stmt->error_code, "HY105");
		        nuodb_throw_zend_exception("HY105", 105, "Invalid parameter number %d", param->paramno);
		        //nuodb_throw_zend_exception("HY105", 105, strdup("Invalid parameter number"));
			    PDO_DBG_RETURN(0);
		    }

		    if (nuodb_params == NULL) {
		        strcpy(stmt->error_code, "HY105");
			pdo_nuodb_db_handle_set_last_app_error(S->H, "Error processing parameters");
			PDO_DBG_RETURN(0);
		    }

		    nuodb_param = &nuodb_params->params[param->paramno];
		    if (nuodb_param == NULL) {
		        strcpy(stmt->error_code, "HY105");
			pdo_nuodb_db_handle_set_last_app_error(S->H, "Error locating parameters");
			PDO_DBG_RETURN(0);
		    }

		    if (param->name != NULL)
		    {
		    	memcpy(nuodb_param->col_name, param->name, (strlen(param->name) + 1));
		    	nuodb_param->col_name_length = param->namelen;
		    }

		    // TODO: add code to process streaming LOBs here.

		    if (PDO_PARAM_TYPE(param->param_type) == PDO_PARAM_NULL ||
			Z_TYPE_P(param->parameter) == IS_NULL) 
		    {
		        nuodb_param->len = 0;
		        nuodb_param->data = NULL;
		    } 
		    else if (Z_TYPE_P(param->parameter) == IS_LONG) 
		    {
		        nuodb_param->len = 4;
		        nuodb_param->data = (void *)Z_LVAL_P(param->parameter);
			pdo_nuodb_stmt_set_integer(S, param->paramno,  (int)nuodb_param->data);
		    } 
		    else if (Z_TYPE_P(param->parameter) == IS_BOOL) 
		    {
		        nuodb_param->len = 1;
		        nuodb_param->data = Z_BVAL_P(param->parameter) ? "t" : "f";
			    pdo_nuodb_stmt_set_boolean(S, param->paramno,  nuodb_param->data[0]);
		    } 
		    else 
		    {
		        SEPARATE_ZVAL_IF_NOT_REF(&param->parameter);
			convert_to_string(param->parameter);
		        nuodb_param->len = Z_STRLEN_P(param->parameter);
		        nuodb_param->data = Z_STRVAL_P(param->parameter);
			pdo_nuodb_stmt_set_string(S, param->paramno,  nuodb_param->data);
		    }
		}
	    }
	    break;
	}
    }
    PDO_DBG_RETURN(1);
}
/* }}} */

static int nuodb_stmt_set_attribute(pdo_stmt_t * stmt, long attr, zval * val TSRMLS_DC) /* {{{ */
{
    pdo_nuodb_stmt * S = (pdo_nuodb_stmt *)stmt->driver_data;

    PDO_DBG_ENTER("nuodb_stmt_set_attribute");
    PDO_DBG_INF_FMT("S=%ld", S);

    switch (attr)
    {
    default:
	PDO_DBG_ERR_FMT("unknown/unsupported attribute: %d", attr);
        PDO_DBG_RETURN(0);
    case PDO_ATTR_CURSOR_NAME:
        convert_to_string(val);
        strlcpy(S->name, Z_STRVAL_P(val), sizeof(S->name));
        break;
    }
    PDO_DBG_RETURN(1);
}
/* }}} */

static int nuodb_stmt_get_attribute(pdo_stmt_t * stmt, long attr, zval * val TSRMLS_DC) /* {{{ */
{
    pdo_nuodb_stmt * S = (pdo_nuodb_stmt *)stmt->driver_data;

    switch (attr)
    {
    default:
        return 0;
    case PDO_ATTR_CURSOR_NAME:
        if (*S->name)
        {
            ZVAL_STRING(val,S->name,1);
        }
        else
        {
            ZVAL_NULL(val);
        }
        break;
    }
    return 1;
}
/* }}} */

static int nuodb_stmt_cursor_closer(pdo_stmt_t * stmt TSRMLS_DC) /* {{{ */
{
    pdo_nuodb_stmt * S = (pdo_nuodb_stmt *)stmt->driver_data;

    if ((*S->name || S->cursor_open))
    {
        _release_PdoNuoDbStatement(S);
    }
    *S->name = 0;
    S->cursor_open = 0;
    return 1;
}
/* }}} */


struct pdo_stmt_methods nuodb_stmt_methods =   /* {{{ */
{
    nuodb_stmt_dtor,
    nuodb_stmt_execute,
    nuodb_stmt_fetch,
    nuodb_stmt_describe,
    nuodb_stmt_get_col,
    nuodb_stmt_param_hook,
    nuodb_stmt_set_attribute,
    nuodb_stmt_get_attribute,
    NULL, /* get_column_meta_func */
    NULL, /* next_rowset_func */
    nuodb_stmt_cursor_closer
};
/* }}} */

