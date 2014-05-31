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

#include "pkgmgrinfo_private.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_INFO"

#define LANGUAGE_LENGTH 2

#define FILTER_QUERY_LIST_APP	"select DISTINCT package_app_info.app_id, package_app_info.app_component " \
				"from package_app_info LEFT OUTER JOIN package_app_localized_info " \
				"ON package_app_info.app_id=package_app_localized_info.app_id " \
				"and package_app_localized_info.app_locale='%s' " \
				"LEFT OUTER JOIN package_app_app_svc " \
				"ON package_app_info.app_id=package_app_app_svc.app_id " \
				"LEFT OUTER JOIN package_app_app_category " \
				"ON package_app_info.app_id=package_app_app_category.app_id where "

#define METADATA_FILTER_QUERY_SELECT_CLAUSE	"select DISTINCT package_app_info.app_id, package_app_info.app_component " \
				"from package_app_info LEFT OUTER JOIN package_app_app_metadata " \
				"ON package_app_info.app_id=package_app_app_metadata.app_id where "

#define METADATA_FILTER_QUERY_UNION_CLAUSE	" UNION "METADATA_FILTER_QUERY_SELECT_CLAUSE

typedef struct _pkgmgr_locale_x {
	char *locale;
} pkgmgr_locale_x;

typedef struct _pkgmgrinfo_appcontrol_x {
	int operation_count;
	int uri_count;
	int mime_count;
	int subapp_count;
	char **operation;
	char **uri;
	char **mime;
	char **subapp;
} pkgmgrinfo_appcontrol_x;


/* get the first locale value*/
static int __fallback_locale_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_locale_x *info = (pkgmgr_locale_x *)data;

	if (ncols >= 1)
		info->locale = strdup(coltxt[0]);
	else
		info->locale = NULL;

	return 0;
}

static int __check_app_locale_from_app_localized_info_by_exact(sqlite3 *db, const char *appid, const char *locale)
{
	int result_query = -1;
	int ret = 0;

	char *query = sqlite3_mprintf("select exists(select app_locale from package_app_localized_info where app_id=%Q and app_locale=%Q)", appid, locale);
	ret = __exec_db_query(db, query, _pkgmgrinfo_validate_cb, (void *)&result_query);
	sqlite3_free(query);
	retvm_if(ret == -1, PMINFO_R_ERROR, "Exec DB query failed");
	return result_query;
}

static int __check_app_locale_from_app_localized_info_by_fallback(sqlite3 *db, const char *appid, const char *locale)
{
	int result_query = -1;
	int ret = 0;
	char wildcard[2] = {'%','\0'};
	char lang[3] = {'\0'};
	strncpy(lang, locale, LANGUAGE_LENGTH);

	char *query = sqlite3_mprintf("select exists(select app_locale from package_app_localized_info where app_id=%Q and app_locale like %Q%Q)", appid, lang, wildcard);
	ret = __exec_db_query(db, query, _pkgmgrinfo_validate_cb, (void *)&result_query);
	sqlite3_free(query);
	retvm_if(ret == -1, PMINFO_R_ERROR, "Exec DB query failed");
	return result_query;
}

static char* __get_app_locale_from_app_localized_info_by_fallback(sqlite3 *db, const char *appid, const char *locale)
{
	int ret = 0;
	char wildcard[2] = {'%','\0'};
	char lang[3] = {'\0'};
	char *locale_new = NULL;
	pkgmgr_locale_x *info = NULL;

	info = (pkgmgr_locale_x *)malloc(sizeof(pkgmgr_locale_x));
	if (info == NULL) {
		_LOGE("Out of Memory!!!\n");
		return NULL;
	}
	memset(info, '\0', sizeof(*info));

	strncpy(lang, locale, LANGUAGE_LENGTH);
	char *query = sqlite3_mprintf("select app_locale from package_app_localized_info where app_id=%Q  and app_locale like %Q%Q", appid, lang, wildcard);

	ret = __exec_db_query(db, query, __fallback_locale_cb, (void *)info);
	sqlite3_free(query);
	if (ret < 0)
		goto catch;

	locale_new = info->locale;
	free(info);
	return locale_new;
catch:
	if (info) {
		free(info);
		info = NULL;
	}
	return NULL;
}

static char* __get_app_locale_by_fallback(sqlite3 *db, const char *appid)
{
	assert(appid);

	char *locale = NULL;
	char *locale_new = NULL;
	int check_result = 0;

	locale = __convert_system_locale_to_manifest_locale();

	/*check exact matching */
	check_result = __check_app_locale_from_app_localized_info_by_exact(db, appid, locale);

	/* Exact found */
	if (check_result == 1) {
//		_LOGD("%s find exact locale(%s)\n", appid, locale);
		return locale;
	}

	/* fallback matching */
	check_result = __check_app_locale_from_app_localized_info_by_fallback(db, appid, locale);
	if(check_result == 1) {
		   locale_new = __get_app_locale_from_app_localized_info_by_fallback(db, appid, locale);
		   free(locale);
		   if (locale_new == NULL)
			   locale_new =  strdup(DEFAULT_LOCALE);
		   return locale_new;
	}

	/* default locale */
	free(locale);
	return	strdup(DEFAULT_LOCALE);
}

static void __get_metadata_filter_condition(gpointer data, char **condition)
{
	pkgmgrinfo_node_x *node = (pkgmgrinfo_node_x*)data;
	char key[MAX_QUERY_LEN] = {'\0'};
	char value[MAX_QUERY_LEN] = {'\0'};
	if (node->key) {
		snprintf(key, MAX_QUERY_LEN, "(package_app_app_metadata.md_key='%s'", node->key);
	}
	if (node->value) {
		snprintf(value, MAX_QUERY_LEN, " AND package_app_app_metadata.md_value='%s')", node->value);
		strcat(key, value);
	} else {
		strcat(key, ")");
	}
	*condition = strdup(key);
	return;
}

static int __mini_appinfo_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)data;
	int i = 0;
	int j = 0;
	uiapplication_x *uiapp = NULL;
	serviceapplication_x *svcapp = NULL;
	for(i = 0; i < ncols; i++)
	{
		if (strcmp(colname[i], "app_component") == 0) {
			if (coltxt[i]) {
				if (strcmp(coltxt[i], "uiapp") == 0) {
					uiapp = calloc(1, sizeof(uiapplication_x));
					if (uiapp == NULL) {
						_LOGE("Out of Memory!!!\n");
						return -1;
					}
					LISTADD(info->manifest_info->uiapplication, uiapp);
					for(j = 0; j < ncols; j++)
					{
						if (strcmp(colname[j], "app_id") == 0) {
							if (coltxt[j])
								info->manifest_info->uiapplication->appid = strdup(coltxt[j]);
						} else if (strcmp(colname[j], "app_exec") == 0) {
							if (coltxt[j])
								info->manifest_info->uiapplication->exec = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->exec = NULL;
						} else if (strcmp(colname[j], "app_nodisplay") == 0) {
							if (coltxt[j])
								info->manifest_info->uiapplication->nodisplay = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->nodisplay = NULL;
						} else if (strcmp(colname[j], "app_type") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->type = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->type = NULL;
						} else if (strcmp(colname[j], "app_multiple") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->multiple = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->multiple = NULL;
						} else if (strcmp(colname[j], "app_taskmanage") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->taskmanage = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->taskmanage = NULL;
						} else if (strcmp(colname[j], "app_hwacceleration") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->hwacceleration = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->hwacceleration = NULL;
						} else if (strcmp(colname[j], "app_screenreader") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->screenreader = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->screenreader = NULL;
						} else if (strcmp(colname[j], "app_enabled") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->enabled= strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->enabled = NULL;
						} else if (strcmp(colname[j], "app_indicatordisplay") == 0){
							if (coltxt[j])
								info->manifest_info->uiapplication->indicatordisplay = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->indicatordisplay = NULL;
						} else if (strcmp(colname[j], "app_portraitimg") == 0){
							if (coltxt[j])
								info->manifest_info->uiapplication->portraitimg = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->portraitimg = NULL;
						} else if (strcmp(colname[j], "app_landscapeimg") == 0){
							if (coltxt[j])
								info->manifest_info->uiapplication->landscapeimg = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->landscapeimg = NULL;
						} else if (strcmp(colname[j], "app_guestmodevisibility") == 0){
							if (coltxt[j])
								info->manifest_info->uiapplication->guestmode_visibility = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->guestmode_visibility = NULL;
						} else if (strcmp(colname[j], "app_recentimage") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->recentimage = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->recentimage = NULL;
						} else if (strcmp(colname[j], "app_mainapp") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->mainapp = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->mainapp = NULL;
						} else if (strcmp(colname[j], "package") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->package = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->package = NULL;
						} else if (strcmp(colname[j], "app_component") == 0) {
							if (coltxt[j])
								info->manifest_info->uiapplication->app_component = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->app_component = NULL;
						} else if (strcmp(colname[j], "app_permissiontype") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->permission_type = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->permission_type = NULL;
						} else if (strcmp(colname[j], "component_type") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->component_type = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->component_type = NULL;
						} else if (strcmp(colname[j], "app_preload") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->preload = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->preload = NULL;
						} else if (strcmp(colname[j], "app_submode") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->submode = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->submode = NULL;
						} else if (strcmp(colname[j], "app_submode_mainid") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->submode_mainid = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->submode_mainid = NULL;
						} else if (strcmp(colname[j], "app_installed_storage") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->installed_storage = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->installed_storage = NULL;
						} else if (strcmp(colname[j], "app_process_pool") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->process_pool = strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->process_pool = NULL;
						} else if (strcmp(colname[j], "app_onboot") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->onboot= strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->onboot = NULL;
						} else if (strcmp(colname[j], "app_autorestart") == 0 ) {
							if (coltxt[j])
								info->manifest_info->uiapplication->autorestart= strdup(coltxt[j]);
							else
								info->manifest_info->uiapplication->autorestart = NULL;

						} else
							continue;
					}
				} else {
					svcapp = calloc(1, sizeof(serviceapplication_x));
					if (svcapp == NULL) {
						_LOGE("Out of Memory!!!\n");
						return -1;
					}
					LISTADD(info->manifest_info->serviceapplication, svcapp);
					for(j = 0; j < ncols; j++)
					{
						if (strcmp(colname[j], "app_id") == 0) {
							if (coltxt[j])
								info->manifest_info->serviceapplication->appid = strdup(coltxt[j]);
						} else if (strcmp(colname[j], "app_exec") == 0) {
							if (coltxt[j])
								info->manifest_info->serviceapplication->exec = strdup(coltxt[j]);
							else
								info->manifest_info->serviceapplication->exec = NULL;
						} else if (strcmp(colname[j], "app_type") == 0 ){
							if (coltxt[j])
								info->manifest_info->serviceapplication->type = strdup(coltxt[j]);
							else
								info->manifest_info->serviceapplication->type = NULL;
						} else if (strcmp(colname[j], "app_onboot") == 0 ){
							if (coltxt[j])
								info->manifest_info->serviceapplication->onboot = strdup(coltxt[j]);
							else
								info->manifest_info->serviceapplication->onboot = NULL;
						} else if (strcmp(colname[j], "app_autorestart") == 0 ){
							if (coltxt[j])
								info->manifest_info->serviceapplication->autorestart = strdup(coltxt[j]);
							else
								info->manifest_info->serviceapplication->autorestart = NULL;
						} else if (strcmp(colname[j], "package") == 0 ){
							if (coltxt[j])
								info->manifest_info->serviceapplication->package = strdup(coltxt[j]);
							else
								info->manifest_info->serviceapplication->package = NULL;
						} else if (strcmp(colname[j], "app_permissiontype") == 0 ) {
							if (coltxt[j])
								info->manifest_info->serviceapplication->permission_type = strdup(coltxt[j]);
							else
								info->manifest_info->serviceapplication->permission_type = NULL;
						} else
							continue;
					}
				}
			}
		} else
			continue;
	}

	return 0;
}

