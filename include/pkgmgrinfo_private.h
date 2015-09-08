/*
 * pkgmgr-info
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


#ifndef __PKGMGRINFO_PRIVATE_H__
#define __PKGMGRINFO_PRIVATE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <db-util.h>
#include <sqlite3.h>
#include <glib.h>
#include <ctype.h>
#include <assert.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>

#include "pkgmgrinfo_basic.h"
#include "pkgmgrinfo_debug.h"
#include "pkgmgr-info.h"

#ifndef DEPRECATED
#define DEPRECATED	__attribute__ ((__deprecated__))
#endif

#ifndef API
#define API __attribute__ ((visibility("default")))
#endif

#define MMC_PATH "/opt/storage/sdcard"
#define PKG_SD_PATH MMC_PATH"/app2sd/"

#define PKG_RW_PATH "/opt/usr/apps/"
#define PKG_RO_PATH "/usr/apps/"
#define BLOCK_SIZE      4096 /*in bytes*/

#define PKG_TYPE_STRING_LEN_MAX			128
#define PKG_VERSION_STRING_LEN_MAX		128
#define PKG_VALUE_STRING_LEN_MAX		512
#define PKG_LOCALE_STRING_LEN_MAX		8

#define MAX_QUERY_LEN	4096
#define MAX_CERT_TYPE	9

#define MANIFEST_DB		"/opt/dbspace/.pkgmgr_parser.db"
#define CERT_DB			"/opt/dbspace/.pkgmgr_cert.db"
#define DATACONTROL_DB	"/opt/usr/dbspace/.app-package.db"

/*String properties for filtering based on package info*/
typedef enum _pkgmgrinfo_pkginfo_filter_prop_str {
	E_PMINFO_PKGINFO_PROP_PACKAGE_MIN_STR = 101,
	E_PMINFO_PKGINFO_PROP_PACKAGE_ID = E_PMINFO_PKGINFO_PROP_PACKAGE_MIN_STR,
	E_PMINFO_PKGINFO_PROP_PACKAGE_TYPE,
	E_PMINFO_PKGINFO_PROP_PACKAGE_VERSION,
	E_PMINFO_PKGINFO_PROP_PACKAGE_INSTALL_LOCATION,
	E_PMINFO_PKGINFO_PROP_PACKAGE_INSTALLED_STORAGE,
	E_PMINFO_PKGINFO_PROP_PACKAGE_AUTHOR_NAME,
	E_PMINFO_PKGINFO_PROP_PACKAGE_AUTHOR_EMAIL,
	E_PMINFO_PKGINFO_PROP_PACKAGE_AUTHOR_HREF,
	E_PMINFO_PKGINFO_PROP_PACKAGE_STORECLIENT_ID,
	E_PMINFO_PKGINFO_PROP_PACKAGE_MAX_STR = E_PMINFO_PKGINFO_PROP_PACKAGE_STORECLIENT_ID
} pkgmgrinfo_pkginfo_filter_prop_str;

/*Boolean properties for filtering based on package info*/
typedef enum _pkgmgrinfo_pkginfo_filter_prop_bool {
	E_PMINFO_PKGINFO_PROP_PACKAGE_MIN_BOOL = 201,
	E_PMINFO_PKGINFO_PROP_PACKAGE_REMOVABLE = E_PMINFO_PKGINFO_PROP_PACKAGE_MIN_BOOL,
	E_PMINFO_PKGINFO_PROP_PACKAGE_PRELOAD,
	E_PMINFO_PKGINFO_PROP_PACKAGE_READONLY,
	E_PMINFO_PKGINFO_PROP_PACKAGE_UPDATE,
	E_PMINFO_PKGINFO_PROP_PACKAGE_APPSETTING,
	E_PMINFO_PKGINFO_PROP_PACKAGE_NODISPLAY_SETTING,
	E_PMINFO_PKGINFO_PROP_PACKAGE_SUPPORT_DISABLE,
	E_PMINFO_PKGINFO_PROP_PACKAGE_DISABLE,
	E_PMINFO_PKGINFO_PROP_PACKAGE_USE_RESET,
	E_PMINFO_PKGINFO_PROP_PACKAGE_MAX_BOOL = E_PMINFO_PKGINFO_PROP_PACKAGE_USE_RESET
} pkgmgrinfo_pkginfo_filter_prop_bool;

