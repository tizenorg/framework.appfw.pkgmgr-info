/*
 * pkgmgrinfo-feature
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

#include "pkgmgrinfo_private.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_INFO"

typedef int (*pkgmgr_handler)(int req_id, const char *pkg_type,
				const char *pkgid, const char *key,
				const char *val, const void *pmsg, void *data);

typedef void pkgmgr_client;
typedef void pkgmgr_info;

typedef enum {
	PM_REQUEST_CSC = 0,
	PM_REQUEST_MOVE = 1,
	PM_REQUEST_GET_SIZE = 2,
	PM_REQUEST_KILL_APP = 3,
	PM_REQUEST_CHECK_APP = 4,
	PM_REQUEST_MAX
}pkgmgr_request_service_type;

typedef enum {
	PM_GET_TOTAL_SIZE= 0,
	PM_GET_DATA_SIZE = 1,
	PM_GET_ALL_PKGS = 2,
	PM_GET_SIZE_INFO = 3,
	PM_GET_TOTAL_AND_DATA = 4,
	PM_GET_SIZE_FILE = 5,
	PM_GET_MAX
}pkgmgr_getsize_type;

typedef enum {
	PC_REQUEST = 0,
	PC_LISTENING,
	PC_BROADCAST,
}client_type;

static int __pkg_list_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_pkginfo_x *udata = (pkgmgr_pkginfo_x *)data;
	int i = 0;
	pkgmgr_pkginfo_x *info = NULL;
	info = calloc(1, sizeof(pkgmgr_pkginfo_x));
	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));

	LISTADD(udata, info);
	for(i = 0; i < ncols; i++)
	{
		if (strcmp(colname[i], "package") == 0) {
			if (coltxt[i])
				info->manifest_info->package = strdup(coltxt[i]);
			else
				info->manifest_info->package = NULL;
		} else
			continue;
	}

	return 0;
}

API int pkgmgrinfo_pkginfo_get_disabled_pkginfo(const char *pkgid, pkgmgrinfo_pkginfo_h *handle)
{
	retvm_if(pkgid == NULL, PMINFO_R_EINVAL, "pkgid is NULL\n");
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *pkginfo = NULL;
	int ret = PMINFO_R_OK;
	char *query = NULL;
	char *locale = NULL;
	label_x *tmp1 = NULL;
	icon_x *tmp2 = NULL;
	description_x *tmp3 = NULL;
	author_x *tmp4 = NULL;
	privilege_x *tmp5 = NULL;
	sqlite3 *pkginfo_db = NULL;

	/*validate pkgid*/
	ret = db_util_open(MANIFEST_DB, &pkginfo_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*get system locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "manifest locale is NULL");

	pkginfo = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(pkginfo == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for pkginfo");

	pkginfo->locale = strdup(locale);

	pkginfo->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(pkginfo->manifest_info == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for manifest info");

	pkginfo->manifest_info->package = strdup(pkgid);
	pkginfo->manifest_info->privileges = (privileges_x *)calloc(1, sizeof(privileges_x));
	tryvm_if(pkginfo->manifest_info->privileges == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for privileges info");

	/*populate manifest_info from DB*/
	query= sqlite3_mprintf("select * from disabled_package_info where package=%Q", pkgid);
	ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");

	query= sqlite3_mprintf("select * from disabled_package_localized_info where package=%Q and package_locale=%Q", pkgid, locale);
	ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");

	/*Also store the values corresponding to default locales*/
	query= sqlite3_mprintf("select * from disabled_package_localized_info where package=%Q and package_locale=%Q", pkgid, DEFAULT_LOCALE);
	ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");

	SAFE_LISTHEAD(pkginfo->manifest_info->label, tmp1);
	SAFE_LISTHEAD(pkginfo->manifest_info->icon, tmp2);
	SAFE_LISTHEAD(pkginfo->manifest_info->description, tmp3);
	SAFE_LISTHEAD(pkginfo->manifest_info->author, tmp4);
	SAFE_LISTHEAD(pkginfo->manifest_info->privileges->privilege, tmp5);

catch:
	if (ret == PMINFO_R_OK)
		*handle = (void*)pkginfo;
	else {
		*handle = NULL;
		__cleanup_pkginfo(pkginfo);
	}
	sqlite3_close(pkginfo_db);

	if (locale) {
		free(locale);
		locale = NULL;
	}
	return ret;
}

API int pkgmgrinfo_pkginfo_get_disabled_list(pkgmgrinfo_pkg_list_cb pkg_list_cb, void *user_data)
{
	retvm_if(pkg_list_cb == NULL, PMINFO_R_EINVAL, "callback function is NULL\n");
	int ret = PMINFO_R_OK;
	char query[MAX_QUERY_LEN] = {'\0'};
	char *locale = NULL;
	pkgmgr_pkginfo_x *pkginfo = NULL;
	label_x *tmp1 = NULL;
	icon_x *tmp2 = NULL;
	description_x *tmp3 = NULL;
	author_x *tmp4 = NULL;
	sqlite3 *pkginfo_db = NULL;
	pkgmgr_pkginfo_x *tmphead = NULL;
	pkgmgr_pkginfo_x *node = NULL;
	pkgmgr_pkginfo_x *temp_node = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &pkginfo_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*get system locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "manifest locale is NULL");

	tmphead = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	if(tmphead == NULL){
		_LOGE("@calloc failed!!");
		FREE_AND_NULL(locale);
		return PMINFO_R_ERROR;
	}

	snprintf(query, MAX_QUERY_LEN, "select * from disabled_package_info");
	ret = __exec_db_query(pkginfo_db, query, __pkg_list_cb, (void *)tmphead);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");

	LISTHEAD(tmphead, node);

	for(node = node->next; node ; node = node->next) {
		pkginfo = node;
		pkginfo->locale = strdup(locale);
		pkginfo->manifest_info->privileges = (privileges_x *)calloc(1, sizeof(privileges_x));
		tryvm_if(pkginfo->manifest_info->privileges == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for privileges info\n");

		/*populate manifest_info from DB*/
		snprintf(query, MAX_QUERY_LEN, "select * from disabled_package_info where package='%s' ", pkginfo->manifest_info->package);
		ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");

		memset(query, '\0', MAX_QUERY_LEN);
		snprintf(query, MAX_QUERY_LEN, "select * from disabled_package_localized_info where" \
			" package='%s' and package_locale='%s'", pkginfo->manifest_info->package, locale);
		ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");

		/*Also store the values corresponding to default locales*/
		memset(query, '\0', MAX_QUERY_LEN);
		snprintf(query, MAX_QUERY_LEN, "select * from disabled_package_localized_info where" \
			" package='%s' and package_locale='%s'", pkginfo->manifest_info->package, DEFAULT_LOCALE);
		ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");

		SAFE_LISTHEAD(pkginfo->manifest_info->label, tmp1);
		SAFE_LISTHEAD(pkginfo->manifest_info->icon, tmp2);
		SAFE_LISTHEAD(pkginfo->manifest_info->description, tmp3);
		SAFE_LISTHEAD(pkginfo->manifest_info->author, tmp4);
	}

	LISTHEAD(tmphead, node);

	for(node = node->next; node ; node = node->next) {
		pkginfo = node;

		ret = pkg_list_cb( (void *)pkginfo, user_data);
		if(ret < 0)
			break;
	}

	ret = PMINFO_R_OK;

catch:
	sqlite3_close(pkginfo_db);
	if (locale) {
		free(locale);
		locale = NULL;
	}
	if (tmphead) {
		LISTHEAD(tmphead, node);
		temp_node = node->next;
		node = temp_node;
		while (node) {
			temp_node = node->next;
			__cleanup_pkginfo(node);
			node = temp_node;
		}
		__cleanup_pkginfo(tmphead);
	}
	return ret;
}

API int pkgmgrinfo_appinfo_get_disabled_appinfo(const char *appid, pkgmgrinfo_appinfo_h *handle)
{
	retvm_if(appid == NULL, PMINFO_R_EINVAL, "appid is NULL");
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");

	pkgmgr_appinfo_x *appinfo = NULL;
	char *locale = NULL;
	int ret = -1;
	int exist = 0;
	label_x *tmp1 = NULL;
	icon_x *tmp2 = NULL;
	category_x *tmp3 = NULL;
	metadata_x *tmp4 = NULL;

	char *query = NULL;
	sqlite3 *appinfo_db = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	retvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*check appid exist on db*/
	query = sqlite3_mprintf("select exists(select * from disabled_package_app_info where app_id=%Q)", appid);
	ret = __exec_db_query(appinfo_db, query, _pkgmgrinfo_validate_cb, (void *)&exist);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "sqlite3_exec fail");
	tryvm_if(exist == 0, ret = PMINFO_R_ERROR, "Appid[%s] not found in DB", appid);

	/*get system locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "manifest locale is NULL");

	/*calloc appinfo*/
	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for appinfo");

	/*calloc app_component*/
	appinfo->uiapp_info = (uiapplication_x *)calloc(1, sizeof(uiapplication_x));
	tryvm_if(appinfo->uiapp_info == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for uiapp info");

	appinfo->locale = strdup(locale);

	/*populate app_info from DB*/
	query = sqlite3_mprintf("select * from disabled_package_app_info where app_id=%Q ", appid);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	query = sqlite3_mprintf("select * from disabled_package_app_localized_info where app_id=%Q and app_locale=%Q", appid, locale);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	/*Also store the values corresponding to default locales*/
	query = sqlite3_mprintf("select * from disabled_package_app_localized_info where app_id=%Q and app_locale=%Q", appid, DEFAULT_LOCALE);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

	/*Populate app category*/
	query = sqlite3_mprintf("select * from disabled_package_app_app_category where app_id=%Q", appid);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Category Info DB Information retrieval failed");

	/*Populate app metadata*/
	query = sqlite3_mprintf("select * from disabled_package_app_app_metadata where app_id=%Q", appid);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Metadata Info DB Information retrieval failed");

	SAFE_LISTHEAD(appinfo->uiapp_info->label, tmp1);
	SAFE_LISTHEAD(appinfo->uiapp_info->icon, tmp2);
	SAFE_LISTHEAD(appinfo->uiapp_info->category, tmp3);
	SAFE_LISTHEAD(appinfo->uiapp_info->metadata, tmp4);

	ret = PMINFO_R_OK;

catch:
	if (ret == PMINFO_R_OK)
		*handle = (void*)appinfo;
	else {
		*handle = NULL;
		__cleanup_appinfo(appinfo);
	}

	sqlite3_close(appinfo_db);
	if (locale) {
		free(locale);
		locale = NULL;
	}
	return ret;
}

API int pkgmgrinfo_appinfo_get_disabled_list(pkgmgrinfo_pkginfo_h handle, pkgmgrinfo_app_component component,
						pkgmgrinfo_app_list_cb app_func, void *user_data)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL");
	retvm_if(app_func == NULL, PMINFO_R_EINVAL, "callback pointer is NULL");
	retvm_if((component != PMINFO_UI_APP) && (component != PMINFO_SVC_APP) && (component != PMINFO_ALL_APP), PMINFO_R_EINVAL, "Invalid App Component Type");

	char *locale = NULL;
	int ret = -1;
	char query[MAX_QUERY_LEN] = {'\0'};
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	pkgmgr_pkginfo_x *allinfo = NULL;
	pkgmgr_appinfo_x *appinfo = NULL;
	icon_x *ptr1 = NULL;
	label_x *ptr2 = NULL;
	category_x *ptr3 = NULL;
	metadata_x *ptr4 = NULL;
	permission_x *ptr5 = NULL;
	image_x *ptr6 = NULL;
	sqlite3 *appinfo_db = NULL;

	/*check installed storage*/
	ret = __pkginfo_check_installed_storage(info);
	retvm_if(ret < 0, PMINFO_R_EINVAL, "[%s] is installed external, but is not in mmc", info->manifest_info->package);

	/*get system locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_EINVAL, "manifest locale is NULL");

	/*calloc allinfo*/
	allinfo = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(allinfo == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for appinfo");

	/*calloc manifest_info*/
	allinfo->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(allinfo->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*calloc appinfo*/
	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for appinfo");

	/*open db */
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	snprintf(query, MAX_QUERY_LEN, "select DISTINCT * from disabled_package_app_info where package='%s'", info->manifest_info->package);

		/*Populate ui app info */
		ret = __exec_db_query(appinfo_db, query, __uiapp_list_cb, (void *)info);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info list retrieval failed");

		uiapplication_x *tmp = NULL;
		SAFE_LISTHEAD(info->manifest_info->uiapplication, tmp);

		/*Populate localized info for default locales and call callback*/
		/*If the callback func return < 0 we break and no more call back is called*/
		while(tmp != NULL)
		{
			appinfo->locale = strdup(locale);
			appinfo->uiapp_info = tmp;
			if (strcmp(appinfo->uiapp_info->type,"c++app") == 0){
				if (locale) {
					free(locale);
				}
				locale = __get_app_locale_by_fallback(appinfo_db, appinfo->uiapp_info->appid);
			}

			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from disabled_package_app_localized_info where app_id='%s' and app_locale='%s'", appinfo->uiapp_info->appid, locale);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from disabled_package_app_localized_info where app_id='%s' and app_locale='%s'", appinfo->uiapp_info->appid, DEFAULT_LOCALE);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

			/*Populate app category*/
			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from disabled_package_app_app_category where app_id=='%s'", appinfo->uiapp_info->appid);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Category Info DB Information retrieval failed");

			/*Populate app metadata*/
			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from disabled_package_app_app_metadata where app_id='%s'", appinfo->uiapp_info->appid);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Metadata Info DB Information retrieval failed");

			SAFE_LISTHEAD(appinfo->uiapp_info->icon, ptr1);
			SAFE_LISTHEAD(appinfo->uiapp_info->label, ptr2);
			SAFE_LISTHEAD(appinfo->uiapp_info->category, ptr3);
			SAFE_LISTHEAD(appinfo->uiapp_info->metadata, ptr4);
			SAFE_LISTHEAD(appinfo->uiapp_info->permission, ptr5);
			SAFE_LISTHEAD(appinfo->uiapp_info->image, ptr6);

			ret = app_func((void *)appinfo, user_data);
			if (ret < 0)
				break;
			tmp = tmp->next;
		}

	ret = PMINFO_R_OK;
catch:
	if (locale) {
		free(locale);
		locale = NULL;
	}
	if (appinfo) {
		free(appinfo);
		appinfo = NULL;
	}
	__cleanup_pkginfo(allinfo);

	sqlite3_close(appinfo_db);
	return ret;
}
