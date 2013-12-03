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
#include <time.h>

extern "C" {
#include "php.h"
#ifdef ZEND_ENGINE_2
# include "zend_exceptions.h"
#endif
#include "php_ini.h"
#include "ext/standard/info.h"
#include "pdo/php_pdo.h"
#include "pdo/php_pdo_driver.h"
#include "php_pdo_nuodb.h"
}


#ifdef _MSC_VER  // Visual Studio specific
#include <stdint.h>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

//#define PHP_DEBUG 1
//#define TRUE 1
//#define FALSE 0


#include "php_pdo_nuodb_c_cpp_common.h"
#include "php_pdo_nuodb_cpp_int.h"

/* describes a column -- stolen from <path-to-php-sdk>/include/ext/pdo/php_pdo_driver.h */

#ifdef NONE
enum pdo_param_type {
        PDO_PARAM_NULL,

        /* int as in long (the php native int type).
         * If you mark a column as an int, PDO expects get_col to return
         * a pointer to a long */
        PDO_PARAM_INT,

        /* get_col ptr should point to start of the string buffer */
        PDO_PARAM_STR,

        /* get_col: when len is 0 ptr should point to a php_stream *,
         * otherwise it should behave like a string. Indicate a NULL field
         * value by setting the ptr to NULL */
        PDO_PARAM_LOB,

        /* get_col: will expect the ptr to point to a new PDOStatement object handle,
         * but this isn't wired up yet */
        PDO_PARAM_STMT, /* hierarchical result set */

        /* get_col ptr should point to a zend_bool */
        PDO_PARAM_BOOL,

        /* get_col ptr should point to a zval*
           and the driver is responsible for adding correct type information to get_column_meta()
         */
        PDO_PARAM_ZVAL
};

struct pdo_column_data {
        char *name;
        int namelen;
        unsigned long maxlen;
        enum pdo_param_type param_type;
        unsigned long precision;

        /* don't touch this unless your name is dbdo */
        void *dbdo_data;
};
#endif

PdoNuoDbGeneratedKeys::PdoNuoDbGeneratedKeys()
    : _qty(0), _keys(NULL)
{
    // empty
}

PdoNuoDbGeneratedKeys::~PdoNuoDbGeneratedKeys()
{
    if (_keys == NULL) return;
    for (int i=0; i < _qty; i++)
        if (_keys[i].columnName != NULL)
            delete _keys[i].columnName;
    delete [] _keys;
}

void PdoNuoDbGeneratedKeys::setKeys(NuoDB::ResultSet *rs)
{
  NuoDB::ResultSetMetaData *rsmd = NULL;
  if (rs == NULL) return;
  try {
          rsmd = rs->getMetaData();
          if (rsmd == NULL) return;
          int col_cnt = rsmd->getColumnCount();
          if (col_cnt < 1) return;
          _qty = col_cnt;
          _keys = new PdoNuoDbGeneratedKeyElement [col_cnt];
          rs->next();
          const char *col_name;
          for (int i=1; i <= col_cnt; i++)
          {
                  col_name = rsmd->getColumnName(i);
                  _keys[i-1].columnName = strdup(col_name);
                  _keys[i-1].columnIndex = i;
                  _keys[i-1].columnKeyValue = rs->getInt(i);
          }
  } catch (...) {
          return;
  }
}

int PdoNuoDbGeneratedKeys::getIdValue()
{
    if (_qty == 0) { // No generated keys
        return 0;
    }

    if (_qty > 1) { // Multiple generated keys
        return 0;
    }

    return _keys[0].columnKeyValue;
}

int PdoNuoDbGeneratedKeys::getIdValue(const char *seqName)
{
    // We currently do not support tables with multiple generated keys
    // so we will throw a "not supported" exception. However, below
    // is a reasonable implementation for multiple keys.  Assuming
    // seqName is a column name.

    for (int i=0; i<_qty; i++)
        if (!strcmp(_keys[i].columnName, seqName))
            return _keys[i].columnKeyValue;

    return 0;
}

PdoNuoDbHandle::PdoNuoDbHandle(pdo_dbh_t *pdo_dbh, SqlOptionArray * options)
    : _pdo_dbh(pdo_dbh), _con(NULL), _opts(NULL), _last_stmt(NULL), _last_keys(NULL)
{
    einfo.errcode = 0;
    einfo.errmsg = NULL;
    einfo.file = NULL;
    einfo.line = 0;
    strcpy(sqlstate, PDO_ERR_NONE);
    for (int i=0; i<4; i++)
    {
        _opt_arr[i].option = NULL;
        _opt_arr[i].extra = NULL;
    }
    setOptions(options);
}

PdoNuoDbHandle::~PdoNuoDbHandle()
{
    if (_last_keys != NULL) delete _last_keys;
        if (einfo.errmsg) free(einfo.errmsg);
    closeConnection();
    deleteOptions();
}

_pdo_dbh_t *PdoNuoDbHandle::getPdoDbh() {
        return _pdo_dbh;
}

int PdoNuoDbHandle::getEinfoLine() {
        return einfo.line;
}

const char *PdoNuoDbHandle::getEinfoFile() {
        return einfo.file;
}

int PdoNuoDbHandle::getEinfoErrcode() {
        return einfo.errcode;
}

const char *PdoNuoDbHandle::getEinfoErrmsg() {
        return einfo.errmsg;
}

pdo_error_type *PdoNuoDbHandle::getSqlstate() {
        return &sqlstate;
}

void PdoNuoDbHandle::setEinfoLine(int line) {
        einfo.line = line;
}

void PdoNuoDbHandle::setEinfoFile(const char *file) {
        if (einfo.file)
                free(einfo.file);
        einfo.file = strdup(file);
}

void PdoNuoDbHandle::setEinfoErrcode(int errcode) {
        einfo.errcode = errcode;
}

void PdoNuoDbHandle::setEinfoErrmsg(const char *errmsg) {
        if (einfo.errmsg)
                free(einfo.errmsg);
        einfo.errmsg = strdup(errmsg);
}

void PdoNuoDbHandle::setSqlstate(const char *sqlState) {
        strncpy(this->sqlstate, sqlState, 6);
}

void PdoNuoDbHandle::deleteOptions()
{
    if (_opts == NULL)
    {
        return;
    }
    for (int i=0; i<4; i++)
    {
        if (_opt_arr[i].option != NULL)
        {
            free((void *)_opt_arr[i].option);
            _opt_arr[i].option = NULL;
        }
        if (_opt_arr[i].extra != NULL)
        {
            free((void *)_opt_arr[i].extra);
            _opt_arr[i].extra = NULL;
        }
    }
    delete _opts;
    _opts = NULL;
}