static int __appinfo_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)data;
	int i = 0;
	icon_x *icon = NULL;
	label_x *label = NULL;
	category_x *category = NULL;
	metadata_x *metadata = NULL;
	permission_x *permission = NULL;
	image_x *image = NULL;

	switch (info->app_component) {
	case PMINFO_UI_APP:
		icon = calloc(1, sizeof(icon_x));
		LISTADD(info->uiapp_info->icon, icon);
		label = calloc(1, sizeof(label_x));
		LISTADD(info->uiapp_info->label, label);
		category = calloc(1, sizeof(category_x));
		LISTADD(info->uiapp_info->category, category);
		metadata = calloc(1, sizeof(metadata_x));
		LISTADD(info->uiapp_info->metadata, metadata);
		permission = calloc(1, sizeof(permission_x));
		LISTADD(info->uiapp_info->permission, permission);
		image = calloc(1, sizeof(image_x));
		LISTADD(info->uiapp_info->image, image);

		for(i = 0; i < ncols; i++)
		{
			if (strcmp(colname[i], "app_id") == 0) {
				/*appid being foreign key, is column in every table
				Hence appid gets strduped every time leading to memory leak.
				If appid is already set, just continue.*/
				if (info->uiapp_info->appid)
					continue;
				if (coltxt[i])
					info->uiapp_info->appid = strdup(coltxt[i]);
				else
					info->uiapp_info->appid = NULL;
			} else if (strcmp(colname[i], "app_exec") == 0) {
				if (coltxt[i])
					info->uiapp_info->exec = strdup(coltxt[i]);
				else
					info->uiapp_info->exec = NULL;
			} else if (strcmp(colname[i], "app_nodisplay") == 0) {
				if (coltxt[i])
					info->uiapp_info->nodisplay = strdup(coltxt[i]);
				else
					info->uiapp_info->nodisplay = NULL;
			} else if (strcmp(colname[i], "app_type") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->type = strdup(coltxt[i]);
				else
					info->uiapp_info->type = NULL;
			} else if (strcmp(colname[i], "app_icon_section") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->icon->section= strdup(coltxt[i]);
				else
					info->uiapp_info->icon->section = NULL;
			} else if (strcmp(colname[i], "app_icon") == 0) {
				if (coltxt[i])
					info->uiapp_info->icon->text = strdup(coltxt[i]);
				else
					info->uiapp_info->icon->text = NULL;
			} else if (strcmp(colname[i], "app_label") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->label->text = strdup(coltxt[i]);
				else
					info->uiapp_info->label->text = NULL;
			} else if (strcmp(colname[i], "app_multiple") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->multiple = strdup(coltxt[i]);
				else
					info->uiapp_info->multiple = NULL;
			} else if (strcmp(colname[i], "app_taskmanage") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->taskmanage = strdup(coltxt[i]);
				else
					info->uiapp_info->taskmanage = NULL;
			} else if (strcmp(colname[i], "app_hwacceleration") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->hwacceleration = strdup(coltxt[i]);
				else
					info->uiapp_info->hwacceleration = NULL;
			} else if (strcmp(colname[i], "app_screenreader") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->screenreader = strdup(coltxt[i]);
				else
					info->uiapp_info->screenreader = NULL;
			} else if (strcmp(colname[i], "app_enabled") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->enabled= strdup(coltxt[i]);
				else
					info->uiapp_info->enabled = NULL;
			} else if (strcmp(colname[i], "app_indicatordisplay") == 0){
				if (coltxt[i])
					info->uiapp_info->indicatordisplay = strdup(coltxt[i]);
				else
					info->uiapp_info->indicatordisplay = NULL;
			} else if (strcmp(colname[i], "app_portraitimg") == 0){
				if (coltxt[i])
					info->uiapp_info->portraitimg = strdup(coltxt[i]);
				else
					info->uiapp_info->portraitimg = NULL;
			} else if (strcmp(colname[i], "app_landscapeimg") == 0){
				if (coltxt[i])
					info->uiapp_info->landscapeimg = strdup(coltxt[i]);
				else
					info->uiapp_info->landscapeimg = NULL;
			} else if (strcmp(colname[i], "app_guestmodevisibility") == 0){
				if (coltxt[i])
					info->uiapp_info->guestmode_visibility = strdup(coltxt[i]);
				else
					info->uiapp_info->guestmode_visibility = NULL;
			} else if (strcmp(colname[i], "category") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->category->name = strdup(coltxt[i]);
				else
					info->uiapp_info->category->name = NULL;
			} else if (strcmp(colname[i], "md_key") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->metadata->key = strdup(coltxt[i]);
				else
					info->uiapp_info->metadata->key = NULL;
			} else if (strcmp(colname[i], "md_value") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->metadata->value = strdup(coltxt[i]);
				else
					info->uiapp_info->metadata->value = NULL;
			} else if (strcmp(colname[i], "pm_type") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->permission->type= strdup(coltxt[i]);
				else
					info->uiapp_info->permission->type = NULL;
			} else if (strcmp(colname[i], "pm_value") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->permission->value = strdup(coltxt[i]);
				else
					info->uiapp_info->permission->value = NULL;
			} else if (strcmp(colname[i], "app_recentimage") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->recentimage = strdup(coltxt[i]);
				else
					info->uiapp_info->recentimage = NULL;
			} else if (strcmp(colname[i], "app_mainapp") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->mainapp = strdup(coltxt[i]);
				else
					info->uiapp_info->mainapp = NULL;
			} else if (strcmp(colname[i], "app_locale") == 0 ) {
				if (coltxt[i]) {
					info->uiapp_info->icon->lang = strdup(coltxt[i]);
					info->uiapp_info->label->lang = strdup(coltxt[i]);
				}
				else {
					info->uiapp_info->icon->lang = NULL;
					info->uiapp_info->label->lang = NULL;
				}
			} else if (strcmp(colname[i], "app_image") == 0) {
					if (coltxt[i])
						info->uiapp_info->image->text= strdup(coltxt[i]);
					else
						info->uiapp_info->image->text = NULL;
			} else if (strcmp(colname[i], "app_image_section") == 0) {
					if (coltxt[i])
						info->uiapp_info->image->section= strdup(coltxt[i]);
					else
						info->uiapp_info->image->section = NULL;
			} else if (strcmp(colname[i], "app_permissiontype") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->permission_type = strdup(coltxt[i]);
				else
					info->uiapp_info->permission_type = NULL;
			} else if (strcmp(colname[i], "component_type") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->component_type = strdup(coltxt[i]);
				else
					info->uiapp_info->component_type = NULL;
			} else if (strcmp(colname[i], "app_preload") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->preload = strdup(coltxt[i]);
				else
					info->uiapp_info->preload = NULL;
			} else if (strcmp(colname[i], "app_submode") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->submode = strdup(coltxt[i]);
				else
					info->uiapp_info->submode = NULL;
			} else if (strcmp(colname[i], "app_submode_mainid") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->submode_mainid = strdup(coltxt[i]);
				else
					info->uiapp_info->submode_mainid = NULL;
			} else if (strcmp(colname[i], "app_installed_storage") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->installed_storage = strdup(coltxt[i]);
				else
					info->uiapp_info->installed_storage = NULL;
			} else if (strcmp(colname[i], "app_process_pool") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->process_pool = strdup(coltxt[i]);
				else
					info->uiapp_info->process_pool = NULL;
			} else if (strcmp(colname[i], "app_onboot") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->onboot= strdup(coltxt[i]);
				else
					info->uiapp_info->onboot = NULL;
			} else if (strcmp(colname[i], "app_autorestart") == 0 ) {
				if (coltxt[i])
					info->uiapp_info->autorestart = strdup(coltxt[i]);
				else
					info->uiapp_info->autorestart = NULL;

			} else
				continue;
		}
		break;
	case PMINFO_SVC_APP:
		icon = calloc(1, sizeof(icon_x));
		LISTADD(info->svcapp_info->icon, icon);
		label = calloc(1, sizeof(label_x));
		LISTADD(info->svcapp_info->label, label);
		category = calloc(1, sizeof(category_x));
		LISTADD(info->svcapp_info->category, category);
		metadata = calloc(1, sizeof(metadata_x));
		LISTADD(info->svcapp_info->metadata, metadata);
		permission = calloc(1, sizeof(permission_x));
		LISTADD(info->svcapp_info->permission, permission);
		for(i = 0; i < ncols; i++)
		{
			if (strcmp(colname[i], "app_id") == 0) {
				/*appid being foreign key, is column in every table
				Hence appid gets strduped every time leading to memory leak.
				If appid is already set, just continue.*/
				if (info->svcapp_info->appid)
					continue;
				if (coltxt[i])
					info->svcapp_info->appid = strdup(coltxt[i]);
				else
					info->svcapp_info->appid = NULL;
			} else if (strcmp(colname[i], "app_exec") == 0) {
				if (coltxt[i])
					info->svcapp_info->exec = strdup(coltxt[i]);
				else
					info->svcapp_info->exec = NULL;
			} else if (strcmp(colname[i], "app_icon") == 0) {
				if (coltxt[i])
					info->svcapp_info->icon->text = strdup(coltxt[i]);
				else
					info->svcapp_info->icon->text = NULL;
			} else if (strcmp(colname[i], "app_label") == 0 ) {
				if (coltxt[i])
					info->svcapp_info->label->text = strdup(coltxt[i]);
				else
					info->svcapp_info->label->text = NULL;
			} else if (strcmp(colname[i], "app_type") == 0 ) {
				if (coltxt[i])
					info->svcapp_info->type = strdup(coltxt[i]);
				else
					info->svcapp_info->type = NULL;
			} else if (strcmp(colname[i], "app_onboot") == 0 ) {
				if (coltxt[i])
					info->svcapp_info->onboot = strdup(coltxt[i]);
				else
					info->svcapp_info->onboot = NULL;
			} else if (strcmp(colname[i], "app_autorestart") == 0 ) {
				if (coltxt[i])
					info->svcapp_info->autorestart = strdup(coltxt[i]);
				else
					info->svcapp_info->autorestart = NULL;
			} else if (strcmp(colname[i], "app_enabled") == 0 ) {
				if (coltxt[i])
					info->svcapp_info->enabled= strdup(coltxt[i]);
				else
					info->svcapp_info->enabled = NULL;
			} else if (strcmp(colname[i], "category") == 0 ) {
				if (coltxt[i])
					info->svcapp_info->category->name = strdup(coltxt[i]);
				else
					info->svcapp_info->category->name = NULL;
			} else if (strcmp(colname[i], "md_key") == 0 ) {
				if (coltxt[i])
					info->svcapp_info->metadata->key = strdup(coltxt[i]);
				else
					info->svcapp_info->metadata->key = NULL;
			} else if (strcmp(colname[i], "md_value") == 0 ) {
				if (coltxt[i])
					info->svcapp_info->metadata->value = strdup(coltxt[i]);
				else
					info->svcapp_info->metadata->value = NULL;
			} else if (strcmp(colname[i], "pm_type") == 0 ) {
				if (coltxt[i])
					info->svcapp_info->permission->type= strdup(coltxt[i]);
				else
					info->svcapp_info->permission->type = NULL;
			} else if (strcmp(colname[i], "pm_value") == 0 ) {
				if (coltxt[i])
					info->svcapp_info->permission->value = strdup(coltxt[i]);
				else
					info->svcapp_info->permission->value = NULL;
			} else if (strcmp(colname[i], "app_locale") == 0 ) {
				if (coltxt[i]) {
					info->svcapp_info->icon->lang = strdup(coltxt[i]);
					info->svcapp_info->label->lang = strdup(coltxt[i]);
				}
				else {
					info->svcapp_info->icon->lang = NULL;
					info->svcapp_info->label->lang = NULL;
				}
			} else if (strcmp(colname[i], "app_permissiontype") == 0 ) {
				if (coltxt[i])
					info->svcapp_info->permission_type = strdup(coltxt[i]);
				else
					info->svcapp_info->permission_type = NULL;
			} else
				continue;
		}
		break;
	default:
		break;
	}

	return 0;
}

static pkgmgrinfo_app_component __appcomponent_convert(const char *comp)
{
	if ( strcasecmp(comp, "uiapp") == 0)
		return PMINFO_UI_APP;
	else if ( strcasecmp(comp, "svcapp") == 0)
		return PMINFO_SVC_APP;
	else
		return -1;
}

static int __appcomponent_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)data;
	int i = 0;
	for(i = 0; i < ncols; i++)
	{
		if (strcmp(colname[i], "app_component") == 0) {
			info->app_component = __appcomponent_convert(coltxt[i]);
		} else if (strcmp(colname[i], "package") == 0) {
			info->package = strdup(coltxt[i]);
		}
	}

	return 0;
}

static int __app_list_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)data;
	int i = 0;
	int j = 0;
	uiapplication_x *uiapp = NULL;
	serviceapplication_x *svcapp = NULL;
	for(i = 0; i < ncols; i++)
	{
		if ((strcmp(colname[i], "app_component") == 0) ||
			(strcmp(colname[i], "package_app_info.app_component") == 0)) {
			if (coltxt[i]) {
				if (strcmp(coltxt[i], "uiapp") == 0) {
					uiapp = calloc(1, sizeof(uiapplication_x));
					if (uiapp == NULL) {
						_LOGE("Out of Memory!!!\n");
						return -1;
					}
					LISTADD(info->manifest_info->uiapplication, uiapp);
					for(j = 0; j < ncols; j++)
					{
						if ((strcmp(colname[j], "app_id") == 0) ||
							(strcmp(colname[j], "package_app_info.app_id") == 0)) {
							if (coltxt[j])
								info->manifest_info->uiapplication->appid = strdup(coltxt[j]);
						} else if (strcmp(colname[j], "package") == 0) {
							if (coltxt[j])
								info->manifest_info->uiapplication->package = strdup(coltxt[j]);
						} else
							continue;
					}
				} else {
					svcapp = calloc(1, sizeof(serviceapplication_x));
					if (svcapp == NULL) {
						_LOGE("Out of Memory!!!\n");
						return -1;
					}
					LISTADD(info->manifest_info->serviceapplication, svcapp);
					for(j = 0; j < ncols; j++)
					{
						if ((strcmp(colname[j], "app_id") == 0) ||
							(strcmp(colname[j], "package_app_info.app_id") == 0)) {
							if (coltxt[j])
								info->manifest_info->serviceapplication->appid = strdup(coltxt[j]);
						} else if (strcmp(colname[j], "package") == 0) {
							if (coltxt[j])
								info->manifest_info->serviceapplication->package = strdup(coltxt[j]);
						} else
							continue;
					}
				}
			}
		} else
			continue;
	}

	return 0;
}


static int __uiapp_list_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)data;
	int i = 0;
	uiapplication_x *uiapp = NULL;
	icon_x *icon = NULL;
	label_x *label = NULL;

	uiapp = calloc(1, sizeof(uiapplication_x));
	LISTADD(info->manifest_info->uiapplication, uiapp);
	icon = calloc(1, sizeof(icon_x));
	LISTADD(info->manifest_info->uiapplication->icon, icon);
	label = calloc(1, sizeof(label_x));
	LISTADD(info->manifest_info->uiapplication->label, label);

	for(i = 0; i < ncols; i++)
	{
		if (strcmp(colname[i], "app_id") == 0) {
			if (coltxt[i])
				info->manifest_info->uiapplication->appid = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->appid = NULL;
		} else if (strcmp(colname[i], "app_exec") == 0) {
			if (coltxt[i])
				info->manifest_info->uiapplication->exec = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->exec = NULL;
		} else if (strcmp(colname[i], "app_type") == 0 ){
			if (coltxt[i])
				info->manifest_info->uiapplication->type = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->type = NULL;
		} else if (strcmp(colname[i], "app_nodisplay") == 0 ){
			if (coltxt[i])
				info->manifest_info->uiapplication->nodisplay = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->nodisplay = NULL;
		} else if (strcmp(colname[i], "app_multiple") == 0 ){
			if (coltxt[i])
				info->manifest_info->uiapplication->multiple = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->multiple = NULL;
		} else if (strcmp(colname[i], "app_taskmanage") == 0 ){
			if (coltxt[i])
				info->manifest_info->uiapplication->taskmanage = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->taskmanage = NULL;
		} else if (strcmp(colname[i], "app_hwacceleration") == 0 ){
			if (coltxt[i])
				info->manifest_info->uiapplication->hwacceleration = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->hwacceleration = NULL;
		} else if (strcmp(colname[i], "app_screenreader") == 0 ){
			if (coltxt[i])
				info->manifest_info->uiapplication->screenreader = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->screenreader = NULL;
		} else if (strcmp(colname[i], "app_indicatordisplay") == 0 ){
			if (coltxt[i])
				info->manifest_info->uiapplication->indicatordisplay = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->indicatordisplay = NULL;
		} else if (strcmp(colname[i], "app_portraitimg") == 0 ){
			if (coltxt[i])
				info->manifest_info->uiapplication->portraitimg = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->portraitimg = NULL;
		} else if (strcmp(colname[i], "app_landscapeimg") == 0 ){
			if (coltxt[i])
				info->manifest_info->uiapplication->landscapeimg = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->landscapeimg = NULL;
		} else if (strcmp(colname[i], "app_guestmodevisibility") == 0 ){
			if (coltxt[i])
				info->manifest_info->uiapplication->guestmode_visibility = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->guestmode_visibility = NULL;
		} else if (strcmp(colname[i], "package") == 0 ){
			if (coltxt[i])
				info->manifest_info->uiapplication->package = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->package = NULL;
		} else if (strcmp(colname[i], "app_icon") == 0) {
			if (coltxt[i])
				info->manifest_info->uiapplication->icon->text = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->icon->text = NULL;
		} else if (strcmp(colname[i], "app_enabled") == 0 ) {
			if (coltxt[i])
				info->manifest_info->uiapplication->enabled= strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->enabled = NULL;
		} else if (strcmp(colname[i], "app_label") == 0 ) {
			if (coltxt[i])
				info->manifest_info->uiapplication->label->text = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->label->text = NULL;
		} else if (strcmp(colname[i], "app_recentimage") == 0 ) {
			if (coltxt[i])
				info->manifest_info->uiapplication->recentimage = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->recentimage = NULL;
		} else if (strcmp(colname[i], "app_mainapp") == 0 ) {
			if (coltxt[i])
				info->manifest_info->uiapplication->mainapp = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->mainapp = NULL;
		} else if (strcmp(colname[i], "app_locale") == 0 ) {
			if (coltxt[i]) {
				info->manifest_info->uiapplication->icon->lang = strdup(coltxt[i]);
				info->manifest_info->uiapplication->label->lang = strdup(coltxt[i]);
			}
			else {
				info->manifest_info->uiapplication->icon->lang = NULL;
				info->manifest_info->uiapplication->label->lang = NULL;
			}
		} else if (strcmp(colname[i], "app_permissiontype") == 0 ) {
			if (coltxt[i])
				info->manifest_info->uiapplication->permission_type = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->permission_type = NULL;
		} else if (strcmp(colname[i], "component_type") == 0 ) {
			if (coltxt[i])
				info->manifest_info->uiapplication->component_type = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->component_type = NULL;
		} else if (strcmp(colname[i], "app_preload") == 0 ) {
			if (coltxt[i])
				info->manifest_info->uiapplication->preload = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->preload = NULL;
		} else if (strcmp(colname[i], "app_submode") == 0 ) {
			if (coltxt[i])
				info->manifest_info->uiapplication->submode = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->submode = NULL;
		} else if (strcmp(colname[i], "app_submode_mainid") == 0 ) {
			if (coltxt[i])
				info->manifest_info->uiapplication->submode_mainid = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->submode_mainid = NULL;
		} else if (strcmp(colname[i], "app_installed_storage") == 0 ) {
			if (coltxt[i])
				info->manifest_info->uiapplication->installed_storage = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->installed_storage = NULL;
		} else if (strcmp(colname[i], "app_process_pool") == 0 ) {
			if (coltxt[i])
				info->manifest_info->uiapplication->process_pool = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->process_pool = NULL;
		} else if (strcmp(colname[i], "app_onboot") == 0 ) {
			if (coltxt[i])
				info->manifest_info->uiapplication->onboot = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->onboot = NULL;
		} else if (strcmp(colname[i], "app_autorestart") == 0 ) {
			if (coltxt[i])
				info->manifest_info->uiapplication->autorestart = strdup(coltxt[i]);
			else
				info->manifest_info->uiapplication->autorestart = NULL;
		} else
			continue;
	}
	return 0;
}

