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

#include "pkgmgr-info-internal.h"
#include "pkgmgr-info-debug.h"
#include "pkgmgr-info.h"

#include "pkgmgr_parser.h"
#include "pkgmgr_parser_db.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_DB"

typedef struct _pkgmgr_datacontrol_x {
	char *appid;
	char *access;
} pkgmgr_datacontrol_x;

static int __datacontrol_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_datacontrol_x *info = (pkgmgr_datacontrol_x *)data;
	int i = 0;
	for(i = 0; i < ncols; i++)
	{
		if (strcmp(colname[i], "PACKAGE_NAME") == 0) {
			if (coltxt[i])
				info->appid = strdup(coltxt[i]);
			else
				info->appid = NULL;
		} else if (strcmp(colname[i], "ACCESS") == 0 ){
			if (coltxt[i])
				info->access = strdup(coltxt[i]);
			else
				info->access = NULL;
		} else
			continue;
	}
	return 0;
}

static int __update_ail_appinfo(manifest_x * mfx)
{
	int ret = -1;
	uiapplication_x *uiapplication = mfx->uiapplication;
	void *lib_handle = NULL;
	int (*ail_desktop_operation) (const char *appid, const char *property, const char *value, bool broadcast);
	char *aop = NULL;

	if ((lib_handle = dlopen(LIBAIL_PATH, RTLD_LAZY)) == NULL) {
		_LOGE("dlopen is failed LIBAIL_PATH[%s]\n", LIBAIL_PATH);
		goto END;
	}

	aop  = "ail_desktop_appinfo_modify_str";

	if ((ail_desktop_operation =
	     dlsym(lib_handle, aop)) == NULL || dlerror() != NULL) {
		_LOGE("can not find symbol \n");
		goto END;
	}

	for(; uiapplication; uiapplication=uiapplication->next) {
		ret = ail_desktop_operation(uiapplication->appid, "AIL_PROP_X_SLP_INSTALLEDSTORAGE_STR", mfx->installed_storage, false);
		if (ret != 0)
			_LOGE("Failed to store info in DB\n");
	}

END:
	if (lib_handle)
		dlclose(lib_handle);

	return ret;
}