void PdoNuoDbHandle::setLastStatement(PdoNuoDbStatement *lastStatement)
{
    _last_stmt = lastStatement;
}

void PdoNuoDbHandle::setLastKeys(PdoNuoDbGeneratedKeys *lastKeys)
{
    if (_last_keys != NULL)
        delete _last_keys;
    _last_keys = lastKeys;
}


void PdoNuoDbHandle::setOptions(SqlOptionArray * options)
{
    deleteOptions();
    _opts = new SqlOptionArray;
    _opts->count = 4;
    _opts->array = _opt_arr;
    for (int i=0; i<_opts->count; i++) {
    	_opt_arr[i].option = (char const *) (options->array[i].option ? strdup(options->array[i].option) : NULL );
    	_opt_arr[i].extra = (void *) (options->array[i].extra ? strdup((const char *)options->array[i].extra) : NULL);
    }
}

NuoDB::Connection * PdoNuoDbHandle::createConnection()
{
        // note: caller will catch exceptions.
    closeConnection();
/*
    _con = NuoDB::Connection::create((const char *)_opts->array[0].extra,
                                    (const char *)_opts->array[1].extra,
                                    (const char *)_opts->array[2].extra,
                                    1,
                                    (const char *)_opts->array[3].option,
                                    (const char *)_opts->array[3].extra);

*/

    _con = NuoDB::Connection::createUtf8();
    NuoDB::Properties* properties = _con->allocProperties();

    properties->putValue("user", (const char *)_opts->array[1].extra);
    properties->putValue("password", (const char *)_opts->array[2].extra);
    properties->putValue((const char *)_opts->array[3].option, (const char *)_opts->array[3].extra);

    _con->openDatabase((const char *)_opts->array[0].extra, properties);

    return _con;
}

NuoDB::Connection * PdoNuoDbHandle::getConnection()
{
    return _con;
}

PdoNuoDbStatement * PdoNuoDbHandle::createStatement(pdo_stmt_t * pdo_stmt, char const * sql)
{
        // NOTE: caller catches exceptions.
    PdoNuoDbStatement * nuodb_stmt = NULL;
    if (sql == NULL)
    {
        return NULL;
    }
    nuodb_stmt = new PdoNuoDbStatement(this, pdo_stmt);
    nuodb_stmt->createStatement(sql);
    setLastStatement(nuodb_stmt);
    return nuodb_stmt;
}

void PdoNuoDbHandle::closeConnection()
{
        // NOTE: caller catches exceptions.
    if (_con == NULL)
    {
        return;
    }
    _con->close();
    _con = NULL;
}

void PdoNuoDbHandle::commit()
{
        // NOTE: caller catches exceptions.
    PDO_DBG_ENTER("PdoNuoDbHandle::commit");
    if (_con == NULL)
    {
        PDO_DBG_VOID_RETURN;
    }
    _con->commit();
    PDO_DBG_VOID_RETURN;
}

void PdoNuoDbHandle::rollback()
{
        // NOTE: caller catches exceptions.
    PDO_DBG_ENTER("PdoNuoDbHandle::rollback");
    if (_con == NULL)
    {
        PDO_DBG_VOID_RETURN;
    }
    _con->rollback();
    PDO_DBG_VOID_RETURN;
}

int PdoNuoDbHandle::getLastId(const char *name)
{
        // NOTE: caller catches exceptions.
    if (_last_keys == NULL) {
        setEinfoErrcode(-40);
        setEinfoErrmsg("No generated keys");
        setEinfoFile(__FILE__);
        setEinfoLine(__LINE__);
        setSqlstate("P0001");
        _pdo_nuodb_error(_pdo_dbh, NULL, getEinfoFile(), getEinfoLine() /*TSRMLS_DC*/);
        return 0;
    }

    if (name == NULL) return _last_keys->getIdValue();


    // We would normally do the following:
    // return _last_keys->getIdValue(name);
    // but NuoDB does not currently support getting IDs by name, so instead we must
    // signal an error as follows:
        setEinfoErrcode(-17);
        setEinfoErrmsg("Unknown Error in PdoNuoDbHandle::getLastId");
        setEinfoFile(__FILE__);
        setEinfoLine(__LINE__);
        setSqlstate("XX000");
        _pdo_nuodb_error(getPdoDbh(), NULL, getEinfoFile(), getEinfoLine()/* TSRMLS_DC*/);
    return 0;

}

void PdoNuoDbHandle::setAutoCommit(bool autoCommit)
{
        // NOTE: caller catches exceptions.
    _con->setAutoCommit(autoCommit);
    return;
}

const char *PdoNuoDbHandle::getNuoDBProductName()
{
        // NOTE: caller catches exceptions.
    NuoDB::DatabaseMetaData *dbmd = _con->getMetaData();
    if (dbmd == NULL) return NULL;
    return dbmd->getDatabaseProductName();
}

const char *PdoNuoDbHandle::getNuoDBProductVersion()
{
        // NOTE: caller catches exceptions.
    NuoDB::DatabaseMetaData *dbmd = _con->getMetaData();
    if (dbmd == NULL) return NULL;
    return dbmd->getDatabaseProductVersion();
}

PdoNuoDbStatement::PdoNuoDbStatement(PdoNuoDbHandle * dbh, pdo_stmt_t *pdo_stmt) : _nuodbh(dbh), _pdo_stmt(pdo_stmt), _sql(NULL), _stmt(NULL), _rs(NULL)
{
        // NOTE: caller catches exceptions.
        if (pdo_stmt != NULL) {
                pdo_nuodb_stmt *S = (pdo_nuodb_stmt *) pdo_stmt->driver_data;
                S->stmt = this;
        }
        einfo.errcode = 0;
        einfo.errmsg = NULL;
        einfo.file = NULL;
        einfo.line = 0;
        strcpy(sqlstate, PDO_ERR_NONE);
}

PdoNuoDbStatement::~PdoNuoDbStatement()
{
        // NOTE: caller catches exceptions.
    if (einfo.errmsg) free(einfo.errmsg);
    _nuodbh->setLastStatement(NULL);
    if (_sql != NULL)
    {
       free((void *)_sql);
    }
    _sql = NULL;
    if (_rs != NULL)
    {
        _rs->close();
    }
    _rs = NULL;
    if (_stmt != NULL)
    {
        _stmt->close();
    }
    _stmt = NULL;
}

