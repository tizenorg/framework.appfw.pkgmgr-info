/*
 * pkgmgrinfo-db
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jayoun Lee <airjany@samsung.com>, Junsuk Oh <junsuk77.oh@samsung.com>,
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

#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <db-util.h>
#include <dlfcn.h>
#include <dirent.h>

#include "pkgmgrinfo_private.h"
#include "pkgmgrinfo_debug.h"
#include "pkgmgr-info.h"

#include "pkgmgr_parser.h"
#include "pkgmgr_parser_db.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_DB"

static int __ail_set_installed_storage(const char *pkgid, INSTALL_LOCATION location)
{
	retvm_if(pkgid == NULL, PMINFO_R_EINVAL, "pkgid is NULL\n");
	int ret = -1;
	int exist = 0;
	sqlite3 *ail_db = NULL;
	char *AIL_DB_PATH = "/opt/dbspace/.app_info.db";
	char *query = NULL;

	ret = db_util_open(AIL_DB_PATH, &ail_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", AIL_DB_PATH);

	/*Begin transaction*/
	ret = sqlite3_exec(ail_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Failed to begin transaction\n");
	_LOGD("Transaction Begin\n");

	/*validate pkgid*/
	query = sqlite3_mprintf("select exists(select * from app_info where x_slp_pkgid=%Q)", pkgid);
	ret = __exec_db_query(ail_db, query, _pkgmgrinfo_validate_cb, (void *)&exist);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "sqlite3_exec[%s] fail", pkgid);
	if (exist == 0) {
		_LOGS("pkgid[%s] not found in DB", pkgid);
		ret = PMINFO_R_ERROR;
		goto catch;
	}
	sqlite3_free(query);

	query = sqlite3_mprintf("update app_info set x_slp_installedstorage=%Q where x_slp_pkgid=%Q", location?"installed_external":"installed_internal", pkgid);

	ret = sqlite3_exec(ail_db, query, NULL, NULL, NULL);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Don't execute query = %s\n", query);

	/*Commit transaction*/
	ret = sqlite3_exec(ail_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to commit transaction. Rollback now\n");
		ret = sqlite3_exec(ail_db, "ROLLBACK", NULL, NULL, NULL);
		tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Don't execute query = %s\n", query);
	}
	_LOGD("Transaction Commit and End\n");

	ret = PMINFO_R_OK;
catch:
	sqlite3_close(ail_db);
	sqlite3_free(query);
	return ret;
}

API int pkgmgrinfo_pkginfo_set_state_enabled(const char *pkgid, bool enabled)
{
	/* Should be implemented later */
	return 0;
}

API int pkgmgrinfo_pkginfo_set_preload(const char *pkgid, int preload)
{
	retvm_if(pkgid == NULL, PMINFO_R_EINVAL, "pkgid is NULL\n");
	int ret = -1;
	int exist = 0;
	sqlite3 *pkginfo_db = NULL;
	char *query = NULL;

	ret = db_util_open(MANIFEST_DB, &pkginfo_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*Begin transaction*/
	ret = sqlite3_exec(pkginfo_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Failed to begin transaction\n");
	_LOGD("Transaction Begin\n");

	/*validate pkgid*/
	query = sqlite3_mprintf("select exists(select * from package_info where package=%Q)", pkgid);
	ret = __exec_db_query(pkginfo_db, query, _pkgmgrinfo_validate_cb, (void *)&exist);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "sqlite3_exec[%s] fail", pkgid);
	if (exist == 0) {
		_LOGS("pkgid[%s] not found in DB", pkgid);
		ret = PMINFO_R_ERROR;
		goto catch;
	}
	sqlite3_free(query);

	query = sqlite3_mprintf("update package_info set package_preload=%Q where package=%Q", preload?"true":"false", pkgid);

	ret = sqlite3_exec(pkginfo_db, query, NULL, NULL, NULL);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Don't execute query = %s\n", query);

	/*Commit transaction*/
	ret = sqlite3_exec(pkginfo_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to commit transaction. Rollback now\n");
		ret = sqlite3_exec(pkginfo_db, "ROLLBACK", NULL, NULL, NULL);
		tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Don't execute query = %s\n", query);
	}
	_LOGD("Transaction Commit and End\n");

	ret = PMINFO_R_OK;
catch:
	sqlite3_close(pkginfo_db);
	sqlite3_free(query);
	return ret;
}


API int pkgmgrinfo_pkginfo_set_installed_storage(const char *pkgid, INSTALL_LOCATION location)
{
	retvm_if(pkgid == NULL, PMINFO_R_EINVAL, "pkgid is NULL\n");
	int ret = -1;
	int exist = 0;
	sqlite3 *pkginfo_db = NULL;
	char *query = NULL;

	ret = db_util_open(MANIFEST_DB, &pkginfo_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*Begin transaction*/
	// Setting Manifest DB
	ret = sqlite3_exec(pkginfo_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Failed to begin transaction\n");
	_LOGD("Transaction Begin\n");

	/*validate pkgid*/
	query = sqlite3_mprintf("select exists(select * from package_info where package=%Q)", pkgid);
	ret = __exec_db_query(pkginfo_db, query, _pkgmgrinfo_validate_cb, (void *)&exist);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "sqlite3_exec[%s] fail", pkgid);
	if (exist == 0) {
		_LOGS("pkgid[%s] not found in DB", pkgid);
		ret = PMINFO_R_ERROR;
		goto catch;
	}
	sqlite3_free(query);

	query = sqlite3_mprintf("select exists(select * from package_app_info where package=%Q)", pkgid);
	ret = __exec_db_query(pkginfo_db, query, _pkgmgrinfo_validate_cb, (void *)&exist);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "sqlite3_exec[%s] fail", pkgid);
	if (exist == 0) {
		_LOGS("pkgid[%s] not found in DB", pkgid);
		ret = PMINFO_R_ERROR;
		goto catch;
	}
	sqlite3_free(query);

	// pkgcakge_info table
	query = sqlite3_mprintf("update package_info set installed_storage=%Q where package=%Q", location?"installed_external":"installed_internal", pkgid);

	ret = sqlite3_exec(pkginfo_db, query, NULL, NULL, NULL);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Don't execute query = %s\n", query);
	sqlite3_free(query);

	// package_app_info table
	query = sqlite3_mprintf("update package_app_info set app_installed_storage=%Q where package=%Q", location?"installed_external":"installed_internal", pkgid);

	ret = sqlite3_exec(pkginfo_db, query, NULL, NULL, NULL);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Don't execute query = %s\n", query);

	// Setting AIL DB
	ret = __ail_set_installed_storage(pkgid, location);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "Setting AIL DB failed.");

	/*Commit transaction*/
	ret = sqlite3_exec(pkginfo_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to commit transaction. Rollback now\n");
		ret = sqlite3_exec(pkginfo_db, "ROLLBACK", NULL, NULL, NULL);
		tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Don't execute query = %s\n", query);
	}
	_LOGD("Transaction Commit and End\n");

	ret = PMINFO_R_OK;
catch:
	sqlite3_close(pkginfo_db);
	sqlite3_free(query);
	return ret;
}

