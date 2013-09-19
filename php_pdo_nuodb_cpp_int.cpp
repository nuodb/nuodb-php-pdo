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

#ifdef _MSC_VER  // Visual Studio specific
#include <stdint.h>
#include <stdio.h>
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

#define PHP_DEBUG 1
#define TRUE 1
#define FALSE 0

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
  if (rs == NULL) return;
  NuoDB::ResultSetMetaData *rsmd = rs->getMetaData();
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

    nuodb_throw_zend_exception("IM001", 1, "getLastId sequence-name argument is not supported");
    return 0;

    for (int i=0; i<_qty; i++)
        if (!strcmp(_keys[i].columnName, seqName))
            return _keys[i].columnKeyValue;

    nuodb_throw_zend_exception("HY001", 1, "No generated ID for specified column name: %s", seqName);
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
    _opt_arr[0].option = (char const *) strdup(options->array[0].option);
    _opt_arr[0].extra = (void *) strdup((const char *)options->array[0].extra);
    _opt_arr[1].option = (char const *) strdup(options->array[1].option);
    _opt_arr[1].extra = (void *) strdup((const char *)options->array[1].extra);
    _opt_arr[2].option = (char const *) strdup(options->array[2].option);
    _opt_arr[2].extra = (void *) strdup((const char *)options->array[2].extra);
    _opt_arr[3].option = (char const *) strdup(options->array[3].option);
    _opt_arr[3].extra = (void *) strdup((const char *)options->array[3].extra);
}

NuoDB::Connection * PdoNuoDbHandle::createConnection()
{
    closeConnection();

    _con = NuoDB::Connection::create((const char *)_opts->array[0].extra,
                                    (const char *)_opts->array[1].extra,
                                    (const char *)_opts->array[2].extra,
                                    1,
                                    (const char *)_opts->array[3].option,
                                    (const char *)_opts->array[3].extra);


/*
    _con = NuoDB::Connection::createUtf8();
    NuoDB::Properties* properties = _con->allocProperties();

    properties->putValue("user", (const char *)_opts->array[1].extra);
    properties->putValue("password", (const char *)_opts->array[2].extra);
    properties->putValue((const char *)_opts->array[3].option, (const char *)_opts->array[3].extra);

    _con->openDatabase((const char *)_opts->array[0].extra, properties);
*/
    //TODO add properties
    return _con;
}

NuoDB::Connection * PdoNuoDbHandle::getConnection()
{
    return _con;
}

PdoNuoDbStatement * PdoNuoDbHandle::createStatement(pdo_stmt_t * pdo_stmt, char const * sql)
{
    PdoNuoDbStatement * rval = NULL;
    if (sql == NULL)
    {
        return NULL;
    }
    rval = new PdoNuoDbStatement(this, pdo_stmt);
    rval->createStatement(sql);
    setLastStatement(rval);
    return rval;
}

void PdoNuoDbHandle::closeConnection()
{
    if (_con == NULL)
    {
        return;
    }
    _con->close();
    _con = NULL;
}

void PdoNuoDbHandle::commit()
{
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
    if (_last_keys == NULL) {
                nuodb_throw_zend_exception("HY002", -40, "No generated keys");
                return 0;
    }

    if (name == NULL) return _last_keys->getIdValue();
    return _last_keys->getIdValue(name);
}

void PdoNuoDbHandle::setAutoCommit(bool autoCommit)
{
    _con->setAutoCommit(autoCommit);
    return;
}

const char *PdoNuoDbHandle::getNuoDBProductName()
{
    NuoDB::DatabaseMetaData *dbmd = _con->getMetaData();
    if (dbmd == NULL) return NULL;
    return dbmd->getDatabaseProductName();
}

const char *PdoNuoDbHandle::getNuoDBProductVersion()
{
    NuoDB::DatabaseMetaData *dbmd = _con->getMetaData();
    if (dbmd == NULL) return NULL;
    return dbmd->getDatabaseProductVersion();
}

