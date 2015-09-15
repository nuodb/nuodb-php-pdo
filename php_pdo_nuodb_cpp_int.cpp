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

#include <nuodb/NuoDB.h>


}

#ifdef _MSC_VER  /* Visual Studio specific */
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

#include "php_pdo_nuodb_c_cpp_common.h"
#include "php_pdo_nuodb_cpp_int.h"

PdoNuoDbGeneratedKeys::PdoNuoDbGeneratedKeys()
    : _qty(0), _keys(NULL)
{
    /* empty */
}


PdoNuoDbGeneratedKeys::~PdoNuoDbGeneratedKeys()
{
    if (_keys == NULL) {
        return;
    }

    for (int i=0; i < _qty; i++) {
        if (_keys[i].columnName != NULL) {
            delete _keys[i].columnName;
        }
    }
    delete [] _keys;
}


void PdoNuoDbGeneratedKeys::setKeys(NuoDB_ResultSet *rs)
{
    NuoDB_ResultSetMetaData *rsmd = NULL;

    if (rs == NULL) {
        return;
    }

    try {
        rsmd = rs->getMetaData(rs);
        if (rsmd == NULL) {
            return;
        }
        int col_cnt = rsmd->getColumnCount(rsmd);
        if (col_cnt < 1) {
            return;
        }
        _qty = col_cnt;
        _keys = new PdoNuoDbGeneratedKeyElement [col_cnt];
        rs->next(rs);

        const char *col_name;
        for (int i=1; i <= col_cnt; i++) {
            col_name = rsmd->getColumnName(rsmd, i);
            _keys[i-1].columnName = strdup(col_name);
            _keys[i-1].columnIndex = i;
            rs->getInt(rs, i, &(_keys[i-1].columnKeyValue));
        }
    } catch (...) {
        return;
    }
}


int PdoNuoDbGeneratedKeys::getIdValue()
{
    if (_qty == 0) { /* No generated keys */
        return 0;
    }

    if (_qty > 1) { /* Multiple generated keys */
        return 0;
    }

    return _keys[0].columnKeyValue;
}


int PdoNuoDbGeneratedKeys::getIdValue(const char *seqName)
{
    /*
     * We currently do not support tables with multiple generated keys
     * so we will throw a "not supported" exception. However, below
     * is a reasonable implementation for multiple keys.  Assuming
     * seqName is a column name.
     */

    for (int i=0; i<_qty; i++) {
        if (!strcmp(_keys[i].columnName, seqName)) {
            return _keys[i].columnKeyValue;
        }
    }

    return 0;
}


PdoNuoDbHandle::PdoNuoDbHandle(pdo_dbh_t *pdo_dbh, SqlOptionArray * options)
    : _pdo_dbh(pdo_dbh), _driverMajorVersion(0), _driverMinorVersion(0),
      _con(NULL), _txn_isolation_level(PDO_NUODB_TXN_CONSISTENT_READ), _opts(NULL), _last_stmt(NULL), _last_keys(NULL)
{
    einfo.errcode = 0;
    einfo.errmsg = NULL;
    einfo.file = NULL;
    einfo.line = 0;

    strcpy(sqlstate, PDO_ERR_NONE);
    for (int i=0; i<PDO_NUODB_OPTIONS_ARR_SIZE; i++) {
        _opt_arr[i].option = NULL;
        _opt_arr[i].extra = NULL;
    }
    setOptions(options);
}


PdoNuoDbHandle::~PdoNuoDbHandle()
{
    if (_last_keys != NULL) {
        delete _last_keys;
    }

    if (einfo.errmsg) {
        free(einfo.errmsg);
    }

    closeConnection();
    deleteOptions();
}


_pdo_dbh_t *PdoNuoDbHandle::getPdoDbh() {
    return _pdo_dbh;
}

int PdoNuoDbHandle::getDriverMajorVersion() {
    return _driverMajorVersion;
}

