/*
 * pkgmgrinfo-appinfo
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

#include <vconf.h>

#include "pkgmgrinfo_private.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_INFO"

struct _pkginfo_str_map_t {
	pkgmgrinfo_pkginfo_filter_prop_str prop;
	const char *property;
};

static struct _pkginfo_str_map_t pkginfo_str_prop_map[] = {
	{E_PMINFO_PKGINFO_PROP_PACKAGE_ID,		PMINFO_PKGINFO_PROP_PACKAGE_ID},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_TYPE, 		PMINFO_PKGINFO_PROP_PACKAGE_TYPE},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_VERSION, 	PMINFO_PKGINFO_PROP_PACKAGE_VERSION},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_INSTALL_LOCATION,PMINFO_PKGINFO_PROP_PACKAGE_INSTALL_LOCATION},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_INSTALLED_STORAGE,PMINFO_PKGINFO_PROP_PACKAGE_INSTALLED_STORAGE},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_AUTHOR_NAME, 	PMINFO_PKGINFO_PROP_PACKAGE_AUTHOR_NAME},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_AUTHOR_EMAIL, 	PMINFO_PKGINFO_PROP_PACKAGE_AUTHOR_EMAIL},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_AUTHOR_HREF, 	PMINFO_PKGINFO_PROP_PACKAGE_AUTHOR_HREF},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_STORECLIENT_ID, 	PMINFO_PKGINFO_PROP_PACKAGE_STORECLIENT_ID}
};

struct _pkginfo_int_map_t {
	pkgmgrinfo_pkginfo_filter_prop_int prop;
	const char *property;
};

static struct _pkginfo_int_map_t pkginfo_int_prop_map[] = {
	{E_PMINFO_PKGINFO_PROP_PACKAGE_SIZE,	PMINFO_PKGINFO_PROP_PACKAGE_SIZE},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_SUPPORT_MODE,	PMINFO_PKGINFO_PROP_PACKAGE_SUPPORT_MODE}
};

struct _pkginfo_bool_map_t {
	pkgmgrinfo_pkginfo_filter_prop_bool prop;
	const char *property;
};

static struct _pkginfo_bool_map_t pkginfo_bool_prop_map[] = {
	{E_PMINFO_PKGINFO_PROP_PACKAGE_REMOVABLE,	PMINFO_PKGINFO_PROP_PACKAGE_REMOVABLE},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_PRELOAD,		PMINFO_PKGINFO_PROP_PACKAGE_PRELOAD},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_READONLY,	PMINFO_PKGINFO_PROP_PACKAGE_READONLY},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_UPDATE,		PMINFO_PKGINFO_PROP_PACKAGE_UPDATE},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_APPSETTING,	PMINFO_PKGINFO_PROP_PACKAGE_APPSETTING},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_NODISPLAY_SETTING,	PMINFO_PKGINFO_PROP_PACKAGE_NODISPLAY_SETTING},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_SUPPORT_DISABLE,	PMINFO_PKGINFO_PROP_PACKAGE_SUPPORT_DISABLE},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_DISABLE,	PMINFO_PKGINFO_PROP_PACKAGE_DISABLE},
	{E_PMINFO_PKGINFO_PROP_PACKAGE_SYSTEM,	PMINFO_PKGINFO_PROP_PACKAGE_SYSTEM}
};

struct _appinfo_str_map_t {
	pkgmgrinfo_appinfo_filter_prop_str prop;
	const char *property;
};

static struct _appinfo_str_map_t appinfo_str_prop_map[] = {
	{E_PMINFO_APPINFO_PROP_APP_ID,		PMINFO_APPINFO_PROP_APP_ID},
	{E_PMINFO_APPINFO_PROP_APP_COMPONENT_TYPE,	PMINFO_APPINFO_PROP_APP_COMPONENT_TYPE},
	{E_PMINFO_APPINFO_PROP_APP_EXEC, 	PMINFO_APPINFO_PROP_APP_EXEC},
	{E_PMINFO_APPINFO_PROP_APP_ICON, 	PMINFO_APPINFO_PROP_APP_ICON},
	{E_PMINFO_APPINFO_PROP_APP_TYPE, 	PMINFO_APPINFO_PROP_APP_TYPE},
	{E_PMINFO_APPINFO_PROP_APP_OPERATION, 	PMINFO_APPINFO_PROP_APP_OPERATION},
	{E_PMINFO_APPINFO_PROP_APP_URI, 	PMINFO_APPINFO_PROP_APP_URI},
	{E_PMINFO_APPINFO_PROP_APP_MIME, 	PMINFO_APPINFO_PROP_APP_MIME},
	{E_PMINFO_APPINFO_PROP_APP_CATEGORY, 	PMINFO_APPINFO_PROP_APP_CATEGORY},
	{E_PMINFO_APPINFO_PROP_APP_HWACCELERATION,	PMINFO_APPINFO_PROP_APP_HWACCELERATION},
	{E_PMINFO_APPINFO_PROP_APP_SCREENREADER,	PMINFO_APPINFO_PROP_APP_SCREENREADER},
	{E_PMINFO_APPINFO_PROP_APP_BG_CATEGORY, PMINFO_APPINFO_PROP_APP_BG_CATEGORY},
};

struct _appinfo_int_map_t {
	pkgmgrinfo_appinfo_filter_prop_int prop;
	const char *property;
};

static struct _appinfo_int_map_t appinfo_int_prop_map[] = {
	{E_PMINFO_APPINFO_PROP_APP_SUPPORT_MODE,	PMINFO_APPINFO_PROP_APP_SUPPORT_MODE}
};

struct _appinfo_bool_map_t {
	pkgmgrinfo_appinfo_filter_prop_bool prop;
	const char *property;
};

static struct _appinfo_bool_map_t appinfo_bool_prop_map[] = {
	{E_PMINFO_APPINFO_PROP_APP_NODISPLAY,		PMINFO_APPINFO_PROP_APP_NODISPLAY},
	{E_PMINFO_APPINFO_PROP_APP_MULTIPLE,		PMINFO_APPINFO_PROP_APP_MULTIPLE},
	{E_PMINFO_APPINFO_PROP_APP_ONBOOT,		PMINFO_APPINFO_PROP_APP_ONBOOT},
	{E_PMINFO_APPINFO_PROP_APP_AUTORESTART,		PMINFO_APPINFO_PROP_APP_AUTORESTART},
	{E_PMINFO_APPINFO_PROP_APP_TASKMANAGE,		PMINFO_APPINFO_PROP_APP_TASKMANAGE},
	{E_PMINFO_APPINFO_PROP_APP_LAUNCHCONDITION,		PMINFO_APPINFO_PROP_APP_LAUNCHCONDITION},
	{E_PMINFO_APPINFO_PROP_APP_SUPPORT_DISABLE,		PMINFO_APPINFO_PROP_APP_SUPPORT_DISABLE},
	{E_PMINFO_APPINFO_PROP_APP_DISABLE,		PMINFO_APPINFO_PROP_APP_DISABLE},
	{E_PMINFO_APPINFO_PROP_APP_REMOVABLE,		PMINFO_APPINFO_PROP_APP_REMOVABLE},
	{E_PMINFO_APPINFO_PROP_APP_BG_USER_DISABLE,		PMINFO_APPINFO_PROP_APP_BG_USER_DISABLE}
};

inline int _pkgmgrinfo_validate_cb(void *data, int ncols, char **coltxt, char **colname)
{
	int *p = (int*)data;
	*p = atoi(coltxt[0]);
	return 0;
}

inline pkgmgrinfo_pkginfo_filter_prop_str _pminfo_pkginfo_convert_to_prop_str(const char *property)
{
	int i = 0;
	int max = 0;
	pkgmgrinfo_pkginfo_filter_prop_str prop = -1;

	if (property == NULL)
		return -1;
	max = E_PMINFO_PKGINFO_PROP_PACKAGE_MAX_STR - E_PMINFO_PKGINFO_PROP_PACKAGE_MIN_STR + 1;
	for (i = 0 ; i < max; i++) {
		if (strcmp(property, pkginfo_str_prop_map[i].property) == 0) {
			prop =	pkginfo_str_prop_map[i].prop;
			break;
		}
	}
	return prop;
}

inline pkgmgrinfo_pkginfo_filter_prop_int _pminfo_pkginfo_convert_to_prop_int(const char *property)
{
	int i = 0;
	int max = 0;
	pkgmgrinfo_pkginfo_filter_prop_int prop = -1;

	if (property == NULL)
		return -1;
	max = E_PMINFO_PKGINFO_PROP_PACKAGE_MAX_INT - E_PMINFO_PKGINFO_PROP_PACKAGE_MIN_INT + 1;
	for (i = 0 ; i < max; i++) {
		if (strcmp(property, pkginfo_int_prop_map[i].property) == 0) {
			prop =	pkginfo_int_prop_map[i].prop;
			break;
		}
	}
	return prop;
}

inline pkgmgrinfo_pkginfo_filter_prop_bool _pminfo_pkginfo_convert_to_prop_bool(const char *property)
{
	int i = 0;
	int max = 0;
	pkgmgrinfo_pkginfo_filter_prop_bool prop = -1;

	if (property == NULL)
		return -1;
	max = E_PMINFO_PKGINFO_PROP_PACKAGE_MAX_BOOL - E_PMINFO_PKGINFO_PROP_PACKAGE_MIN_BOOL + 1;
	for (i = 0 ; i < max; i++) {
		if (strcmp(property, pkginfo_bool_prop_map[i].property) == 0) {
			prop =	pkginfo_bool_prop_map[i].prop;
			break;
		}
	}
	return prop;
}

inline pkgmgrinfo_appinfo_filter_prop_str _pminfo_appinfo_convert_to_prop_str(const char *property)
{
	int i = 0;
	int max = 0;
	pkgmgrinfo_appinfo_filter_prop_str prop = -1;

	if (property == NULL)
		return -1;
	max = E_PMINFO_APPINFO_PROP_APP_MAX_STR - E_PMINFO_APPINFO_PROP_APP_MIN_STR + 1;
	for (i = 0 ; i < max; i++) {
		if (strcmp(property, appinfo_str_prop_map[i].property) == 0) {
			prop =	appinfo_str_prop_map[i].prop;
			break;
		}
	}
	return prop;
}

inline pkgmgrinfo_appinfo_filter_prop_int _pminfo_appinfo_convert_to_prop_int(const char *property)
{
	int i = 0;
	int max = 0;
	pkgmgrinfo_appinfo_filter_prop_int prop = -1;

	if (property == NULL)
		return -1;
	max = E_PMINFO_APPINFO_PROP_APP_MAX_INT - E_PMINFO_APPINFO_PROP_APP_MIN_INT + 1;
	for (i = 0 ; i < max; i++) {
		if (strcmp(property, appinfo_int_prop_map[i].property) == 0) {
			prop =	appinfo_int_prop_map[i].prop;
			break;
		}
	}
	return prop;
}

inline pkgmgrinfo_appinfo_filter_prop_bool _pminfo_appinfo_convert_to_prop_bool(const char *property)
{
	int i = 0;
	int max = 0;
	pkgmgrinfo_appinfo_filter_prop_bool prop = -1;

	if (property == NULL)
		return -1;
	max = E_PMINFO_APPINFO_PROP_APP_MAX_BOOL - E_PMINFO_APPINFO_PROP_APP_MIN_BOOL + 1;
	for (i = 0 ; i < max; i++) {
		if (strcmp(property, appinfo_bool_prop_map[i].property) == 0) {
			prop =	appinfo_bool_prop_map[i].prop;
			break;
		}
	}
	return prop;
}

static int __db_busy_handler(void *pData, int count)
{
	_LOGD("count=[%d]", count);

	// waiting time : 10sec = 500 * 20ms
	if (count < 500) {
		_LOGE("__db_busy_handler(count=%d) is called. pid=[%d]", count, getpid());
		usleep(20*1000);
		return 1;
	} else {
		_LOGE("__db_busy_handler(count=%d) is failed. pid=[%d]", count, getpid());
		return 0;
	}
}

int _pminfo_db_open(const char *dbfile, sqlite3 **database)
{
	int ret = 0;
	retvm_if(dbfile == NULL, PMINFO_R_ERROR, "dbfile is NULL");
	retvm_if(database == NULL, PMINFO_R_ERROR, "database is NULL");

	ret = sqlite3_open_v2(dbfile, database, SQLITE_OPEN_READONLY, NULL);
	tryvm_if(ret != SQLITE_OK, , "sqlite3_open(%s) failed. [ret = %d]", dbfile, ret);

	ret = sqlite3_busy_handler(*database, __db_busy_handler, NULL);
	tryvm_if(ret != SQLITE_OK, , "sqlite3_busy_handler(%s) failed. [ret = %d]", dbfile, ret);

catch:
	if (ret != SQLITE_OK) {
		if (*database) {
			_LOGE("error: sqlite3_close(%s) is done.", dbfile);
			sqlite3_close(*database);
		}
	}

	return ret;
}

int _pminfo_db_open_with_options(const char *dbfile, sqlite3 **database, int flags)
{
	int ret = 0;
	retvm_if(dbfile == NULL, PMINFO_R_ERROR, "dbfile is NULL");
	retvm_if(database == NULL, PMINFO_R_ERROR, "database is NULL");

	ret = sqlite3_open_v2(dbfile, database, flags, NULL);
	tryvm_if(ret != SQLITE_OK, , "sqlite3_open_v2(%s) failed. [ret = %d]", dbfile, ret);

	ret = sqlite3_busy_handler(*database, __db_busy_handler, NULL);
	tryvm_if(ret != SQLITE_OK, , "sqlite3_busy_handler(%s) failed. [ret = %d]", dbfile, ret);

catch:
	if (ret != SQLITE_OK) {
		if (*database) {
			_LOGE("error: sqlite3_close(%s) is done.", dbfile);
			sqlite3_close(*database);
		}
	}

	return ret;
}

int __exec_db_query(sqlite3 *db, char *query, sqlite_query_callback callback, void *data)
{
	char *error_message = NULL;
	if (SQLITE_OK !=
	    sqlite3_exec(db, query, callback, data, &error_message)) {
		_LOGE("Don't execute query = %s error message = %s\n", query,
		       error_message);
		sqlite3_free(error_message);
		return -1;
	}
	sqlite3_free(error_message);
	return 0;
}

void __cleanup_list_pkginfo(GList **list_pkginfo)
{
	GList *node = NULL;
	if (*list_pkginfo != NULL) {
		for(node = *list_pkginfo; node; node = node->next){
			__cleanup_pkginfo((pkgmgr_pkginfo_x *)node->data);
		}
		g_list_free(*list_pkginfo);
		*list_pkginfo = NULL;
	}
}

void __cleanup_pkginfo(pkgmgr_pkginfo_x *data)
{
	if(data == NULL)
		return;

	FREE_AND_NULL(data->locale);

	_pkgmgrinfo_basic_free_manifest_x(data->manifest_info);
	FREE_AND_NULL(data);
	return;
}

void __cleanup_appinfo(pkgmgr_appinfo_x *data)
{
	if(data == NULL)
		return;

	FREE_AND_NULL(data->locale);
	_pkgmgrinfo_basic_free_uiapplication_x(data->uiapp_info);
	FREE_AND_NULL(data);
	return;
}

char* __convert_system_locale_to_manifest_locale()
{
	char *syslocale = NULL;

	syslocale = vconf_get_str(VCONFKEY_LANGSET);

	if (syslocale == NULL) {
		_LOGE("syslocale is null\n");
		return strdup(DEFAULT_LOCALE);
	}

	unsigned int size = 6 * sizeof(char);
	char *locale = malloc(size);
	if (!locale) {
		_LOGE("Malloc Failed\n");
		FREE_AND_NULL(syslocale);
		return strdup(DEFAULT_LOCALE);
	}

	snprintf(locale, size, "%c%c-%c%c", syslocale[0], syslocale[1], tolower(syslocale[3]), tolower(syslocale[4]));

	FREE_AND_NULL(syslocale);
	return locale;
}

gint __compare_func(gconstpointer data1, gconstpointer data2)
{
	pkgmgrinfo_node_x *node1 = (pkgmgrinfo_node_x*)data1;
	pkgmgrinfo_node_x *node2 = (pkgmgrinfo_node_x*)data2;
	if (node1->prop == node2->prop)
		return 0;
	else if (node1->prop > node2->prop)
		return 1;
	else
		return -1;
}


void __get_filter_condition(gpointer data, char **condition)
{
	pkgmgrinfo_node_x *node = (pkgmgrinfo_node_x*)data;
	char buf[MAX_QUERY_LEN + 1] = {'\0'};
	char temp[PKG_STRING_LEN_MAX] = {'\0'};
	switch (node->prop) {
	case E_PMINFO_PKGINFO_PROP_PACKAGE_ID:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.package=%Q", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_TYPE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.package_type=%Q", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_VERSION:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.package_version=%Q", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_INSTALL_LOCATION:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.install_location=%Q", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_INSTALLED_STORAGE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.installed_storage=%Q", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_AUTHOR_NAME:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.author_name=%Q", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_AUTHOR_HREF:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.author_href=%Q", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_STORECLIENT_ID:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.storeclient_id=%Q", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_AUTHOR_EMAIL:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.author_email=%Q", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_SIZE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.package_size=%Q", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_REMOVABLE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.package_removable IN %s", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_PRELOAD:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.package_preload IN %s", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_READONLY:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.package_readonly IN %s", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_UPDATE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.package_update IN %s", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_APPSETTING:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.package_appsetting IN %s", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_NODISPLAY_SETTING:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.package_nodisplay IN %s", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_SUPPORT_DISABLE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.package_support_disable IN %s", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_DISABLE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.package_disable IN %s", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_SUPPORT_MODE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.package_support_mode & %s", node->value);
		break;
	case E_PMINFO_PKGINFO_PROP_PACKAGE_SYSTEM:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_info.package_system IN %s", node->value);
		break;

	case E_PMINFO_APPINFO_PROP_APP_ID:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_id=%Q", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_COMPONENT_TYPE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.component_type=%Q", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_EXEC:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_exec=%Q", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_ICON:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_localized_info.app_icon=%Q", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_TYPE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_type=%Q", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_OPERATION:
		snprintf(temp, PKG_STRING_LEN_MAX, "(%s)", node->value);
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_app_svc.operation IN %s", temp);
		break;
	case E_PMINFO_APPINFO_PROP_APP_URI:
		snprintf(temp, PKG_STRING_LEN_MAX, "(%s)", node->value);
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_app_svc.uri_scheme IN %s", temp);
		break;
	case E_PMINFO_APPINFO_PROP_APP_MIME:
		snprintf(temp, PKG_STRING_LEN_MAX, "(%s)", node->value);
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_app_svc.mime_type IN %s", temp);
		break;
	case E_PMINFO_APPINFO_PROP_APP_CATEGORY:
		snprintf(temp, PKG_STRING_LEN_MAX, "(%s)", node->value);
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_app_category.category IN %s", temp);
		break;
	case E_PMINFO_APPINFO_PROP_APP_NODISPLAY:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_nodisplay IN %s", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_MULTIPLE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_multiple IN %s", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_ONBOOT:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_onboot IN %s", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_AUTORESTART:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_autorestart IN %s", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_TASKMANAGE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_taskmanage IN %s", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_HWACCELERATION:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_hwacceleration=%Q", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_SCREENREADER:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_screenreader=%Q", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_BG_USER_DISABLE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_background_category & 1 = %q", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_BG_CATEGORY:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_background_category & %q = %q", node->value, node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_LAUNCHCONDITION:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_launchcondition IN %s", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_SUPPORT_DISABLE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_support_disable IN %s", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_DISABLE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_disable IN %s", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_REMOVABLE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_removable IN %s", node->value);
		break;
	case E_PMINFO_APPINFO_PROP_APP_SUPPORT_MODE:
		sqlite3_snprintf(MAX_QUERY_LEN, buf, "package_app_info.app_support_mode & %s", node->value);
		break;
	default:
		_LOGE("Invalid Property Type\n");
		*condition = NULL;
		return;
	}
	*condition = strdup(buf);
	return;
}

int __pkginfo_check_installed_storage(pkgmgr_pkginfo_x *pkginfo)
{
	char buf[MAX_QUERY_LEN] = {'\0'};
	retvm_if(pkginfo->manifest_info->package == NULL, PMINFO_R_OK, "pkgid is NULL\n");
	retvm_if(pkginfo->manifest_info->installed_storage == NULL, PMINFO_R_ERROR, "installed_storage is NULL\n");

	if (strcmp(pkginfo->manifest_info->installed_storage,"installed_external") == 0) {
		snprintf(buf, MAX_QUERY_LEN - 1, "%s%s", PKG_SD_PATH, pkginfo->manifest_info->package);
		if (access(buf, R_OK) != 0) {
			_LOGE("can not access [%s]", buf);
			return PMINFO_R_ERROR;
		}
	}

	return PMINFO_R_OK;
}

int __appinfo_check_installed_storage(pkgmgr_appinfo_x *appinfo)
{
	char buf[MAX_QUERY_LEN] = {'\0'};
	char pkgid[MAX_QUERY_LEN] = {'\0'};

	snprintf(pkgid, MAX_QUERY_LEN - 1, "%s", appinfo->uiapp_info->package);

	retvm_if(appinfo->uiapp_info->installed_storage == NULL, PMINFO_R_ERROR, "installed_storage is NULL\n");

	if (strcmp(appinfo->uiapp_info->installed_storage,"installed_external") == 0) {
		snprintf(buf, MAX_QUERY_LEN - 1, "%s%s", PKG_SD_PATH, pkgid);
		if (access(buf, R_OK) != 0) {
			_LOGE("can not access [%s]", buf);
			return PMINFO_R_ERROR;
		}
	}

	return PMINFO_R_OK;
}

void _pminfo_destroy_node(gpointer data)
{
	ret_if(data == NULL);
	pkgmgrinfo_node_x *node = (pkgmgrinfo_node_x*)data;
	FREE_AND_NULL(node->value);
	FREE_AND_NULL(node->key);
	FREE_AND_NULL(node);
}
