/*
 * pkgmgr_parser_db_util
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jayoun Lee <airjany@samsung.com>, Sewook Park <sewook7.park@samsung.com>,
 * Jaeho Lee <jaeho81.lee@samsung.com>, Shobhit Srivastava <shobhit.s@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <db-util.h>
#include "pkgmgr_parser.h"
#include "pkgmgrinfo_debug.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_PARSER"

int _pkgmgr_db_open(const char *dbfile, sqlite3 **database)
{
	int ret = 0;
	retvm_if(dbfile == NULL, PM_PARSER_R_ERROR, "dbfile is NULL");
	retvm_if(database == NULL, PM_PARSER_R_ERROR, "database is NULL");

	ret = db_util_open(dbfile, database, 0);
	retvm_if(ret != SQLITE_OK, PM_PARSER_R_ERROR, "db_open fail[ret = %d]\n", ret);

	return 0;
}

int _pkgmgr_db_prepare(sqlite3 *database, const char *query, sqlite3_stmt **stmt)
{
	int ret = 0;
	retvm_if(database == NULL, PM_PARSER_R_ERROR, "database is NULL");
	retvm_if(query == NULL, PM_PARSER_R_ERROR, "query is NULL");
	retvm_if(stmt == NULL, PM_PARSER_R_ERROR, "stmt is NULL");

	ret = sqlite3_prepare_v2(database, query, strlen(query), stmt, NULL);
	retvm_if(ret != SQLITE_OK, PM_PARSER_R_ERROR, "db_prepare fail[ret = %d]\n", ret);

	return 0;
}

int _pkgmgr_db_step(sqlite3_stmt *stmt)
{
	int ret = 0;
	retvm_if(stmt == NULL, PM_PARSER_R_ERROR, "stmt is NULL");

	ret = sqlite3_step(stmt);
	retvm_if(ret != SQLITE_ROW, PM_PARSER_R_ERROR, "db_step fail[ret = %d]\n", ret);

	return 0;
}

int _pkgmgr_db_get_str(sqlite3_stmt *stmt, int index, char **str)
{
	retvm_if(stmt == NULL, PM_PARSER_R_ERROR, "stmt is NULL");
	retvm_if(str == NULL, PM_PARSER_R_ERROR, "str is NULL");

	*str = (char *)sqlite3_column_text(stmt, index);

	return 0;
}

int _pkgmgr_db_finalize(sqlite3_stmt *stmt)
{
	int ret = 0;
	retvm_if(stmt == NULL, PM_PARSER_R_ERROR, "stmt is NULL");

	ret = sqlite3_finalize(stmt);
	retvm_if(ret != SQLITE_OK, PM_PARSER_R_ERROR, "db_finalize fail[ret = %d]\n", ret);

	return 0;
}

int _pkgmgr_db_close(sqlite3 *database)
{
	int ret = 0;
	ret = sqlite3_close(database);
	retvm_if(ret != SQLITE_OK, PM_PARSER_R_ERROR, "db_close fail[ret = %d]\n", ret);

	return 0;
}