int PdoNuoDbHandle::getDriverMinorVersion() {
    return _driverMinorVersion;
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

void PdoNuoDbHandle::setTransactionIsolation(int level) {
    _txn_isolation_level = level;
    if (_con != NULL) {
        _con->setTransactionIsolation(_con, (NuoDB_TransactionIsolationLevel)level);
    }
}

int PdoNuoDbHandle::getTransactionIsolation() {
        int status = 0;
    if (_con != NULL) {
        status = _con->getTransactionIsolation(_con, (NuoDB_TransactionIsolationLevel*)&_txn_isolation_level);
    }
    if (status) {
        NuoDB_Error *e = NuoDB_Error_getLastErrorInfo();
        const char *sqlState = e->getSqlState(e);
        setEinfoErrcode(e->getCode(e));
        setEinfoErrmsg(e->getText(e));
        setEinfoFile(__FILE__);
        setEinfoLine(__LINE__);
        if ((sqlState == NULL) || (sqlState[0] == '\0'))
        {
            setSqlstate("XX000");
        } else {
            setSqlstate(sqlState);
        }
        _pdo_nuodb_error(getPdoDbh(), NULL, getEinfoFile(), getEinfoLine());
        return 0;
    }
    return _txn_isolation_level;
}

void PdoNuoDbHandle::setEinfoLine(int line) {
    einfo.line = line;
}


void PdoNuoDbHandle::setEinfoFile(const char *file) {
    if (einfo.file) {
        free(einfo.file);
    }

    einfo.file = strdup(file);
}


void PdoNuoDbHandle::setEinfoErrcode(int errcode) {
    einfo.errcode = errcode;
}


void PdoNuoDbHandle::setEinfoErrmsg(const char *errmsg) {
        if (errmsg == NULL) {
                return;
        }
    if (einfo.errmsg) {
        free(einfo.errmsg);
    }

    einfo.errmsg = strdup(errmsg);
}


void PdoNuoDbHandle::setSqlstate(const char *sqlState) {
        if (sqlState == NULL) {
                this->sqlstate[0] = 0;
                return;
        }
    strncpy(this->sqlstate, sqlState, PDO_NUODB_SQLSTATE_LEN);
}


void PdoNuoDbHandle::deleteOptions()
{
    if (_opts == NULL) {
        return;
    }
    for (int i=0; i<PDO_NUODB_OPTIONS_ARR_SIZE; i++)
    {
        if (_opt_arr[i].option != NULL) {
            free((void *)_opt_arr[i].option);
            _opt_arr[i].option = NULL;
        }
        if (_opt_arr[i].extra != NULL) {
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
    if (_last_keys != NULL) {
        delete _last_keys;
    }

    _last_keys = lastKeys;
}


void PdoNuoDbHandle::setOptions(SqlOptionArray * options)
{
    deleteOptions();
    _opts = new SqlOptionArray;
    _opts->count = PDO_NUODB_OPTIONS_ARR_SIZE;
    _opts->array = _opt_arr;
    for (int i=0; i<_opts->count; i++) {
        _opt_arr[i].option = (char const *) (options->array[i].option ? strdup(options->array[i].option) : NULL );
        _opt_arr[i].extra = (void *) (options->array[i].extra ? strdup((const char *)options->array[i].extra) : NULL);
    }
}


NuoDB_Connection * PdoNuoDbHandle::createConnection()
{
    int status = 0;
    NuoDB_Options *options = NULL;

    /* note: caller will catch exceptions. */
    closeConnection();
    _con = NuoDB_Connection_CreateUtf8();
    options = NuoDB_Options_Create();
    options->add(options,(const char *)_opts->array[3].option, (const char *)_opts->array[3].extra);
    status = _con->openDatabase(_con,
                       (const char *)_opts->array[0].extra, /* connection string */
                       (const char *)_opts->array[1].extra, /* username */
                       (const char *)_opts->array[2].extra, /* password */
                       options);
    NuoDB_Options_Free(options);
    if (status != 0) {
        NuoDB_Connection_Free(_con);
        _con = NULL;
        NuoDB_Error *e = NuoDB_Error_getLastErrorInfo();
        const char *sqlState = e->getSqlState(e);
        setEinfoErrcode(e->getCode(e));
        setEinfoErrmsg(e->getText(e));
        setEinfoFile(__FILE__);
        setEinfoLine(__LINE__);
        if ((sqlState == NULL) || (sqlState[0] == '\0'))
        {
            setSqlstate("XX000");
        } else {
            setSqlstate(sqlState);
        }
        _pdo_nuodb_error(getPdoDbh(), NULL, getEinfoFile(), getEinfoLine());
        return NULL;
    }

    if (_txn_isolation_level != PDO_NUODB_TXN_CONSISTENT_READ) {
        _con->setTransactionIsolation(_con, (NuoDB_TransactionIsolationLevel)_txn_isolation_level);
    }

    /* Get NuoDB Client major and minor version numbers */
    NuoDB_DatabaseMetaData *dbmd = _con->getMetaData(_con);
    if (dbmd != NULL) {
      _driverMajorVersion = dbmd->getDriverMajorVersion(dbmd);
      _driverMinorVersion = dbmd->getDriverMinorVersion(dbmd);
    }
    return _con;
}


NuoDB_Connection * PdoNuoDbHandle::getConnection()
{
    return _con;
}


PdoNuoDbStatement * PdoNuoDbHandle::createStatement(pdo_stmt_t * pdo_stmt, char const * sql)
{
    /* NOTE: caller catches exceptions. */
    PdoNuoDbStatement * nuodb_stmt = NULL;

    if (sql == NULL) {
        return NULL;
    }

    nuodb_stmt = new PdoNuoDbStatement(this, pdo_stmt);
    nuodb_stmt->createStatement(sql);
    setLastStatement(nuodb_stmt);
    return nuodb_stmt;
}


void PdoNuoDbHandle::closeConnection()
{
    /* NOTE: caller catches exceptions. */
    if (_con == NULL) {
        return;
    }

    NuoDB_Connection_Free(_con);
    _con = NULL;
}

void PdoNuoDbHandle::commit()
{
    /* NOTE: caller catches exceptions. */
    PDO_DBG_ENTER("PdoNuoDbHandle::commit", _pdo_dbh);
    if (_con == NULL) {
        PDO_DBG_VOID_RETURN(_pdo_dbh);
    }
    _con->commit(_con);
    PDO_DBG_VOID_RETURN(_pdo_dbh);
}

void PdoNuoDbHandle::rollback()
{
    /* NOTE: caller catches exceptions. */
    PDO_DBG_ENTER("PdoNuoDbHandle::rollback", _pdo_dbh);
    if (_con == NULL) {
        PDO_DBG_VOID_RETURN(_pdo_dbh);
    }
    _con->rollback(_con);
    PDO_DBG_VOID_RETURN(_pdo_dbh);
}

int PdoNuoDbHandle::getLastId(const char *name)
{
    /* NOTE: caller catches exceptions. */
    if (_last_keys == NULL) {
        setEinfoErrcode(-40);
        setEinfoErrmsg("No generated keys");
        setEinfoFile(__FILE__);
        setEinfoLine(__LINE__);
        setSqlstate("P0001");
        _pdo_nuodb_error(_pdo_dbh, NULL, getEinfoFile(),
                         getEinfoLine());
        return 0;
    }

    if (name == NULL) {
        return _last_keys->getIdValue();
    }

    /*
     * We would normally do the following:
     *
     *   return _last_keys->getIdValue(name);
     *
     * but NuoDB does not currently support getting IDs by name,
     * so instead we must signal an error as follows:
     */
    setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
    setEinfoErrmsg("Unknown Error in PdoNuoDbHandle::getLastId");
    setEinfoFile(__FILE__);
    setEinfoLine(__LINE__);
    setSqlstate("XX000");
    _pdo_nuodb_error(getPdoDbh(), NULL, getEinfoFile(), getEinfoLine());
    return 0;
}


void PdoNuoDbHandle::setAutoCommit(bool autoCommit)
{
    /* NOTE: caller catches exceptions. */
    _con->setAutoCommit(_con, autoCommit);
    return;
}


const char *PdoNuoDbHandle::getNuoDBProductName()
{
    /* NOTE: caller catches exceptions. */
    NuoDB_DatabaseMetaData *dbmd = _con->getMetaData(_con);

    if (dbmd == NULL) {
        return NULL;
    }

    return dbmd->getDatabaseProductName(dbmd);
}


const char *PdoNuoDbHandle::getNuoDBProductVersion()
{
    /* NOTE: caller catches exceptions. */
    NuoDB_DatabaseMetaData *dbmd = _con->getMetaData(_con);

    if (dbmd == NULL) {
        return NULL;
    }

    return dbmd->getDatabaseProductVersion(dbmd);
}


PdoNuoDbStatement::PdoNuoDbStatement(PdoNuoDbHandle * dbh, pdo_stmt_t *pdo_stmt) : _nuodbh(dbh), _pdo_stmt(pdo_stmt), _sql(NULL), _stmt(NULL), _rs(NULL)
{
    /* NOTE: caller catches exceptions. */
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
    /* NOTE: caller catches exceptions. */
    if (einfo.errmsg) {
        free(einfo.errmsg);
    }

    _nuodbh->setLastStatement(NULL);

    if (_sql != NULL) {
        free((void *)_sql);
    }
    _sql = NULL;

    if (_rs != NULL) {
        NuoDB_ResultSet_Free(_rs);
    }
    _rs = NULL;

    if (_stmt != NULL) {
        NuoDB_Statement_Free(_stmt);
    }
    _stmt = NULL;
}


NuoDB_Statement * PdoNuoDbStatement::createStatement(char const * sql)
{
        int status = 0;
    /* NOTE: caller catches exceptions. */
    if (sql == NULL) {
        return NULL;
    }
    _sql = strdup(sql);

    NuoDB_Connection * _con = NULL;
    _con = _nuodbh->getConnection();
    if (_con == NULL) {
        return NULL;
    }

    _stmt = NULL;
    try {
        _stmt = NuoDB_Statement_Create(_con);
        if (_stmt == NULL) {
                NuoDB_Error *e = NuoDB_Error_getLastErrorInfo();
                status = e->getCode(e);
        } else {
                status = _stmt->prepare(_stmt, sql, NUODB_AUTOGENERATEDKEYS);
        }
        if (status != 0) {
                NuoDB_Error *e = NuoDB_Error_getLastErrorInfo();
                _nuodbh->setEinfoErrcode(e->getCode(e));
                _nuodbh->setEinfoErrmsg(e->getText(e));
                _nuodbh->setEinfoFile(__FILE__);
                _nuodbh->setEinfoLine(__LINE__);
                /* Workaround DB-4112 */
                _nuodbh->setSqlstate(nuodb_get_sqlstate(e->getCode(e)));
                _pdo_nuodb_error(_nuodbh->getPdoDbh(), NULL, _nuodbh->getEinfoFile(), _nuodbh->getEinfoLine());
                NuoDB_Statement_Free(_stmt);
                _stmt = NULL;
        }
    } catch (...) {
        _nuodbh->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
        _nuodbh->setEinfoErrmsg("Unknown Error in PdoNuoDbStatement::createStatement");
        _nuodbh->setEinfoFile(__FILE__);
        _nuodbh->setEinfoLine(__LINE__);
        _nuodbh->setSqlstate("XX000");
        _pdo_nuodb_error(_nuodbh->getPdoDbh(), NULL, _nuodbh->getEinfoFile(), _nuodbh->getEinfoLine());
        NuoDB_Statement_Free(_stmt);
        _stmt = NULL;
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
    if (einfo.errmsg) {
        free(einfo.errmsg);
    }
    einfo.errmsg = strdup(errmsg);
}


void PdoNuoDbStatement::setSqlstate(const char *sqlState) {
        if (sqlState == NULL) {
                return;
        }
    strncpy(this->sqlstate, sqlState, PDO_NUODB_SQLSTATE_LEN);
}


int PdoNuoDbStatement::execute()
{
        int status = 0;
    int update_count = 0;
    struct pdo_nuodb_timer_t timer;
    bool result = FALSE;
    int elapsed = 0;

    /* NOTE: caller catches exceptions. */
    PDO_DBG_ENTER("PdoNuoDbHandle::execute", getNuoDbHandle()->getPdoDbh());
    if (_stmt == NULL) {
        PDO_DBG_RETURN(0, getNuoDbHandle()->getPdoDbh());
    }

    pdo_nuodb_timer_init(&timer);
    pdo_nuodb_timer_start(&timer);
    status = _stmt->execute(_stmt);
    pdo_nuodb_timer_end(&timer);

    elapsed = pdo_nuodb_get_elapsed_time_in_microseconds(&timer);
    PDO_DBG_INF_FMT(": dbh=%p : Elapsed time=%d (microseconds) : SQL=%s", getNuoDbHandle()->getPdoDbh(), elapsed, this->_sql);

    if (status == 0) {
        _rs = _stmt->getResultSet(_stmt);
        update_count = _stmt->getUpdateCount(_stmt);
        PDO_DBG_INF_FMT(": dbh=%p : Update Count=%d", getNuoDbHandle()->getPdoDbh(), update_count);
        if (update_count != 0)
        {
            NuoDB_ResultSet *_rs_gen_keys = NULL;

            _rs_gen_keys = _stmt->getGeneratedKeys(_stmt);
            if (_rs_gen_keys != NULL)
            {
                PdoNuoDbGeneratedKeys *keys = new PdoNuoDbGeneratedKeys();
                keys->setKeys(_rs_gen_keys);
                _nuodbh->setLastKeys(keys);
                NuoDB_ResultSet_Free(_rs_gen_keys);
            }
        }
    } else {
        update_count = -1; /* use -1 to signal an error */
    }

    PDO_DBG_RETURN(update_count, getNuoDbHandle()->getPdoDbh());
}


bool PdoNuoDbStatement::hasResultSet()
{
    return (_rs != NULL);
}


bool PdoNuoDbStatement::next()
{
    /* NOTE: caller catches exceptions. */
    if (_rs == NULL) {
        return false;
    }
    return (_rs->next(_rs) == 0);
}


size_t PdoNuoDbStatement::getColumnCount()
{
        size_t columnCount;
    /* NOTE: caller catches exceptions. */
    if (_rs == NULL) {
        return false;
    }
    NuoDB_ResultSetMetaData * md = _rs->getMetaData(_rs);
    columnCount = md->getColumnCount(md);
    return columnCount;
}


char const * PdoNuoDbStatement::getColumnName(size_t column)
{
    char const * rval = NULL;

    if (_rs == NULL) {
        return NULL;
    }

    try  {
        NuoDB_Error_clearLastErrorInfo();
        NuoDB_ResultSetMetaData * md = _rs->getMetaData(_rs);
        if (md != NULL) {
            rval = md->getColumnLabel(md, column+1);
        }
        if (rval == NULL) {
            NuoDB_Error *e = NuoDB_Error_getLastErrorInfo();
            _nuodbh->setEinfoErrcode(e->getCode(e));
            _nuodbh->setEinfoErrmsg(e->getText(e));
            _nuodbh->setEinfoFile(__FILE__);
            _nuodbh->setEinfoLine(__LINE__);
            /* Workaround DB-4112 */
            _nuodbh->setSqlstate(nuodb_get_sqlstate(e->getCode(e)));
            _pdo_nuodb_error(_nuodbh->getPdoDbh(), NULL, _nuodbh->getEinfoFile(), _nuodbh->getEinfoLine());
        }
    } catch (...) {
        setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
        setEinfoErrmsg("Unknown Error in PdoNuoDbStatement::getColumnName");
        setEinfoFile(__FILE__);
        setEinfoLine(__LINE__);
        setSqlstate("XX000");
        _pdo_nuodb_error(_nuodbh->getPdoDbh(), _pdo_stmt, getEinfoFile(), getEinfoLine());
    }

    return rval;
}


int PdoNuoDbStatement::getSqlType(size_t column)
{
    int status = 0;
    NuoDB_ResultSetMetaData * md = NULL;
    int sqlType = 0;

    PDO_DBG_ENTER("PdoNuoDbStatement::getSqlType", getNuoDbHandle()->getPdoDbh());
    if (_rs == NULL) {
        PDO_DBG_RETURN(0, getNuoDbHandle()->getPdoDbh());
    }
    try {
        md = _rs->getMetaData(_rs);
        status = md->getColumnType(md, column+1, (NuoDB_Type *)&sqlType);
        if (status) {
            setEinfoErrcode(status);
            setEinfoErrmsg("Unknown Error in PdoNuoDbStatement::getSqlType");
            setEinfoFile(__FILE__);
            setEinfoLine(__LINE__);
            setSqlstate("XX000");
            _pdo_nuodb_error(_nuodbh->getPdoDbh(), _pdo_stmt, getEinfoFile(), getEinfoLine());
        }
        switch (sqlType)
        {
            case NUODB_TYPE_NULL:
                return PDO_NUODB_SQLTYPE_NULL;
            case NUODB_TYPE_BOOLEAN:
                return PDO_NUODB_SQLTYPE_BOOLEAN;
            case NUODB_TYPE_INTEGER:
            case NUODB_TYPE_SMALLINT:
            case NUODB_TYPE_TINYINT:
                return PDO_NUODB_SQLTYPE_INTEGER;

                /*
                 * We are returning numeric types as a string
                 * because of DB-2288
                */
            case NUODB_TYPE_BIGINT:
            case NUODB_TYPE_FLOAT:
            case NUODB_TYPE_DOUBLE:
            case NUODB_TYPE_DECIMAL:
            case NUODB_TYPE_NUMERIC:
                return PDO_NUODB_SQLTYPE_STRING;

            case NUODB_TYPE_CHAR:
            case NUODB_TYPE_VARCHAR:
            case NUODB_TYPE_LONGVARCHAR:
                return PDO_NUODB_SQLTYPE_STRING;

            case NUODB_TYPE_DATE:
                return PDO_NUODB_SQLTYPE_DATE;

            case NUODB_TYPE_TIME:
                return PDO_NUODB_SQLTYPE_TIME;

            case NUODB_TYPE_TIMESTAMP:
                return PDO_NUODB_SQLTYPE_TIMESTAMP;

            case NUODB_TYPE_BLOB:
                return PDO_NUODB_SQLTYPE_BLOB;

            case NUODB_TYPE_CLOB:
                return PDO_NUODB_SQLTYPE_CLOB;

            default: {
                PDO_DBG_ERR_FMT(": dbh=%p : unknown/unsupported type: %d", getNuoDbHandle()->getPdoDbh(), sqlType);
                break;
            }
        }
    } catch (...) {
        setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
        setEinfoErrmsg("Unknown Error in PdoNuoDbStatement::getSqlType");
        setEinfoFile(__FILE__);
        setEinfoLine(__LINE__);
        setSqlstate("XX000");
        _pdo_nuodb_error(_nuodbh->getPdoDbh(), _pdo_stmt, getEinfoFile(), getEinfoLine());
    }
    PDO_DBG_RETURN(0, getNuoDbHandle()->getPdoDbh());
}


char const * PdoNuoDbStatement::getString(size_t column)
{
    /* NOTE: caller catches exceptions. */
        int status = 0;
    char const *res = NULL;

    if (_rs == NULL) {
        return res;
    }
    status = _rs->getString(_rs, column+1, NULL, &res);
    if (status) {
        return NULL;
    }

    if (_rs->wasNull(_rs)) {
        res = NULL;
    }
    return res;
}


void PdoNuoDbStatement::getInteger(size_t column, int **int_val)
{
        int status = 0;
    /* NOTE: caller catches exceptions. */
    if (_rs == NULL) {
        return;
    }

    status = _rs->getInt(_rs, column+1, *int_val);
    if (status) {
        *int_val = NULL;
        return;
    }
    if (_rs->wasNull(_rs)) {
        *int_val = NULL;
    }
}


bool PdoNuoDbStatement::getBoolean(size_t column, char **bool_val)
{
        int status = 0;
        nuodb_bool_t b;
    /* NOTE: caller catches exceptions. */
    if (_rs == NULL) {
        return 0;
    }
    status = _rs->getBoolean(_rs, column+1, &b);
    if (status) {
        *bool_val = NULL;
        return 0;
    }
    if (_rs->wasNull(_rs)) {
        *bool_val = NULL;
        return 0;
    }
    **bool_val = b;
    return 1;
}


void PdoNuoDbStatement::getLong(size_t column, int64_t **long_val)
{
        int status = 0;
    /* NOTE: caller catches exceptions. */
    if (_rs == NULL) {
        return;
    }

    status = _rs->getLong(_rs, column+1, *long_val);
    if (status) {
        *long_val = NULL;
        return;
    }
    if (_rs->wasNull(_rs)) {
        *long_val = NULL;
    }
}


char const *PdoNuoDbStatement::getTimestamp(size_t column)
{
        int status = 0;
    /* NOTE: caller catches exceptions. */
    char const *res = NULL;

    if (_rs == NULL) {
        return res;
    }

    status = _rs->getString(_rs, column+1, NULL, &res);
    if (status) {
        return NULL;
    }
    if (_rs->wasNull(_rs)) {
        res = NULL;
    }
    return res;
}


void PdoNuoDbStatement::getTime(size_t column, int64_t **time_val)
{
        int status = 0;
    NuoDB_Temporal *time = NULL;
    /* NOTE: caller catches exceptions. */
    if (_rs == NULL) {
        return;
    }

    status = _rs->getTemporal(_rs, column+1, NUODB_TEMPORAL_TIME, &time);
    if (status) {
        *time_val = NULL;
        return;
    }
    if (_rs->wasNull(_rs)) {
        *time_val = NULL;
        return;
    }
    **time_val = time->getMilliSeconds(time) / 1000;
}


void PdoNuoDbStatement::getDate(size_t column, int64_t **date_val)
{
        int status = 0;
    NuoDB_Temporal *date = NULL;

    /* NOTE: caller catches exceptions. */
    if (_rs == NULL) {
        return;
    }

    status = _rs->getTemporal(_rs, column+1, NUODB_TEMPORAL_DATE, &date);
    if (status) {
        *date_val = NULL;
        return;
    }
    if (_rs->wasNull(_rs)) {
        *date_val = NULL;
        return;
    }
    **date_val = date->getMilliSeconds(date) / 1000;
}


void PdoNuoDbStatement::getBlob(size_t column, char ** ptr, unsigned long * len, void * (*erealloc)(void *ptr, size_t size, int allow_failure ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC))
{
        int status = 0;
    NuoDB_Lob *blob = NULL;
    /* NOTE: caller catches exceptions. */
    if (_rs == NULL) {
        return;
    }

    status = _rs->getLob(_rs, column+1, NUODB_BLOB_TYPE, &blob);
    if (status) {
        *ptr = NULL;
        return;
    }
    *len = blob->getLength(blob);
    if ((*len) == 0) {
        *ptr = NULL;
    } else {
        *ptr = (char *)(*erealloc)((void *)*ptr, *len+1, 0 ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC);
        memcpy(*ptr, blob->getData(blob), *len);
        (*ptr)[*len] = '\0';
    }
    NuoDB_Lob_Free(blob);
    return;
}


void PdoNuoDbStatement::getClob(size_t column, char ** ptr, unsigned long * len, void * (*erealloc)(void *ptr, size_t size, int allow_failure ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC))
{
        int status = 0;
    NuoDB_Lob *clob = NULL;
    /* NOTE: caller catches exceptions. */
    if (_rs == NULL) {
        return;
    }

    status = _rs->getLob(_rs, column+1, NUODB_CLOB_TYPE, &clob);
    if (status) {
        *ptr = NULL;
        return;
    }
    *len = clob->getLength(clob);
    if ((*len) == 0) {
        *ptr = NULL;
    } else {
        *ptr = (char *)(*erealloc)((void *)*ptr, *len+1, 0 ZEND_FILE_LINE_CC ZEND_FILE_LINE_EMPTY_CC);
        memcpy(*ptr, clob->getData(clob), *len);
        (*ptr)[*len] = '\0';
    }
    NuoDB_Lob_Free(clob);
    return;
}


size_t PdoNuoDbStatement::getNumberOfParameters()
{
    if (_stmt == NULL) {
        return 0;
    }

/* NuoDB Parameter Meta Data does not exist in the NuoDB CDriver so
  disable this
  NuoDB_ParameterMetaData *pmd =
      _stmt->getParameterMetaData(_stmt);

    if (pmd == NULL) {
        return 0;
    }
    return pmd->getParameterCount(pmd); */
    return 0;
}


void PdoNuoDbStatement::setInteger(size_t index, int value)
{
    /* NOTE: caller catches exceptions. */
    if (_stmt == NULL) {
        return;
    }
    _stmt->setInt(_stmt, index+1, value);
    return;
}


void PdoNuoDbStatement::setBoolean(size_t index, bool value)
{
    /* NOTE: caller catches exceptions. */
    if (_stmt == NULL) {
        return;
    }
    _stmt->setBoolean(_stmt, index+1, value);
    return;
}


void PdoNuoDbStatement::setString(size_t index, const char *value)
{
    /* NOTE: caller catches exceptions. */
    if (_stmt == NULL) {
        return;
    }

    _stmt->setString(_stmt, index+1, value, strlen(value));
    return;
}

void PdoNuoDbStatement::setString(size_t index, const char *value, int length)
{
    /* NOTE: caller catches exceptions. */
    if (_stmt == NULL) {
        return;
    }
    _stmt->setString(_stmt, index+1, value, length);
    return;
}


void PdoNuoDbStatement::setBytes(size_t index, const void *value, int length)
{
    /* NOTE: caller catches exceptions. */
    if (_stmt == NULL) {
        return;
    }

    NuoDB_Connection *con = _nuodbh->getConnection();
    NuoDB_Lob *bytes = NuoDB_Lob_Create(con, NUODB_BYTES_TYPE);
    if (value != NULL) {
        bytes->setData(bytes, (char *)value, length);
    }
    _stmt->setLob(_stmt, index+1, bytes);
    return;
}


void PdoNuoDbStatement::setBlob(size_t index, const char *value, int len)
{
    /* NOTE: caller catches exceptions. */
    if (_stmt == NULL) {
        return;
    }
    NuoDB_Connection *con = _nuodbh->getConnection();
    NuoDB_Lob *blob = NuoDB_Lob_Create(con, NUODB_BLOB_TYPE);
    if (value != NULL) {
        blob->setData(blob, (char *)value, len);
    }
    _stmt->setLob(_stmt, index+1, blob);
    NuoDB_Lob_Free(blob);
    return;
}

void PdoNuoDbStatement::setClob(size_t index, const char *value, int len)
{
    /* NOTE: caller catches exceptions. */
    if (_stmt == NULL) {
        return;
    }

    NuoDB_Connection *con = _nuodbh->getConnection();
    NuoDB_Lob *clob = NuoDB_Lob_Create(con, NUODB_CLOB_TYPE);
    if (value != NULL) {
        clob->setData(clob, (char *)value, len);
    }
    _stmt->setLob(_stmt, index+1, clob);
    NuoDB_Lob_Free(clob);
    return;
}

/* C/C++ jump functions */

extern "C" {


    void pdo_nuodb_timer_init(struct pdo_nuodb_timer_t *timer)
    {
        if (timer == NULL) {
            return;
        }

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
        if (timer == NULL) {
            return;
        }

        timer->stopped = 0; /* reset stop flag */

#ifdef WIN32
        QueryPerformanceCounter(&(timer->startCount));
#else
        gettimeofday(&(timer->startCount), NULL);
#endif

    }


    void pdo_nuodb_timer_end(struct pdo_nuodb_timer_t *timer)
    {
        if (timer == NULL) {
            return;
        }

        timer->stopped = 1; /* set timer stopped flag */

#ifdef WIN32
        QueryPerformanceCounter(&(timer->endCount));
#else
        gettimeofday(&(timer->endCount), NULL);
#endif
    }


    int pdo_nuodb_get_elapsed_time_in_microseconds(struct pdo_nuodb_timer_t *timer)
    {
        if (timer == NULL) {
            return 0.0;
        }

#ifdef WIN32
        if(!timer->stopped) {
            QueryPerformanceCounter(&(timer->endCount));
        }

        timer->startTimeInMicroSec = timer->startCount.QuadPart * (1000000 / timer->frequency.QuadPart);
        timer->endTimeInMicroSec = timer->endCount.QuadPart * (1000000 / timer->frequency.QuadPart);
#else
        if(!timer->stopped) {
            gettimeofday(&(timer->endCount), NULL);
        }

        timer->startTimeInMicroSec = (timer->startCount.tv_sec * 1000000) + timer->startCount.tv_usec;
        timer->endTimeInMicroSec = (timer->endCount.tv_sec * 1000000) + timer->endCount.tv_usec;
#endif

        return timer->endTimeInMicroSec - timer->startTimeInMicroSec;
    }


    int pdo_nuodb_db_handle_errno(pdo_nuodb_db_handle *H) {
        if (H == NULL) {
            return 0;
        }

        if (H->db == NULL) {
            return 0;
        }

        PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
        return db->getEinfoErrcode();
    }


    const char *pdo_nuodb_db_handle_errmsg(pdo_nuodb_db_handle *H) {
        if (H == NULL) {
            return 0;
        }

        if (H->db == NULL) {
            return 0;
        }

        PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
        return db->getEinfoErrmsg();
    }


    pdo_error_type * pdo_nuodb_db_handle_sqlstate(pdo_nuodb_db_handle *H) {
        if (H == NULL) {
            return 0;
        }

        if (H->db == NULL) {
            return 0;
        }

        PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
        return db->getSqlstate();
    }


    int pdo_nuodb_db_handle_commit(pdo_nuodb_db_handle *H) {
        try {
            PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
            db->commit();
        } catch (...) {
            return 0; /* TODO - save the error message text. */
        }
        return 1;
    }

    int pdo_nuodb_db_handle_rollback(pdo_nuodb_db_handle *H) {
        try {
            PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
            db->rollback();
        } catch (...) {
            return 0; /* TODO - save the error message text. */
        }
        return 1;
    }


    int pdo_nuodb_db_handle_close_connection(pdo_nuodb_db_handle *H) {
        try {
            PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
            db->closeConnection();
        } catch (...) {
            return 0; /* TODO - save the error message text. */
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
            return 0; /* TODO - save the error message text. */
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
        catch (...)
        {
            db->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            db->setEinfoErrmsg("UNKNOWN ERROR in pdo_nuodb_db_handle_set_auto_commit()");
            db->setEinfoFile(__FILE__);
            db->setEinfoLine(__LINE__);
            db->setSqlstate("XX000");
            _pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine());
            return 0;
        }
        return 1;
    }


    const char *pdo_nuodb_db_handle_get_nuodb_product_name(pdo_nuodb_db_handle *H)
    {
        PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
        const char *rval = NULL;

        try {
            rval = db->getNuoDBProductName();
        }
        catch (...)
        {
            db->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            db->setEinfoErrmsg("UNKNOWN ERROR in pdo_nuodb_db_handle_get_nuodb_product_name()");
            db->setEinfoFile(__FILE__);
            db->setEinfoLine(__LINE__);
            db->setSqlstate("XX000");
            _pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine());
            return NULL;
        }
        return rval;
    }


    const char *pdo_nuodb_db_handle_get_nuodb_product_version(pdo_nuodb_db_handle *H)
    {
        PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
        const char *rval = "unknown";

        try {
            rval = db->getNuoDBProductVersion();
        }
        catch (...)
        {
            db->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            db->setEinfoErrmsg("UNKNOWN ERROR in pdo_nuodb_db_handle_get_nuodb_product_version()");
            db->setEinfoFile(__FILE__);
            db->setEinfoLine(__LINE__);
            db->setSqlstate("XX000");
            _pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine());
            return NULL;
        }
        return rval;
    }


    void *pdo_nuodb_db_handle_create_statement(pdo_nuodb_db_handle * H, pdo_stmt_t * pdo_stmt, const char * sql)
    {
        PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
        PdoNuoDbStatement *nuodb_stmt = NULL;

        try {
            nuodb_stmt = db->createStatement(pdo_stmt, sql);
        }
        catch (...)
        {
            nuodb_stmt = NULL; /* TODO - save the error message text. */
        }
        return (void *)nuodb_stmt;
    }


    long pdo_nuodb_db_handle_doer(pdo_nuodb_db_handle * H, void *dbh_opaque, const char * sql, unsigned in_txn, unsigned auto_commit, void (*pt2pdo_dbh_t_set_in_txn)(void *dbh_opaque, unsigned in_txn))
    {
        PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);
        PdoNuoDbStatement * nuodb_stmt = NULL;
        unsigned in_txn_state = in_txn;
        long res;

        try
        {
            nuodb_stmt = (PdoNuoDbStatement *) pdo_nuodb_db_handle_create_statement(H, NULL, sql);

            if ((H->pdo_dbh->auto_commit == 0) &&
                (H->pdo_dbh->in_txn == 0))
            {
                if ((H->in_nuodb_implicit_txn == 0) && (H->in_nuodb_explicit_txn == 0)) {
                    H->in_nuodb_implicit_txn = 1;
                }
            }

            res = nuodb_stmt->execute();

            if (H->in_nuodb_implicit_txn == 1) {
                if (res == -1) {
                    pdo_nuodb_db_handle_rollback(H);
                } else {
                    pdo_nuodb_db_handle_commit(H);
                }
                H->in_nuodb_implicit_txn = 0;
                H->in_nuodb_explicit_txn = 0;
            }
        }
        catch (...)
        {
            db->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            db->setEinfoErrmsg("UNKNOWN ERROR in pdo_nuodb_db_handle_doer()");
            db->setEinfoFile(__FILE__);
            db->setEinfoLine(__LINE__);
            db->setSqlstate("XX000");
            _pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine());
            (*pt2pdo_dbh_t_set_in_txn)(dbh_opaque, in_txn_state);
            delete nuodb_stmt;
            return -1;
        }
        if (res == -1) {
            NuoDB_Error *e = NuoDB_Error_getLastErrorInfo();
            const char *sqlState = e->getSqlState(e);
            db->setEinfoErrcode(e->getCode(e));
            db->setEinfoErrmsg(e->getText(e));
            db->setEinfoFile(__FILE__);
            db->setEinfoLine(__LINE__);
            if ((sqlState == NULL) || (sqlState[0] == '\0'))
            {
                db->setSqlstate("XX000");
            } else {
                db->setSqlstate(sqlState);
            }
            _pdo_nuodb_error(H->pdo_dbh, NULL, db->getEinfoFile(), db->getEinfoLine());
            (*pt2pdo_dbh_t_set_in_txn)(dbh_opaque, in_txn_state);
            delete nuodb_stmt;
            return -1;
        }
        (*pt2pdo_dbh_t_set_in_txn)(dbh_opaque, in_txn_state);
        delete nuodb_stmt;
        return res;
    }


    int pdo_nuodb_db_handle_factory(pdo_nuodb_db_handle * H, SqlOptionArray *optionsArray, char **errMessage)
    {
        PdoNuoDbHandle *db = NULL;
        *errMessage = NULL;

        try {
            db = new PdoNuoDbHandle(H->pdo_dbh, optionsArray);
            H->db = (void *) db;
            NuoDB_Connection *con = db->createConnection();
            if (con == NULL) {
                NuoDB_Error *e = NuoDB_Error_getLastErrorInfo();
                *errMessage = (char *)e->getText(e);
                return 0;
            }
            NuoDB_DatabaseMetaData *dbmd = con->getMetaData(con);
            const char *connectionString  = dbmd->getConnectionURL(dbmd);
            PDO_DBG_LEVEL_FMT(PDO_NUODB_LOG_SQL, "dbh=%p : pdo_nuodb_handle_factory : Connected to database on URL=%s connectionSring=%s user=%s schema=%s",
                        H->pdo_dbh, connectionString, optionsArray->array[0].extra, optionsArray->array[1].extra, optionsArray->array[3].extra);
            db->setTransactionIsolation(H->default_txn_isolation_level);
        } catch (...) {
            if (db != NULL) {
                db->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
                db->setEinfoErrmsg("Unknown Error in pdo_nuodb_db_handle_factory()");
                db->setEinfoFile(__FILE__);
                db->setEinfoLine(__LINE__);
                db->setSqlstate("XX000");
                _pdo_nuodb_error(db->getPdoDbh(), NULL, db->getEinfoFile(), db->getEinfoLine());
            }
            *errMessage = strdup("Error constructing a NuoDB handle");
            return 0;
        }
        return 1;
    }

    int pdo_nuodb_db_handle_last_id(pdo_nuodb_db_handle *H, const char *name) {
        int last_id = 0;
        PdoNuoDbHandle *db = (PdoNuoDbHandle *) (H->db);

        if (db == NULL) {
            return 0;
        }

        try {
            last_id = db->getLastId(name);
        }
        catch (...) {
            /* Swallow it. */
            return 0;
        }

        return last_id;
    }


    int pdo_nuodb_stmt_errno(pdo_nuodb_stmt *S) {
        if (S == NULL) {
            return 0;
        }

        if (S->stmt == NULL) {
            return 0;
        }

        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
        return nuodb_stmt->getEinfoErrcode();
    }


    const char *pdo_nuodb_stmt_errmsg(pdo_nuodb_stmt *S) {
        if (S == NULL) {
            return 0;
        }

        if (S->stmt == NULL) {
            return 0;
        }

        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
        return nuodb_stmt->getEinfoErrmsg();
    }


    pdo_error_type *pdo_nuodb_stmt_sqlstate(pdo_nuodb_stmt *S) {
        if (S == NULL) {
            return 0;
        }

        if (S->stmt == NULL) {
            return 0;
        }

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
        catch (...) {
            db->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            db->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_delete()");
            db->setEinfoFile(__FILE__);
            db->setEinfoLine(__LINE__);
            db->setSqlstate("XX000");
            _pdo_nuodb_error(db->getPdoDbh(), NULL, db->getEinfoFile(), db->getEinfoLine());
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
                _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
                return 0;
            }
        }

        try {
            int res = nuodb_stmt->execute();
            if (res == -1) {
                NuoDB_Error *e = NuoDB_Error_getLastErrorInfo();
                const char *sqlState = e->getSqlState(e);
                nuodb_stmt->setEinfoErrcode(e->getCode(e));
                nuodb_stmt->setEinfoErrmsg(e->getText(e));
                nuodb_stmt->setEinfoFile(__FILE__);
                nuodb_stmt->setEinfoLine(__LINE__);
                if ((sqlState == NULL) || (sqlState[0] == '\0'))
                {
                    nuodb_stmt->setSqlstate("XX000");
                } else {
                    nuodb_stmt->setSqlstate(sqlState);
                }
                _pdo_nuodb_error(H->pdo_dbh, nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
                return 0;
            }
            affected_rows = res;
            S->cursor_open = nuodb_stmt->hasResultSet();
            *column_count = nuodb_stmt->getColumnCount();
        }
        catch (...) {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_execute()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(H->pdo_dbh, nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
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
                if (nuodb_stmt->next()) {
                    (*row_count)++;
                    return 1;
                } else {
                    S->exhausted = 1;
                    return 0;
                }
            } catch (...) {
                nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
                nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_fetch()");
                nuodb_stmt->setEinfoFile(__FILE__);
                nuodb_stmt->setEinfoLine(__LINE__);
                nuodb_stmt->setSqlstate("XX000");
                _pdo_nuodb_error(H->pdo_dbh, nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
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
        } catch (...) {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_column_name()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
            return 0;
        }

        return column_name;
    }

    int pdo_nuodb_stmt_get_sql_type(pdo_nuodb_stmt * S, int colno) {
        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;
        int sql_type = 0;

        try {
            sql_type = nuodb_stmt->getSqlType(colno);
        } catch (...) {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_sql_type()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
            return 0;
        }
        return sql_type;
    }

    int pdo_nuodb_stmt_set_integer(pdo_nuodb_stmt *S, int paramno, long int_val)
    {
        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            nuodb_stmt->setInteger(paramno,  int_val);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_set_integer()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
            return 0;
        }
        return 1;
    }

    int pdo_nuodb_stmt_set_boolean(pdo_nuodb_stmt *S, int paramno, char bool_val)
    {
        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            nuodb_stmt->setBoolean(paramno,  (bool_val == 't') ? 1 : 0);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_set_boolean()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
            return 0;
        }
        return 1;
    }


    int pdo_nuodb_stmt_set_string(pdo_nuodb_stmt *S, int paramno, char *str_val)
    {

        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            nuodb_stmt->setString(paramno,  str_val);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_set_string()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
            return 0;
        }
        return 1;
    }

    int pdo_nuodb_stmt_set_string_with_length(pdo_nuodb_stmt *S, int paramno, const char *str_val, int length)
    {

        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            nuodb_stmt->setString(paramno,  str_val, length);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_set_string_with_length()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
            return 0;
        }
        return 1;
    }


    int pdo_nuodb_stmt_set_bytes(pdo_nuodb_stmt *S, int paramno, const void *val, int length)
    {
        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            nuodb_stmt->setBytes(paramno, val, length);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_set_bytes()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
            return 0;
        }
        return 1;
    }


    int pdo_nuodb_stmt_set_blob(pdo_nuodb_stmt *S, int paramno, char *blob_val, int len)
    {
        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            nuodb_stmt->setBlob(paramno,  blob_val, len);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_set_blob()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
            return 0;
        }
        return 1;
    }


    int pdo_nuodb_stmt_set_clob(pdo_nuodb_stmt *S, int paramno, char *clob_val, int len)
    {
        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            nuodb_stmt->setClob(paramno,  clob_val, len);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_set_clob()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
            return 0;
        }
        return 1;
    }


    void pdo_nuodb_stmt_get_integer(pdo_nuodb_stmt *S, int colno, int **int_val)
    {
        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            nuodb_stmt->getInteger(colno, int_val);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_integer()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
        }
        return;
    }


    void pdo_nuodb_stmt_get_boolean(pdo_nuodb_stmt *S, int colno, char **bool_val)
    {
        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            nuodb_stmt->getBoolean(colno, bool_val);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_boolean()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
        }
        return;
    }


    void pdo_nuodb_stmt_get_long(pdo_nuodb_stmt *S, int colno, int64_t **long_val)
    {
        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            nuodb_stmt->getLong(colno, long_val);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_long()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
        }
        return;
    }


    const char *pdo_nuodb_stmt_get_string(pdo_nuodb_stmt *S, int colno)
    {
        const char *res = NULL;
        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            res = nuodb_stmt->getString(colno);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_string()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
            return NULL;
        }
        return res;
    }


    void pdo_nuodb_stmt_get_date(pdo_nuodb_stmt *S, int colno, int64_t **date_val)
    {
        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            nuodb_stmt->getDate(colno, date_val);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_date()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
        }
        return;
    }


    void pdo_nuodb_stmt_get_time(pdo_nuodb_stmt *S, int colno, int64_t **time_val)
    {
        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            nuodb_stmt->getTime(colno, time_val);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_time()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
        }
        return;
    }


    const char *pdo_nuodb_stmt_get_timestamp(pdo_nuodb_stmt *S, int colno)
    {
        const char *res = NULL;
        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            res = nuodb_stmt->getTimestamp(colno);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_timestamp()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
        }
        return res;
    }


    void pdo_nuodb_stmt_get_blob(pdo_nuodb_stmt *S, int colno, char ** ptr, unsigned long * len,
                                 void * (*erealloc)(void *ptr, size_t size, int allow_failure ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC))
    {

        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            nuodb_stmt->getBlob(colno, ptr, len, erealloc);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_blob()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
        }
        return;
    }


    void pdo_nuodb_stmt_get_clob(pdo_nuodb_stmt *S, int colno, char ** ptr, unsigned long * len,
                                 void * (*erealloc)(void *ptr, size_t size, int allow_failure ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC))
    {
        PdoNuoDbStatement *nuodb_stmt = (PdoNuoDbStatement *) S->stmt;

        try {
            nuodb_stmt->getClob(colno, ptr, len, erealloc);
        }
        catch (...)
        {
            nuodb_stmt->setEinfoErrcode(PDO_NUODB_SQLCODE_INTERNAL_ERROR);
            nuodb_stmt->setEinfoErrmsg("Unknown Error in pdo_nuodb_stmt_get_clob()");
            nuodb_stmt->setEinfoFile(__FILE__);
            nuodb_stmt->setEinfoLine(__LINE__);
            nuodb_stmt->setSqlstate("XX000");
            _pdo_nuodb_error(nuodb_stmt->getNuoDbHandle()->getPdoDbh(), nuodb_stmt->getPdoStmt(), nuodb_stmt->getEinfoFile(), nuodb_stmt->getEinfoLine());
        }
        return;
    }

} /* end of extern "C" */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