NuoDB::PreparedStatement * PdoNuoDbStatement::createStatement(char const * sql)
{
        // NOTE: caller catches exceptions.
    if (sql == NULL)
    {
        return NULL;
    }
    _sql = strdup(sql);
    NuoDB::Connection * _con = NULL;
    _con = _nuodbh->getConnection();
    if (_con == NULL)
    {
        return NULL;
    }

    _stmt = NULL;
    try {
        _stmt = _con->prepareStatement(sql, NuoDB::RETURN_GENERATED_KEYS);
    } catch (NuoDB::SQLException & e) {
        // because the exception happened in the process of creating a prepared
        // statement, PHP won't be looking for the error information on the
        // statement handle. Instead PHP will look for error information in
        // the DB handle.
        _nuodbh->setEinfoErrcode(e.getSqlcode());
        _nuodbh->setEinfoErrmsg(e.getText());
        _nuodbh->setEinfoFile(__FILE__);
        _nuodbh->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // _dbh->setSqlstate(e.getSQLState());
        _nuodbh->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(_nuodbh->getPdoDbh(), NULL, _nuodbh->getEinfoFile(), _nuodbh->getEinfoLine()/* TSRMLS_DC*/);
    } catch (...) {
        _nuodbh->setEinfoErrcode(-17);
        _nuodbh->setEinfoErrmsg("Unknown Error in PdoNuoDbStatement::createStatement");
        _nuodbh->setEinfoFile(__FILE__);
        _nuodbh->setEinfoLine(__LINE__);
        _nuodbh->setSqlstate("XX000");
        _pdo_nuodb_error(_nuodbh->getPdoDbh(), NULL, _nuodbh->getEinfoFile(), _nuodbh->getEinfoLine()/* TSRMLS_DC*/);
    }

    return _stmt;
}

PdoNuoDbHandle * PdoNuoDbStatement::getNuoDbHandle() {
        return _nuodbh;
}

pdo_stmt_t *PdoNuoDbStatement::getPdoStmt() {
        return _pdo_stmt;
}


int PdoNuoDbStatement::getEinfoLine() {
        return einfo.line;
}

const char *PdoNuoDbStatement::getEinfoFile() {
        return einfo.file;
}

int PdoNuoDbStatement::getEinfoErrcode() {
        return einfo.errcode;
}

const char *PdoNuoDbStatement::getEinfoErrmsg() {
        return einfo.errmsg;
}

pdo_error_type *PdoNuoDbStatement::getSqlstate() {
        return &sqlstate;
}

void PdoNuoDbStatement::setEinfoLine(int line) {
        einfo.line = line;
}

void PdoNuoDbStatement::setEinfoFile(const char *file) {
        if (einfo.file)
                free(einfo.file);
        einfo.file = strdup(file);
}

void PdoNuoDbStatement::setEinfoErrcode(int errcode) {
        einfo.errcode = errcode;
}

void PdoNuoDbStatement::setEinfoErrmsg(const char *errmsg) {
        if (einfo.errmsg)
                free(einfo.errmsg);
        einfo.errmsg = strdup(errmsg);
}

void PdoNuoDbStatement::setSqlstate(const char *sqlState) {
        strncpy(this->sqlstate, sqlState, 6);
}

int PdoNuoDbStatement::execute()
{
        // NOTE: caller catches exceptions.
    PDO_DBG_ENTER("PdoNuoDbHandle::execute");
    if (_stmt == NULL)
    {
        PDO_DBG_RETURN(0);
    }

    int update_count = 0;
    struct pdo_nuodb_timer_t timer;
    pdo_nuodb_timer_init(&timer);
    pdo_nuodb_timer_start(&timer);
    bool result = _stmt->execute();
    pdo_nuodb_timer_end(&timer);
    int elapsed = pdo_nuodb_get_elapsed_time_in_microseconds(&timer);
    PDO_DBG_INF_FMT("Elapsed time=%d (microseconds) : SQL=%s", elapsed, this->_sql);
    if (result == TRUE) {  // true means there was no UPDATE or INSERT
       _rs = _stmt->getResultSet();
    } else {
       update_count = _stmt->getUpdateCount();
       PDO_DBG_INF_FMT("Update Count=%d", update_count);
       if (update_count != 0)
       {
         NuoDB::ResultSet *_rs_gen_keys = NULL;

         _rs_gen_keys = _stmt->getGeneratedKeys();
         if (_rs_gen_keys != NULL)
         {
           PdoNuoDbGeneratedKeys *keys = new PdoNuoDbGeneratedKeys();
           keys->setKeys(_rs_gen_keys);
           _nuodbh->setLastKeys(keys);
           _rs_gen_keys->close();
         }
       }
    }

    PDO_DBG_RETURN(update_count);
}

bool PdoNuoDbStatement::hasResultSet()
{
    return (_rs != NULL);
}

bool PdoNuoDbStatement::next()
{
        // NOTE: caller catches exceptions.
    if (_rs == NULL)
    {
        return false;
    }
    return _rs->next();
}

size_t PdoNuoDbStatement::getColumnCount()
{
        // NOTE: caller catches exceptions.
    if (_rs == NULL)
    {
        return false;
    }
    NuoDB::ResultSetMetaData * md = _rs->getMetaData();
    return md->getColumnCount();
}

char const * PdoNuoDbStatement::getColumnName(size_t column)
{
    char const * rval = NULL;
    if (_rs == NULL)
    {
        return NULL;
    }
    try
    {
        NuoDB::ResultSetMetaData * md = _rs->getMetaData();
        rval = md->getColumnLabel(column+1);

    } catch (NuoDB::SQLException & e) {
        setEinfoErrcode(e.getSqlcode());
        setEinfoErrmsg(e.getText());
        setEinfoFile(__FILE__);
        setEinfoLine(__LINE__);
        // Workaround DB-4112
        // _dbh->setSqlstate(e.getSQLState());
        setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(_nuodbh->getPdoDbh(), _pdo_stmt, getEinfoFile(), getEinfoLine() /*TSRMLS_DC*/);
    } catch (...) {
        setEinfoErrcode(-17);
        setEinfoErrmsg("Unknown Error in PdoNuoDbStatement::getColumnName");
        setEinfoFile(__FILE__);
        setEinfoLine(__LINE__);
        setSqlstate("XX000");
        _pdo_nuodb_error(_nuodbh->getPdoDbh(), _pdo_stmt, getEinfoFile(), getEinfoLine() /*TSRMLS_DC*/);
    }

    return rval;
}