PdoNuoDbStatement::PdoNuoDbStatement(PdoNuoDbHandle * dbh, pdo_stmt_t *pdo_stmt) : _dbh(dbh), _pdo_stmt(pdo_stmt), _sql(NULL), _stmt(NULL), _rs(NULL)
{
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
	if (einfo.errmsg) free(einfo.errmsg);
    _dbh->setLastStatement(NULL);
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
    if (sql == NULL)
    {
        return NULL;
    }
    _sql = strdup(sql);
    NuoDB::Connection * _con = NULL;
    _con = _dbh->getConnection();
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
    	_dbh->setEinfoErrcode(e.getSqlcode());
    	_dbh->setEinfoErrmsg(e.getText());
    	_dbh->setEinfoFile(__FILE__);
    	_dbh->setEinfoLine(__LINE__);
    	// Workaround DB-4112
    	// _dbh->setSqlstate(e.getSQLState());
    	_dbh->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
    	_pdo_nuodb_error(_dbh->getPdoDbh(), NULL, getEinfoFile(), getEinfoLine() TSRMLS_DC);
    } catch (...) {
        nuodb_throw_zend_exception("HY004", 0, "UNKNOWN ERROR caught in PdoNuoDbStatement::createStatement()");
    }

    return _stmt;
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
    PDO_DBG_INF_FMT("Elapsed time=%d (microsconds) : SQL=%s", elapsed, this->_sql);
    if (result == TRUE) {  // true means there was no UPDATE or INSERT
       _rs = _stmt->getResultSet();
    } else {
       update_count = _stmt->getUpdateCount();
       if (update_count != 0)
       {
         NuoDB::ResultSet *_rs_gen_keys = NULL;

         _rs_gen_keys = _stmt->getGeneratedKeys();
         if (_rs_gen_keys != NULL)
         {
           PdoNuoDbGeneratedKeys *keys = new PdoNuoDbGeneratedKeys();
           keys->setKeys(_rs_gen_keys);
           _dbh->setLastKeys(keys);
           _rs_gen_keys->close();
         }
       }
    }

    PDO_DBG_RETURN(update_count);
}

void PdoNuoDbStatement::executeQuery()
{
    if (_stmt == NULL)
    {
        return;
    }
    _rs = _stmt->executeQuery();
}

bool PdoNuoDbStatement::hasResultSet()
{
    return (_rs != NULL);
}

bool PdoNuoDbStatement::next()
{
    if (_rs == NULL)
    {
        return false;
    }
    return _rs->next();
}

size_t PdoNuoDbStatement::getColumnCount()
{
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
    	// TODO: need to write a test case for this.
    	setEinfoErrcode(e.getSqlcode());
    	setEinfoErrmsg(e.getText());
    	setEinfoFile(__FILE__);
    	setEinfoLine(__LINE__);
    	// Workaround DB-4112
    	// _dbh->setSqlstate(e.getSQLState());
    	setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
    	_pdo_nuodb_error(_dbh->getPdoDbh(), _pdo_stmt, getEinfoFile(), getEinfoLine() TSRMLS_DC);


//        int error_code = e.getSqlcode();
//        const char *error_text = e.getText();
//        nuodb_throw_zend_exception("HY005", error_code, error_text);

    } catch (...) {
        nuodb_throw_zend_exception("HY006", 0, "UNKNOWN ERROR caught in doNuoDbStatement::getColumnName()");
    }

    return rval;
}

int PdoNuoDbStatement::getSqlType(size_t column)
{
    PDO_DBG_ENTER("PdoNuoDbStatement::getSqlType");
    if (_rs == NULL)
    {
        PDO_DBG_RETURN(0);
    }
    NuoDB::ResultSetMetaData * md = _rs->getMetaData();
    int sqlType = md->getColumnType(column+1);
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
    PDO_DBG_RETURN(0);
}

char const * PdoNuoDbStatement::getString(size_t column)
{
    char const *res = NULL;

    if (_rs == NULL)
    {
        return res;
    }
    res =  _rs->getString(column+1);
    return res;
}

int PdoNuoDbStatement::getInteger(size_t column)
{
    if (_rs == NULL)
    {
        return 0;
    }
    return _rs->getInt(column+1);
}

bool PdoNuoDbStatement::getBoolean(size_t column)
{
    if (_rs == NULL)
    {
        return 0;
    }
    return _rs->getBoolean(column+1);
}

int64_t PdoNuoDbStatement::getLong(size_t column)
{
    if (_rs == NULL)
    {
        return 0;
    }
    return _rs->getLong(column+1);
}

/*
unsigned long PdoNuoDbStatement::getTimestamp(size_t column)
{
    if (_rs == NULL)
    {
        return 0;
    }
    NuoDB::Timestamp *ts = _rs->getTimestamp(column+1);
    return ts->getSeconds();
}
*/
char const *PdoNuoDbStatement::getTimestamp(size_t column)
{
    char const *res = NULL;

    if (_rs == NULL)
    {
        return res;
    }
    res =  _rs->getString(column+1);
    return res;
}