/*Integer properties for filtering based on package info*/
typedef enum _pkgmgrinfo_pkginfo_filter_prop_int {
	E_PMINFO_PKGINFO_PROP_PACKAGE_MIN_INT = 301,
	E_PMINFO_PKGINFO_PROP_PACKAGE_SIZE = E_PMINFO_PKGINFO_PROP_PACKAGE_MIN_INT,
	E_PMINFO_PKGINFO_PROP_PACKAGE_SUPPORT_MODE,
	E_PMINFO_PKGINFO_PROP_PACKAGE_MAX_INT = E_PMINFO_PKGINFO_PROP_PACKAGE_SUPPORT_MODE
} pkgmgrinfo_pkginfo_filter_prop_int;

/*String properties for filtering based on app info*/
typedef enum _pkgmgrinfo_appinfo_filter_prop_str {
	E_PMINFO_APPINFO_PROP_APP_MIN_STR = 401,
	E_PMINFO_APPINFO_PROP_APP_ID = E_PMINFO_APPINFO_PROP_APP_MIN_STR,
	E_PMINFO_APPINFO_PROP_APP_COMPONENT,
	E_PMINFO_APPINFO_PROP_APP_COMPONENT_TYPE,
	E_PMINFO_APPINFO_PROP_APP_EXEC,
	E_PMINFO_APPINFO_PROP_APP_AMBIENT_SUPPORT,
	E_PMINFO_APPINFO_PROP_APP_ICON,
	E_PMINFO_APPINFO_PROP_APP_TYPE,
	E_PMINFO_APPINFO_PROP_APP_OPERATION,
	E_PMINFO_APPINFO_PROP_APP_URI,
	E_PMINFO_APPINFO_PROP_APP_MIME,
	E_PMINFO_APPINFO_PROP_APP_HWACCELERATION,
	E_PMINFO_APPINFO_PROP_APP_CATEGORY,
	E_PMINFO_APPINFO_PROP_APP_SCREENREADER,
	E_PMINFO_APPINFO_PROP_APP_MAX_STR = E_PMINFO_APPINFO_PROP_APP_CATEGORY
} pkgmgrinfo_appinfo_filter_prop_str;

/*Boolean properties for filtering based on app info*/
typedef enum _pkgmgrinfo_appinfo_filter_prop_bool {
	E_PMINFO_APPINFO_PROP_APP_MIN_BOOL = 501,
	E_PMINFO_APPINFO_PROP_APP_NODISPLAY = E_PMINFO_APPINFO_PROP_APP_MIN_BOOL,
	E_PMINFO_APPINFO_PROP_APP_MULTIPLE,
	E_PMINFO_APPINFO_PROP_APP_ONBOOT,
	E_PMINFO_APPINFO_PROP_APP_AUTORESTART,
	E_PMINFO_APPINFO_PROP_APP_TASKMANAGE,
	E_PMINFO_APPINFO_PROP_APP_LAUNCHCONDITION,
	E_PMINFO_APPINFO_PROP_APP_SUPPORT_DISABLE,
	E_PMINFO_APPINFO_PROP_APP_DISABLE,
	E_PMINFO_APPINFO_PROP_APP_REMOVABLE,
	E_PMINFO_APPINFO_PROP_APP_MAX_BOOL = E_PMINFO_APPINFO_PROP_APP_REMOVABLE
} pkgmgrinfo_appinfo_filter_prop_bool;