int PdoNuoDbStatement::getSqlType(size_t column)
{
        NuoDB::ResultSetMetaData * md = NULL;
        int sqlType = 0;
    PDO_DBG_ENTER("PdoNuoDbStatement::getSqlType");
    if (_rs == NULL) {
        PDO_DBG_RETURN(0);
    }
    try {
        md = _rs->getMetaData();
        sqlType = md->getColumnType(column+1);

        switch (sqlType)
        {
                case NuoDB::NUOSQL_BOOLEAN:
                        return PDO_NUODB_SQLTYPE_BOOLEAN;
                case NuoDB::NUOSQL_INTEGER:
                case NuoDB::NUOSQL_SMALLINT:
                case NuoDB::NUOSQL_TINYINT:
                        return PDO_NUODB_SQLTYPE_INTEGER;

                // We are returning numeric types as a string because of
                // DB-2288.
                case NuoDB::NUOSQL_BIGINT:
                case NuoDB::NUOSQL_FLOAT:
                case NuoDB::NUOSQL_DOUBLE:
                case NuoDB::NUOSQL_DECIMAL:
                case NuoDB::NUOSQL_NUMERIC:
                        return PDO_NUODB_SQLTYPE_STRING;

                case NuoDB::NUOSQL_CHAR:
                case NuoDB::NUOSQL_VARCHAR:
                case NuoDB::NUOSQL_LONGVARCHAR:
                        return PDO_NUODB_SQLTYPE_STRING;

                case NuoDB::NUOSQL_DATE:
                        return PDO_NUODB_SQLTYPE_DATE;

                case NuoDB::NUOSQL_TIME:
                        return PDO_NUODB_SQLTYPE_TIME;

                case NuoDB::NUOSQL_TIMESTAMP:
                        return PDO_NUODB_SQLTYPE_TIMESTAMP;

                case NuoDB::NUOSQL_BLOB:
                        return PDO_NUODB_SQLTYPE_BLOB;

                case NuoDB::NUOSQL_CLOB:
                        return PDO_NUODB_SQLTYPE_CLOB;

                default: {
                        PDO_DBG_ERR_FMT("unknown/unsupported type: %d", sqlType);
                        break;
                }
        }
    } catch (NuoDB::SQLException & e) {
        setEinfoErrcode(e.getSqlcode());
        setEinfoErrmsg(e.getText());
        setEinfoFile(__FILE__);
        setEinfoLine(__LINE__);
        // Workaround DB-4112
        // _dbh->setSqlstate(e.getSQLState());
        setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(_nuodbh->getPdoDbh(), _pdo_stmt, getEinfoFile(), getEinfoLine() /*TSRMLS_DC*/);
    } catch (...) {
        setEinfoErrcode(-17);
        setEinfoErrmsg("Unknown Error in PdoNuoDbStatement::getSqlType");
        setEinfoFile(__FILE__);
        setEinfoLine(__LINE__);
        setSqlstate("XX000");
        _pdo_nuodb_error(_nuodbh->getPdoDbh(), _pdo_stmt, getEinfoFile(), getEinfoLine() /*TSRMLS_DC*/);
    }
    PDO_DBG_RETURN(0);
}

char const * PdoNuoDbStatement::getString(size_t column)
{
        // NOTE: caller catches exceptions.
    char const *res = NULL;

    if (_rs == NULL)
    {
        return res;
    }
    res =  _rs->getString(column+1);
    if (_rs->wasNull()) {
    	res = NULL;
    }
    return res;
}

void PdoNuoDbStatement::getInteger(size_t column, int **int_val)
{
    // NOTE: caller catches exceptions.
    if (_rs == NULL)
    {
        return;
    }
    **int_val = _rs->getInt(column+1);
    if (_rs->wasNull()) {
    	*int_val = NULL;
    }
}

bool PdoNuoDbStatement::getBoolean(size_t column, char **bool_val)
{
        // NOTE: caller catches exceptions.
    if (_rs == NULL)
    {
        return 0;
    }
    **bool_val = _rs->getBoolean(column+1);
    if (_rs->wasNull()) {
    	*bool_val = NULL;
    }

}

void PdoNuoDbStatement::getLong(size_t column, int64_t **long_val)
{
    // NOTE: caller catches exceptions.
    if (_rs == NULL)
    {
        return;
    }
    **long_val = _rs->getLong(column+1);
    if (_rs->wasNull()) {
    	*long_val = NULL;
    }
}

char const *PdoNuoDbStatement::getTimestamp(size_t column)
{
    // NOTE: caller catches exceptions.
    char const *res = NULL;

    if (_rs == NULL)
    {
        return res;
    }
    res =  _rs->getString(column+1);
    if (_rs->wasNull()) {
    	res = NULL;
    }
    return res;
}

void PdoNuoDbStatement::getTime(size_t column, int64_t **time_val)
{
    // NOTE: caller catches exceptions.
    if (_rs == NULL)
    {
        return;
    }
    NuoDB::Time *time = _rs->getTime(column+1);
    if (_rs->wasNull()) {
    	*time_val = NULL;
    	return;
    }
    **time_val = time->getSeconds();
}

void PdoNuoDbStatement::getDate(size_t column, int64_t **date_val)
{
    // NOTE: caller catches exceptions.
    if (_rs == NULL)
    {
        return;
    }
    NuoDB::Date *date = _rs->getDate(column+1);
    if (_rs->wasNull()) {
    	*date_val = NULL;
    	return;
    }
    **date_val = date->getSeconds();
}

void PdoNuoDbStatement::getBlob(size_t column, char ** ptr, unsigned long * len, void * (*erealloc)(void *ptr, size_t size, int allow_failure ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC))
{
        // NOTE: caller catches exceptions.
    if (_rs == NULL)
    {
        return;
    }
    NuoDB::Blob *blob = _rs->getBlob(column+1);
    *len = blob->length();
    if ((*len) == 0) {
        *ptr = NULL;
    } else {

        *ptr = (char *)(*erealloc)((void *)*ptr, *len+1, 0 ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC);
        blob->getBytes(0, *len, (unsigned char *)*ptr);
        (*ptr)[*len] = '\0';
    }
    return;
}

void PdoNuoDbStatement::getClob(size_t column, char ** ptr, unsigned long * len, void * (*erealloc)(void *ptr, size_t size, int allow_failure ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC))
{
        // NOTE: caller catches exceptions.
    if (_rs == NULL)
    {
        return;
    }
    NuoDB::Clob *clob = _rs->getClob(column+1);
    *len = clob->length();
    if ((*len) == 0) {
        *ptr = NULL;
    } else {
        *ptr = (char *)(*erealloc)((void *)*ptr, *len+1, 0 ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC);
        clob->getChars(0, *len, (char *)*ptr);
        (*ptr)[*len] = '\0';
    }
    return;
}

size_t PdoNuoDbStatement::getNumberOfParameters()
{
    if (_stmt == NULL) {
        return 0;
    }
    NuoDB::ParameterMetaData *pmd = _stmt->getParameterMetaData();
    if (pmd == NULL) {
        return 0;
    }
    return pmd->getParameterCount();
}

void PdoNuoDbStatement::setInteger(size_t index, int value)
{
        // NOTE: caller catches exceptions.
    if (_stmt == NULL) {
        return;
    }
        _stmt->setInt(index+1, value);
    return;
}