unsigned long PdoNuoDbStatement::getTime(size_t column)
{
    if (_rs == NULL)
    {
        return 0;
    }
    NuoDB::Time *time = _rs->getTime(column+1);
    return time->getSeconds();
}

unsigned long PdoNuoDbStatement::getDate(size_t column)
{
    if (_rs == NULL)
    {
        return 0;
    }
    NuoDB::Date *date = _rs->getDate(column+1);
    return date->getSeconds();
}

void PdoNuoDbStatement::getBlob(size_t column, char ** ptr, unsigned long * len, void * (*erealloc)(void *ptr, size_t size, int, char *, unsigned int, char *, unsigned int))
{
    if (_rs == NULL)
    {
        return;
    }
    NuoDB::Blob *blob = _rs->getBlob(column+1);
    *len = blob->length();
    if ((*len) == 0) {
        *ptr = NULL;
    } else {
        *ptr = (char *)(*erealloc)((void *)*ptr, *len+1, 0, __FILE__, __LINE__, NULL, 0);
        blob->getBytes(0, *len, (unsigned char *)*ptr);
        (*ptr)[*len] = '\0';
    }
    return;
}

void PdoNuoDbStatement::getClob(size_t column, char ** ptr, unsigned long * len, void * (*erealloc)(void *ptr, size_t size, int, char *, unsigned int, char *, unsigned int))
{
    if (_rs == NULL)
    {
        return;
    }
    NuoDB::Clob *clob = _rs->getClob(column+1);
    *len = clob->length();
    if ((*len) == 0) {
        *ptr = NULL;
    } else {
        *ptr = (char *)(*erealloc)((void *)*ptr, *len+1, 0, __FILE__, __LINE__, NULL, 0);
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
    if (_stmt == NULL) {
        return;
    }
    _stmt->setInt(index+1, value);
    return;
}

void PdoNuoDbStatement::setBoolean(size_t index, bool value)
{
    if (_stmt == NULL) {
        return;
    }
    _stmt->setBoolean(index+1, value);
    return;
}

void PdoNuoDbStatement::setString(size_t index, const char *value)
{
    if (_stmt == NULL) {
        return;
    }
    _stmt->setString(index+1, value);
    return;
}

void PdoNuoDbStatement::setBlob(size_t index, const char *value, int len)
{
    if (_stmt == NULL) {
        return;
    }
    NuoDB::Blob *blob = _dbh->getConnection()->createBlob();
    if (value != NULL)
        blob->setBytes(len, (const unsigned char *)value);
    _stmt->setBlob(index+1, blob);
    return;
}

void PdoNuoDbStatement::setClob(size_t index, const char *value, int len)
{
    if (_stmt == NULL) {
        return;
    }
    NuoDB::Clob *clob = _dbh->getConnection()->createClob();
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
    	// TODO: need to write a test case for this.
    	db->setEinfoErrcode(e.getSqlcode());
    	db->setEinfoErrmsg(e.getText());
    	db->setEinfoFile(__FILE__);
    	db->setEinfoLine(__LINE__);
    	// Workaround DB-4112
    	// _dbh->setSqlstate(e.getSQLState());
    	db->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
    	_pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine() TSRMLS_DC);


        //int error_code = e.getSqlcode();
        //const char *error_text = e.getText();
        //pdo_nuodb_db_handle_set_last_app_error(H, error_text);
        // TODO: Optionally throw an error depending on PDO::ATTR_ERRMODE
        return 0;
    }
    catch (...)
    {
        pdo_nuodb_db_handle_set_last_app_error(H, "UNKNOWN ERROR in pdo_nuodb_db_handle_doer()");
        // TODO: Optionally throw an error depending on PDO::ATTR_ERRMODE
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
    	// TODO: need to write a test case for this.
    	db->setEinfoErrcode(e.getSqlcode());
    	db->setEinfoErrmsg(e.getText());
    	db->setEinfoFile(__FILE__);
    	db->setEinfoLine(__LINE__);
    	// Workaround DB-4112
    	// _dbh->setSqlstate(e.getSQLState());
    	db->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
    	_pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine() TSRMLS_DC);

        return rval;
    }
    catch (...)
    {
        pdo_nuodb_db_handle_set_last_app_error(H, "UNKNOWN ERROR in pdo_nuodb_db_handle_get_nuodb_product_name()");
        // TODO: Optionally throw an error depending on PDO::ATTR_ERRMODE
        return rval;
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
    	_pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine() TSRMLS_DC);

        return rval;
    }
    catch (...)
    {
        pdo_nuodb_db_handle_set_last_app_error(H, "UNKNOWN ERROR in pdo_nuodb_db_handle_get_nuodb_product_version()");
        // TODO: Optionally throw an error depending on PDO::ATTR_ERRMODE
        return rval;
    }
    return rval;
}