API int pkgmgrinfo_create_pkgdbinfo(const char *pkgid, pkgmgrinfo_pkgdbinfo_h *handle)
{
	retvm_if(!pkgid, PMINFO_R_EINVAL, "pkgid is NULL");
	retvm_if(!handle, PMINFO_R_EINVAL, "Argument supplied is NULL");

	char *manifest = NULL;
	manifest_x *mfx = NULL;

	manifest = pkgmgr_parser_get_manifest_file(pkgid);
	retvm_if(manifest == NULL, PMINFO_R_EINVAL, "pkg[%s] dont have manifest file", pkgid);

	mfx = pkgmgr_parser_process_manifest_xml(manifest);
	if (manifest) {
		free(manifest);
		manifest = NULL;
	}
	retvm_if(mfx == NULL, PMINFO_R_EINVAL, "pkg[%s] parsing fail", pkgid);

	*handle = (void *)mfx;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_set_type_to_pkgdbinfo(pkgmgrinfo_pkgdbinfo_h handle, const char *type)
{
	retvm_if(!type, PMINFO_R_EINVAL, "Argument supplied is NULL");
	retvm_if(!handle, PMINFO_R_EINVAL, "Argument supplied is NULL");

	int len = strlen(type);
	retvm_if(len > PKG_TYPE_STRING_LEN_MAX, PMINFO_R_EINVAL, "pkg type length exceeds the max limit");

	manifest_x *mfx = (manifest_x *)handle;
	free((void *)mfx->type);

	mfx->type = strndup(type, PKG_TYPE_STRING_LEN_MAX);
	return PMINFO_R_OK;
}

API int pkgmgrinfo_set_version_to_pkgdbinfo(pkgmgrinfo_pkgdbinfo_h handle, const char *version)
{
	retvm_if(!version, PMINFO_R_EINVAL, "Argument supplied is NULL");
	retvm_if(!handle, PMINFO_R_EINVAL, "Argument supplied is NULL");

	int len = strlen(version);
	retvm_if(len > PKG_TYPE_STRING_LEN_MAX, PMINFO_R_EINVAL, "pkg type length exceeds the max limit");

	manifest_x *mfx = (manifest_x *)handle;
	free((void *)mfx->version);

	mfx->version = strndup(version, PKG_VERSION_STRING_LEN_MAX);
	return PMINFO_R_OK;
}

API int pkgmgrinfo_set_install_location_to_pkgdbinfo(pkgmgrinfo_pkgdbinfo_h handle, INSTALL_LOCATION location)
{
	retvm_if(!handle, PMINFO_R_EINVAL, "Argument supplied is NULL");
	retvm_if((location < 0) || (location > 1), PMINFO_R_EINVAL, "Argument supplied is NULL");

	manifest_x *mfx = (manifest_x *)handle;
	free((void *)mfx->installlocation);

	if (location == INSTALL_INTERNAL)
		mfx->installlocation= strdup("internal-only");
	else if (location == INSTALL_EXTERNAL)
		mfx->installlocation= strdup("prefer-external");

	return PMINFO_R_OK;
}

API int pkgmgrinfo_set_size_to_pkgdbinfo(pkgmgrinfo_pkgdbinfo_h handle, const char *size)
{
	retvm_if(!handle, PMINFO_R_EINVAL, "Argument supplied is NULL");
	retvm_if(size == NULL, PMINFO_R_EINVAL, "Argument supplied is NULL");

	manifest_x *mfx = (manifest_x *)handle;

	mfx->package_size = strdup(size);

	return PMINFO_R_OK;
}

API int pkgmgrinfo_set_label_to_pkgdbinfo(pkgmgrinfo_pkgdbinfo_h handle, const char *label_txt, const char *locale)
{
	retvm_if(!handle, PMINFO_R_EINVAL, "Argument supplied is NULL");
	retvm_if(!label_txt, PMINFO_R_EINVAL, "Argument supplied is NULL");

	int len = strlen(label_txt);
	retvm_if(len > PKG_TYPE_STRING_LEN_MAX, PMINFO_R_EINVAL, "pkg type length exceeds the max limit");

	manifest_x *mfx = (manifest_x *)handle;

	label_x *label = calloc(1, sizeof(label_x));
	retvm_if(label == NULL, PMINFO_R_EINVAL, "Malloc Failed");

	LISTADD(mfx->label, label);
	if (locale)
		mfx->label->lang = strdup(locale);
	else
		mfx->label->lang = strdup(DEFAULT_LOCALE);
	mfx->label->text = strdup(label_txt);

	return PMINFO_R_OK;
}

API int pkgmgrinfo_set_icon_to_pkgdbinfo(pkgmgrinfo_pkgdbinfo_h handle, const char *icon_txt, const char *locale)
{
	retvm_if(!handle, PMINFO_R_EINVAL, "Argument supplied is NULL");
	retvm_if(!icon_txt, PMINFO_R_EINVAL, "Argument supplied is NULL");

	int len = strlen(icon_txt);
	retvm_if(len > PKG_TYPE_STRING_LEN_MAX, PMINFO_R_EINVAL, "pkg type length exceeds the max limit");

	manifest_x *mfx = (manifest_x *)handle;

	icon_x *icon = calloc(1, sizeof(icon_x));
	retvm_if(icon == NULL, PMINFO_R_EINVAL, "Malloc Failed");

	LISTADD(mfx->icon, icon);
	if (locale)
		mfx->icon->lang = strdup(locale);
	else
		mfx->icon->lang = strdup(DEFAULT_LOCALE);
	mfx->icon->text = strdup(icon_txt);

	return PMINFO_R_OK;
}

API int pkgmgrinfo_set_description_to_pkgdbinfo(pkgmgrinfo_pkgdbinfo_h handle, const char *desc_txt, const char *locale)
{
	retvm_if(!handle, PMINFO_R_EINVAL, "Argument supplied is NULL");
	retvm_if(!desc_txt, PMINFO_R_EINVAL, "Argument supplied is NULL");

	int len = strlen(desc_txt);
	retvm_if(len > PKG_TYPE_STRING_LEN_MAX, PMINFO_R_EINVAL, "pkg type length exceeds the max limit");

	manifest_x *mfx = (manifest_x *)handle;

	description_x *description = calloc(1, sizeof(description_x));
	retvm_if(description == NULL, PMINFO_R_EINVAL, "Malloc Failed");

	LISTADD(mfx->description, description);
	if (locale)
		mfx->description->lang = strdup(locale);
	else
		mfx->description->lang = strdup(DEFAULT_LOCALE);
	mfx->description->text = strdup(desc_txt);

	return PMINFO_R_OK;
}

API int pkgmgrinfo_set_author_to_pkgdbinfo(pkgmgrinfo_pkgdbinfo_h handle, const char *author_name,
		const char *author_email, const char *author_href, const char *locale)
{
	retvm_if(!handle, PMINFO_R_EINVAL, "Argument supplied is NULL");
	manifest_x *mfx = (manifest_x *)handle;
	author_x *author = calloc(1, sizeof(author_x));
	retvm_if(author == NULL, PMINFO_R_EINVAL, "Argument supplied is NULL");

	LISTADD(mfx->author, author);
	if (author_name)
		mfx->author->text = strdup(author_name);
	if (author_email)
		mfx->author->email = strdup(author_email);
	if (author_href)
		mfx->author->href = strdup(author_href);
	if (locale)
		mfx->author->lang = strdup(locale);
	else
		mfx->author->lang = strdup(DEFAULT_LOCALE);
	return PMINFO_R_OK;
}

API int pkgmgrinfo_set_removable_to_pkgdbinfo(pkgmgrinfo_pkgdbinfo_h handle, int removable)
{
	retvm_if(!handle, PMINFO_R_EINVAL, "Argument supplied is NULL");
	retvm_if((removable < 0) || (removable > 1), PMINFO_R_EINVAL, "Argument supplied is NULL");

	manifest_x *mfx = (manifest_x *)handle;
	free((void *)mfx->removable);

	if (removable == 0)
		mfx->removable= strdup("false");
	else if (removable == 1)
		mfx->removable= strdup("true");

	return PMINFO_R_OK;
}

API int pkgmgrinfo_set_preload_to_pkgdbinfo(pkgmgrinfo_pkgdbinfo_h handle, int preload)
{
	retvm_if(!handle, PMINFO_R_EINVAL, "Argument supplied is NULL");
	retvm_if((preload < 0) || (preload > 1), PMINFO_R_EINVAL, "Argument supplied is NULL");

	manifest_x *mfx = (manifest_x *)handle;

	free((void *)mfx->preload);

	if (preload == 0)
		mfx->preload= strdup("false");
	else if (preload == 1)
		mfx->preload= strdup("true");

	return PMINFO_R_OK;
}

API int pkgmgrinfo_set_installed_storage_to_pkgdbinfo(pkgmgrinfo_pkgdbinfo_h handle, INSTALL_LOCATION location)
{
	retvm_if(!handle, PMINFO_R_EINVAL, "Argument supplied is NULL");
	retvm_if((location < 0) || (location > 1), PMINFO_R_EINVAL, "Argument supplied is NULL");

	manifest_x *mfx = (manifest_x *)handle;

	free((void *)mfx->installed_storage);

	if (location == INSTALL_INTERNAL)
		mfx->installed_storage= strdup("installed_internal");
	else if (location == INSTALL_EXTERNAL)
		mfx->installed_storage= strdup("installed_external");

	return PMINFO_R_OK;
}

API int pkgmgrinfo_save_pkgdbinfo(pkgmgrinfo_pkgdbinfo_h handle)
{
	retvm_if(!handle, PMINFO_R_EINVAL, "Argument supplied is NULL");

	int ret = 0;
	manifest_x *mfx = NULL;
	mfx = (manifest_x *)handle;

	ret = pkgmgr_parser_update_manifest_info_in_db(mfx);
	retvm_if(ret != 0, PMINFO_R_ERROR, "Failed to store info in DB\n");
	
	ret = __update_ail_appinfo(mfx);
	retvm_if(ret != 0, PMINFO_R_ERROR, "Failed to store info in DB\n");

	_LOGE("Successfully stored info in DB\n");
	return PMINFO_R_OK;
}

API int pkgmgrinfo_destroy_pkgdbinfo(pkgmgrinfo_pkgdbinfo_h handle)
{
	retvm_if(!handle, PMINFO_R_EINVAL, "Argument supplied is NULL");

	manifest_x *mfx = NULL;
	mfx = (manifest_x *)handle;
	pkgmgr_parser_free_manifest_xml(mfx);
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_set_state_enabled(const char *pkgid, bool enabled)
{
	/* Should be implemented later */
	return 0;
}

API int pkgmgrinfo_appinfo_set_state_enabled(const char *appid, bool enabled)
{
	retvm_if(appid == NULL, PMINFO_R_EINVAL, "appid is NULL\n");
	int ret = -1;
	sqlite3 *pkginfo_db = NULL;

	ret = db_util_open(MANIFEST_DB, &pkginfo_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*Begin transaction*/
	ret = sqlite3_exec(pkginfo_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to begin transaction\n");
		sqlite3_close(pkginfo_db);
		return PMINFO_R_ERROR;
	}
	_LOGD("Transaction Begin\n");

	char *query = sqlite3_mprintf("update package_app_info set app_enabled=%Q where app_id=%Q", enabled?"true":"false", appid);

	char *error_message = NULL;
	if (SQLITE_OK !=
	    sqlite3_exec(pkginfo_db, query, NULL, NULL, &error_message)) {
		_LOGE("Don't execute query = %s error message = %s\n", query,
		       error_message);
		sqlite3_free(error_message);
		sqlite3_free(query);
		return PMINFO_R_ERROR;
	}
	sqlite3_free(error_message);

	/*Commit transaction*/
	ret = sqlite3_exec(pkginfo_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to commit transaction. Rollback now\n");
		ret = sqlite3_exec(pkginfo_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Don't execute query = %s\n", query);
		sqlite3_close(pkginfo_db);
		sqlite3_free(query);
		return PMINFO_R_ERROR;
	}
	_LOGD("Transaction Commit and End\n");
	sqlite3_close(pkginfo_db);
	sqlite3_free(query);

	return PMINFO_R_OK;
}


API int pkgmgrinfo_datacontrol_get_info(const char *providerid, const char * type, char **appid, char **access)
{
	retvm_if(providerid == NULL, PMINFO_R_EINVAL, "Argument supplied is NULL\n");
	retvm_if(type == NULL, PMINFO_R_EINVAL, "Argument supplied is NULL\n");
	retvm_if(appid == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	retvm_if(access == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	int ret = PMINFO_R_OK;
	char *error_message = NULL;
	pkgmgr_datacontrol_x *data = NULL;

	sqlite3 *datacontrol_info_db = NULL;

	/*open db*/
	ret = db_util_open(DATACONTROL_DB, &datacontrol_info_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	data = (pkgmgr_datacontrol_x *)calloc(1, sizeof(pkgmgr_datacontrol_x));
	if (data == NULL) {
		_LOGE("Failed to allocate memory for pkgmgr_datacontrol_x\n");
		sqlite3_close(datacontrol_info_db);
		return PMINFO_R_ERROR;
	}
	char *query = sqlite3_mprintf("select appinfo.package_name, datacontrol.access from appinfo, datacontrol where datacontrol.id=appinfo.unique_id and datacontrol.provider_id = %Q and datacontrol.type=%Q COLLATE NOCASE",
		providerid, type);

	if (SQLITE_OK !=
		sqlite3_exec(datacontrol_info_db, query, __datacontrol_cb, (void *)data, &error_message)) {
		_LOGE("Don't execute query = %s error message = %s\n", query,
			   error_message);
		sqlite3_free(error_message);
		sqlite3_free(query);
		sqlite3_close(datacontrol_info_db);
		return PMINFO_R_ERROR;
	}

	*appid = (char *)data->appid;
	*access = (char *)data->access;
	free(data);
	sqlite3_free(query);
	sqlite3_close(datacontrol_info_db);

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_set_default_label(const char *appid, const char *label)
{
	retvm_if(appid == NULL, PMINFO_R_EINVAL, "appid is NULL\n");
	int ret = -1;
	char *error_message = NULL;
	sqlite3 *pkginfo_db = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &pkginfo_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*Begin transaction*/
	ret = sqlite3_exec(pkginfo_db, "BEGIN EXCLUSIVE", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to begin transaction\n");
		sqlite3_close(pkginfo_db);
		return PMINFO_R_ERROR;
	}
	_LOGD("Transaction Begin\n");

	char *query = sqlite3_mprintf("update package_app_localized_info set app_label=%Q where app_id=%Q", label, appid);

	ret = sqlite3_exec(pkginfo_db, query, NULL, NULL, &error_message);
	if (ret != SQLITE_OK) {
		_LOGE("Don't execute query = %s error message = %s\n", query, error_message);
		sqlite3_free(error_message);
		sqlite3_free(query);
		return PMINFO_R_ERROR;
	}

	/*Commit transaction*/
	ret = sqlite3_exec(pkginfo_db, "COMMIT", NULL, NULL, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("Failed to commit transaction. Rollback now\n");
		ret = sqlite3_exec(pkginfo_db, "ROLLBACK", NULL, NULL, NULL);
		if (ret != SQLITE_OK)
			_LOGE("Don't execute query = %s\n", query);
		sqlite3_free(query);
		sqlite3_close(pkginfo_db);
		return PMINFO_R_ERROR;
	}
	_LOGD("Transaction Commit and End\n");
	sqlite3_free(query);
	sqlite3_close(pkginfo_db);

	return PMINFO_R_OK;
}