API int pkgmgrinfo_appinfo_set_state_enabled(const char *appid, bool enabled)
{
	retvm_if(appid == NULL, PMINFO_R_EINVAL, "appid is NULL\n");
	int ret = -1;
	int exist = 0;
	sqlite3 *pkginfo_db = NULL;
	char *query = NULL;

	ret = db_util_open(MANIFEST_DB, &pkginfo_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*Begin transaction*/
	ret = sqlite3_exec(pkginfo_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Failed to begin transaction\n");
	_LOGD("Transaction Begin\n");

	/*validate appid*/
	query = sqlite3_mprintf("select exists(select * from package_app_info where app_id=%Q)", appid);
	ret = __exec_db_query(pkginfo_db, query, _pkgmgrinfo_validate_cb, (void *)&exist);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "sqlite3_exec[%s] fail", appid);
	if (exist == 0) {
		_LOGS("appid[%s] not found in DB", appid);
		ret = PMINFO_R_ERROR;
		goto catch;
	}
	sqlite3_free(query);

	query = sqlite3_mprintf("update package_app_info set app_enabled=%Q where app_id=%Q", enabled?"true":"false", appid);

	ret = sqlite3_exec(pkginfo_db, query, NULL, NULL, NULL);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Don't execute query = %s\n", query);

	/*Commit transaction*/
	ret = sqlite3_exec(pkginfo_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to commit transaction. Rollback now\n");
		ret = sqlite3_exec(pkginfo_db, "ROLLBACK", NULL, NULL, NULL);
		tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Don't execute query = %s\n", query);
	}
	_LOGD("Transaction Commit and End\n");

	ret = PMINFO_R_OK;
catch:
	sqlite3_close(pkginfo_db);
	sqlite3_free(query);
	return ret;
}

API int pkgmgrinfo_appinfo_set_default_label(const char *appid, const char *label)
{
	retvm_if(appid == NULL, PMINFO_R_EINVAL, "appid is NULL\n");
	int ret = -1;
	int exist = 0;
	sqlite3 *pkginfo_db = NULL;
	char *query = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &pkginfo_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*Begin transaction*/
	ret = sqlite3_exec(pkginfo_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Failed to begin transaction\n");
	_LOGD("Transaction Begin\n");

	/*validate appid*/
	query = sqlite3_mprintf("select exists(select * from package_app_localized_info where app_id=%Q)", appid);
	ret = __exec_db_query(pkginfo_db, query, _pkgmgrinfo_validate_cb, (void *)&exist);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "sqlite3_exec[%s] fail", appid);
	if (exist == 0) {
		_LOGS("appid[%s] not found in DB", appid);
		ret = PMINFO_R_ERROR;
		goto catch;
	}
	sqlite3_free(query);

	query = sqlite3_mprintf("update package_app_localized_info set app_label=%Q where app_id=%Q", label, appid);

	ret = sqlite3_exec(pkginfo_db, query, NULL, NULL, NULL);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Don't execute query = %s\n", query);

	/*Commit transaction*/
	ret = sqlite3_exec(pkginfo_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to commit transaction. Rollback now\n");
		ret = sqlite3_exec(pkginfo_db, "ROLLBACK", NULL, NULL, NULL);
		tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Don't execute query = %s\n", query);
	}
	_LOGD("Transaction Commit and End\n");

	ret = PMINFO_R_OK;
catch:
	sqlite3_free(query);
	sqlite3_close(pkginfo_db);
	return ret;
}