void *pdo_nuodb_db_handle_create_statement(pdo_nuodb_db_handle * H, pdo_stmt_t * pdo_stmt, const char * sql)
{
	PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
    PdoNuoDbStatement *stmt = NULL;
    try
    {
        stmt = db->createStatement(pdo_stmt, sql);
    }
    catch (...)
    {
        stmt = NULL; // TODO - save the error message text.
    }
    return (void *)stmt;
}

long pdo_nuodb_db_handle_doer(pdo_nuodb_db_handle * H, void *dbh_opaque, const char * sql, unsigned in_txn, unsigned auto_commit, void (*pt2pdo_dbh_t_set_in_txn)(void *dbh_opaque, unsigned in_txn))
{
	PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
    unsigned in_txn_state = in_txn;
    long res;
    try
    {
        PdoNuoDbStatement * stmt = (PdoNuoDbStatement *) pdo_nuodb_db_handle_create_statement(H, NULL, sql);
        (*pt2pdo_dbh_t_set_in_txn)(dbh_opaque, 1);
        res = stmt->execute();
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
    	_pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine() TSRMLS_DC);

        (*pt2pdo_dbh_t_set_in_txn)(dbh_opaque, in_txn_state);
        return -1;
    }
    catch (...)
    {
        pdo_nuodb_db_handle_set_last_app_error(H, "UNKNOWN ERROR in pdo_nuodb_db_handle_doer()");
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
    	// TODO: need to write a test case for this.
    	db->setEinfoErrcode(e.getSqlcode());
    	db->setEinfoErrmsg(e.getText());
    	db->setEinfoFile(__FILE__);
    	db->setEinfoLine(__LINE__);
    	// Workaround DB-4112
    	// _dbh->setSqlstate(e.getSQLState());
    	db->setSqlstate(nuodb_get_sqlstate(e.getSqlcode()));
    	_pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine() TSRMLS_DC);
        return 0;
    } catch (...) {
        *errMessage = strdup("Error constructing a NuoDB handle");
        return 0;
    }
    return 1;
}

void pdo_nuodb_db_handle_set_last_app_error(pdo_nuodb_db_handle *H, const char *err_msg) {
        H->last_app_error = err_msg;
}

int pdo_nuodb_db_handle_last_id(pdo_nuodb_db_handle *H, const char *name) {
  int last_id = 0;
  PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
  if (db == NULL) return 0;
  last_id = db->getLastId(name);
  return last_id;
}

int pdo_nuodb_stmt_errno(pdo_nuodb_stmt *S) {
	if (S == NULL) return 0;
	if (S->stmt == NULL) return 0;
    PdoNuoDbStatement *pdo_nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    return pdo_nuodb_stmt->getEinfoErrcode();
}

const char *pdo_nuodb_stmt_errmsg(pdo_nuodb_stmt *S) {
	if (S == NULL) return 0;
	if (S->stmt == NULL) return 0;
    PdoNuoDbStatement *pdo_nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    return pdo_nuodb_stmt->getEinfoErrmsg();
}

pdo_error_type *pdo_nuodb_stmt_sqlstate(pdo_nuodb_stmt *S) {
	if (S == NULL) return 0;
	if (S->stmt == NULL) return 0;
    PdoNuoDbStatement *pdo_nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
    return pdo_nuodb_stmt->getSqlstate();
}

int pdo_nuodb_stmt_delete(pdo_nuodb_stmt * S) {
        try {
                if (S == NULL) {
                        return 1;
                }
                if (S->stmt == NULL) {
                        return 1;
                }
                PdoNuoDbStatement *pdo_nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
                delete pdo_nuodb_stmt;
                S->stmt = NULL;

    }
    catch (NuoDB::SQLException & e) {
        S->error_code = e.getSqlcode();
        const char *error_text = e.getText();
        S->error_msg = strdup(error_text);
        return 0;
    }
    catch (...) {
        S->error_code = 0;
        S->error_msg = strdup("UNKNOWN ERROR");
        return 0;
    }
   return 1;
}

