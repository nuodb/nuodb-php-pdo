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

#ifndef PHP_PDO_NUODB_C_CPP_COMMON_H
#define PHP_PDO_NUODB_C_CPP_COMMON_H

/*
** This is a common file that will be compiled by both C and C++.
** The definitions here are use by both the "C/PHP/Zend" parts of this
** driver as well as the "C++/NuoDB" parts of this driver.
**
*/
#define SHORT_MAX (1 << (8*sizeof(short)-1))

#define PDO_NUODB_SQLTYPE_BOOLEAN   1
#define PDO_NUODB_SQLTYPE_INTEGER   2
#define PDO_NUODB_SQLTYPE_BIGINT    3
#define PDO_NUODB_SQLTYPE_DOUBLE    4
#define PDO_NUODB_SQLTYPE_STRING    5
#define PDO_NUODB_SQLTYPE_DATE      6
#define PDO_NUODB_SQLTYPE_TIME      7
#define PDO_NUODB_SQLTYPE_TIMESTAMP 8
#define PDO_NUODB_SQLTYPE_ARRAY     9  // Not Yet Supported by this driver.
#define PDO_NUODB_SQLTYPE_BLOB     10
#define PDO_NUODB_SQLTYPE_CLOB     11

#ifdef __cplusplus
extern "C" {
#endif
struct pdo_nuodb_timer_t {
    int startTimeInMicroSec;
    int endTimeInMicroSec;
    int stopped;
#ifdef WIN32
    LARGE_INTEGER frequency; // ticks per second
    LARGE_INTEGER startCount;
    LARGE_INTEGER endCount;
#else 
    struct timeval startCount;
    struct timeval endCount;
#endif    
};
void pdo_nuodb_timer_init(struct pdo_nuodb_timer_t *timer);
void pdo_nuodb_timer_start(struct pdo_nuodb_timer_t *timer);
void pdo_nuodb_timer_end(struct pdo_nuodb_timer_t *timer);
int pdo_nuodb_get_elapsed_time_in_microseconds(struct pdo_nuodb_timer_t *timer);
#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C" {
#endif
void pdo_nuodb_log(int lineno, const char *file, const char *log_level, const char *log_msg);
void pdo_nuodb_log_va(int lineno, const char *file, const char *log_level, char *format, ...);
int pdo_nuodb_func_enter(int lineno, const char *file, const char *func_name, int func_name_len);
void pdo_nuodb_func_leave(int lineno, const char *file);
#ifdef __cplusplus
}
#endif

#if PHP_DEBUG && !defined(PHP_WIN32)
#define PDO_DBG_ENABLED 1

#define PDO_DBG_INF(msg) do { if (dbg_skip_trace == FALSE) pdo_nuodb_log(__LINE__, __FILE__, "info", (msg)); } while (0)
#define PDO_DBG_ERR(msg) do { if (dbg_skip_trace == FALSE) pdo_nuodb_log(__LINE__, __FILE__, "error", (msg)); } while (0)
#define PDO_DBG_INF_FMT(...) do { if (dbg_skip_trace == FALSE) pdo_nuodb_log_va(__LINE__, __FILE__, "info", __VA_ARGS__); } while (0)
#define PDO_DBG_ERR_FMT(...) do { if (dbg_skip_trace == FALSE) pdo_nuodb_log_va(__LINE__, __FILE__, "error", __VA_ARGS__); } while (0)
#define PDO_DBG_ENTER(func_name) int dbg_skip_trace = TRUE; dbg_skip_trace = !pdo_nuodb_func_enter(__LINE__, __FILE__, func_name, strlen(func_name));
#define PDO_DBG_RETURN(value)	do { pdo_nuodb_func_leave(__LINE__, __FILE__); return (value); } while (0)
#define PDO_DBG_VOID_RETURN	do { pdo_nuodb_func_leave(__LINE__, __FILE__); return; } while (0)

#else
#define PDO_DBG_ENABLED 0

static inline void PDO_DBG_INF(char *msg) {}
static inline void PDO_DBG_ERR(char *msg) {}
static inline void PDO_DBG_INF_FMT(char *format, ...) {}
static inline void PDO_DBG_ERR_FMT(char *format, ...) {}
static inline void PDO_DBG_ENTER(const char *func_name) {}
#define PDO_DBG_RETURN(value)	return (value)
#define PDO_DBG_VOID_RETURN		return;

#endif

typedef struct SqlOption_t
{
    char const * option;
    void * extra;
} SqlOption;

typedef struct SqlOptionArray_t
{
    size_t count;
    SqlOption const * array;
} SqlOptionArray;


typedef struct
{
    short sqltype;  // datatype
    short scale; // scale factor
    short col_name_length; // length of column name
    char  col_name[32];
    short len; // length of data buffer
    char *data; // address of data buffer
} nuo_param; // XSQLVAR

typedef struct
{
    short num_params;  // number of actual params (sqld)
    short num_alloc;  // number of allocated params (sqln)
    nuo_param params[1]; // address of first param
} nuo_params; //XSQLDA

#define NUO_PARAMS_LENGTH(n)   (sizeof(nuo_params) + (n-1) * sizeof(nuo_param))