void PdoNuoDbStatement::setBoolean(size_t index, bool value)
{
        // NOTE: caller catches exceptions.
    if (_stmt == NULL) {
        return;
    }
        _stmt->setBoolean(index+1, value);
    return;
}

void PdoNuoDbStatement::setString(size_t index, const char *value)
{
        // NOTE: caller catches exceptions.
    if (_stmt == NULL) {
        return;
    }

        _stmt->setString(index+1, value);
    return;
}

void PdoNuoDbStatement::setBytes(size_t index, const void *value, int length)
{
        // NOTE: caller catches exceptions.
    if (_stmt == NULL) {
        return;
    }

        _stmt->setBytes(index+1, length, value);
    return;
}


void PdoNuoDbStatement::setBlob(size_t index, const char *value, int len)
{
        // NOTE: caller catches exceptions.
    if (_stmt == NULL) {
        return;
    }
    NuoDB::Blob *blob = _nuodbh->getConnection()->createBlob();
    if (value != NULL)
        blob->setBytes(len, (const unsigned char *)value);
    _stmt->setBlob(index+1, blob);
    return;
}

void PdoNuoDbStatement::setClob(size_t index, const char *value, int len)
{
        // NOTE: caller catches exceptions.
    if (_stmt == NULL) {
        return;
    }
    NuoDB::Clob *clob = _nuodbh->getConnection()->createClob();
    if (value != NULL)
        clob->setChars(len, (const char *)value);
    _stmt->setClob(index+1, clob);
    return;
}

// C/C++ jump functions

extern "C" {

/* Timer fuctions */

void pdo_nuodb_timer_init(struct pdo_nuodb_timer_t *timer)
{
    if (timer == NULL) return;
#ifdef WIN32
    QueryPerformanceFrequency(&(timer->frequency));
    timer->startCount.QuadPart = 0;
    timer->endCount.QuadPart = 0;
#else
    timer->startCount.tv_sec = timer->startCount.tv_usec = 0;
    timer->endCount.tv_sec = timer->endCount.tv_usec = 0;
#endif
    timer->stopped = 0;
    timer->startTimeInMicroSec = 0;
    timer->endTimeInMicroSec = 0;
}

void pdo_nuodb_timer_start(struct pdo_nuodb_timer_t *timer)
{
    if (timer == NULL) return;
    timer->stopped = 0; // reset stop flag
#ifdef WIN32
    QueryPerformanceCounter(&(timer->startCount));
#else
    gettimeofday(&(timer->startCount), NULL);
#endif

}

void pdo_nuodb_timer_end(struct pdo_nuodb_timer_t *timer)
{
    if (timer == NULL) return;
    timer->stopped = 1; // set timer stopped flag

#ifdef WIN32
    QueryPerformanceCounter(&(timer->endCount));
#else
    gettimeofday(&(timer->endCount), NULL);
#endif
}

int pdo_nuodb_get_elapsed_time_in_microseconds(struct pdo_nuodb_timer_t *timer)
{
    if (timer == NULL) return 0.0;
#ifdef WIN32
    if(!timer->stopped)
        QueryPerformanceCounter(&(timer->endCount));

    timer->startTimeInMicroSec = timer->startCount.QuadPart * (1000000 / timer->frequency.QuadPart);
    timer->endTimeInMicroSec = timer->endCount.QuadPart * (1000000 / timer->frequency.QuadPart);
#else
    if(!timer->stopped)
        gettimeofday(&(timer->endCount), NULL);

    timer->startTimeInMicroSec = (timer->startCount.tv_sec * 1000000) + timer->startCount.tv_usec;
    timer->endTimeInMicroSec = (timer->endCount.tv_sec * 1000000) + timer->endCount.tv_usec;
#endif

    return timer->endTimeInMicroSec - timer->startTimeInMicroSec;
}

/* Logging */

extern FILE *nuodb_log_fp;

void get_timestamp(char *time_buffer)
{
  char fmt[64];

  struct timeval  tv;
  struct tm       *tm;

#ifdef WIN32
  __time64_t long_time;
#endif

  if (nuodb_log_fp == NULL) return;

#ifdef WIN32
        _time64( &long_time );           // Get time as 64-bit integer.
                                     // Convert to local time.
   tm = _localtime64( &long_time ); // C4996
        // Note: _localtime64 deprecated; consider _localetime64_s

#else
  gettimeofday(&tv, NULL);
  tm = localtime(&tv.tv_sec);
#endif
  if(tm != NULL)
  {
       strftime(fmt, sizeof fmt, "%Y-%m-%d %H:%M:%S.%%06u %z", tm);
#ifdef WIN32
       _snprintf(time_buffer, 64, fmt, tv.tv_usec);
#else
       snprintf(time_buffer, 64, fmt, tv.tv_usec);
#endif
  }
}

void pdo_nuodb_log(int lineno, const char *file, const char *log_level, const char *log_msg)
{
  char buf[64] = "";
  if (nuodb_log_fp == NULL) return;
  get_timestamp(buf);
  fprintf(nuodb_log_fp, "%s : %s : %s(%d) : %s\n", buf, log_level, file, lineno, log_msg);
  fflush(nuodb_log_fp);
}

void pdo_nuodb_log_va(int lineno, const char *file, const char *log_level, char *format, ...)
{
  char buf[64] = "";
  va_list args;

  if (nuodb_log_fp == NULL) return;
  va_start(args, format);
  get_timestamp(buf);
  fprintf(nuodb_log_fp, "%s : %s : %s(%d) : ", buf, log_level, file, lineno);
  vfprintf(nuodb_log_fp, format, args);
  va_end(args);
  fputs("\n", nuodb_log_fp);
  fflush(nuodb_log_fp);
}

int pdo_nuodb_func_enter(int lineno, const char *file, const char *func_name, int func_name_len) {
  char buf[64] = "";
  if (nuodb_log_fp == NULL) return FALSE;
  get_timestamp(buf);
  fprintf(nuodb_log_fp, "%s : info : %s(%d) : ENTER FUNCTION : %s\n", buf, file, lineno, func_name);
  fflush(nuodb_log_fp);
  return TRUE;
}

void pdo_nuodb_func_leave(int lineno, const char *file) {
  char buf[64] = "";
  if (nuodb_log_fp == NULL) return;
  get_timestamp(buf);
  fprintf(nuodb_log_fp, "%s : info : %s(%d) : LEAVE FUNCTION\n", buf, file, lineno);
  fflush(nuodb_log_fp);
  return;
}

int pdo_nuodb_db_handle_errno(pdo_nuodb_db_handle *H) {
        if (H == NULL) return 0;
        if (H->db == NULL) return 0;
    PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
    return db->getEinfoErrcode();
}

const char *pdo_nuodb_db_handle_errmsg(pdo_nuodb_db_handle *H) {
        if (H == NULL) return 0;
        if (H->db == NULL) return 0;
    PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
    return db->getEinfoErrmsg();
}

pdo_error_type * pdo_nuodb_db_handle_sqlstate(pdo_nuodb_db_handle *H) {
        if (H == NULL) return 0;
        if (H->db == NULL) return 0;
    PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
    return db->getSqlstate();
}


int pdo_nuodb_db_handle_commit(pdo_nuodb_db_handle *H) {
        try {
                PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
                db->commit();
        } catch (...) {
        return 0; // TODO - save the error message text.
    }
    return 1;
}

int pdo_nuodb_db_handle_rollback(pdo_nuodb_db_handle *H) {
        try {
                PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
                db->rollback();
        } catch (...) {
        return 0; // TODO - save the error message text.
    }
    return 1;
}

int pdo_nuodb_db_handle_close_connection(pdo_nuodb_db_handle *H) {
        try {
                PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
                db->closeConnection();
        } catch (...) {
        return 0; // TODO - save the error message text.
    }
    return 1;
}

int pdo_nuodb_db_handle_delete(pdo_nuodb_db_handle *H) {
        try {
                PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
                delete db;
                H->db = NULL;
        } catch (...) {
                H->db = NULL;
        return 0; // TODO - save the error message text.
    }
    return 1;
}

int pdo_nuodb_db_handle_set_auto_commit(pdo_nuodb_db_handle *H, unsigned int auto_commit)
{
    PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
    try {
        bool bAutoCommit = (auto_commit != 0);
        db->setAutoCommit(bAutoCommit);
    }
    catch (NuoDB::SQLException & e)
    {
        db->setEinfoErrcode(e.getSqlcode());
        db->setEinfoErrmsg(e.getText());
        db->setEinfoFile(__FILE__);
        db->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // _dbh->setSqlstate(e.getSQLState());
        db->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine() /*TSRMLS_DC*/);
        return 0;
    }
    catch (...)
    {
        db->setEinfoErrcode(-17);
        db->setEinfoErrmsg("UNKNOWN ERROR in pdo_nuodb_db_handle_set_auto_commit()");
        db->setEinfoFile(__FILE__);
        db->setEinfoLine(__LINE__);
        db->setSqlstate("XX000");
        _pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine() /*TSRMLS_DC*/);
        return 0;
    }
    return 1;
}