static int __svcapp_list_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)data;
	int i = 0;
	serviceapplication_x *svcapp = NULL;
	icon_x *icon = NULL;
	label_x *label = NULL;

	svcapp = calloc(1, sizeof(serviceapplication_x));
	LISTADD(info->manifest_info->serviceapplication, svcapp);
	icon = calloc(1, sizeof(icon_x));
	LISTADD(info->manifest_info->serviceapplication->icon, icon);
	label = calloc(1, sizeof(label_x));
	LISTADD(info->manifest_info->serviceapplication->label, label);
	for(i = 0; i < ncols; i++)
	{
		if (strcmp(colname[i], "app_id") == 0) {
			if (coltxt[i])
				info->manifest_info->serviceapplication->appid = strdup(coltxt[i]);
			else
				info->manifest_info->serviceapplication->appid = NULL;
		} else if (strcmp(colname[i], "app_exec") == 0) {
			if (coltxt[i])
				info->manifest_info->serviceapplication->exec = strdup(coltxt[i]);
			else
				info->manifest_info->serviceapplication->exec = NULL;
		} else if (strcmp(colname[i], "app_type") == 0 ){
			if (coltxt[i])
				info->manifest_info->serviceapplication->type = strdup(coltxt[i]);
			else
				info->manifest_info->serviceapplication->type = NULL;
		} else if (strcmp(colname[i], "app_onboot") == 0 ){
			if (coltxt[i])
				info->manifest_info->serviceapplication->onboot = strdup(coltxt[i]);
			else
				info->manifest_info->serviceapplication->onboot = NULL;
		} else if (strcmp(colname[i], "app_autorestart") == 0 ){
			if (coltxt[i])
				info->manifest_info->serviceapplication->autorestart = strdup(coltxt[i]);
			else
				info->manifest_info->serviceapplication->autorestart = NULL;
		} else if (strcmp(colname[i], "package") == 0 ){
			if (coltxt[i])
				info->manifest_info->serviceapplication->package = strdup(coltxt[i]);
			else
				info->manifest_info->serviceapplication->package = NULL;
		} else if (strcmp(colname[i], "app_icon") == 0) {
			if (coltxt[i])
				info->manifest_info->serviceapplication->icon->text = strdup(coltxt[i]);
			else
				info->manifest_info->serviceapplication->icon->text = NULL;
		} else if (strcmp(colname[i], "app_label") == 0 ) {
			if (coltxt[i])
				info->manifest_info->serviceapplication->label->text = strdup(coltxt[i]);
			else
				info->manifest_info->serviceapplication->label->text = NULL;
		} else if (strcmp(colname[i], "app_locale") == 0 ) {
			if (coltxt[i]) {
				info->manifest_info->serviceapplication->icon->lang = strdup(coltxt[i]);
				info->manifest_info->serviceapplication->label->lang = strdup(coltxt[i]);
			}
			else {
				info->manifest_info->serviceapplication->icon->lang = NULL;
				info->manifest_info->serviceapplication->label->lang = NULL;
			}
		} else if (strcmp(colname[i], "app_permissiontype") == 0 ) {
			if (coltxt[i])
				info->manifest_info->serviceapplication->permission_type = strdup(coltxt[i]);
			else
				info->manifest_info->serviceapplication->permission_type = NULL;
		} else
			continue;
	}
	return 0;
}

static int __allapp_list_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)data;
	int i = 0;
	int j = 0;
	uiapplication_x *uiapp = NULL;
	serviceapplication_x *svcapp = NULL;
	for(j = 0; j < ncols; j++)
	{
		if (strcmp(colname[j], "app_component") == 0) {
			if (coltxt[j]) {
				if (strcmp(coltxt[j], "uiapp") == 0) {
					uiapp = calloc(1, sizeof(uiapplication_x));
					if (uiapp == NULL) {
						_LOGE("Out of Memory!!!\n");
						return -1;
					}
					LISTADD(info->manifest_info->uiapplication, uiapp);
					for(i = 0; i < ncols; i++)
					{
						if (strcmp(colname[i], "app_id") == 0) {
							if (coltxt[i])
								info->manifest_info->uiapplication->appid = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->appid = NULL;
						} else if (strcmp(colname[i], "app_exec") == 0) {
							if (coltxt[i])
								info->manifest_info->uiapplication->exec = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->exec = NULL;
						} else if (strcmp(colname[i], "app_type") == 0 ){
							if (coltxt[i])
								info->manifest_info->uiapplication->type = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->type = NULL;
						} else if (strcmp(colname[i], "app_nodisplay") == 0 ){
							if (coltxt[i])
								info->manifest_info->uiapplication->nodisplay = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->nodisplay = NULL;
						} else if (strcmp(colname[i], "app_multiple") == 0 ){
							if (coltxt[i])
								info->manifest_info->uiapplication->multiple = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->multiple = NULL;
						} else if (strcmp(colname[i], "app_taskmanage") == 0 ){
							if (coltxt[i])
								info->manifest_info->uiapplication->taskmanage = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->taskmanage = NULL;
						} else if (strcmp(colname[i], "app_hwacceleration") == 0 ){
							if (coltxt[i])
								info->manifest_info->uiapplication->hwacceleration = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->hwacceleration = NULL;
						} else if (strcmp(colname[i], "app_screenreader") == 0 ){
							if (coltxt[i])
								info->manifest_info->uiapplication->screenreader = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->screenreader = NULL;
						} else if (strcmp(colname[i], "app_indicatordisplay") == 0 ){
							if (coltxt[i])
								info->manifest_info->uiapplication->indicatordisplay = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->indicatordisplay = NULL;
						} else if (strcmp(colname[i], "app_portraitimg") == 0 ){
							if (coltxt[i])
								info->manifest_info->uiapplication->portraitimg = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->portraitimg = NULL;
						} else if (strcmp(colname[i], "app_landscapeimg") == 0 ){
							if (coltxt[i])
								info->manifest_info->uiapplication->landscapeimg = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->landscapeimg = NULL;
						} else if (strcmp(colname[i], "app_guestmodevisibility") == 0 ){
							if (coltxt[i])
								info->manifest_info->uiapplication->guestmode_visibility = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->guestmode_visibility = NULL;
						} else if (strcmp(colname[i], "package") == 0 ){
							if (coltxt[i])
								info->manifest_info->uiapplication->package = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->package = NULL;
						} else if (strcmp(colname[i], "app_icon") == 0) {
							if (coltxt[i])
								info->manifest_info->uiapplication->icon->text = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->icon->text = NULL;
						} else if (strcmp(colname[i], "app_label") == 0 ) {
							if (coltxt[i])
								info->manifest_info->uiapplication->label->text = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->label->text = NULL;
						} else if (strcmp(colname[i], "app_recentimage") == 0 ) {
							if (coltxt[i])
								info->manifest_info->uiapplication->recentimage = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->recentimage = NULL;
						} else if (strcmp(colname[i], "app_mainapp") == 0 ) {
							if (coltxt[i])
								info->manifest_info->uiapplication->mainapp= strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->mainapp = NULL;
						} else if (strcmp(colname[i], "app_locale") == 0 ) {
							if (coltxt[i]) {
								info->manifest_info->uiapplication->icon->lang = strdup(coltxt[i]);
								info->manifest_info->uiapplication->label->lang = strdup(coltxt[i]);
							}
							else {
								info->manifest_info->uiapplication->icon->lang = NULL;
								info->manifest_info->uiapplication->label->lang = NULL;
							}
						} else if (strcmp(colname[i], "app_permissiontype") == 0 ) {
							if (coltxt[i])
								info->manifest_info->uiapplication->permission_type = strdup(coltxt[i]);
							else
								info->manifest_info->uiapplication->permission_type = NULL;
						} else
							continue;
					}
				} else {
					svcapp = calloc(1, sizeof(serviceapplication_x));
					if (svcapp == NULL) {
						_LOGE("Out of Memory!!!\n");
						return -1;
					}
					LISTADD(info->manifest_info->serviceapplication, svcapp);
					for(i = 0; i < ncols; i++)
					{
						if (strcmp(colname[i], "app_id") == 0) {
							if (coltxt[i])
								info->manifest_info->serviceapplication->appid = strdup(coltxt[i]);
							else
								info->manifest_info->serviceapplication->appid = NULL;
						} else if (strcmp(colname[i], "app_exec") == 0) {
							if (coltxt[i])
								info->manifest_info->serviceapplication->exec = strdup(coltxt[i]);
							else
								info->manifest_info->serviceapplication->exec = NULL;
						} else if (strcmp(colname[i], "app_type") == 0 ){
							if (coltxt[i])
								info->manifest_info->serviceapplication->type = strdup(coltxt[i]);
							else
								info->manifest_info->serviceapplication->type = NULL;
						} else if (strcmp(colname[i], "app_onboot") == 0 ){
							if (coltxt[i])
								info->manifest_info->serviceapplication->onboot = strdup(coltxt[i]);
							else
								info->manifest_info->serviceapplication->onboot = NULL;
						} else if (strcmp(colname[i], "app_autorestart") == 0 ){
							if (coltxt[i])
								info->manifest_info->serviceapplication->autorestart = strdup(coltxt[i]);
							else
								info->manifest_info->serviceapplication->autorestart = NULL;
						} else if (strcmp(colname[i], "package") == 0 ){
							if (coltxt[i])
								info->manifest_info->serviceapplication->package = strdup(coltxt[i]);
							else
								info->manifest_info->serviceapplication->package = NULL;
						} else if (strcmp(colname[i], "app_icon") == 0) {
							if (coltxt[i])
								info->manifest_info->serviceapplication->icon->text = strdup(coltxt[i]);
							else
								info->manifest_info->serviceapplication->icon->text = NULL;
						} else if (strcmp(colname[i], "app_label") == 0 ) {
							if (coltxt[i])
								info->manifest_info->serviceapplication->label->text = strdup(coltxt[i]);
							else
								info->manifest_info->serviceapplication->label->text = NULL;
						} else if (strcmp(colname[i], "app_locale") == 0 ) {
							if (coltxt[i]) {
								info->manifest_info->serviceapplication->icon->lang = strdup(coltxt[i]);
								info->manifest_info->serviceapplication->label->lang = strdup(coltxt[i]);
							}
							else {
								info->manifest_info->serviceapplication->icon->lang = NULL;
								info->manifest_info->serviceapplication->label->lang = NULL;
							}
						} else if (strcmp(colname[i], "app_permissiontype") == 0 ) {
							if (coltxt[i])
								info->manifest_info->serviceapplication->permission_type = strdup(coltxt[i]);
							else
								info->manifest_info->serviceapplication->permission_type = NULL;
						} else
							continue;
					}
				}
			}
		} else
			continue;
	}

	return 0;
}

API int pkgmgrinfo_appinfo_get_list(pkgmgrinfo_pkginfo_h handle, pkgmgrinfo_app_component component,
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

	/*set component type*/
	if (component == PMINFO_UI_APP)
		appinfo->app_component = PMINFO_UI_APP;
	if (component == PMINFO_SVC_APP)
		appinfo->app_component = PMINFO_SVC_APP;
	if (component == PMINFO_ALL_APP)
		appinfo->app_component = PMINFO_ALL_APP;

	/*open db */
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	appinfo->package = strdup(info->manifest_info->package);
	snprintf(query, MAX_QUERY_LEN, "select DISTINCT * " \
			"from package_app_info where " \
			"package='%s' and app_component='%s'",
			info->manifest_info->package,
			(appinfo->app_component==PMINFO_UI_APP ? "uiapp" : "svcapp"));

	switch(component) {
	case PMINFO_UI_APP:
		/*Populate ui app info */
		ret = __exec_db_query(appinfo_db, query, __uiapp_list_cb, (void *)info);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info list retrieval failed");

		uiapplication_x *tmp = NULL;
		if (info->manifest_info->uiapplication) {
			LISTHEAD(info->manifest_info->uiapplication, tmp);
			info->manifest_info->uiapplication = tmp;
		}
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
			snprintf(query, MAX_QUERY_LEN, "select * from package_app_localized_info where app_id='%s' and app_locale='%s'", appinfo->uiapp_info->appid, locale);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from package_app_localized_info where app_id='%s' and app_locale='%s'", appinfo->uiapp_info->appid, DEFAULT_LOCALE);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

			/*store setting notification icon section*/
			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from package_app_icon_section_info where app_id='%s'", appinfo->uiapp_info->appid);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App icon section Info DB Information retrieval failed");
			
			/*store app preview image info*/
			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select app_image_section, app_image from package_app_image_info where app_id='%s'", appinfo->uiapp_info->appid);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App image Info DB Information retrieval failed");

			/*Populate app category*/
			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from package_app_app_category where app_id=='%s'%", appinfo->uiapp_info->appid);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Category Info DB Information retrieval failed");

			/*Populate app metadata*/
			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from package_app_app_metadata where app_id='%s'", appinfo->uiapp_info->appid);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Metadata Info DB Information retrieval failed");

			if (appinfo->uiapp_info->label) {
				LISTHEAD(appinfo->uiapp_info->label, ptr2);
				appinfo->uiapp_info->label = ptr2;
			}
			if (appinfo->uiapp_info->icon) {
				LISTHEAD(appinfo->uiapp_info->icon, ptr1);
				appinfo->uiapp_info->icon = ptr1;
			}
			if (appinfo->uiapp_info->category) {
				LISTHEAD(appinfo->uiapp_info->category, ptr3);
				appinfo->uiapp_info->category = ptr3;
			}
			if (appinfo->uiapp_info->metadata) {
				LISTHEAD(appinfo->uiapp_info->metadata, ptr4);
				appinfo->uiapp_info->metadata = ptr4;
			}
			if (appinfo->uiapp_info->permission) {
				LISTHEAD(appinfo->uiapp_info->permission, ptr5);
				appinfo->uiapp_info->permission = ptr5;
			}
			if (appinfo->uiapp_info->image) {
				LISTHEAD(appinfo->uiapp_info->image, ptr6);
				appinfo->uiapp_info->image = ptr6;
			}
			ret = app_func((void *)appinfo, user_data);
			if (ret < 0)
				break;
			tmp = tmp->next;
		}
		break;
	case PMINFO_SVC_APP:
		/*Populate svc app info */
		ret = __exec_db_query(appinfo_db, query, __svcapp_list_cb, (void *)info);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info list retrieval failed");

		serviceapplication_x *tmp1 = NULL;
		if (info->manifest_info->serviceapplication) {
			LISTHEAD(info->manifest_info->serviceapplication, tmp1);
			info->manifest_info->serviceapplication = tmp1;
		}
		/*Populate localized info for default locales and call callback*/
		/*If the callback func return < 0 we break and no more call back is called*/
		while(tmp1 != NULL)
		{
			appinfo->locale = strdup(locale);
			appinfo->svcapp_info = tmp1;
			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from package_app_localized_info where app_id='%s' and app_locale='%s'", appinfo->svcapp_info->appid, locale);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from package_app_localized_info where app_id='%s' and app_locale='%s'", appinfo->svcapp_info->appid, DEFAULT_LOCALE);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

			if (appinfo->svcapp_info->label) {
				LISTHEAD(appinfo->svcapp_info->label, ptr2);
				appinfo->svcapp_info->label = ptr2;
			}
			if (appinfo->svcapp_info->icon) {
				LISTHEAD(appinfo->svcapp_info->icon, ptr1);
				appinfo->svcapp_info->icon = ptr1;
			}
			if (appinfo->svcapp_info->category) {
				LISTHEAD(appinfo->svcapp_info->category, ptr3);
				appinfo->svcapp_info->category = ptr3;
			}
			if (appinfo->svcapp_info->metadata) {
				LISTHEAD(appinfo->svcapp_info->metadata, ptr4);
				appinfo->svcapp_info->metadata = ptr4;
			}
			if (appinfo->svcapp_info->permission) {
				LISTHEAD(appinfo->svcapp_info->permission, ptr5);
				appinfo->svcapp_info->permission = ptr5;
			}
			ret = app_func((void *)appinfo, user_data);
			if (ret < 0)
				break;
			tmp1 = tmp1->next;
		}
		break;
	case PMINFO_ALL_APP:
		memset(query, '\0', MAX_QUERY_LEN);
		snprintf(query, MAX_QUERY_LEN, "select * from package_app_info where package='%s'", info->manifest_info->package);

		/*Populate all app info */
		ret = __exec_db_query(appinfo_db, query, __allapp_list_cb, (void *)allinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info list retrieval failed");

		/*UI Apps*/
		appinfo->app_component = PMINFO_UI_APP;
		uiapplication_x *tmp2 = NULL;
		if (allinfo->manifest_info->uiapplication) {
			LISTHEAD(allinfo->manifest_info->uiapplication, tmp2);
			allinfo->manifest_info->uiapplication = tmp2;
		}
		/*Populate localized info for default locales and call callback*/
		/*If the callback func return < 0 we break and no more call back is called*/
		while(tmp2 != NULL)
		{
			appinfo->locale = strdup(locale);
			appinfo->uiapp_info = tmp2;
			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from package_app_localized_info where app_id='%s' and app_locale='%s'", appinfo->uiapp_info->appid, locale);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from package_app_localized_info where app_id='%s' and app_locale='%s'", appinfo->uiapp_info->appid, DEFAULT_LOCALE);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

			/*store setting notification icon section*/
			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from package_app_icon_section_info where app_id='%s'", appinfo->uiapp_info->appid);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App icon section Info DB Information retrieval failed");
			
			/*store app preview image info*/
			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select app_image_section, app_image from package_app_image_info where app_id='%s'", appinfo->uiapp_info->appid);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App image Info DB Information retrieval failed");

			if (appinfo->uiapp_info->label) {
				LISTHEAD(appinfo->uiapp_info->label, ptr2);
				appinfo->uiapp_info->label = ptr2;
			}
			if (appinfo->uiapp_info->icon) {
				LISTHEAD(appinfo->uiapp_info->icon, ptr1);
				appinfo->uiapp_info->icon = ptr1;
			}
			if (appinfo->uiapp_info->category) {
				LISTHEAD(appinfo->uiapp_info->category, ptr3);
				appinfo->uiapp_info->category = ptr3;
			}
			if (appinfo->uiapp_info->metadata) {
				LISTHEAD(appinfo->uiapp_info->metadata, ptr4);
				appinfo->uiapp_info->metadata = ptr4;
			}
			if (appinfo->uiapp_info->permission) {
				LISTHEAD(appinfo->uiapp_info->permission, ptr5);
				appinfo->uiapp_info->permission = ptr5;
			}
			if (appinfo->uiapp_info->image) {
				LISTHEAD(appinfo->uiapp_info->image, ptr6);
				appinfo->uiapp_info->image = ptr6;
			}
			ret = app_func((void *)appinfo, user_data);
			if (ret < 0)
				break;
			tmp2 = tmp2->next;
		}

		/*SVC Apps*/
		appinfo->app_component = PMINFO_SVC_APP;
		serviceapplication_x *tmp3 = NULL;
		if (allinfo->manifest_info->serviceapplication) {
			LISTHEAD(allinfo->manifest_info->serviceapplication, tmp3);
			allinfo->manifest_info->serviceapplication = tmp3;
		}
		/*Populate localized info for default locales and call callback*/
		/*If the callback func return < 0 we break and no more call back is called*/
		while(tmp3 != NULL)
		{
			appinfo->locale = strdup(locale);
			appinfo->svcapp_info = tmp3;
			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from package_app_localized_info where app_id='%s' and app_locale='%s'", appinfo->svcapp_info->appid, locale);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from package_app_localized_info where app_id='%s' and app_locale='%s'", appinfo->svcapp_info->appid, DEFAULT_LOCALE);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

			if (appinfo->svcapp_info->label) {
				LISTHEAD(appinfo->svcapp_info->label, ptr2);
				appinfo->svcapp_info->label = ptr2;
			}
			if (appinfo->svcapp_info->icon) {
				LISTHEAD(appinfo->svcapp_info->icon, ptr1);
				appinfo->svcapp_info->icon = ptr1;
			}
			if (appinfo->svcapp_info->category) {
				LISTHEAD(appinfo->svcapp_info->category, ptr3);
				appinfo->svcapp_info->category = ptr3;
			}
			if (appinfo->svcapp_info->metadata) {
				LISTHEAD(appinfo->svcapp_info->metadata, ptr4);
				appinfo->svcapp_info->metadata = ptr4;
			}
			if (appinfo->svcapp_info->permission) {
				LISTHEAD(appinfo->svcapp_info->permission, ptr5);
				appinfo->svcapp_info->permission = ptr5;
			}
			ret = app_func((void *)appinfo, user_data);
			if (ret < 0)
				break;
			tmp3 = tmp3->next;
		}
		appinfo->app_component = PMINFO_ALL_APP;
		break;

	}

	ret = PMINFO_R_OK;