/*Integer properties for filtering based on app info*/
typedef enum _pkgmgrinfo_appinfo_filter_prop_int {
	/*Currently No Fields*/
	E_PMINFO_APPINFO_PROP_APP_MIN_INT = 601,
	E_PMINFO_APPINFO_PROP_APP_SUPPORT_MODE = E_PMINFO_APPINFO_PROP_APP_MIN_INT,
	E_PMINFO_APPINFO_PROP_APP_MAX_INT = E_PMINFO_APPINFO_PROP_APP_SUPPORT_MODE
} pkgmgrinfo_appinfo_filter_prop_int;

/*Integer properties for filtering based on app info*/
typedef enum _pkgmgrinfo_pkginfo_filter_prop_range {
	/*Currently No Fields*/
	E_PMINFO_PKGINFO_PROP_RANGE_MIN_INT = 701,
	E_PMINFO_PKGINFO_PROP_RANGE_BASIC,
	E_PMINFO_PKGINFO_PROP_RANGE_MAX_INT = E_PMINFO_PKGINFO_PROP_RANGE_BASIC
} pkgmgrinfo_pkginfo_filter_prop_range;

typedef struct _pkgmgr_pkginfo_x {
	manifest_x *manifest_info;
	char *locale;

	struct _pkgmgr_pkginfo_x *prev;
	struct _pkgmgr_pkginfo_x *next;
} pkgmgr_pkginfo_x;

typedef struct _pkgmgr_appinfo_x {
	char *locale;
	union {
		uiapplication_x *uiapp_info;
		serviceapplication_x *svcapp_info;
	};
} pkgmgr_appinfo_x;

/*For filter APIs*/
typedef struct _pkgmgrinfo_filter_x {
	GSList *list;
} pkgmgrinfo_filter_x;

typedef struct _pkgmgrinfo_node_x {
	int prop;
	char *key;
	char *value;
} pkgmgrinfo_node_x;

typedef int (*sqlite_query_callback)(void *data, int ncols, char **coltxt, char **colname);

int __exec_db_query(sqlite3 *db, char *query, sqlite_query_callback callback, void *data);
char* __convert_system_locale_to_manifest_locale();
gint __compare_func(gconstpointer data1, gconstpointer data2);
void __get_filter_condition(gpointer data, char **condition);

void __cleanup_pkginfo(pkgmgr_pkginfo_x *data);
void __cleanup_appinfo(pkgmgr_appinfo_x *data);
void __cleanup_list_pkginfo(pkgmgr_pkginfo_x *list_pkginfo, pkgmgr_pkginfo_x *node);

int __pkginfo_check_installed_storage(pkgmgr_pkginfo_x *pkginfo);
int __appinfo_check_installed_storage(pkgmgr_appinfo_x *appinfo);

int _pkgmgrinfo_validate_cb(void *data, int ncols, char **coltxt, char **colname);

int __pkginfo_cb(void *data, int ncols, char **coltxt, char **colname);
int __appinfo_cb(void *data, int ncols, char **coltxt, char **colname);
int __uiapp_list_cb(void *data, int ncols, char **coltxt, char **colname);
char* __get_app_locale_by_fallback(sqlite3 *db, const char *appid);

pkgmgrinfo_pkginfo_filter_prop_str _pminfo_pkginfo_convert_to_prop_str(const char *property);
pkgmgrinfo_pkginfo_filter_prop_int _pminfo_pkginfo_convert_to_prop_int(const char *property);
pkgmgrinfo_pkginfo_filter_prop_bool _pminfo_pkginfo_convert_to_prop_bool(const char *property);

pkgmgrinfo_appinfo_filter_prop_str _pminfo_appinfo_convert_to_prop_str(const char *property);
pkgmgrinfo_appinfo_filter_prop_int _pminfo_appinfo_convert_to_prop_int(const char *property);
pkgmgrinfo_appinfo_filter_prop_bool _pminfo_appinfo_convert_to_prop_bool(const char *property);
pkgmgrinfo_pkginfo_filter_prop_range _pminfo_pkginfo_convert_to_prop_range(const char *property);

#endif  /* __PKGMGRINFO_PRIVATE_H__ */