const char *pdo_nuodb_db_handle_get_nuodb_product_name(pdo_nuodb_db_handle *H)
{
    PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
    const char *rval = NULL;
    try
    {
        rval = db->getNuoDBProductName();
    }
    catch (NuoDB::SQLException & e)
    {
        db->setEinfoErrcode(e.getSqlcode());
        db->setEinfoErrmsg(e.getText());
        db->setEinfoFile(__FILE__);
        db->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // _dbh->setSqlstate(e.getSQLState());
        db->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine() /*TSRMLS_DC*/);
        return NULL;
    }
    catch (...)
    {
        db->setEinfoErrcode(-17);
        db->setEinfoErrmsg("UNKNOWN ERROR in pdo_nuodb_db_handle_get_nuodb_product_name()");
        db->setEinfoFile(__FILE__);
        db->setEinfoLine(__LINE__);
        db->setSqlstate("XX000");
        _pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine() /*TSRMLS_DC*/);
        return NULL;
    }
    return rval;
}

const char *pdo_nuodb_db_handle_get_nuodb_product_version(pdo_nuodb_db_handle *H)
{
    PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
    char *default_version = "2.0"; // Workaround DB-962.
    const char *rval = NULL;
    try
    {
        rval = db->getNuoDBProductVersion();
        if (strcmp(rval, "%%PRODUCT_VERSION%%") == 0) { // DB-2559 -- Workaround DB-962
            rval = default_version;
        }
    }
    catch (NuoDB::SQLException & e)
    {
        // TODO: need to write a test case for this.
        db->setEinfoErrcode(e.getSqlcode());
        db->setEinfoErrmsg(e.getText());
        db->setEinfoFile(__FILE__);
        db->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // _dbh->setSqlstate(e.getSQLState());
        db->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine() /*TSRMLS_DC*/);
        return NULL;
    }
    catch (...)
    {
        db->setEinfoErrcode(-17);
        db->setEinfoErrmsg("UNKNOWN ERROR in pdo_nuodb_db_handle_get_nuodb_product_version()");
        db->setEinfoFile(__FILE__);
        db->setEinfoLine(__LINE__);
        db->setSqlstate("XX000");
        _pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine() /*TSRMLS_DC*/);
        return NULL;
    }
    return rval;
}


void *pdo_nuodb_db_handle_create_statement(pdo_nuodb_db_handle * H, pdo_stmt_t * pdo_stmt, const char * sql)
{
    PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
    PdoNuoDbStatement *nuodb_stmt = NULL;
    try
    {
        nuodb_stmt = db->createStatement(pdo_stmt, sql);
    }
    catch (...)
    {
        nuodb_stmt = NULL; // TODO - save the error message text.
    }
    return (void *)nuodb_stmt;
}

// Should this change???  T.GATES 10/10/2013
long pdo_nuodb_db_handle_doer(pdo_nuodb_db_handle * H, void *dbh_opaque, const char * sql, unsigned in_txn, unsigned auto_commit, void (*pt2pdo_dbh_t_set_in_txn)(void *dbh_opaque, unsigned in_txn))
{
    PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
    unsigned in_txn_state = in_txn;
    long res;
    try
    {
        PdoNuoDbStatement * nuodb_stmt = (PdoNuoDbStatement *) pdo_nuodb_db_handle_create_statement(H, NULL, sql);

        // Not needed?  T.GATES 10/10/2013
        //(*pt2pdo_dbh_t_set_in_txn)(dbh_opaque, 1);

        if ((H->pdo_dbh->auto_commit == 0) &&
                        (H->pdo_dbh->in_txn == 0))
        {
                if ((H->in_nuodb_implicit_txn == 0) && (H->in_nuodb_explicit_txn == 0)) {
                        H->in_nuodb_implicit_txn = 1;
                }
        }

        res = nuodb_stmt->execute();

        if (H->in_nuodb_implicit_txn == 1) {
                pdo_nuodb_db_handle_commit(H);
                H->in_nuodb_implicit_txn = 0;
                H->in_nuodb_explicit_txn = 0;
        }
    }
    catch (NuoDB::SQLException & e)
    {
        // TODO: need to write a test case for this.
        db->setEinfoErrcode(e.getSqlcode());
        db->setEinfoErrmsg(e.getText());
        db->setEinfoFile(__FILE__);
        db->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // _dbh->setSqlstate(e.getSQLState());
        db->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine() /*TSRMLS_DC*/);
        (*pt2pdo_dbh_t_set_in_txn)(dbh_opaque, in_txn_state);
        return -1;
    }
    catch (...)
    {
        db->setEinfoErrcode(-17);
        db->setEinfoErrmsg("UNKNOWN ERROR in pdo_nuodb_db_handle_doer()");
        db->setEinfoFile(__FILE__);
        db->setEinfoLine(__LINE__);
        db->setSqlstate("XX000");
        _pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine() /*TSRMLS_DC*/);
        (*pt2pdo_dbh_t_set_in_txn)(dbh_opaque, in_txn_state);
        return -1;
    }
    (*pt2pdo_dbh_t_set_in_txn)(dbh_opaque, in_txn_state);
    return res;
}