catch:
	if (locale) {
		free(locale);
		locale = NULL;
	}
	if (appinfo) {
		if (appinfo->package) {
			free((void *)appinfo->package);
			appinfo->package = NULL;
		}
		free(appinfo);
		appinfo = NULL;
	}
	__cleanup_pkginfo(allinfo);

	sqlite3_close(appinfo_db);
	return ret;
}

API int pkgmgrinfo_appinfo_get_install_list(pkgmgrinfo_app_list_cb app_func, void *user_data)
{
	retvm_if(app_func == NULL, PMINFO_R_EINVAL, "callback function is NULL");

	int ret = PMINFO_R_OK;
	char query[MAX_QUERY_LEN] = {'\0'};
	pkgmgr_appinfo_x *appinfo = NULL;
	uiapplication_x *ptr1 = NULL;
	serviceapplication_x *ptr2 = NULL;
	sqlite3 *appinfo_db = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	retvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*calloc pkginfo*/
	pkgmgr_pkginfo_x *info = NULL;
	info = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*calloc manifest_info*/
	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(info->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*calloc appinfo*/
	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	snprintf(query, MAX_QUERY_LEN, "select * from package_app_info");
	ret = __exec_db_query(appinfo_db, query, __mini_appinfo_cb, (void *)info);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	if (info->manifest_info->uiapplication) {
		LISTHEAD(info->manifest_info->uiapplication, ptr1);
		info->manifest_info->uiapplication = ptr1;
	}
	if (info->manifest_info->serviceapplication) {
		LISTHEAD(info->manifest_info->serviceapplication, ptr2);
		info->manifest_info->serviceapplication = ptr2;
	}

	/*UI Apps*/
	for(ptr1 = info->manifest_info->uiapplication; ptr1; ptr1 = ptr1->next)
	{
		appinfo->app_component = PMINFO_UI_APP;
		appinfo->package = strdup(ptr1->package);
		appinfo->uiapp_info = ptr1;

		ret = app_func((void *)appinfo, user_data);
		if (ret < 0)
			break;
		free((void *)appinfo->package);
		appinfo->package = NULL;
	}
	/*Service Apps*/
	for(ptr2 = info->manifest_info->serviceapplication; ptr2; ptr2 = ptr2->next)
	{
		appinfo->app_component = PMINFO_SVC_APP;
		appinfo->package = strdup(ptr2->package);
		appinfo->svcapp_info = ptr2;

		ret = app_func((void *)appinfo, user_data);
		if (ret < 0)
			break;
		free((void *)appinfo->package);
		appinfo->package = NULL;
	}
	ret = PMINFO_R_OK;

catch:
	sqlite3_close(appinfo_db);

	if (appinfo) {
		free(appinfo);
		appinfo = NULL;
	}
	__cleanup_pkginfo(info);
	return ret;
}

API int pkgmgrinfo_appinfo_get_installed_list(pkgmgrinfo_app_list_cb app_func, void *user_data)
{
	retvm_if(app_func == NULL, PMINFO_R_EINVAL, "callback function is NULL");

	int ret = PMINFO_R_OK;
	char query[MAX_QUERY_LEN] = {'\0'};
	char *locale = NULL;
	pkgmgr_appinfo_x *appinfo = NULL;
	uiapplication_x *ptr1 = NULL;
	serviceapplication_x *ptr2 = NULL;
	label_x *tmp1 = NULL;
	icon_x *tmp2 = NULL;
	category_x *tmp3 = NULL;
	metadata_x *tmp4 = NULL;
	permission_x *tmp5 = NULL;
	image_x *tmp6 = NULL;
	sqlite3 *appinfo_db = NULL;

	/*get system locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "manifest locale is NULL");

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	retvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*calloc pkginfo*/
	pkgmgr_pkginfo_x *info = NULL;
	info = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*calloc manifest_info*/
	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(info->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*calloc appinfo*/
	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	snprintf(query, MAX_QUERY_LEN, "select * from package_app_info");
	ret = __exec_db_query(appinfo_db, query, __app_list_cb, (void *)info);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	if (info->manifest_info->uiapplication) {
		LISTHEAD(info->manifest_info->uiapplication, ptr1);
		info->manifest_info->uiapplication = ptr1;
	}
	if (info->manifest_info->serviceapplication) {
		LISTHEAD(info->manifest_info->serviceapplication, ptr2);
		info->manifest_info->serviceapplication = ptr2;
	}

	/*UI Apps*/
	for(ptr1 = info->manifest_info->uiapplication; ptr1; ptr1 = ptr1->next)
	{
		appinfo->locale = strdup(locale);
		appinfo->app_component = PMINFO_UI_APP;
		appinfo->package = strdup(ptr1->package);
		appinfo->uiapp_info = ptr1;
		snprintf(query, MAX_QUERY_LEN, "select DISTINCT * " \
				"from package_app_info where " \
				"app_id='%s'", ptr1->appid);
		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

		if (strcmp(appinfo->uiapp_info->type,"c++app") == 0){
			if (locale) {
				free(locale);
			}
			locale = __get_app_locale_by_fallback(appinfo_db, ptr1->appid);
		}

		memset(query, '\0', MAX_QUERY_LEN);
		snprintf(query, MAX_QUERY_LEN, "select DISTINCT * " \
				"from package_app_localized_info where " \
				"app_id='%s' and app_locale='%s'",
				ptr1->appid, locale);
		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

		memset(query, '\0', MAX_QUERY_LEN);
		snprintf(query, MAX_QUERY_LEN, "select DISTINCT * " \
				"from package_app_localized_info where " \
				"app_id='%s' and app_locale='%s'",
				ptr1->appid, DEFAULT_LOCALE);

		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

		/*store setting notification icon section*/
		memset(query, '\0', MAX_QUERY_LEN);
		snprintf(query, MAX_QUERY_LEN, "select * from package_app_icon_section_info where app_id='%s'", ptr1->appid);
		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App icon section Info DB Information retrieval failed");
		
		/*store app preview image info*/
		memset(query, '\0', MAX_QUERY_LEN);
		snprintf(query, MAX_QUERY_LEN, "select app_image_section, app_image from package_app_image_info where app_id='%s'", ptr1->appid);
		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App image Info DB Information retrieval failed");

		if (appinfo->uiapp_info->label) {
			LISTHEAD(appinfo->uiapp_info->label, tmp1);
			appinfo->uiapp_info->label = tmp1;
		}
		if (appinfo->uiapp_info->icon) {
			LISTHEAD(appinfo->uiapp_info->icon, tmp2);
			appinfo->uiapp_info->icon= tmp2;
		}
		if (appinfo->uiapp_info->category) {
			LISTHEAD(appinfo->uiapp_info->category, tmp3);
			appinfo->uiapp_info->category = tmp3;
		}
		if (appinfo->uiapp_info->metadata) {
			LISTHEAD(appinfo->uiapp_info->metadata, tmp4);
			appinfo->uiapp_info->metadata = tmp4;
		}
		if (appinfo->uiapp_info->permission) {
			LISTHEAD(appinfo->uiapp_info->permission, tmp5);
			appinfo->uiapp_info->permission = tmp5;
		}
		if (appinfo->uiapp_info->image) {
			LISTHEAD(appinfo->uiapp_info->image, tmp6);
			appinfo->uiapp_info->image = tmp6;
		}

		ret = __appinfo_check_installed_storage(appinfo);
		if(ret < 0)
			continue;

		ret = app_func((void *)appinfo, user_data);
		if (ret < 0)
			break;
		free((void *)appinfo->package);
		appinfo->package = NULL;
	}
	/*Service Apps*/
	for(ptr2 = info->manifest_info->serviceapplication; ptr2; ptr2 = ptr2->next)
	{
		appinfo->locale = strdup(locale);
		appinfo->app_component = PMINFO_SVC_APP;
		appinfo->package = strdup(ptr2->package);
		appinfo->svcapp_info = ptr2;
		memset(query, '\0', MAX_QUERY_LEN);
		snprintf(query, MAX_QUERY_LEN, "select DISTINCT * " \
				"from package_app_info where " \
				"app_id='%s'", ptr2->appid);
		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

		memset(query, '\0', MAX_QUERY_LEN);
		snprintf(query, MAX_QUERY_LEN, "select DISTINCT * " \
				"from package_app_localized_info where " \
				"app_id='%s' and app_locale='%s'",
				ptr2->appid, locale);
		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

		memset(query, '\0', MAX_QUERY_LEN);
		snprintf(query, MAX_QUERY_LEN, "select DISTINCT * " \
				"from package_app_localized_info where " \
				"app_id='%s' and app_locale='%s'",
				ptr2->appid, DEFAULT_LOCALE);
		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

		if (appinfo->svcapp_info->label) {
			LISTHEAD(appinfo->svcapp_info->label, tmp1);
			appinfo->svcapp_info->label = tmp1;
		}
		if (appinfo->svcapp_info->icon) {
			LISTHEAD(appinfo->svcapp_info->icon, tmp2);
			appinfo->svcapp_info->icon= tmp2;
		}
		if (appinfo->svcapp_info->category) {
			LISTHEAD(appinfo->svcapp_info->category, tmp3);
			appinfo->svcapp_info->category = tmp3;
		}
		if (appinfo->svcapp_info->metadata) {
			LISTHEAD(appinfo->svcapp_info->metadata, tmp4);
			appinfo->svcapp_info->metadata = tmp4;
		}
		if (appinfo->svcapp_info->permission) {
			LISTHEAD(appinfo->svcapp_info->permission, tmp5);
			appinfo->svcapp_info->permission = tmp5;
		}
		ret = app_func((void *)appinfo, user_data);
		if (ret < 0)
			break;
		free((void *)appinfo->package);
		appinfo->package = NULL;
	}
	ret = PMINFO_R_OK;

catch:
	if (locale) {
		free(locale);
		locale = NULL;
	}
	sqlite3_close(appinfo_db);
	if (appinfo) {
		free(appinfo);
		appinfo = NULL;
	}
	__cleanup_pkginfo(info);
	return ret;
}

API int pkgmgrinfo_appinfo_get_mounted_list(pkgmgrinfo_app_list_cb app_func, void *user_data)
{
	retvm_if(app_func == NULL, PMINFO_R_EINVAL, "callback function is NULL");

	int ret = PMINFO_R_OK;
	char query[MAX_QUERY_LEN] = {'\0'};
	char *locale = NULL;
	pkgmgr_appinfo_x *appinfo = NULL;
	uiapplication_x *ptr1 = NULL;
	label_x *tmp1 = NULL;
	sqlite3 *appinfo_db = NULL;

	/*get system locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "manifest locale is NULL");

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	retvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*calloc pkginfo*/
	pkgmgr_pkginfo_x *info = NULL;
	info = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*calloc manifest_info*/
	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(info->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*calloc appinfo*/
	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	snprintf(query, MAX_QUERY_LEN, "select * from package_app_info where app_installed_storage='installed_external'");
	ret = __exec_db_query(appinfo_db, query, __app_list_cb, (void *)info);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	if (info->manifest_info->uiapplication) {
		LISTHEAD(info->manifest_info->uiapplication, ptr1);
		info->manifest_info->uiapplication = ptr1;
	}

	/*UI Apps*/
	for(ptr1 = info->manifest_info->uiapplication; ptr1; ptr1 = ptr1->next)
	{
		appinfo->locale = strdup(locale);
		appinfo->app_component = PMINFO_UI_APP;
		appinfo->package = strdup(ptr1->package);
		appinfo->uiapp_info = ptr1;
		snprintf(query, MAX_QUERY_LEN, "select DISTINCT * " \
				"from package_app_info where " \
				"app_id='%s'", ptr1->appid);
		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

		if (strcmp(appinfo->uiapp_info->type,"c++app") == 0){
			if (locale) {
				free(locale);
			}
			locale = __get_app_locale_by_fallback(appinfo_db, ptr1->appid);
		}

		memset(query, '\0', MAX_QUERY_LEN);
		snprintf(query, MAX_QUERY_LEN, "select DISTINCT * " \
				"from package_app_localized_info where " \
				"app_id='%s' and app_locale='%s'",
				ptr1->appid, locale);
		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

		memset(query, '\0', MAX_QUERY_LEN);
		snprintf(query, MAX_QUERY_LEN, "select DISTINCT * " \
				"from package_app_localized_info where " \
				"app_id='%s' and app_locale='%s'",
				ptr1->appid, DEFAULT_LOCALE);

		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");


		if (appinfo->uiapp_info->label) {
			LISTHEAD(appinfo->uiapp_info->label, tmp1);
			appinfo->uiapp_info->label = tmp1;
		}

		ret = __appinfo_check_installed_storage(appinfo);
		if(ret < 0)
			continue;

		ret = app_func((void *)appinfo, user_data);
		if (ret < 0)
			break;
		free((void *)appinfo->package);
		appinfo->package = NULL;
	}

	ret = PMINFO_R_OK;

catch:
	if (locale) {
		free(locale);
		locale = NULL;
	}
	sqlite3_close(appinfo_db);
	if (appinfo) {
		free(appinfo);
		appinfo = NULL;
	}
	__cleanup_pkginfo(info);
	return ret;
}

API int pkgmgrinfo_appinfo_get_unmounted_list(pkgmgrinfo_app_list_cb app_func, void *user_data)
{
	retvm_if(app_func == NULL, PMINFO_R_EINVAL, "callback function is NULL");

	int ret = PMINFO_R_OK;
	char query[MAX_QUERY_LEN] = {'\0'};
	char *locale = NULL;
	pkgmgr_appinfo_x *appinfo = NULL;
	uiapplication_x *ptr1 = NULL;
	label_x *tmp1 = NULL;
	sqlite3 *appinfo_db = NULL;

	/*get system locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "manifest locale is NULL");

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	retvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*calloc pkginfo*/
	pkgmgr_pkginfo_x *info = NULL;
	info = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*calloc manifest_info*/
	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(info->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*calloc appinfo*/
	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	snprintf(query, MAX_QUERY_LEN, "select * from package_app_info where app_installed_storage='installed_external'");
	ret = __exec_db_query(appinfo_db, query, __app_list_cb, (void *)info);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	if (info->manifest_info->uiapplication) {
		LISTHEAD(info->manifest_info->uiapplication, ptr1);
		info->manifest_info->uiapplication = ptr1;
	}

	/*UI Apps*/
	for(ptr1 = info->manifest_info->uiapplication; ptr1; ptr1 = ptr1->next)
	{
		appinfo->locale = strdup(locale);
		appinfo->app_component = PMINFO_UI_APP;
		appinfo->package = strdup(ptr1->package);
		appinfo->uiapp_info = ptr1;
		snprintf(query, MAX_QUERY_LEN, "select DISTINCT * " \
				"from package_app_info where " \
				"app_id='%s'", ptr1->appid);
		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

		if (strcmp(appinfo->uiapp_info->type,"c++app") == 0){
			if (locale) {
				free(locale);
			}
			locale = __get_app_locale_by_fallback(appinfo_db, ptr1->appid);
		}

		memset(query, '\0', MAX_QUERY_LEN);
		snprintf(query, MAX_QUERY_LEN, "select DISTINCT * " \
				"from package_app_localized_info where " \
				"app_id='%s' and app_locale='%s'",
				ptr1->appid, locale);
		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

		memset(query, '\0', MAX_QUERY_LEN);
		snprintf(query, MAX_QUERY_LEN, "select DISTINCT * " \
				"from package_app_localized_info where " \
				"app_id='%s' and app_locale='%s'",
				ptr1->appid, DEFAULT_LOCALE);

		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

		if (appinfo->uiapp_info->label) {
			LISTHEAD(appinfo->uiapp_info->label, tmp1);
			appinfo->uiapp_info->label = tmp1;
		}

		ret = app_func((void *)appinfo, user_data);
		if (ret < 0)
			break;
		free((void *)appinfo->package);
		appinfo->package = NULL;
	}

	ret = PMINFO_R_OK;

catch:
	if (locale) {
		free(locale);
		locale = NULL;
	}
	sqlite3_close(appinfo_db);
	if (appinfo) {
		free(appinfo);
		appinfo = NULL;
	}
	__cleanup_pkginfo(info);
	return ret;
}

API int pkgmgrinfo_appinfo_get_unmounted_appinfo(const char *appid, pkgmgrinfo_appinfo_h *handle)
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
	permission_x *tmp5 = NULL;
	image_x *tmp6 = NULL;
	char *query = NULL;
	sqlite3 *appinfo_db = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	retvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*check appid exist on db*/
	query = sqlite3_mprintf("select exists(select * from package_app_info where app_id=%Q)", appid);
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

	/*check app_component from DB*/
	query = sqlite3_mprintf("select app_component, package from package_app_info where app_id=%Q", appid);
	ret = __exec_db_query(appinfo_db, query, __appcomponent_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	/*calloc app_component*/
	if (appinfo->app_component == PMINFO_UI_APP) {
		appinfo->uiapp_info = (uiapplication_x *)calloc(1, sizeof(uiapplication_x));
		tryvm_if(appinfo->uiapp_info == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for uiapp info");
	} else {
		appinfo->svcapp_info = (serviceapplication_x *)calloc(1, sizeof(serviceapplication_x));
		tryvm_if(appinfo->svcapp_info == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for svcapp info");
	}
	appinfo->locale = strdup(locale);

	/*populate app_info from DB*/
	query = sqlite3_mprintf("select * from package_app_info where app_id=%Q ", appid);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	query = sqlite3_mprintf("select * from package_app_localized_info where app_id=%Q and app_locale=%Q", appid, locale);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	/*Also store the values corresponding to default locales*/
	query = sqlite3_mprintf("select * from package_app_localized_info where app_id=%Q and app_locale=%Q", appid, DEFAULT_LOCALE);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

	/*Populate app category*/
	query = sqlite3_mprintf("select * from package_app_app_category where app_id=%Q", appid);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Category Info DB Information retrieval failed");

	/*Populate app metadata*/
	query = sqlite3_mprintf("select * from package_app_app_metadata where app_id=%Q", appid);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Metadata Info DB Information retrieval failed");

	/*Populate app permission*/
	query = sqlite3_mprintf("select * from package_app_app_permission where app_id=%Q", appid);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App permission Info DB Information retrieval failed");

	/*store setting notification icon section*/
	query = sqlite3_mprintf("select * from package_app_icon_section_info where app_id=%Q", appid);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App icon section Info DB Information retrieval failed");

	/*store app preview image info*/
	query = sqlite3_mprintf("select app_image_section, app_image from package_app_image_info where app_id=%Q", appid);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App image Info DB Information retrieval failed");

	switch (appinfo->app_component) {
	case PMINFO_UI_APP:
		if (appinfo->uiapp_info->label) {
			LISTHEAD(appinfo->uiapp_info->label, tmp1);
			appinfo->uiapp_info->label = tmp1;
		}
		if (appinfo->uiapp_info->icon) {
			LISTHEAD(appinfo->uiapp_info->icon, tmp2);
			appinfo->uiapp_info->icon = tmp2;
		}
		if (appinfo->uiapp_info->category) {
			LISTHEAD(appinfo->uiapp_info->category, tmp3);
			appinfo->uiapp_info->category = tmp3;
		}
		if (appinfo->uiapp_info->metadata) {
			LISTHEAD(appinfo->uiapp_info->metadata, tmp4);
			appinfo->uiapp_info->metadata = tmp4;
		}
		if (appinfo->uiapp_info->permission) {
			LISTHEAD(appinfo->uiapp_info->permission, tmp5);
			appinfo->uiapp_info->permission = tmp5;
		}
		if (appinfo->uiapp_info->image) {
			LISTHEAD(appinfo->uiapp_info->image, tmp6);
			appinfo->uiapp_info->image = tmp6;
		}
		break;
	case PMINFO_SVC_APP:
		if (appinfo->svcapp_info->label) {
			LISTHEAD(appinfo->svcapp_info->label, tmp1);
			appinfo->svcapp_info->label = tmp1;
		}
		if (appinfo->svcapp_info->icon) {
			LISTHEAD(appinfo->svcapp_info->icon, tmp2);
			appinfo->svcapp_info->icon = tmp2;
		}
		if (appinfo->svcapp_info->category) {
			LISTHEAD(appinfo->svcapp_info->category, tmp3);
			appinfo->svcapp_info->category = tmp3;
		}
		if (appinfo->svcapp_info->metadata) {
			LISTHEAD(appinfo->svcapp_info->metadata, tmp4);
			appinfo->svcapp_info->metadata = tmp4;
		}
		if (appinfo->svcapp_info->permission) {
			LISTHEAD(appinfo->svcapp_info->permission, tmp5);
			appinfo->svcapp_info->permission = tmp5;
		}
		break;
	default:
		break;
	}

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

	/*set component type*/
	if (component == PMINFO_UI_APP)
		appinfo->app_component = PMINFO_UI_APP;
	if (component == PMINFO_SVC_APP)
		appinfo->app_component = PMINFO_SVC_APP;
	if (component == PMINFO_ALL_APP)
		appinfo->app_component = PMINFO_ALL_APP;

	/*open db */
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	appinfo->package = strdup(info->manifest_info->package);
	snprintf(query, MAX_QUERY_LEN, "select DISTINCT * " \
			"from disabled_package_app_info where " \
			"package='%s' and app_component='%s'",
			info->manifest_info->package,
			(appinfo->app_component==PMINFO_UI_APP ? "uiapp" : "svcapp"));

	switch(component) {
	case PMINFO_UI_APP:
		/*Populate ui app info */
		ret = __exec_db_query(appinfo_db, query, __uiapp_list_cb, (void *)info);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info list retrieval failed");

		uiapplication_x *tmp = NULL;
		if (info->manifest_info->uiapplication) {
			LISTHEAD(info->manifest_info->uiapplication, tmp);
			info->manifest_info->uiapplication = tmp;
		}
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
			snprintf(query, MAX_QUERY_LEN, "select * from disabled_package_app_app_category where app_id=='%s'%", appinfo->uiapp_info->appid);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Category Info DB Information retrieval failed");

			/*Populate app metadata*/
			memset(query, '\0', MAX_QUERY_LEN);
			snprintf(query, MAX_QUERY_LEN, "select * from disabled_package_app_app_metadata where app_id='%s'", appinfo->uiapp_info->appid);
			ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
			tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Metadata Info DB Information retrieval failed");

			if (appinfo->uiapp_info->label) {
				LISTHEAD(appinfo->uiapp_info->label, ptr2);
				appinfo->uiapp_info->label = ptr2;
			}
			if (appinfo->uiapp_info->icon) {
				LISTHEAD(appinfo->uiapp_info->icon, ptr1);
				appinfo->uiapp_info->icon = ptr1;
			}
			if (appinfo->uiapp_info->category) {
				LISTHEAD(appinfo->uiapp_info->category, ptr3);
				appinfo->uiapp_info->category = ptr3;
			}
			if (appinfo->uiapp_info->metadata) {
				LISTHEAD(appinfo->uiapp_info->metadata, ptr4);
				appinfo->uiapp_info->metadata = ptr4;
			}
			if (appinfo->uiapp_info->permission) {
				LISTHEAD(appinfo->uiapp_info->permission, ptr5);
				appinfo->uiapp_info->permission = ptr5;
			}
			if (appinfo->uiapp_info->image) {
				LISTHEAD(appinfo->uiapp_info->image, ptr6);
				appinfo->uiapp_info->image = ptr6;
			}
			ret = app_func((void *)appinfo, user_data);
			if (ret < 0)
				break;
			tmp = tmp->next;
		}
		break;
	}

	ret = PMINFO_R_OK;
catch:
	if (locale) {
		free(locale);
		locale = NULL;
	}
	if (appinfo) {
		if (appinfo->package) {
			free((void *)appinfo->package);
			appinfo->package = NULL;
		}
		free(appinfo);
		appinfo = NULL;
	}
	__cleanup_pkginfo(allinfo);

	sqlite3_close(appinfo_db);
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

	/*check app_component from DB*/
	query = sqlite3_mprintf("select app_component, package from disabled_package_app_info where app_id=%Q", appid);
	ret = __exec_db_query(appinfo_db, query, __appcomponent_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

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

	switch (appinfo->app_component) {
	case PMINFO_UI_APP:
		if (appinfo->uiapp_info->label) {
			LISTHEAD(appinfo->uiapp_info->label, tmp1);
			appinfo->uiapp_info->label = tmp1;
		}
		if (appinfo->uiapp_info->icon) {
			LISTHEAD(appinfo->uiapp_info->icon, tmp2);
			appinfo->uiapp_info->icon = tmp2;
		}
		if (appinfo->uiapp_info->category) {
			LISTHEAD(appinfo->uiapp_info->category, tmp3);
			appinfo->uiapp_info->category = tmp3;
		}
		if (appinfo->uiapp_info->metadata) {
			LISTHEAD(appinfo->uiapp_info->metadata, tmp4);
			appinfo->uiapp_info->metadata = tmp4;
		}
		break;
	default:
		break;
	}

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

API int pkgmgrinfo_appinfo_get_appinfo(const char *appid, pkgmgrinfo_appinfo_h *handle)
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
	permission_x *tmp5 = NULL;
	image_x *tmp6 = NULL;
	char *query = NULL;
	sqlite3 *appinfo_db = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	retvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*check appid exist on db*/
	query = sqlite3_mprintf("select exists(select * from package_app_info where app_id=%Q)", appid);
	ret = __exec_db_query(appinfo_db, query, _pkgmgrinfo_validate_cb, (void *)&exist);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "sqlite3_exec fail");
	if (exist == 0) {
		_LOGS("Appid[%s] not found in DB", appid);
		ret = PMINFO_R_ERROR;
		goto catch;
	}

	/*get system locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "manifest locale is NULL");

	/*calloc appinfo*/
	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for appinfo");

	/*check app_component from DB*/
	query = sqlite3_mprintf("select app_component, package from package_app_info where app_id=%Q", appid);
	ret = __exec_db_query(appinfo_db, query, __appcomponent_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	/*calloc app_component*/
	if (appinfo->app_component == PMINFO_UI_APP) {
		appinfo->uiapp_info = (uiapplication_x *)calloc(1, sizeof(uiapplication_x));
		tryvm_if(appinfo->uiapp_info == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for uiapp info");
	} else {
		appinfo->svcapp_info = (serviceapplication_x *)calloc(1, sizeof(serviceapplication_x));
		tryvm_if(appinfo->svcapp_info == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for svcapp info");
	}
	appinfo->locale = strdup(locale);

	/*populate app_info from DB*/
	query = sqlite3_mprintf("select * from package_app_info where app_id=%Q ", appid);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	query = sqlite3_mprintf("select * from package_app_localized_info where app_id=%Q and app_locale=%Q", appid, locale);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	/*Also store the values corresponding to default locales*/
	query = sqlite3_mprintf("select * from package_app_localized_info where app_id=%Q and app_locale=%Q", appid, DEFAULT_LOCALE);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

	/*Populate app category*/
	query = sqlite3_mprintf("select * from package_app_app_category where app_id=%Q", appid);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Category Info DB Information retrieval failed");

	/*Populate app metadata*/
	query = sqlite3_mprintf("select * from package_app_app_metadata where app_id=%Q", appid);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Metadata Info DB Information retrieval failed");

	/*Populate app permission*/
	query = sqlite3_mprintf("select * from package_app_app_permission where app_id=%Q", appid);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App permission Info DB Information retrieval failed");

	/*store setting notification icon section*/
	query = sqlite3_mprintf("select * from package_app_icon_section_info where app_id=%Q", appid);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App icon section Info DB Information retrieval failed");

	/*store app preview image info*/
	query = sqlite3_mprintf("select app_image_section, app_image from package_app_image_info where app_id=%Q", appid);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App image Info DB Information retrieval failed");

	ret = __appinfo_check_installed_storage(appinfo);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "[%s] is installed external, but is not in mmc", appinfo->package);

	switch (appinfo->app_component) {
	case PMINFO_UI_APP:
		if (appinfo->uiapp_info->label) {
			LISTHEAD(appinfo->uiapp_info->label, tmp1);
			appinfo->uiapp_info->label = tmp1;
		}
		if (appinfo->uiapp_info->icon) {
			LISTHEAD(appinfo->uiapp_info->icon, tmp2);
			appinfo->uiapp_info->icon = tmp2;
		}
		if (appinfo->uiapp_info->category) {
			LISTHEAD(appinfo->uiapp_info->category, tmp3);
			appinfo->uiapp_info->category = tmp3;
		}
		if (appinfo->uiapp_info->metadata) {
			LISTHEAD(appinfo->uiapp_info->metadata, tmp4);
			appinfo->uiapp_info->metadata = tmp4;
		}
		if (appinfo->uiapp_info->permission) {
			LISTHEAD(appinfo->uiapp_info->permission, tmp5);
			appinfo->uiapp_info->permission = tmp5;
		}
		if (appinfo->uiapp_info->image) {
			LISTHEAD(appinfo->uiapp_info->image, tmp6);
			appinfo->uiapp_info->image = tmp6;
		}
		break;
	case PMINFO_SVC_APP:
		if (appinfo->svcapp_info->label) {
			LISTHEAD(appinfo->svcapp_info->label, tmp1);
			appinfo->svcapp_info->label = tmp1;
		}
		if (appinfo->svcapp_info->icon) {
			LISTHEAD(appinfo->svcapp_info->icon, tmp2);
			appinfo->svcapp_info->icon = tmp2;
		}
		if (appinfo->svcapp_info->category) {
			LISTHEAD(appinfo->svcapp_info->category, tmp3);
			appinfo->svcapp_info->category = tmp3;
		}
		if (appinfo->svcapp_info->metadata) {
			LISTHEAD(appinfo->svcapp_info->metadata, tmp4);
			appinfo->svcapp_info->metadata = tmp4;
		}
		if (appinfo->svcapp_info->permission) {
			LISTHEAD(appinfo->svcapp_info->permission, tmp5);
			appinfo->svcapp_info->permission = tmp5;
		}
		break;
	default:
		break;
	}

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


API int pkgmgrinfo_appinfo_get_appid(pkgmgrinfo_appinfo_h  handle, char **appid)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(appid == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	if (info->app_component == PMINFO_UI_APP)
		*appid = (char *)info->uiapp_info->appid;
	else if (info->app_component == PMINFO_SVC_APP)
		*appid = (char *)info->svcapp_info->appid;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_pkgname(pkgmgrinfo_appinfo_h  handle, char **pkg_name)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(pkg_name == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	*pkg_name = (char *)info->package;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_pkgid(pkgmgrinfo_appinfo_h  handle, char **pkgid)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(pkgid == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	*pkgid = (char *)info->package;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_exec(pkgmgrinfo_appinfo_h  handle, char **exec)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(exec == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	if (info->app_component == PMINFO_UI_APP)
		*exec = (char *)info->uiapp_info->exec;
	if (info->app_component == PMINFO_SVC_APP)
		*exec = (char *)info->svcapp_info->exec;

	return PMINFO_R_OK;
}


API int pkgmgrinfo_appinfo_get_icon(pkgmgrinfo_appinfo_h  handle, char **icon)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(icon == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
        char *locale = NULL;
        icon_x *ptr = NULL;
        icon_x *start = NULL;
        *icon = NULL;

        pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
		locale = info->locale;
		retvm_if(locale == NULL, PMINFO_R_ERROR, "manifest locale is NULL");

        if (info->app_component == PMINFO_UI_APP)
                start = info->uiapp_info->icon;
        if (info->app_component == PMINFO_SVC_APP)
                start = info->svcapp_info->icon;
        for(ptr = start; ptr != NULL; ptr = ptr->next)
        {
                if (ptr->lang) {
                        if (strcmp(ptr->lang, locale) == 0) {
							if (ptr->text) {
                                *icon = (char *)ptr->text;
                                if (strcasecmp(*icon, "(null)") == 0) {
                                        locale = DEFAULT_LOCALE;
                                        continue;
                                } else
                                        break;
							} else {
								locale = DEFAULT_LOCALE;
								continue;
							}
                        } else if (strcmp(ptr->lang, DEFAULT_LOCALE) == 0) {
                                *icon = (char *)ptr->text;
                                break;
                        }
                }
        }
	return PMINFO_R_OK;
}


API int pkgmgrinfo_appinfo_get_label(pkgmgrinfo_appinfo_h  handle, char **label)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(label == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	char *locale = NULL;
	label_x *ptr = NULL;
	label_x *start = NULL;
	*label = NULL;

	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	locale = info->locale;
	retvm_if(locale == NULL, PMINFO_R_ERROR, "manifest locale is NULL");

	if (info->app_component == PMINFO_UI_APP)
		start = info->uiapp_info->label;
	if (info->app_component == PMINFO_SVC_APP)
		start = info->svcapp_info->label;
	for(ptr = start; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->lang) {
			if (strcmp(ptr->lang, locale) == 0) {
				if (ptr->text) {
					*label = (char *)ptr->text;
					if (strcasecmp(*label, "(null)") == 0) {
						locale = DEFAULT_LOCALE;
						continue;
					} else
						break;
				} else {
					locale = DEFAULT_LOCALE;
					continue;
				}
			} else if (strncasecmp(ptr->lang, locale, 2) == 0) {
				*label = (char *)ptr->text;
				if (ptr->text) {
					if (strcasecmp(*label, "(null)") == 0) {
							locale = DEFAULT_LOCALE;
							continue;
					} else
							break;
				} else {
					locale = DEFAULT_LOCALE;
					continue;
				}
			} else if (strcmp(ptr->lang, DEFAULT_LOCALE) == 0) {
				*label = (char *)ptr->text;
				break;
			}
		}
	}
	return PMINFO_R_OK;
}


API int pkgmgrinfo_appinfo_get_component(pkgmgrinfo_appinfo_h  handle, pkgmgrinfo_app_component *component)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(component == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	if (info->app_component == PMINFO_UI_APP)
		*component = PMINFO_UI_APP;
	else if (info->app_component == PMINFO_SVC_APP)
		*component = PMINFO_SVC_APP;
	else
		return PMINFO_R_ERROR;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_apptype(pkgmgrinfo_appinfo_h  handle, char **app_type)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(app_type == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	if (info->app_component == PMINFO_UI_APP)
		*app_type = (char *)info->uiapp_info->type;
	if (info->app_component == PMINFO_SVC_APP)
		*app_type = (char *)info->svcapp_info->type;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_operation(pkgmgrinfo_appcontrol_h  handle,
					int *operation_count, char ***operation)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(operation == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	retvm_if(operation_count == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgrinfo_appcontrol_x *data = (pkgmgrinfo_appcontrol_x *)handle;
	*operation_count = data->operation_count;
	*operation = data->operation;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_uri(pkgmgrinfo_appcontrol_h  handle,
					int *uri_count, char ***uri)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(uri == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	retvm_if(uri_count == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgrinfo_appcontrol_x *data = (pkgmgrinfo_appcontrol_x *)handle;
	*uri_count = data->uri_count;
	*uri = data->uri;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_mime(pkgmgrinfo_appcontrol_h  handle,
					int *mime_count, char ***mime)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(mime == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	retvm_if(mime_count == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgrinfo_appcontrol_x *data = (pkgmgrinfo_appcontrol_x *)handle;
	*mime_count = data->mime_count;
	*mime = data->mime;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_subapp(pkgmgrinfo_appcontrol_h  handle,
					int *subapp_count, char ***subapp)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(subapp == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	retvm_if(subapp_count == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgrinfo_appcontrol_x *data = (pkgmgrinfo_appcontrol_x *)handle;
	*subapp_count = data->subapp_count;
	*subapp = data->subapp;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_setting_icon(pkgmgrinfo_appinfo_h  handle, char **icon)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(icon == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	char *val = NULL;
	icon_x *ptr = NULL;
	icon_x *start = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	start = info->uiapp_info->icon;

	for(ptr = start; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->section) {
			val = (char *)ptr->section;
			if (strcmp(val, "setting") == 0){
				*icon = (char *)ptr->text;
				break;
			}
		}
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_small_icon(pkgmgrinfo_appinfo_h  handle, char **icon)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(icon == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	char *val = NULL;
	icon_x *ptr = NULL;
	icon_x *start = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	start = info->uiapp_info->icon;

	for(ptr = start; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->section) {
			val = (char *)ptr->section;
			if (strcmp(val, "small") == 0){
				*icon = (char *)ptr->text;
				break;
			}
		}
	}

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_notification_icon(pkgmgrinfo_appinfo_h  handle, char **icon)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(icon == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	char *val = NULL;
	icon_x *ptr = NULL;
	icon_x *start = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	start = info->uiapp_info->icon;

	for(ptr = start; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->section) {
			val = (char *)ptr->section;

			if (strcmp(val, "notification") == 0){
				*icon = (char *)ptr->text;
				break;
			}
		}
	}

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_recent_image_type(pkgmgrinfo_appinfo_h  handle, pkgmgrinfo_app_recentimage *type)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(type == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->recentimage;
	if (val) {
		if (strcasecmp(val, "capture") == 0)
			*type = PMINFO_RECENTIMAGE_USE_CAPTURE;
		else if (strcasecmp(val, "icon") == 0)
			*type = PMINFO_RECENTIMAGE_USE_ICON;
		else
			*type = PMINFO_RECENTIMAGE_USE_NOTHING;
	}

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_preview_image(pkgmgrinfo_appinfo_h  handle, char **preview_img)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(preview_img == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	char *val = NULL;
	image_x *ptr = NULL;
	image_x *start = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	start = info->uiapp_info->image;

	for(ptr = start; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->section) {
			val = (char *)ptr->section;

			if (strcmp(val, "preview") == 0)
				*preview_img = (char *)ptr->text;

			break;
		}
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_permission_type(pkgmgrinfo_appinfo_h  handle, pkgmgrinfo_permission_type *permission)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(permission == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	if (info->app_component == PMINFO_UI_APP)
		val = info->uiapp_info->permission_type;
	else if (info->app_component == PMINFO_SVC_APP)
		val = info->svcapp_info->permission_type;
	else
		return PMINFO_R_ERROR;

	if (strcmp(val, "signature") == 0)
		*permission = PMINFO_PERMISSION_SIGNATURE;
	else if (strcmp(val, "privilege") == 0)
		*permission = PMINFO_PERMISSION_PRIVILEGE;
	else
		*permission = PMINFO_PERMISSION_NORMAL;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_component_type(pkgmgrinfo_appinfo_h  handle, char **component_type)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(component_type == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	*component_type = (char *)info->uiapp_info->component_type;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_hwacceleration(pkgmgrinfo_appinfo_h  handle, pkgmgrinfo_app_hwacceleration *hwacceleration)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(hwacceleration == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->hwacceleration;
	if (val) {
		if (strcasecmp(val, "not-use-GL") == 0)
			*hwacceleration = PMINFO_HWACCELERATION_NOT_USE_GL;
		else if (strcasecmp(val, "use-GL") == 0)
			*hwacceleration = PMINFO_HWACCELERATION_USE_GL;
		else
			*hwacceleration = PMINFO_HWACCELERATION_USE_SYSTEM_SETTING;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_screenreader(pkgmgrinfo_appinfo_h  handle, pkgmgrinfo_app_screenreader *screenreader)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(screenreader == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->screenreader;
	if (val) {
		if (strcasecmp(val, "screenreader-off") == 0)
			*screenreader = PMINFO_SCREENREADER_OFF;
		else if (strcasecmp(val, "screenreader-on") == 0)
			*screenreader = PMINFO_SCREENREADER_ON;
		else
			*screenreader = PMINFO_SCREENREADER_USE_SYSTEM_SETTING;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_effectimage(pkgmgrinfo_appinfo_h  handle, char **portrait_img, char **landscape_img)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(portrait_img == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	retvm_if(landscape_img == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	if (info->app_component == PMINFO_UI_APP){
		*portrait_img = (char *)info->uiapp_info->portraitimg;
		*landscape_img = (char *)info->uiapp_info->landscapeimg;
	}

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_submode_mainid(pkgmgrinfo_appinfo_h  handle, char **submode_mainid)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(submode_mainid == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	*submode_mainid = (char *)info->uiapp_info->submode_mainid;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_localed_label(const char *appid, const char *locale, char **label)
{
	int col = 0;
	int cols = 0;
	int ret = -1;
	char *val = NULL;
	char *localed_label = NULL;

	retvm_if(appid == NULL || locale == NULL || label == NULL, PMINFO_R_EINVAL, "Argument is NULL");

	sqlite3_stmt *stmt = NULL;
	sqlite3 *pkgmgr_parser_db = NULL;

	char *query = sqlite3_mprintf("select app_label from package_app_localized_info where app_id=%Q and app_locale=%Q", appid, locale);

	ret = sqlite3_open(MANIFEST_DB, &pkgmgr_parser_db);
	if (ret != SQLITE_OK) {
		_LOGE("open fail\n");
		sqlite3_free(query);
		return -1;
	}

	ret = sqlite3_prepare_v2(pkgmgr_parser_db, query, strlen(query), &stmt, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("prepare_v2 fail\n");
		sqlite3_close(pkgmgr_parser_db);
		sqlite3_free(query);
		return -1;
	}

	cols = sqlite3_column_count(stmt);
	while(1)
	{
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {
			for(col = 0; col < cols; col++)
			{
				val = (char*)sqlite3_column_text(stmt, col);
				if (val == NULL)
					break;

				_LOGE("success find localed_label[%s]\n", val);

				localed_label = strdup(val);
				if (localed_label == NULL)
					break;

				*label = localed_label;


			}
			ret = 0;
		} else {
			break;
		}
	}

	/*find default label when exact matching failed*/
	if (localed_label == NULL) {
		sqlite3_free(query);
		query = sqlite3_mprintf("select app_label from package_app_localized_info where app_id=%Q and app_locale=%Q", appid, DEFAULT_LOCALE);
		ret = sqlite3_prepare_v2(pkgmgr_parser_db, query, strlen(query), &stmt, NULL);
		if (ret != SQLITE_OK) {
			_LOGE("prepare_v2 fail\n");
			sqlite3_close(pkgmgr_parser_db);
			sqlite3_free(query);
			return -1;
		}
		
		cols = sqlite3_column_count(stmt);
		while(1)
		{
			ret = sqlite3_step(stmt);
			if(ret == SQLITE_ROW) {
				for(col = 0; col < cols; col++)
				{
					val = (char*)sqlite3_column_text(stmt, col);
					if (val == NULL)
						break;
		
					_LOGE("success find default localed_label[%s]\n", val);
		
					localed_label = strdup(val);
					if (localed_label == NULL)
						break;
		
					*label = localed_label;
		
		
				}
				ret = 0;
			} else {
				break;
			}
		}

	}

	sqlite3_finalize(stmt);
	sqlite3_close(pkgmgr_parser_db);
	sqlite3_free(query);
	return ret;

}

API int pkgmgrinfo_appinfo_is_category_exist(pkgmgrinfo_appinfo_h handle, const char *category, bool *exist)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(category == NULL, PMINFO_R_EINVAL, "category is NULL");
	retvm_if(exist == NULL, PMINFO_R_EINVAL, "exist is NULL");

	int ret = -1;
	category_x *ptr = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	*exist = 0;

	if (info->app_component == PMINFO_UI_APP)
		ptr = info->uiapp_info->category;
	else if (info->app_component == PMINFO_SVC_APP)
		ptr = info->svcapp_info->category;
	else
		return PMINFO_R_EINVAL;

	for (; ptr; ptr = ptr->next) {
		if (ptr->name) {
			if (strcasecmp(ptr->name, category) == 0)
			{
				*exist = 1;
				break;
			}
		}
	}

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_metadata_value(pkgmgrinfo_appinfo_h handle, const char *metadata_key, char **metadata_value)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(metadata_key == NULL, PMINFO_R_EINVAL, "metadata_key is NULL");
	retvm_if(metadata_value == NULL, PMINFO_R_EINVAL, "metadata_value is NULL");

	int ret = -1;
	metadata_x *ptr = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	if (info->app_component == PMINFO_UI_APP)
		ptr = info->uiapp_info->metadata;
	else if (info->app_component == PMINFO_SVC_APP)
		ptr = info->svcapp_info->metadata;
	else
		return PMINFO_R_EINVAL;

	for (; ptr; ptr = ptr->next) {
		if (ptr->key) {
			if (strcasecmp(ptr->key, metadata_key) == 0)
			{
				*metadata_value = ptr->value;
				return PMINFO_R_OK;
			}
		}
	}

	return PMINFO_R_EINVAL;
}

API int pkgmgrinfo_appinfo_foreach_permission(pkgmgrinfo_appinfo_h handle,
			pkgmgrinfo_app_permission_list_cb permission_func, void *user_data)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(permission_func == NULL, PMINFO_R_EINVAL, "Callback function is NULL");
	int ret = -1;
	permission_x *ptr = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	if (info->app_component == PMINFO_UI_APP)
		ptr = info->uiapp_info->permission;
	else if (info->app_component == PMINFO_SVC_APP)
		ptr = info->svcapp_info->permission;
	else
		return PMINFO_R_EINVAL;
	for (; ptr; ptr = ptr->next) {
		if (ptr->value) {
			ret = permission_func(ptr->value, user_data);
			if (ret < 0)
				break;
		}
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_foreach_category(pkgmgrinfo_appinfo_h handle,
			pkgmgrinfo_app_category_list_cb category_func, void *user_data)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(category_func == NULL, PMINFO_R_EINVAL, "Callback function is NULL");
	int ret = -1;
	category_x *ptr = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	if (info->app_component == PMINFO_UI_APP)
		ptr = info->uiapp_info->category;
	else if (info->app_component == PMINFO_SVC_APP)
		ptr = info->svcapp_info->category;
	else
		return PMINFO_R_EINVAL;
	for (; ptr; ptr = ptr->next) {
		if (ptr->name) {
			ret = category_func(ptr->name, user_data);
			if (ret < 0)
				break;
		}
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_foreach_metadata(pkgmgrinfo_appinfo_h handle,
			pkgmgrinfo_app_metadata_list_cb metadata_func, void *user_data)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(metadata_func == NULL, PMINFO_R_EINVAL, "Callback function is NULL");
	int ret = -1;
	metadata_x *ptr = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	if (info->app_component == PMINFO_UI_APP)
		ptr = info->uiapp_info->metadata;
	else if (info->app_component == PMINFO_SVC_APP)
		ptr = info->svcapp_info->metadata;
	else
		return PMINFO_R_EINVAL;
	for (; ptr; ptr = ptr->next) {
		if (ptr->key) {
			ret = metadata_func(ptr->key, ptr->value, user_data);
			if (ret < 0)
				break;
		}
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_foreach_appcontrol(pkgmgrinfo_appinfo_h handle,
			pkgmgrinfo_app_control_list_cb appcontrol_func, void *user_data)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(appcontrol_func == NULL, PMINFO_R_EINVAL, "Callback function is NULL");
	int i = 0;
	int ret = -1;
	int oc = 0;
	int mc = 0;
	int uc = 0;
	int sc = 0;
	char *pkgid = NULL;
	char *manifest = NULL;
	char **operation = NULL;
	char **uri = NULL;
	char **mime = NULL;
	char **subapp = NULL;
	appcontrol_x *appcontrol = NULL;
	manifest_x *mfx = NULL;
	operation_x *op = NULL;
	uri_x *ui = NULL;
	mime_x *mi = NULL;
	subapp_x *sa = NULL;
	pkgmgrinfo_app_component component;
	pkgmgrinfo_appcontrol_x *ptr = NULL;
	ret = pkgmgrinfo_appinfo_get_pkgid(handle, &pkgid);
	if (ret < 0) {
		_LOGE("Failed to get package name\n");
		return PMINFO_R_ERROR;
	}
	ret = pkgmgrinfo_appinfo_get_component(handle, &component);
	if (ret < 0) {
		_LOGE("Failed to get app component name\n");
		return PMINFO_R_ERROR;
	}
	manifest = pkgmgr_parser_get_manifest_file(pkgid);
	if (manifest == NULL) {
		_LOGE("Failed to fetch package manifest file\n");
		return PMINFO_R_ERROR;
	}
	mfx = pkgmgr_parser_process_manifest_xml(manifest);
	if (mfx == NULL) {
		_LOGE("Failed to parse package manifest file\n");
		free(manifest);
		manifest = NULL;
		return PMINFO_R_ERROR;
	}
	free(manifest);
	ptr  = calloc(1, sizeof(pkgmgrinfo_appcontrol_x));
	if (ptr == NULL) {
		_LOGE("Out of Memory!!!\n");
		pkgmgr_parser_free_manifest_xml(mfx);
		return PMINFO_R_ERROR;
	}
	/*Get Operation, Uri, Mime*/
	switch (component) {
	case PMINFO_UI_APP:
		if (mfx->uiapplication) {
			if (mfx->uiapplication->appsvc) {
				appcontrol = mfx->uiapplication->appsvc;
			}
		}
		break;
	case PMINFO_SVC_APP:
		if (mfx->serviceapplication) {
			if (mfx->serviceapplication->appsvc) {
				appcontrol = mfx->serviceapplication->appsvc;
			}
		}
		break;
	default:
		break;
	}
	for (; appcontrol; appcontrol = appcontrol->next) {
		op = appcontrol->operation;
		for (; op; op = op->next)
			oc = oc + 1;
		op = appcontrol->operation;

		ui = appcontrol->uri;
		for (; ui; ui = ui->next)
			uc = uc + 1;
		ui = appcontrol->uri;

		mi = appcontrol->mime;
		for (; mi; mi = mi->next)
			mc = mc + 1;
		mi = appcontrol->mime;

		sa = appcontrol->subapp;
		for (; sa; sa = sa->next)
			sc = sc + 1;
		sa = appcontrol->subapp;

		operation = (char **)calloc(oc, sizeof(char *));
		for (i = 0; i < oc; i++) {
			operation[i] = strndup(op->name, PKG_STRING_LEN_MAX - 1);
			op = op->next;
		}

		uri = (char **)calloc(uc, sizeof(char *));
		for (i = 0; i < uc; i++) {
			uri[i] = strndup(ui->name, PKG_STRING_LEN_MAX - 1);
			ui = ui->next;
		}

		mime = (char **)calloc(mc, sizeof(char *));
		for (i = 0; i < mc; i++) {
			mime[i] = strndup(mi->name, PKG_STRING_LEN_MAX - 1);
			mi = mi->next;
		}

		subapp = (char **)calloc(sc, sizeof(char *));
		for (i = 0; i < sc; i++) {
			subapp[i] = strndup(sa->name, PKG_STRING_LEN_MAX - 1);
			sa = sa->next;
		}

		/*populate appcontrol handle*/
		ptr->operation_count = oc;
		ptr->uri_count = uc;
		ptr->mime_count = mc;
		ptr->subapp_count = sc;
		ptr->operation = operation;
		ptr->uri = uri;
		ptr->mime = mime;
		ptr->subapp = subapp;

		ret = appcontrol_func((void *)ptr, user_data);
		for (i = 0; i < oc; i++) {
			if (operation[i]) {
				free(operation[i]);
				operation[i] = NULL;
			}
		}
		if (operation) {
			free(operation);
			operation = NULL;
		}
		for (i = 0; i < uc; i++) {
			if (uri[i]) {
				free(uri[i]);
				uri[i] = NULL;
			}
		}
		if (uri) {
			free(uri);
			uri = NULL;
		}
		for (i = 0; i < mc; i++) {
			if (mime[i]) {
				free(mime[i]);
				mime[i] = NULL;
			}
		}
		if (mime) {
			free(mime);
			mime = NULL;
		}
		for (i = 0; i < sc; i++) {
			if (subapp[i]) {
				free(subapp[i]);
				subapp[i] = NULL;
			}
		}
		if (subapp) {
			free(subapp);
			subapp = NULL;
		}
		if (ret < 0)
			break;
		uc = 0;
		mc = 0;
		oc = 0;
		sc = 0;
	}
	pkgmgr_parser_free_manifest_xml(mfx);
	if (ptr) {
		free(ptr);
		ptr = NULL;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_is_nodisplay(pkgmgrinfo_appinfo_h  handle, bool *nodisplay)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(nodisplay == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->nodisplay;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*nodisplay = 1;
		else if (strcasecmp(val, "false") == 0)
			*nodisplay = 0;
		else
			*nodisplay = 0;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_is_multiple(pkgmgrinfo_appinfo_h  handle, bool *multiple)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(multiple == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->multiple;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*multiple = 1;
		else if (strcasecmp(val, "false") == 0)
			*multiple = 0;
		else
			*multiple = 0;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_is_indicator_display_allowed(pkgmgrinfo_appinfo_h handle, bool *indicator_disp)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(indicator_disp == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->indicatordisplay;
	if (val) {
		if (strcasecmp(val, "true") == 0){
			*indicator_disp = 1;
		}else if (strcasecmp(val, "false") == 0){
			*indicator_disp = 0;
		}else{
			*indicator_disp = 0;
		}
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_is_taskmanage(pkgmgrinfo_appinfo_h  handle, bool *taskmanage)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(taskmanage == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->taskmanage;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*taskmanage = 1;
		else if (strcasecmp(val, "false") == 0)
			*taskmanage = 0;
		else
			*taskmanage = 0;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_is_enabled(pkgmgrinfo_appinfo_h  handle, bool *enabled)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(enabled == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	if (info->app_component == PMINFO_UI_APP)
		val = (char *)info->uiapp_info->enabled;
	else if (info->app_component == PMINFO_SVC_APP)
		val = (char *)info->uiapp_info->enabled;
	else {
		_LOGE("invalid component type\n");
		return PMINFO_R_EINVAL;
	}

	if (val) {
		if (strcasecmp(val, "true") == 0)
			*enabled = 1;
		else if (strcasecmp(val, "false") == 0)
			*enabled = 0;
		else
			*enabled = 1;
	}
	return PMINFO_R_OK;

}

API int pkgmgrinfo_appinfo_is_onboot(pkgmgrinfo_appinfo_h  handle, bool *onboot)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(onboot == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	if (info->app_component == PMINFO_UI_APP)
		val = (char *)info->uiapp_info->onboot;
	else if (info->app_component == PMINFO_SVC_APP)
		val = (char *)info->svcapp_info->onboot;
	else {
		_LOGE("invalid component type\n");
		return PMINFO_R_EINVAL;
	}

	if (val) {
		if (strcasecmp(val, "true") == 0)
			*onboot = 1;
		else if (strcasecmp(val, "false") == 0)
			*onboot = 0;
		else
			*onboot = 0;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_is_autorestart(pkgmgrinfo_appinfo_h  handle, bool *autorestart)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(autorestart == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	if (info->app_component == PMINFO_UI_APP)
		val = (char *)info->uiapp_info->autorestart;
	else if (info->app_component == PMINFO_SVC_APP)
		val = (char *)info->svcapp_info->autorestart;
	else {
		_LOGE("invalid component type\n");
		return PMINFO_R_EINVAL;
	}

	if (val) {
		if (strcasecmp(val, "true") == 0)
			*autorestart = 1;
		else if (strcasecmp(val, "false") == 0)
			*autorestart = 0;
		else
			*autorestart = 0;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_is_mainapp(pkgmgrinfo_appinfo_h  handle, bool *mainapp)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(mainapp == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->mainapp;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*mainapp = 1;
		else if (strcasecmp(val, "false") == 0)
			*mainapp = 0;
		else
			*mainapp = 0;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_is_preload(pkgmgrinfo_appinfo_h handle, bool *preload)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(preload == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->preload;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*preload = 1;
		else if (strcasecmp(val, "false") == 0)
			*preload = 0;
		else
			*preload = 0;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_is_submode(pkgmgrinfo_appinfo_h handle, bool *submode)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(submode == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->submode;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*submode = 1;
		else if (strcasecmp(val, "false") == 0)
			*submode = 0;
		else
			*submode = 0;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_is_process_pool(pkgmgrinfo_appinfo_h handle, bool *process_pool)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(process_pool == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->process_pool;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*process_pool = 1;
		else if (strcasecmp(val, "false") == 0)
			*process_pool = 0;
		else
			*process_pool = 0;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_destroy_appinfo(pkgmgrinfo_appinfo_h  handle)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	__cleanup_appinfo(info);
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_filter_create(pkgmgrinfo_appinfo_filter_h *handle)
{
	return (pkgmgrinfo_pkginfo_filter_create(handle));
}

API int pkgmgrinfo_appinfo_filter_destroy(pkgmgrinfo_appinfo_filter_h handle)
{
	return (pkgmgrinfo_pkginfo_filter_destroy(handle));
}

API int pkgmgrinfo_appinfo_filter_add_int(pkgmgrinfo_appinfo_filter_h handle,
				const char *property, const int value)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	retvm_if(property == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	char buf[PKG_VALUE_STRING_LEN_MAX] = {'\0'};
	char *val = NULL;
	GSList *link = NULL;
	int prop = -1;
	prop = _pminfo_appinfo_convert_to_prop_int(property);
	if (prop < E_PMINFO_APPINFO_PROP_APP_MIN_INT ||
		prop > E_PMINFO_APPINFO_PROP_APP_MAX_INT) {
		_LOGE("Invalid Integer Property\n");
		return PMINFO_R_EINVAL;
	}
	pkgmgrinfo_filter_x *filter = (pkgmgrinfo_filter_x*)handle;
	pkgmgrinfo_node_x *node = (pkgmgrinfo_node_x*)calloc(1, sizeof(pkgmgrinfo_node_x));
	if (node == NULL) {
		_LOGE("Out of Memory!!!\n");
		return PMINFO_R_ERROR;
	}
	snprintf(buf, PKG_VALUE_STRING_LEN_MAX - 1, "%d", value);
	val = strndup(buf, PKG_VALUE_STRING_LEN_MAX - 1);
	if (val == NULL) {
		_LOGE("Out of Memory\n");
		free(node);
		node = NULL;
		return PMINFO_R_ERROR;
	}
	node->prop = prop;
	node->value = val;
	/*If API is called multiple times for same property, we should override the previous values.
	Last value set will be used for filtering.*/
	link = g_slist_find_custom(filter->list, (gconstpointer)node, __compare_func);
	if (link)
		filter->list = g_slist_delete_link(filter->list, link);
	filter->list = g_slist_append(filter->list, (gpointer)node);
	return PMINFO_R_OK;

}

API int pkgmgrinfo_appinfo_filter_add_bool(pkgmgrinfo_appinfo_filter_h handle,
				const char *property, const bool value)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	retvm_if(property == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	char *val = NULL;
	GSList *link = NULL;
	int prop = -1;
	prop = _pminfo_appinfo_convert_to_prop_bool(property);
	if (prop < E_PMINFO_APPINFO_PROP_APP_MIN_BOOL ||
		prop > E_PMINFO_APPINFO_PROP_APP_MAX_BOOL) {
		_LOGE("Invalid Boolean Property\n");
		return PMINFO_R_EINVAL;
	}
	pkgmgrinfo_filter_x *filter = (pkgmgrinfo_filter_x*)handle;
	pkgmgrinfo_node_x *node = (pkgmgrinfo_node_x*)calloc(1, sizeof(pkgmgrinfo_node_x));
	if (node == NULL) {
		_LOGE("Out of Memory!!!\n");
		return PMINFO_R_ERROR;
	}
	if (value)
		val = strndup("('true','True')", 15);
	else
		val = strndup("('false','False')", 17);
	if (val == NULL) {
		_LOGE("Out of Memory\n");
		free(node);
		node = NULL;
		return PMINFO_R_ERROR;
	}
	node->prop = prop;
	node->value = val;
	/*If API is called multiple times for same property, we should override the previous values.
	Last value set will be used for filtering.*/
	link = g_slist_find_custom(filter->list, (gconstpointer)node, __compare_func);
	if (link)
		filter->list = g_slist_delete_link(filter->list, link);
	filter->list = g_slist_append(filter->list, (gpointer)node);
	return PMINFO_R_OK;

}

API int pkgmgrinfo_appinfo_filter_add_string(pkgmgrinfo_appinfo_filter_h handle,
				const char *property, const char *value)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	retvm_if(property == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	retvm_if(value == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	char *val = NULL;
	pkgmgrinfo_node_x *ptr = NULL;
	char prev[PKG_STRING_LEN_MAX] = {'\0'};
	char temp[PKG_STRING_LEN_MAX] = {'\0'};
	GSList *link = NULL;
	int prop = -1;
	prop = _pminfo_appinfo_convert_to_prop_str(property);
	if (prop < E_PMINFO_APPINFO_PROP_APP_MIN_STR ||
		prop > E_PMINFO_APPINFO_PROP_APP_MAX_STR) {
		_LOGE("Invalid String Property\n");
		return PMINFO_R_EINVAL;
	}
	pkgmgrinfo_filter_x *filter = (pkgmgrinfo_filter_x*)handle;
	pkgmgrinfo_node_x *node = (pkgmgrinfo_node_x*)calloc(1, sizeof(pkgmgrinfo_node_x));
	if (node == NULL) {
		_LOGE("Out of Memory!!!\n");
		return PMINFO_R_ERROR;
	}
	node->prop = prop;
	switch (prop) {
	case E_PMINFO_APPINFO_PROP_APP_COMPONENT:
		if (strcmp(value, PMINFO_APPINFO_UI_APP) == 0)
			val = strndup("uiapp", PKG_STRING_LEN_MAX - 1);
		else
			val = strndup("svcapp", PKG_STRING_LEN_MAX - 1);
		node->value = val;
		link = g_slist_find_custom(filter->list, (gconstpointer)node, __compare_func);
		if (link)
			filter->list = g_slist_delete_link(filter->list, link);
		filter->list = g_slist_append(filter->list, (gpointer)node);
		break;
	case E_PMINFO_APPINFO_PROP_APP_CATEGORY:
	case E_PMINFO_APPINFO_PROP_APP_OPERATION:
	case E_PMINFO_APPINFO_PROP_APP_URI:
	case E_PMINFO_APPINFO_PROP_APP_MIME:
		val = (char *)calloc(1, PKG_STRING_LEN_MAX);
		if (val == NULL) {
			_LOGE("Out of Memory\n");
			free(node);
			node = NULL;
			return PMINFO_R_ERROR;
		}
		link = g_slist_find_custom(filter->list, (gconstpointer)node, __compare_func);
		if (link) {
			ptr = (pkgmgrinfo_node_x *)link->data;
			strncpy(prev, ptr->value, PKG_STRING_LEN_MAX - 1);
			_LOGS("Previous value is %s\n", prev);
			filter->list = g_slist_delete_link(filter->list, link);
			snprintf(temp, PKG_STRING_LEN_MAX - 1, "%s , '%s'", prev, value);
			strncpy(val, temp, PKG_STRING_LEN_MAX - 1);
			_LOGS("New value is %s\n", val);
			node->value = val;
			filter->list = g_slist_append(filter->list, (gpointer)node);
			memset(temp, '\0', PKG_STRING_LEN_MAX);
		} else {
			snprintf(temp, PKG_STRING_LEN_MAX - 1, "'%s'", value);
			strncpy(val, temp, PKG_STRING_LEN_MAX - 1);
			_LOGS("First value is %s\n", val);
			node->value = val;
			filter->list = g_slist_append(filter->list, (gpointer)node);
			memset(temp, '\0', PKG_STRING_LEN_MAX);
		}
		break;
	default:
		node->value = strndup(value, PKG_STRING_LEN_MAX - 1);
		link = g_slist_find_custom(filter->list, (gconstpointer)node, __compare_func);
		if (link)
			filter->list = g_slist_delete_link(filter->list, link);
		filter->list = g_slist_append(filter->list, (gpointer)node);
		break;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_filter_count(pkgmgrinfo_appinfo_filter_h handle, int *count)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	retvm_if(count == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	char *locale = NULL;
	char *condition = NULL;
	char query[MAX_QUERY_LEN] = {'\0'};
	char where[MAX_QUERY_LEN] = {'\0'};
	GSList *list;
	int ret = 0;
	uiapplication_x *ptr1 = NULL;
	serviceapplication_x *ptr2 = NULL;
	pkgmgrinfo_filter_x *filter = (pkgmgrinfo_filter_x*)handle;
	sqlite3 *pkginfo_db = NULL;
	int filter_count = 0;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &pkginfo_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*get system locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "manifest locale is NULL");

	/*Start constructing query*/
	snprintf(query, MAX_QUERY_LEN - 1, FILTER_QUERY_LIST_APP, locale);
	/*Get where clause*/
	for (list = filter->list; list; list = g_slist_next(list)) {
		__get_filter_condition(list->data, &condition);
		if (condition) {
			strncat(where, condition, sizeof(where) - strlen(where) -1);
			where[sizeof(where) - 1] = '\0';
			free(condition);
			condition = NULL;
		}
		if (g_slist_next(list)) {
			strncat(where, " and ", sizeof(where) - strlen(where) - 1);
			where[sizeof(where) - 1] = '\0';
		}
	}
	_LOGS("where = %s\n", where);
	if (strlen(where) > 0) {
		strncat(query, where, sizeof(query) - strlen(query) - 1);
		query[sizeof(query) - 1] = '\0';
	}
	_LOGS("query = %s\n", query);
	/*To get filtered list*/
	pkgmgr_pkginfo_x *info = NULL;
	info = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(info->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	/*To get detail app info for each member of filtered list*/
	pkgmgr_pkginfo_x *filtinfo = NULL;
	filtinfo = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(filtinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	filtinfo->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(filtinfo->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	pkgmgr_appinfo_x *appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	ret = __exec_db_query(pkginfo_db, query, __app_list_cb, (void *)info);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");

	memset(query, '\0', MAX_QUERY_LEN);
	if (info->manifest_info->uiapplication) {
		LISTHEAD(info->manifest_info->uiapplication, ptr1);
		info->manifest_info->uiapplication = ptr1;
	}
	if (info->manifest_info->serviceapplication) {
		LISTHEAD(info->manifest_info->serviceapplication, ptr2);
		info->manifest_info->serviceapplication = ptr2;
	}
	/*Filtered UI Apps*/
	for(ptr1 = info->manifest_info->uiapplication; ptr1; ptr1 = ptr1->next)
	{
		snprintf(query, MAX_QUERY_LEN, "select * from package_app_info where app_id='%s' and app_component='%s'",
							ptr1->appid, "uiapp");
		ret = __exec_db_query(pkginfo_db, query, __uiapp_list_cb, (void *)filtinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");
	}
	for(ptr2 = info->manifest_info->serviceapplication; ptr2; ptr2 = ptr2->next)
	{
		snprintf(query, MAX_QUERY_LEN, "select * from package_app_info where app_id='%s' and app_component='%s'",
							ptr2->appid, "svcapp");
		ret = __exec_db_query(pkginfo_db, query, __svcapp_list_cb, (void *)filtinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");
	}
	if (filtinfo->manifest_info->uiapplication) {
		LISTHEAD(filtinfo->manifest_info->uiapplication, ptr1);
		filtinfo->manifest_info->uiapplication = ptr1;
	}
	/*If the callback func return < 0 we break and no more call back is called*/
	while(ptr1 != NULL)
	{
		appinfo->locale = strdup(locale);
		appinfo->uiapp_info = ptr1;
		appinfo->app_component = PMINFO_UI_APP;

		ret = __appinfo_check_installed_storage(appinfo);
		if(ret < 0) {
			ptr1 = ptr1->next;
			continue;
		}

		filter_count++;

		ptr1 = ptr1->next;
	}
	/*Filtered Service Apps*/
	if (filtinfo->manifest_info->serviceapplication) {
		LISTHEAD(filtinfo->manifest_info->serviceapplication, ptr2);
		filtinfo->manifest_info->serviceapplication = ptr2;
	}
	/*If the callback func return < 0 we break and no more call back is called*/
	while(ptr2 != NULL)
	{
		appinfo->locale = strdup(locale);
		appinfo->svcapp_info = ptr2;
		appinfo->app_component = PMINFO_SVC_APP;
		filter_count++;
		ptr2 = ptr2->next;
	}
	*count = filter_count;

	ret = PMINFO_R_OK;
catch:
	if (locale) {
		free(locale);
		locale = NULL;
	}
	sqlite3_close(pkginfo_db);
	if (appinfo) {
		free(appinfo);
		appinfo = NULL;
	}
	__cleanup_pkginfo(info);
	__cleanup_pkginfo(filtinfo);
	return ret;
}

API int pkgmgrinfo_appinfo_filter_foreach_appinfo(pkgmgrinfo_appinfo_filter_h handle,
				pkgmgrinfo_app_list_cb app_cb, void * user_data)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	retvm_if(app_cb == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	char *locale = NULL;
	char *condition = NULL;
	char query[MAX_QUERY_LEN] = {'\0'};
	char where[MAX_QUERY_LEN] = {'\0'};
	GSList *list;
	int ret = 0;
	uiapplication_x *ptr1 = NULL;
	serviceapplication_x *ptr2 = NULL;
	pkgmgrinfo_filter_x *filter = (pkgmgrinfo_filter_x*)handle;
	sqlite3 *pkginfo_db = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &pkginfo_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*get system locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "manifest locale is NULL");

	/*Start constructing query*/
	snprintf(query, MAX_QUERY_LEN - 1, FILTER_QUERY_LIST_APP, locale);
	/*Get where clause*/
	for (list = filter->list; list; list = g_slist_next(list)) {
		__get_filter_condition(list->data, &condition);
		if (condition) {
			strncat(where, condition, sizeof(where) - strlen(where) -1);
			where[sizeof(where) - 1] = '\0';
			free(condition);
			condition = NULL;
		}
		if (g_slist_next(list)) {
			strncat(where, " and ", sizeof(where) - strlen(where) - 1);
			where[sizeof(where) - 1] = '\0';
		}
	}
	_LOGS("where = %s\n", where);
	if (strlen(where) > 0) {
		strncat(query, where, sizeof(query) - strlen(query) - 1);
		query[sizeof(query) - 1] = '\0';
	}
	_LOGS("query = %s\n", query);
	/*To get filtered list*/
	pkgmgr_pkginfo_x *info = NULL;
	info = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(info->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	/*To get detail app info for each member of filtered list*/
	pkgmgr_pkginfo_x *filtinfo = NULL;
	filtinfo = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(filtinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	filtinfo->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(filtinfo->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	pkgmgr_appinfo_x *appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	ret = __exec_db_query(pkginfo_db, query, __app_list_cb, (void *)info);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");

	memset(query, '\0', MAX_QUERY_LEN);
	if (info->manifest_info->uiapplication) {
		LISTHEAD(info->manifest_info->uiapplication, ptr1);
		info->manifest_info->uiapplication = ptr1;
	}
	if (info->manifest_info->serviceapplication) {
		LISTHEAD(info->manifest_info->serviceapplication, ptr2);
		info->manifest_info->serviceapplication = ptr2;
	}
	/*Filtered UI Apps*/
	for(ptr1 = info->manifest_info->uiapplication; ptr1; ptr1 = ptr1->next)
	{
		snprintf(query, MAX_QUERY_LEN, "select * from package_app_info where app_id='%s' and app_component='%s'",
							ptr1->appid, "uiapp");
		ret = __exec_db_query(pkginfo_db, query, __uiapp_list_cb, (void *)filtinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");
	}
	for(ptr2 = info->manifest_info->serviceapplication; ptr2; ptr2 = ptr2->next)
	{
		snprintf(query, MAX_QUERY_LEN, "select * from package_app_info where app_id='%s' and app_component='%s'",
							ptr2->appid, "svcapp");
		ret = __exec_db_query(pkginfo_db, query, __svcapp_list_cb, (void *)filtinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");
	}
	if (filtinfo->manifest_info->uiapplication) {
		LISTHEAD(filtinfo->manifest_info->uiapplication, ptr1);
		filtinfo->manifest_info->uiapplication = ptr1;
	}
	/*If the callback func return < 0 we break and no more call back is called*/
	while(ptr1 != NULL)
	{
		appinfo->locale = strdup(locale);
		appinfo->uiapp_info = ptr1;
		appinfo->app_component = PMINFO_UI_APP;

		ret = __appinfo_check_installed_storage(appinfo);
		if(ret < 0) {
			ptr1 = ptr1->next;
			continue;
		}

		ret = app_cb((void *)appinfo, user_data);
		if (ret < 0)
			break;
		ptr1 = ptr1->next;
	}
	/*Filtered Service Apps*/
	if (filtinfo->manifest_info->serviceapplication) {
		LISTHEAD(filtinfo->manifest_info->serviceapplication, ptr2);
		filtinfo->manifest_info->serviceapplication = ptr2;
	}
	/*If the callback func return < 0 we break and no more call back is called*/
	while(ptr2 != NULL)
	{
		appinfo->locale = strdup(locale);
		appinfo->svcapp_info = ptr2;
		appinfo->app_component = PMINFO_SVC_APP;
		ret = app_cb((void *)appinfo, user_data);
		if (ret < 0)
			break;
		ptr2 = ptr2->next;
	}
	ret = PMINFO_R_OK;
catch:
	if (locale) {
		free(locale);
		locale = NULL;
	}
	sqlite3_close(pkginfo_db);
	if (appinfo) {
		free(appinfo);
		appinfo = NULL;
	}
	__cleanup_pkginfo(info);
	__cleanup_pkginfo(filtinfo);
	return ret;
}

API int pkgmgrinfo_appinfo_metadata_filter_create(pkgmgrinfo_appinfo_metadata_filter_h *handle)
{
	return (pkgmgrinfo_pkginfo_filter_create(handle));
}

API int pkgmgrinfo_appinfo_metadata_filter_destroy(pkgmgrinfo_appinfo_metadata_filter_h handle)
{
	return (pkgmgrinfo_pkginfo_filter_destroy(handle));
}

API int pkgmgrinfo_appinfo_metadata_filter_add(pkgmgrinfo_appinfo_metadata_filter_h handle,
		const char *key, const char *value)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "filter handle is NULL\n");
	retvm_if(key == NULL, PMINFO_R_EINVAL, "metadata key supplied is NULL\n");
	/*value can be NULL. In that case all apps with specified key should be displayed*/
	int ret = 0;
	char *k = NULL;
	char *v = NULL;
	pkgmgrinfo_filter_x *filter = (pkgmgrinfo_filter_x*)handle;
	pkgmgrinfo_node_x *node = (pkgmgrinfo_node_x*)calloc(1, sizeof(pkgmgrinfo_node_x));
	retvm_if(node == NULL, PMINFO_R_ERROR, "Out of Memory!!!\n");
	k = strdup(key);
	tryvm_if(k == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");
	node->key = k;
	if (value) {
		v = strdup(value);
		tryvm_if(v == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");
	}
	node->value = v;
	/*If API is called multiple times, we should OR all conditions.*/
	filter->list = g_slist_append(filter->list, (gpointer)node);
	/*All memory will be freed in destroy API*/
	return PMINFO_R_OK;
catch:
	if (node) {
		if (node->key) {
			free(node->key);
			node->key = NULL;
		}
		if (node->value) {
			free(node->value);
			node->value = NULL;
		}
		free(node);
		node = NULL;
	}
	return ret;
}

API int pkgmgrinfo_appinfo_metadata_filter_foreach(pkgmgrinfo_appinfo_metadata_filter_h handle,
		pkgmgrinfo_app_list_cb app_cb, void *user_data)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "filter handle is NULL\n");
	retvm_if(app_cb == NULL, PMINFO_R_EINVAL, "Callback function supplied is NULL\n");
	char *locale = NULL;
	char *condition = NULL;
	char *error_message = NULL;
	char query[MAX_QUERY_LEN] = {'\0'};
	char where[MAX_QUERY_LEN] = {'\0'};
	GSList *list;
	int ret = 0;
	pkgmgr_pkginfo_x *info = NULL;
	pkgmgr_pkginfo_x *filtinfo = NULL;
	pkgmgr_appinfo_x *appinfo = NULL;
	uiapplication_x *ptr1 = NULL;
	serviceapplication_x *ptr2 = NULL;
	pkgmgrinfo_filter_x *filter = (pkgmgrinfo_filter_x*)handle;
	sqlite3 *pkginfo_db = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &pkginfo_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*Get current locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "manifest locale is NULL\n");

	/*Start constructing query*/
	memset(where, '\0', MAX_QUERY_LEN);
	memset(query, '\0', MAX_QUERY_LEN);
	snprintf(query, MAX_QUERY_LEN - 1, METADATA_FILTER_QUERY_SELECT_CLAUSE);
	/*Get where clause*/
	for (list = filter->list; list; list = g_slist_next(list)) {
		__get_metadata_filter_condition(list->data, &condition);
		if (condition) {
			strncat(where, condition, sizeof(where) - strlen(where) -1);
			free(condition);
			condition = NULL;
		}
		if (g_slist_next(list)) {
			strncat(where, METADATA_FILTER_QUERY_UNION_CLAUSE, sizeof(where) - strlen(where) - 1);
		}
	}
	_LOGE("where = %s (%d)\n", where, strlen(where));
	if (strlen(where) > 0) {
		strncat(query, where, sizeof(query) - strlen(query) - 1);
	}
	_LOGE("query = %s (%d)\n", query, strlen(query));
	/*To get filtered list*/
	info = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(info->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	/*To get detail app info for each member of filtered list*/
	filtinfo = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(filtinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	filtinfo->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(filtinfo->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	ret = sqlite3_exec(pkginfo_db, query, __app_list_cb, (void *)info, &error_message);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Don't execute query = %s error message = %s\n", query, error_message);
	memset(query, '\0', MAX_QUERY_LEN);

	if (info->manifest_info->uiapplication) {
		LISTHEAD(info->manifest_info->uiapplication, ptr1);
		info->manifest_info->uiapplication = ptr1;
	}
	if (info->manifest_info->serviceapplication) {
		LISTHEAD(info->manifest_info->serviceapplication, ptr2);
		info->manifest_info->serviceapplication = ptr2;
	}

	/*UI Apps*/
	for(ptr1 = info->manifest_info->uiapplication; ptr1; ptr1 = ptr1->next)
	{
		snprintf(query, MAX_QUERY_LEN, "select * from package_app_info where app_id='%s' and app_component='%s'",
							ptr1->appid, "uiapp");
		ret = sqlite3_exec(pkginfo_db, query, __uiapp_list_cb, (void *)filtinfo, &error_message);
		tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Don't execute query = %s error message = %s\n", query, error_message);
		memset(query, '\0', MAX_QUERY_LEN);
	}
	/*Service Apps*/
	for(ptr2 = info->manifest_info->serviceapplication; ptr2; ptr2 = ptr2->next)
	{
		snprintf(query, MAX_QUERY_LEN, "select * from package_app_info where app_id='%s' and app_component='%s'",
							ptr2->appid, "svcapp");
		ret = sqlite3_exec(pkginfo_db, query, __svcapp_list_cb, (void *)filtinfo, &error_message);
		tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "Don't execute query = %s error message = %s\n", query, error_message);
		memset(query, '\0', MAX_QUERY_LEN);
	}
	/*Filtered UI Apps*/
	if (filtinfo->manifest_info->uiapplication) {
		LISTHEAD(filtinfo->manifest_info->uiapplication, ptr1);
		filtinfo->manifest_info->uiapplication = ptr1;
	}
	/*If the callback func return < 0 we break and no more call back is called*/
	while(ptr1 != NULL)
	{
		appinfo->locale = strdup(locale);
		appinfo->uiapp_info = ptr1;
		appinfo->app_component = PMINFO_UI_APP;
		ret = app_cb((void *)appinfo, user_data);
		if (ret < 0)
			break;
		ptr1 = ptr1->next;
	}
	/*Filtered Service Apps*/
	if (filtinfo->manifest_info->serviceapplication) {
		LISTHEAD(filtinfo->manifest_info->serviceapplication, ptr2);
		filtinfo->manifest_info->serviceapplication = ptr2;
	}
	/*If the callback func return < 0 we break and no more call back is called*/
	while(ptr2 != NULL)
	{
		appinfo->locale = strdup(locale);
		appinfo->svcapp_info = ptr2;
		appinfo->app_component = PMINFO_SVC_APP;
		ret = app_cb((void *)appinfo, user_data);
		if (ret < 0)
			break;
		ptr2 = ptr2->next;
	}
	ret = PMINFO_R_OK;
catch:
	if (locale) {
		free(locale);
		locale = NULL;
	}
	sqlite3_free(error_message);
	sqlite3_close(pkginfo_db);
	if (appinfo) {
		free(appinfo);
		appinfo = NULL;
	}
	__cleanup_pkginfo(info);
	__cleanup_pkginfo(filtinfo);
	return ret;
}
