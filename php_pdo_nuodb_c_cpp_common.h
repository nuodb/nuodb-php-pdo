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

#ifndef PHP_PDO_NUODB_C_CPP_COMMON_H
#define PHP_PDO_NUODB_C_CPP_COMMON_H

/*
 * This is a common file that will be compiled by both C and C++.
 * The definitions here are use by both the "C/PHP/Zend" parts of this
 * driver as well as the "C++/NuoDB" parts of this driver.
 *
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
#define PDO_NUODB_SQLTYPE_ARRAY     9  /* Not Yet Supported by this
                                        * driver. */
#define PDO_NUODB_SQLTYPE_BLOB     10
#define PDO_NUODB_SQLTYPE_CLOB     11
#define PDO_NUODB_SQLTYPE_NULL     12

/*
 * Levels of detail for logging.
 */
#define PDO_NUODB_LOG_ERRORS        1
#define PDO_NUODB_LOG_SQL           2
#define PDO_NUODB_LOG_API           3
#define PDO_NUODB_LOG_FUNCTIONS     4
#define PDO_NUODB_LOG_EVERYTHING    5

#define PDO_NUODB_OPTIONS_ARR_SIZE  4
#define PDO_NUODB_TIMESTAMP_BUFFER 64
#define PDO_NUODB_PNAME_BUFFER     64
#define PDO_NUODB_SQLSTATE_LEN      6

#define PDO_NUODB_SQLCODE_CONNECTION_ERROR   -10
#define PDO_NUODB_SQLCODE_APPLICATION_ERROR  -12
#define PDO_NUODB_SQLCODE_INTERNAL_ERROR     -17

#ifdef __cplusplus
extern "C" {
#endif
    struct pdo_nuodb_timer_t {
        int startTimeInMicroSec;
        int endTimeInMicroSec;
        int stopped;
#ifdef WIN32
        LARGE_INTEGER frequency; /* ticks per second */
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
    void pdo_nuodb_log(int lineno, const char *file, long log_level, const char *log_msg);
    void pdo_nuodb_log_va(int lineno, const char *file, long log_level, char *format, ...);
    int pdo_nuodb_func_enter(int lineno, const char *file, const char *func_name, int func_name_len, void *dbh);
    void pdo_nuodb_func_leave(int lineno, const char *file, void *dbh);
#ifdef __cplusplus
}
#endif

#define PDO_DBG_ENABLED 1

#ifdef PDO_DBG_ENABLED
#define PDO_DBG_INF(msg) do { pdo_nuodb_log(__LINE__, __FILE__, PDO_NUODB_LOG_EVERYTHING, (msg)); } while (0)
#define PDO_DBG_ERR(msg) do { pdo_nuodb_log(__LINE__, __FILE__, PDO_NUODB_LOG_ERRORS, (msg)); } while (0)
#define PDO_DBG_LEVEL_FMT(loglevel, ...) do { pdo_nuodb_log_va(__LINE__, __FILE__, (loglevel), __VA_ARGS__); } while (0)
#define PDO_DBG_INF_FMT(...) do { pdo_nuodb_log_va(__LINE__, __FILE__, PDO_NUODB_LOG_EVERYTHING, __VA_ARGS__); } while (0)
#define PDO_DBG_ERR_FMT(...) do { pdo_nuodb_log_va(__LINE__, __FILE__, PDO_NUODB_LOG_ERRORS, __VA_ARGS__); } while (0)
#define PDO_DBG_ENTER(func_name, dbh) pdo_nuodb_func_enter(__LINE__, __FILE__, func_name, strlen(func_name), dbh);
#define PDO_DBG_RETURN(value, dbh) do { pdo_nuodb_func_leave(__LINE__, __FILE__, dbh); return (value); } while (0)
#define PDO_DBG_VOID_RETURN(dbh) do { pdo_nuodb_func_leave(__LINE__, __FILE__, dbh); return; } while (0)

#else

static inline void PDO_DBG_INF(char *msg) {}
static inline void PDO_DBG_ERR(char *msg) {}
static inline void PDO_DBG_INF_FMT(char *format, ...) {}
static inline void PDO_DBG_ERR_FMT(char *format, ...) {}
static inline void PDO_DBG_ENTER(const char *func_name, void *dbh) {}
#define PDO_DBG_RETURN(value, dbh) return (value)
#define PDO_DBG_VOID_RETURN(dbh) return;

#endif

typedef struct {
    char *file;
    int line;
    int errcode;
    char *errmsg;
} pdo_nuodb_error_info;

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
    short sqltype;  /* datatype */
    short scale; /* scale factor */
    short col_name_length; /* length of column name */
    char  col_name[32];
    int len; /* length of data buffer */
    char *data; /* address of data buffer */
} nuo_param;