int pdo_nuodb_db_handle_factory(pdo_nuodb_db_handle * H, SqlOptionArray *optionsArray, char **errMessage)
{
    PdoNuoDbHandle *db = NULL;
    *errMessage = NULL;
    try {
        db = new PdoNuoDbHandle(H->pdo_dbh, optionsArray);
        H->db = (void *) db;
        db->createConnection();
    } catch (NuoDB::SQLException & e) {
        if (db != NULL) {
           db->setEinfoErrcode(e.getSqlcode());
           db->setEinfoErrmsg(e.getText());
           db->setEinfoFile(__FILE__);
           db->setEinfoLine(__LINE__);
           // Workaround DB-4112
           // _dbh->setSqlstate(e.getSQLState());
           db->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
           _pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine() /*TSRMLS_DC*/);
        }
        return 0;
    } catch (...) {
        if (db != NULL) {
           db->setEinfoErrcode(-17);
           db->setEinfoErrmsg("Unknown Error in pdo_nuodb_db_handle_factory()");
           db->setEinfoFile(__FILE__);
           db->setEinfoLine(__LINE__);
           db->setSqlstate("XX000");
           _pdo_nuodb_error(db->getPdoDbh(), NULL, db->getEinfoFile(), db->getEinfoLine() /*TSRMLS_DC*/);
        }
        *errMessage = strdup("Error constructing a NuoDB handle");
        return 0;
    }
    return 1;
}

int pdo_nuodb_db_handle_last_id(pdo_nuodb_db_handle *H, const char *name) {
  int last_id = 0;
  PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
  if (db == NULL) return 0;
  try {
          last_id = db->getLastId(name);
  }
  catch (NuoDB::SQLException & e)
  {
          // Swallow it.
      return 0;
  }
  catch (...)
  {
          // Swallow it.
      return 0;
  }

  return last_id;
}

int pdo_nuodb_stmt_errno(pdo_nuodb_stmt *S) {
    if (S == NULL) return 0;
     if (S->stmt == NULL) return 0;
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    return nuodb_stmt->getEinfoErrcode();
}

const char *pdo_nuodb_stmt_errmsg(pdo_nuodb_stmt *S) {
    if (S == NULL) return 0;
    if (S->stmt == NULL) return 0;
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    return nuodb_stmt->getEinfoErrmsg();
}

pdo_error_type *pdo_nuodb_stmt_sqlstate(pdo_nuodb_stmt *S) {
    if (S == NULL) return 0;
    if (S->stmt == NULL) return 0;
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    return nuodb_stmt->getSqlstate();
}

int pdo_nuodb_stmt_delete(pdo_nuodb_stmt * S) {
    PdoNuoDbStatement *nuodb_stmt = NULL;
    if (S == NULL) {
       return 1;
    }
    if (S->stmt == NULL) {
       return 1;
    }
    nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    PdoNuoDbHandle *db = nuodb_stmt->getNuoDbHandle();
    try {
       delete nuodb_stmt;
       S->stmt = NULL;
    }
    catch (NuoDB::SQLException & e) {
        db->setEinfoErrcode(e.getSqlcode());
        db->setEinfoErrmsg(e.getText());
        db->setEinfoFile(__FILE__);
        db->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // _dbh->setSqlstate(e.getSQLState());
        db->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(db->getPdoDbh(), NULL, db->getEinfoFile(), db->getEinfoLine() /*TSRMLS_DC*/);
        return 0;
    }
    catch (...) {
        db->setEinfoErrcode(-17);
        db->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_delete()");
        db->setEinfoFile(__FILE__);
        db->setEinfoLine(__LINE__);
        db->setSqlstate("XX000");
        _pdo_nuodb_error(db->getPdoDbh(), NULL, db->getEinfoFile(), db->getEinfoLine() /*TSRMLS_DC*/);
        return 0;
    }
   return 1;
}

