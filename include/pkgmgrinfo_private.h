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

#include "pkgmgr_parser.h"
#include "pkgmgr-info-internal.h"
#include "pkgmgr-info-debug.h"
#include "pkgmgr-info.h"

typedef struct _pkgmgr_pkginfo_x {
	manifest_x *manifest_info;
	char *locale;

	struct _pkgmgr_pkginfo_x *prev;
	struct _pkgmgr_pkginfo_x *next;
} pkgmgr_pkginfo_x;

typedef struct _pkgmgr_appinfo_x {
	const char *package;
	char *locale;
	pkgmgrinfo_app_component app_component;
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

int __pkginfo_check_installed_storage(pkgmgr_pkginfo_x *pkginfo);
int __appinfo_check_installed_storage(pkgmgr_appinfo_x *appinfo);

#endif  /* __PKGMGRINFO_PRIVATE_H__ */