int pdo_nuodb_stmt_execute(pdo_nuodb_stmt * S, int *column_count, long *row_count) {
    pdo_nuodb_db_handle * H = (pdo_nuodb_db_handle *) S->H;
    if (!H) {
        return 0;
    }
    unsigned long affected_rows = 0;
    if (!S->stmt) {
        return 0;
    }

    // TODO: check that (!stmt->executed) here?

    try
    {
        PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
        affected_rows = pdo_stmt->execute();
        S->cursor_open = pdo_stmt->hasResultSet();
        *column_count = pdo_stmt->getColumnCount();
    }
    catch (NuoDB::SQLException & e)
    {
        S->error_code = e.getSqlcode();
        const char *error_text = e.getText();
        S->error_msg = strdup(error_text);
        return 0;
    }
    catch (...) {
        S->error_code = 0;
        S->error_msg = strdup("UNKNOWN ERROR");
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
        PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
        try {
            if (pdo_stmt->next())
            {
                (*row_count)++;
                return 1;
            } else {
                S->exhausted = 1;
                return 0;
            }
        } catch (NuoDB::SQLException & e) {
            int error_code = e.getSqlcode();
            const char *error_text = e.getText();
            pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
            //nuodb_throw_zend_exception("HY008", error_code, error_text);
        } catch (...) {
            const char *error_text = "UNKNOWN ERROR in pdo_nuodb_stmt_get_integer()";
            pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
            nuodb_throw_zend_exception("HY009", 0, error_text);
        }
    }
    S->exhausted = 1;
    return 0;

}

char const *pdo_nuodb_stmt_get_column_name(pdo_nuodb_stmt * S, int colno) {
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    char const * column_name = pdo_stmt->getColumnName(colno);
    return column_name;
}

int pdo_nuodb_stmt_get_sql_type(pdo_nuodb_stmt * S, int colno) {
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    int sql_type = pdo_stmt->getSqlType(colno);
    return sql_type;
}

int pdo_nuodb_stmt_set_integer(pdo_nuodb_stmt *S, int paramno, long int_val)
{
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    pdo_stmt->setInteger(paramno,  int_val);
    return 1;
}

int pdo_nuodb_stmt_set_boolean(pdo_nuodb_stmt *S, int paramno, char bool_val)
{
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    pdo_stmt->setBoolean(paramno,  (bool_val == 't') ? 1 : 0);
    return 1;
}

int pdo_nuodb_stmt_set_string(pdo_nuodb_stmt *S, int paramno, char *str_val)
{
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    pdo_stmt->setString(paramno,  str_val);
    return 1;
}

int pdo_nuodb_stmt_set_blob(pdo_nuodb_stmt *S, int paramno, char *blob_val, int len)
{
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    pdo_stmt->setBlob(paramno,  blob_val, len);
    return 1;
}

int pdo_nuodb_stmt_set_clob(pdo_nuodb_stmt *S, int paramno, char *clob_val, int len)
{
        PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
        pdo_stmt->setClob(paramno,  clob_val, len);
        return 1;
}

int pdo_nuodb_stmt_get_integer(pdo_nuodb_stmt *S, int colno)
{
    int res = 0;
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        res = pdo_stmt->getInteger(colno);
    } catch (NuoDB::SQLException & e) {
        int error_code = e.getSqlcode();
        const char *error_text = e.getText();
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY010", error_code, error_text);
    }
    catch (...)
    {
        const char *error_text = "UNKNOWN ERROR in pdo_nuodb_stmt_get_integer()";
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY011", 0, error_text);
    }
    return res;
}

char pdo_nuodb_stmt_get_boolean(pdo_nuodb_stmt *S, int colno)
{
    char res = 0;
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        res = pdo_stmt->getBoolean(colno);
    } catch (NuoDB::SQLException & e) {
        int error_code = e.getSqlcode();
        const char *error_text = e.getText();
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY012", error_code, error_text);
    }
    catch (...)
    {
        const char *error_text = "UNKNOWN ERROR in pdo_nuodb_stmt_get_boolean()";
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY013", 0, error_text);
    }
    return res;
}

int64_t pdo_nuodb_stmt_get_long(pdo_nuodb_stmt *S, int colno)
{
    int64_t res = 0;
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        res = pdo_stmt->getLong(colno);
    } catch (NuoDB::SQLException & e) {
        int error_code = e.getSqlcode();
        const char *error_text = e.getText();
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY014", error_code, error_text);
    }
    catch (...)
    {
        const char *error_text = "UNKNOWN ERROR in pdo_nuodb_stmt_get_long()";
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY015", 0, error_text);
    }
    return res;
}