int pdo_nuodb_stmt_execute(pdo_nuodb_stmt * S, int *column_count, long *row_count) {
    pdo_nuodb_db_handle * H = (pdo_nuodb_db_handle *) S->H;
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    if (!H) {
        return 0;
    }
    unsigned long affected_rows = 0;
    if (!S->stmt) {
        return 0;
    }

    // TODO: check that (!stmt->executed) here?

    if (S->qty_input_params != 0)
    {
    	pdo_stmt_t *pdo_stmt = nuodb_stmt->getPdoStmt();
    	if ((pdo_stmt->bound_param_map == NULL) ||
    		(S->qty_input_params != pdo_stmt->bound_params->nNumOfElements))
    	{
    		nuodb_stmt->setEinfoErrcode(-12);
    		nuodb_stmt->setEinfoErrmsg("number of bound variables does not match number of tokens");
    		nuodb_stmt->setEinfoFile(__FILE__);
    		nuodb_stmt->setEinfoLine(__LINE__);
    		nuodb_stmt->setSqlstate("HY093");
    		_pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
    		return 0;
    	}
    }

    try
    {
        affected_rows = nuodb_stmt->execute();
        S->cursor_open = nuodb_stmt->hasResultSet();
        *column_count = nuodb_stmt->getColumnCount();
    }
    catch (NuoDB::SQLException & e)
    {
// Workaround DB-4327 for Drupal
        //if (e.getSqlcode() == -27) {
        //        return 0;
        //}
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
        return 0;
    }
    catch (...) {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_execute()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(H->pdo_dbh, nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
        return 0;
    }
    *row_count = affected_rows;

    S->exhausted = !S->cursor_open;

    return 1;
}

int pdo_nuodb_stmt_fetch(pdo_nuodb_stmt * S, long *row_count) {
    pdo_nuodb_db_handle * H = (pdo_nuodb_db_handle *)S->H;

    if (!S->exhausted)
    {
        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
        try {
            if (nuodb_stmt->next())
            {
                (*row_count)++;
                return 1;
            } else {
                S->exhausted = 1;
                return 0;
            }
        } catch (NuoDB::SQLException & e) {
                if (e.getSqlcode() == -5) return 0;
                nuodb_stmt->setEinfoErrcode(e.getSqlcode());
                nuodb_stmt->setEinfoErrmsg(e.getText());
                nuodb_stmt->setEinfoFile(__FILE__);
                nuodb_stmt->setEinfoLine(__LINE__);
                // Workaround DB-4112
                // pdo_stmt->setSqlstate(e.getSQLState());
                nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
                _pdo_nuodb_error(H->pdo_dbh, nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
        } catch (...) {
                nuodb_stmt->setEinfoErrcode(-17);
                nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_fetch()");
                nuodb_stmt->setEinfoFile(__FILE__);
                nuodb_stmt->setEinfoLine(__LINE__);
                nuodb_stmt->setSqlstate("XX000");
                _pdo_nuodb_error(H->pdo_dbh, nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
        }
    }
    S->exhausted = 1;
    return 0;

}

char const *pdo_nuodb_stmt_get_column_name(pdo_nuodb_stmt * S, int colno) {
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    char const * column_name = NULL;
    try {
        column_name = nuodb_stmt->getColumnName(colno);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
        return 0;
    } catch (...) {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_column_name()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
        return 0;
    }

    return column_name;
}

int pdo_nuodb_stmt_get_sql_type(pdo_nuodb_stmt * S, int colno) {
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    int sql_type = 0;
    try {
        sql_type = nuodb_stmt->getSqlType(colno);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
        return 0;
    } catch (...) {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_sql_type()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
        return 0;
    }
    return sql_type;
}

int pdo_nuodb_stmt_set_integer(pdo_nuodb_stmt *S, int paramno, long int_val)
{
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        nuodb_stmt->setInteger(paramno,  int_val);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
        return 0;
    }
    catch (...)
    {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_set_integer()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
        return 0;
    }
    return 1;
}

int pdo_nuodb_stmt_set_boolean(pdo_nuodb_stmt *S, int paramno, char bool_val)
{
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        nuodb_stmt->setBoolean(paramno,  (bool_val == 't') ? 1 : 0);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
        return 0;
    }
    catch (...)
    {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_set_boolean()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
        return 0;
    }
    return 1;
}

int pdo_nuodb_stmt_set_string(pdo_nuodb_stmt *S, int paramno, char *str_val)
{

    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        nuodb_stmt->setString(paramno,  str_val);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
        return 0;
    }
    catch (...)
    {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_set_string()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
        return 0;
    }
    return 1;
}

int pdo_nuodb_stmt_set_bytes(pdo_nuodb_stmt *S, int paramno, const void *val, int length)
{

    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        nuodb_stmt->setBytes(paramno, val, length);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
        return 0;
    }
    catch (...)
    {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_set_string()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
        return 0;
    }
    return 1;
}

int pdo_nuodb_stmt_set_blob(pdo_nuodb_stmt *S, int paramno, char *blob_val, int len)
{
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        nuodb_stmt->setBlob(paramno,  blob_val, len);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
        return 0;
    }
    catch (...)
    {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_set_blob()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
        return 0;
    }
    return 1;
}

int pdo_nuodb_stmt_set_clob(pdo_nuodb_stmt *S, int paramno, char *clob_val, int len)
{
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        nuodb_stmt->setClob(paramno,  clob_val, len);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
        return 0;
    }
    catch (...)
    {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_set_clob()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
        return 0;
    }
    return 1;
}

void pdo_nuodb_stmt_get_integer(pdo_nuodb_stmt *S, int colno, int **int_val)
{
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        nuodb_stmt->getInteger(colno, int_val);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
    }
    catch (...)
    {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_integer()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
    }
    return;
}

void pdo_nuodb_stmt_get_boolean(pdo_nuodb_stmt *S, int colno, char **bool_val)
{
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        nuodb_stmt->getBoolean(colno, bool_val);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
    }
    catch (...)
    {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_boolean()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
    }
    return;
}

void pdo_nuodb_stmt_get_long(pdo_nuodb_stmt *S, int colno, int64_t **long_val)
{
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        nuodb_stmt->getLong(colno, long_val);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
    }
    catch (...)
    {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_long()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
    }
    return;
}

const char *pdo_nuodb_stmt_get_string(pdo_nuodb_stmt *S, int colno)
{
    const char *res = NULL;
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        res = nuodb_stmt->getString(colno);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
        return NULL;
    }
    catch (...)
    {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_string()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
        return NULL;
    }
    return res;
}

void pdo_nuodb_stmt_get_date(pdo_nuodb_stmt *S, int colno, int64_t **date_val)
{
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        nuodb_stmt->getDate(colno, date_val);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
    }
    catch (...)
    {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_date()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
    }
    return;
}

void pdo_nuodb_stmt_get_time(pdo_nuodb_stmt *S, int colno, int64_t **time_val)
{
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        nuodb_stmt->getTime(colno, time_val);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
    }
    catch (...)
    {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_time()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
    }
    return;
}

const char *pdo_nuodb_stmt_get_timestamp(pdo_nuodb_stmt *S, int colno)
{
    const char *res = NULL;
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        res = nuodb_stmt->getTimestamp(colno);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
    }
    catch (...)
    {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_timestamp()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
    }
    return res;
}


void pdo_nuodb_stmt_get_blob(pdo_nuodb_stmt *S, int colno, char ** ptr, unsigned long * len,
                             void * (*erealloc)(void *ptr, size_t size, int allow_failure ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC))
{
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        nuodb_stmt->getBlob(colno, ptr, len, erealloc);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
    }
    catch (...)
    {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_blob()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
    }
    return;
}

void pdo_nuodb_stmt_get_clob(pdo_nuodb_stmt *S, int colno, char ** ptr, unsigned long * len,
                             void * (*erealloc)(void *ptr, size_t size, int allow_failure ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC))
{
    PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        nuodb_stmt->getClob(colno, ptr, len, erealloc);
    } catch (NuoDB::SQLException & e) {
        nuodb_stmt->setEinfoErrcode(e.getSqlcode());
        nuodb_stmt->setEinfoErrmsg(e.getText());
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        // Workaround DB-4112
        // pdo_stmt->setSqlstate(e.getSQLState());
        nuodb_stmt->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine() /*TSRMLS_DC*/);
    }
    catch (...)
    {
        nuodb_stmt->setEinfoErrcode(-17);
        nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_clob()");
        nuodb_stmt->setEinfoFile(__FILE__);
        nuodb_stmt->setEinfoLine(__LINE__);
        nuodb_stmt->setSqlstate("XX000");
        _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine()/* TSRMLS_DC*/);
    }
    return;
}

} // end of extern "C"
