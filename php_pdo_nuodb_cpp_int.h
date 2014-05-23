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

#ifndef PHP_PDO_NUODB_CPP_INT_H
#define PHP_PDO_NUODB_CPP_INT_H

#include "Connection.h"
#include "Properties.h"
#include "ResultSet.h"
#include "DatabaseMetaData.h"
#include "PreparedStatement.h"

class PdoNuoDbStatement;

struct PdoNuoDbGeneratedKeyElement
{
    char *columnName;
    int columnIndex;
    int columnKeyValue;
};

class PdoNuoDbGeneratedKeys
{
private:
    int _qty;
    PdoNuoDbGeneratedKeyElement *_keys;
public:
    PdoNuoDbGeneratedKeys();
    ~PdoNuoDbGeneratedKeys();
    void setKeys(NuoDB::ResultSet *rs);
    int getIdValue();
    int getIdValue(const char *seqName);
};

class PdoNuoDbHandle
{
private:
    NuoDB::Connection * _con;
    _pdo_dbh_t *_pdo_dbh;
    SqlOptionArray * _opts;
    SqlOption _opt_arr[4];
    pdo_nuodb_error_info einfo; /* NuoDB error information */
    pdo_error_type sqlstate;
    PdoNuoDbStatement * _last_stmt; /* will be NULL if the last
                                       statement was closed. */
    PdoNuoDbGeneratedKeys *_last_keys;  /* pointer to array of
                                           generated keys. */
    void deleteOptions();
public:
    PdoNuoDbHandle(pdo_dbh_t *pdo_dbh, SqlOptionArray * options);
    ~PdoNuoDbHandle();
    _pdo_dbh_t *getPdoDbh();
    int getEinfoLine();
    const char *getEinfoFile();
    int getEinfoErrcode();
    const char *getEinfoErrmsg();
    pdo_error_type *getSqlstate();
    void setEinfoLine(int line);
    void setEinfoFile(const char *file);
    void setEinfoErrcode(int errcode);
    void setEinfoErrmsg(const char *errmsg);
    void setSqlstate(const char *sqlstate);
    void setLastStatement(PdoNuoDbStatement *lastStatement);
    void setLastKeys(PdoNuoDbGeneratedKeys *lastKeys);
    void setOptions(SqlOptionArray * options);
    NuoDB::Connection * createConnection();
    NuoDB::Connection * getConnection();
    PdoNuoDbStatement * createStatement(pdo_stmt_t *pdo_stmt, char const * sql);
    void closeConnection();
    void commit();
    void rollback();
    int getLastId(const char *name);
    void setAutoCommit(bool autoCommit);
    const char *getNuoDBProductName();
    const char *getNuoDBProductVersion();
};

class PdoNuoDbStatement
{
private:
    PdoNuoDbHandle * _nuodbh;
    pdo_stmt_t *_pdo_stmt;
    pdo_nuodb_error_info einfo; /* NuoDB error information */
    pdo_error_type sqlstate;
    const char *_sql;
    NuoDB::PreparedStatement * _stmt;
    NuoDB::ResultSet * _rs;
public:
    PdoNuoDbStatement(PdoNuoDbHandle * dbh, pdo_stmt_t *pdo_stmt);
    ~PdoNuoDbStatement();
    NuoDB::PreparedStatement * createStatement(char const * sql);
    PdoNuoDbHandle * getNuoDbHandle();
    pdo_stmt_t *getPdoStmt();
    int getEinfoLine();
    const char *getEinfoFile();
    int getEinfoErrcode();
    const char *getEinfoErrmsg();
    pdo_error_type *getSqlstate();
    void setEinfoLine(int line);
    void setEinfoFile(const char *file);
    void setEinfoErrcode(int errcode);
    void setEinfoErrmsg(const char *errmsg);
    void setSqlstate(const char *sqlstate);
    int execute();
    bool hasResultSet();
    bool next();
    size_t getColumnCount();
    char const * getColumnName(size_t column);
    int getSqlType(size_t column);
    char const * getString(size_t column);
    void getInteger(size_t column, int **int_val);
    bool getBoolean(size_t column, char **bool_val);
    void getLong(size_t column, int64_t **long_val);
    char const * getTimestamp(size_t column);
    void getDate(size_t column, int64_t **date_val);
    void getTime(size_t column, int64_t **time_val);
    void getBlob(size_t column, char ** ptr, unsigned long * len, void * (*erealloc)(void *ptr, size_t size, int ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC));
    void getClob(size_t column, char ** ptr, unsigned long * len, void * (*erealloc)(void *ptr, size_t size, int ZEND_FILE_LINE_DC ZEND_FILE_LINE_ORIG_DC));
    size_t getNumberOfParameters();
    int getGeneratedKeyLastId(const char *name);

    void setInteger(size_t index, int value);
    void setBoolean(size_t index, bool value);
    void setString(size_t index, const char *value);
    void setBytes(size_t index, const void *value, int length);
    void setBlob(size_t index, const char *value, int len);
    void setClob(size_t index, const char *value, int len);
};

#endif	/* end of: PHP_PDO_NUODB_INT_CPP_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