const char *pdo_nuodb_stmt_get_string(pdo_nuodb_stmt *S, int colno)
{
    const char *res = NULL;
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        res = pdo_stmt->getString(colno);
    } catch (NuoDB::SQLException & e) {
        int error_code = e.getSqlcode();
        const char *error_text = e.getText();
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY016", error_code, error_text);
    }
    catch (...)
    {
        const char *error_text = "UNKNOWN ERROR in pdo_nuodb_stmt_get_string()";
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY017", 0, error_text);
    }
    return res;
}

unsigned long pdo_nuodb_stmt_get_date(pdo_nuodb_stmt *S, int colno)
{
    unsigned long res = 0;
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        res = pdo_stmt->getDate(colno);
    } catch (NuoDB::SQLException & e) {
        int error_code = e.getSqlcode();
        const char *error_text = e.getText();
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY018", error_code, error_text);
    }
    catch (...)
    {
        const char *error_text = "UNKNOWN ERROR in pdo_nuodb_stmt_get_date()";
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY019", 0, error_text);
    }
    return res;
}

unsigned long pdo_nuodb_stmt_get_time(pdo_nuodb_stmt *S, int colno)
{
    unsigned long res = 0;
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        res = pdo_stmt->getTime(colno);
    } catch (NuoDB::SQLException & e) {
        int error_code = e.getSqlcode();
        const char *error_text = e.getText();
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY020", error_code, error_text);
    }
    catch (...)
    {
        const char *error_text = "UNKNOWN ERROR in pdo_nuodb_stmt_get_time()";
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY021", 0, error_text);
    }
    return res;
}

/*
unsigned long pdo_nuodb_stmt_get_timestamp(pdo_nuodb_stmt *S, int colno)
{
    unsigned long res = 0;
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        res = pdo_stmt->getTimestamp(colno);
    } catch (NuoDB::SQLException & e) {
        int error_code = e.getSqlcode();
        const char *error_text = e.getText();
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY022", error_code, error_text);
    }
    catch (...)
    {
        const char *error_text = "UNKNOWN ERROR in pdo_nuodb_stmt_get_timestamp()";
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY023", 0, error_text);
    }
    return res;
}
*/

const char *pdo_nuodb_stmt_get_timestamp(pdo_nuodb_stmt *S, int colno)
{
    const char *res = NULL;
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        res = pdo_stmt->getTimestamp(colno);
    } catch (NuoDB::SQLException & e) {
        int error_code = e.getSqlcode();
        const char *error_text = e.getText();
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY024", error_code, error_text);
    }
    catch (...)
    {
        const char *error_text = "UNKNOWN ERROR in pdo_nuodb_stmt_get_timestamp()";
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY025", 0, error_text);
    }
    return res;
}


void pdo_nuodb_stmt_get_blob(pdo_nuodb_stmt *S, int colno, char ** ptr, unsigned long * len,
                             void * (*erealloc)(void *ptr, size_t size, int, char *, unsigned int, char *, unsigned int))
{
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        pdo_stmt->getBlob(colno, ptr, len, erealloc);
    } catch (NuoDB::SQLException & e) {
        int error_code = e.getSqlcode();
        const char *error_text = e.getText();
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY026", error_code, error_text);
    }
    catch (...)
    {
        const char *error_text = "UNKNOWN ERROR in pdo_nuodb_stmt_get_blob()";
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY027", 0, error_text);
    }
    return;
}

void pdo_nuodb_stmt_get_clob(pdo_nuodb_stmt *S, int colno, char ** ptr, unsigned long * len,
                             void * (*erealloc)(void *ptr, size_t size, int, char *, unsigned int, char *, unsigned int))
{
    PdoNuoDbStatement *pdo_stmt = (PdoNuoDbStatement *) S->stmt;
    try {
        pdo_stmt->getClob(colno, ptr, len, erealloc);
    } catch (NuoDB::SQLException & e) {
        int error_code = e.getSqlcode();
        const char *error_text = e.getText();
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY028", error_code, error_text);
    }
    catch (...)
    {
        const char *error_text = "UNKNOWN ERROR in pdo_nuodb_stmt_get_clob()";
        pdo_nuodb_db_handle_set_last_app_error((pdo_nuodb_db_handle *)S->H, error_text);
        nuodb_throw_zend_exception("HY029", 0, error_text);
    }
    return;
}

} // end of extern "C"