typedef struct
{
    short num_params;  /* number of actual params (sqld) */
    short num_alloc;  /* number of allocated params (sqln) */
    nuo_param params[1]; /* address of first param */
} nuo_params;

#define NUO_PARAMS_LENGTH(n)   (sizeof(nuo_params) + (n-1) * sizeof(nuo_param))

#ifdef __cplusplus
extern "C" {
#endif

/* Workaround DB-4112 */
    const char *nuodb_get_sqlstate(int sqlcode);

    void nuodb_throw_zend_exception(const char *sql_state, int code, const char *format, ...);
/*void _nuodb_error_new(pdo_dbh_t * dbh, pdo_stmt_t * stmt, char const
 * * file, long line, const char *sql_state, int nuodb_error_code,
 * const char *format, ...); */
    int _pdo_nuodb_error(pdo_dbh_t *dbh, pdo_stmt_t *stmt, const char *file, int line);
    int _record_error(pdo_dbh_t *dbh, pdo_stmt_t *stmt, const char *file, int line, const char *sql_state,  int error_code, const char *error_message);
    int _record_error_formatted(pdo_dbh_t *dbh, pdo_stmt_t *stmt, const char *file, int line, const char *sql_state,  int error_code, const char *format, ...);


    typedef struct
    {
        /* the connection handle */
        void *db; /* opaque for PdoNuoDbHandle * db; */

        /* PHP PDO parent pdo_dbh_t handle */
        pdo_dbh_t *pdo_dbh;

        /* NuoDB error information */
        pdo_nuodb_error_info einfo;

        char in_nuodb_implicit_txn;  /* may not be the same as pdo_dbh->in_txn */
        char in_nuodb_explicit_txn;  /* may not be the same as pdo_dbh->in_txn */

        /* prepend table names on column names in fetch */
        unsigned fetch_table_names:1;

    } pdo_nuodb_db_handle;

    int pdo_nuodb_db_handle_errno(pdo_nuodb_db_handle *H);
    const char *pdo_nuodb_db_handle_errmsg(pdo_nuodb_db_handle *H);
    pdo_error_type * pdo_nuodb_db_handle_sqlstate(pdo_nuodb_db_handle *H);
    int pdo_nuodb_db_handle_commit(pdo_nuodb_db_handle *H);
    int pdo_nuodb_db_handle_rollback(pdo_nuodb_db_handle *H);
    int pdo_nuodb_db_handle_close_connection(pdo_nuodb_db_handle *H);
    int pdo_nuodb_db_handle_delete(pdo_nuodb_db_handle *H);
    int pdo_nuodb_db_handle_set_auto_commit(pdo_nuodb_db_handle *H, unsigned int auto_commit);
    void *pdo_nuodb_db_handle_create_statement(pdo_nuodb_db_handle * H, pdo_stmt_t * stmt, const char *sql) ;
    long pdo_nuodb_db_handle_doer(pdo_nuodb_db_handle * H, void *dbh_opaque, const char *sql, unsigned in_txn, unsigned auto_commit, void (*pt2pdo_dbh_t_set_in_txn)(void *dbh_opaque, unsigned in_txn));
    int pdo_nuodb_db_handle_factory(pdo_nuodb_db_handle * H, SqlOptionArray *optionsArray, char **errMessage);
    int pdo_nuodb_db_handle_last_id(pdo_nuodb_db_handle *H, const char *name);
    const char *pdo_nuodb_db_handle_get_nuodb_product_name(pdo_nuodb_db_handle *H);
    const char *pdo_nuodb_db_handle_get_nuodb_product_version(pdo_nuodb_db_handle *H);

    typedef struct
    {
        /* the link that owns this statement */
        void *H; /* opaque for pdo_nuodb_db_handle *H; */

        /* the statement handle */
        void *stmt; /* opaque for PdoNuoDbStatement *stmt; */

        /* NuoDB error information */
        pdo_nuodb_error_info einfo;

        /* copy of the sql statement */
        char *sql;

        /* the name of the cursor (if it has one) */
        char name[32];

        /* implicit txn - true when autocommit is disabled and
         * NuoDB will create a implicit txn because the app didn't
         * start one by calling nuodb_handle_begin. */
        char implicit_txn;

        /* for an implicit_txn, do we want to commit on close? */
        char commit_on_close;

        /* whether EOF was reached for this statement */
        unsigned exhausted:1;

        /* successful execute opens a cursor */
        unsigned cursor_open:1;

        unsigned _reserved:22;

        unsigned qty_input_params;
        /* the input params */
        nuo_params * in_params;

        /* the output params */
        nuo_params * out_params;

    } pdo_nuodb_stmt;

    int pdo_nuodb_stmt_errno(pdo_nuodb_stmt *S);
    const char *pdo_nuodb_stmt_errmsg(pdo_nuodb_stmt *S);
    pdo_error_type *pdo_nuodb_stmt_sqlstate(pdo_nuodb_stmt *S);
    int pdo_nuodb_stmt_delete(pdo_nuodb_stmt *S);
    int pdo_nuodb_stmt_execute(pdo_nuodb_stmt *S, int *column_count, long *row_count);
    int pdo_nuodb_stmt_fetch(pdo_nuodb_stmt *S, long *row_count);
    char const *pdo_nuodb_stmt_get_column_name(pdo_nuodb_stmt * S, int colno);
    int pdo_nuodb_stmt_get_sql_type(pdo_nuodb_stmt * S, int colno);
    int pdo_nuodb_stmt_set_integer(pdo_nuodb_stmt *S, int paramno, long int_val);
    int pdo_nuodb_stmt_set_boolean(pdo_nuodb_stmt *S, int paramno, char bool_val);
    int pdo_nuodb_stmt_set_string(pdo_nuodb_stmt *S, int paramno, char *str_val);
    int pdo_nuodb_stmt_set_bytes(pdo_nuodb_stmt *S, int paramno, const void *val, int length);
    void pdo_nuodb_stmt_get_integer(pdo_nuodb_stmt *S, int colno, int **int_val);
    void pdo_nuodb_stmt_get_boolean(pdo_nuodb_stmt *S, int colno, char **bool_val);
    void pdo_nuodb_stmt_get_long(pdo_nuodb_stmt *S, int colno, int64_t **long_val);
    const char *pdo_nuodb_stmt_get_string(pdo_nuodb_stmt *S, int colno);
    void pdo_nuodb_stmt_get_date(pdo_nuodb_stmt *S, int colno, int64_t **date_val);
    void pdo_nuodb_stmt_get_time(pdo_nuodb_stmt *S, int colno, int64_t **time_val);
    const char * pdo_nuodb_stmt_get_timestamp(pdo_nuodb_stmt *S, int colno);
    void pdo_nuodb_stmt_get_blob(pdo_nuodb_stmt *S, int colno, char ** ptr, unsigned long * len, void * (*erealloc)(void *ptr, size_t size, int allow_failure ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC));
    void pdo_nuodb_stmt_get_clob(pdo_nuodb_stmt *S, int colno, char ** ptr, unsigned long * len, void * (*erealloc)(void *ptr, size_t size, int allow_failure ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC));

#ifdef __cplusplus
} /* end of extern "C" { */
#endif

#endif  /* PHP_PDO_NUODB_C_CPP_COMMON_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