#ifdef __cplusplus
extern "C" {
#endif

void nuodb_throw_zend_exception(const char *sql_state, int code, const char *msg);

typedef struct
{
    /* the connection handle */
	void *db; // PdoNuoDbHandle * db;

    /* the last error that didn't come from the API */
    char const * last_app_error;

    /* prepend table names on column names in fetch */
    unsigned fetch_table_names:1;

} pdo_nuodb_db_handle;

int pdo_nuodb_db_handle_commit(pdo_nuodb_db_handle *H);
int pdo_nuodb_db_handle_rollback(pdo_nuodb_db_handle *H);
int pdo_nuodb_db_handle_close_connection(pdo_nuodb_db_handle *H);
int pdo_nuodb_db_handle_delete(pdo_nuodb_db_handle *H);
int pdo_nuodb_db_handle_set_auto_commit(pdo_nuodb_db_handle *H, unsigned int auto_commit);
void *pdo_nuodb_db_handle_create_statement(pdo_nuodb_db_handle * H, const char *sql) ;
long pdo_nuodb_db_handle_doer(pdo_nuodb_db_handle * H, void *dbh_opaque, const char *sql, unsigned in_txn, unsigned auto_commit, void (*pt2pdo_dbh_t_set_in_txn)(void *dbh_opaque, unsigned in_txn));
int pdo_nuodb_db_handle_factory(pdo_nuodb_db_handle * H, SqlOptionArray *optionsArray, char **errMessage);
void pdo_nuodb_db_handle_set_last_app_error(pdo_nuodb_db_handle *H, const char *err_msg);
int pdo_nuodb_db_handle_last_id(pdo_nuodb_db_handle *H, const char *name);
const char *pdo_nuodb_db_handle_get_nuodb_product_name(pdo_nuodb_db_handle *H); 
const char *pdo_nuodb_db_handle_get_nuodb_product_version(pdo_nuodb_db_handle *H);

typedef struct
{
    /* the link that owns this statement */
    void *H; // pdo_nuodb_db_handle * H;

    /* the statement handle */
    void *stmt; // PdoNuoDbStatement * stmt;

    /* copy of the sql statement */
    char *sql;

    /* the name of the cursor (if it has one) */
    char name[32];

    /* whether EOF was reached for this statement */
    unsigned exhausted:1;

    /* successful execute opens a cursor */
    unsigned cursor_open:1;

    unsigned _reserved:22;

    int error_code;
    char *error_msg;  // pointer to error_msg.  NULL if no error.

    /* the input params */
    nuo_params * in_params;

    /* the output params */
    nuo_params * out_params;

} pdo_nuodb_stmt;

int pdo_nuodb_stmt_delete(pdo_nuodb_stmt *S);
int pdo_nuodb_stmt_execute(pdo_nuodb_stmt *S, int *column_count, long *row_count);
int pdo_nuodb_stmt_fetch(pdo_nuodb_stmt *S, long *row_count);
char const *pdo_nuodb_stmt_get_column_name(pdo_nuodb_stmt * S, int colno);
int pdo_nuodb_stmt_get_sql_type(pdo_nuodb_stmt * S, int colno);
int pdo_nuodb_stmt_set_boolean(pdo_nuodb_stmt *S, int paramno, int flag_val);
int pdo_nuodb_stmt_set_integer(pdo_nuodb_stmt *S, int paramno, long int_val);
int pdo_nuodb_stmt_set_boolean(pdo_nuodb_stmt *S, int paramno, char bool_val);
int pdo_nuodb_stmt_set_string(pdo_nuodb_stmt *S, int paramno, char *str_val);
int pdo_nuodb_stmt_get_boolean(pdo_nuodb_stmt *S, int colno);
int pdo_nuodb_stmt_get_integer(pdo_nuodb_stmt *S, int colno);
char pdo_nuodb_stmt_get_boolean(pdo_nuodb_stmt *S, int colno);
int64_t pdo_nuodb_stmt_get_long(pdo_nuodb_stmt *S, int colno);
const char *pdo_nuodb_stmt_get_string(pdo_nuodb_stmt *S, int colno);
unsigned long pdo_nuodb_stmt_get_date(pdo_nuodb_stmt *S, int colno);
unsigned long pdo_nuodb_stmt_get_time(pdo_nuodb_stmt *S, int colno);
unsigned long pdo_nuodb_stmt_get_timestamp(pdo_nuodb_stmt *S, int colno);
void pdo_nuodb_stmt_get_blob(pdo_nuodb_stmt *S, int colno, char ** ptr, unsigned long * len, void * (*erealloc)(void *ptr, size_t size, int, char *, unsigned int, char *, unsigned int));
void pdo_nuodb_stmt_get_clob(pdo_nuodb_stmt *S, int colno, char ** ptr, unsigned long * len, void * (*erealloc)(void *ptr, size_t size, int, char *, unsigned int, char *, unsigned int));

#ifdef __cplusplus
} // end of extern "C" {
#endif

#endif	/* PHP_PDO_NUODB_C_CPP_COMMON_H */
