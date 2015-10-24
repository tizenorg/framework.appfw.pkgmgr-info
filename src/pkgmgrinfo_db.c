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
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <vasum.h>
#include "pkgmgrinfo_private.h"
#include "pkgmgrinfo_debug.h"
#include "pkgmgr-info.h"

#include "pkgmgr_parser.h"
#include "pkgmgr_parser_db.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_DB"

#define OPT_USR_APPS		__pkgmgrinfo_get_apps_path()

#define _DEFAULT_MANIFEST_DB		"/opt/dbspace/.pkgmgr_parser.db"
#define _DEFAULT_CERT_DB			"/opt/dbspace/.pkgmgr_cert.db"
#define _DEFAULT_DATACONTROL_DB		"/opt/usr/dbspace/.app-package.db"
#define _DEFAULT_SD_PATH			"/opt/storage/sdcard/app2sd/"
#define _DEFAULT_OPT_USR_APPS		"/opt/usr/apps"

static char *_manifest_db = NULL;
static char *_cert_db = NULL;
static char *_data_control_db = NULL;
static char *_sd_path = NULL;
static char *_apps_path = NULL;
static char *_cur_zone = NULL;

static const char *__pkgmgrinfo_get_apps_path();

const char *__pkgmgrinfo_get_default_manifest_db()
{
	if (_manifest_db == NULL)
		return _DEFAULT_MANIFEST_DB;

	return _manifest_db;
}

const char *__pkgmgrinfo_get_default_cert_db()
{
	if (_cert_db == NULL)
		return _DEFAULT_CERT_DB;

	return _cert_db;
}

const char *__pkgmgrinfo_get_default_data_control_db()
{
	if (_data_control_db == NULL)
		return _DEFAULT_DATACONTROL_DB;

	return _data_control_db;
}

const char *__pkgmgrinfo_get_default_sd_path()
{
	if (_sd_path == NULL)
		return _DEFAULT_SD_PATH;

	return _sd_path;
}

static const char *__pkgmgrinfo_get_apps_path()
{
	if (_apps_path == NULL)
		return _DEFAULT_OPT_USR_APPS;

	return _apps_path;
}

API int pkgmgrinfo_pkginfo_set_zone(const char *zone, char *old_zone, int len)
{
	retvm_if(vsm_is_virtualized() != 0, PMINFO_R_EINVAL, "Not host side\n");
	char db_path[PKG_STRING_LEN_MAX] = { '\0', };

	if (old_zone != NULL && _cur_zone != NULL) {
		old_zone[len - 1] = '\0';
		strncpy(old_zone, _cur_zone, len - 1);
	} else if(old_zone != NULL && _cur_zone == NULL)
		old_zone[0] = '\0';

	if (_cur_zone)
		free(_cur_zone);

	if (zone == NULL || zone[0] == '\0') {
		if (_manifest_db != NULL)
			free(_manifest_db);
		_manifest_db = NULL;

		if (_cert_db != NULL)
			free(_cert_db);
		_cert_db = NULL;

		if (_data_control_db != NULL)
			free(_data_control_db);
		_data_control_db = NULL;

		if (_sd_path != NULL)
			free(_sd_path);
		_sd_path = NULL;

		if (_apps_path != NULL)
			free(_apps_path);
		_apps_path = NULL;
		_cur_zone = NULL;
		return PMINFO_R_OK;
	}

	_cur_zone = strdup(zone);
	snprintf(db_path, PKG_STRING_LEN_MAX, "/var/lib/lxc/%s/rootfs%s",
			zone, _DEFAULT_MANIFEST_DB);

	if (_manifest_db != NULL)
		free(_manifest_db);
	_manifest_db = strdup(db_path);

	snprintf(db_path, PKG_STRING_LEN_MAX, "/var/lib/lxc/%s/rootfs%s",
			zone, _DEFAULT_CERT_DB);

	if (_cert_db != NULL)
		free(_cert_db);
	_cert_db = strdup(db_path);

	snprintf(db_path, PKG_STRING_LEN_MAX, "/var/lib/lxc/%s/rootfs%s",
			zone, _DEFAULT_DATACONTROL_DB);

	if (_data_control_db != NULL)
		free(_data_control_db);
	_data_control_db = strdup(db_path);

	snprintf(db_path, PKG_STRING_LEN_MAX, "/var/lib/lxc/%s/rootfs%s",
			zone, _DEFAULT_SD_PATH);

	if (_sd_path != NULL)
		free(_sd_path);
	_sd_path = strdup(db_path);

	snprintf(db_path, PKG_STRING_LEN_MAX, "/var/lib/lxc/%s/rootfs%s",
			zone, _DEFAULT_OPT_USR_APPS);

	if (_apps_path != NULL)
		free(_apps_path);
	_apps_path = strdup(db_path);

	return PMINFO_R_OK;
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

	ret = _pminfo_db_open_with_options(MANIFEST_DB, &pkginfo_db, SQLITE_OPEN_READWRITE);
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

	ret = _pminfo_db_open_with_options(MANIFEST_DB, &pkginfo_db, SQLITE_OPEN_READWRITE);
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

	ret = _pminfo_db_open_with_options(MANIFEST_DB, &pkginfo_db, SQLITE_OPEN_READWRITE);
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
	ret = _pminfo_db_open_with_options(MANIFEST_DB, &pkginfo_db, SQLITE_OPEN_READWRITE);
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

