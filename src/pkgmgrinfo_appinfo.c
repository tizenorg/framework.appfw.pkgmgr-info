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
#include "pkgmgr_parser.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_INFO"

#define LANGUAGE_LENGTH 2
#define MAX_PACKAGE_STR_SIZE 512

#define SAT_UI_APPID_1 	"org.tizen.sat-ui"
#define SAT_UI_APPID_2 	"org.tizen.sat-ui-2"
#define PKG_DATA_PATH	"/opt/usr/data/pkgmgr"


#define FILTER_QUERY_COUNT_APP	"select DISTINCT package_app_info.app_id, package_app_info.app_component, package_app_info.app_installed_storage " \
				"from package_app_info LEFT OUTER JOIN package_app_localized_info " \
				"ON package_app_info.app_id=package_app_localized_info.app_id " \
				"and package_app_localized_info.app_locale='%s' " \
				"LEFT OUTER JOIN package_app_app_svc " \
				"ON package_app_info.app_id=package_app_app_svc.app_id " \
				"LEFT OUTER JOIN package_app_app_category " \
				"ON package_app_info.app_id=package_app_app_category.app_id where "

#define FILTER_QUERY_LIST_APP	"select DISTINCT package_app_info.*, package_app_localized_info.app_locale, package_app_localized_info.app_label, package_app_localized_info.app_icon " \
				"from package_app_info LEFT OUTER JOIN package_app_localized_info " \
				"ON package_app_info.app_id=package_app_localized_info.app_id " \
				"and package_app_localized_info.app_locale IN ('%s', '%s') " \
				"LEFT OUTER JOIN package_app_app_svc " \
				"ON package_app_info.app_id=package_app_app_svc.app_id " \
				"LEFT OUTER JOIN package_app_app_category " \
				"ON package_app_info.app_id=package_app_app_category.app_id where "

#define METADATA_FILTER_QUERY_SELECT_CLAUSE	"select DISTINCT package_app_info.* " \
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

static char* __get_aliasid_from_db(sqlite3 *appinfo_db, const char *appid)
{
	int ret = PMINFO_R_OK;
	char *alias_id = NULL;
	char *query = NULL;
	sqlite3_stmt *stmt = NULL;

	query = sqlite3_mprintf("select alias_id from package_app_aliasid where app_id=%Q", appid);
	tryvm_if(query == NULL, ret = PMINFO_R_ERROR,"Malloc failed!!");

	/*Prepare query*/
	ret = sqlite3_prepare_v2(appinfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	alias_id = (char*)malloc(MAX_PACKAGE_STR_SIZE);
	tryvm_if(alias_id == NULL, ret = PMINFO_R_ERROR,"Malloc failed!!");
	memset(alias_id,'\0',MAX_PACKAGE_STR_SIZE);

	/*Step query*/
	ret = sqlite3_step(stmt);
	if(ret == SQLITE_ROW){
		/*Get the alias id*/
		snprintf(alias_id, MAX_PACKAGE_STR_SIZE, "%s", (const char *)sqlite3_column_text(stmt, 0));
		_LOGD("alias id [%s] id found for [%s] in DB",alias_id,appid);
	}

catch:
	if (query)
		sqlite3_free(query);
	if (stmt)
		sqlite3_finalize(stmt);

	/*If alias id is not found then set the appid as alias id*/
	if ( alias_id == NULL || strlen(alias_id) == 0 ) {
		FREE_AND_NULL(alias_id);
		alias_id = strdup(appid);
	}

	return alias_id;

}

static void __get_appinfo_from_db(char *colname, char *coltxt, uiapplication_x *uiapp)
{
	if (colname == NULL || uiapp == NULL)
		return;

	if (strcmp(colname, "app_id") == 0) {
		if (uiapp->appid)
			return;

		if (coltxt)
			uiapp->appid = strdup(coltxt);

	} else if (strcmp(colname, "app_component") == 0) {
		if (coltxt)
			uiapp->app_component = strdup(coltxt);
		else
			uiapp->app_component = NULL;
	} else if (strcmp(colname, "app_exec") == 0) {
		if (coltxt)
			uiapp->exec = strdup(coltxt);
		else
			uiapp->exec = NULL;
	} else if (strcmp(colname, "app_ambient_support") == 0) {
		if (coltxt)
			uiapp->ambient_support = strdup(coltxt);
		else
			uiapp->ambient_support = NULL;
	} else if (strcmp(colname, "app_nodisplay") == 0) {
		if (coltxt)
			uiapp->nodisplay = strdup(coltxt);
		else
			uiapp->nodisplay = NULL;
	} else if (strcmp(colname, "app_type") == 0 ) {
		if (coltxt)
			uiapp->type = strdup(coltxt);
		else
			uiapp->type = NULL;
	} else if (strcmp(colname, "app_onboot") == 0 ) {
		if (coltxt)
			uiapp->onboot= strdup(coltxt);
		else
			uiapp->onboot = NULL;
	} else if (strcmp(colname, "app_multiple") == 0 ) {
		if (coltxt)
			uiapp->multiple = strdup(coltxt);
		else
			uiapp->multiple = NULL;
	} else if (strcmp(colname, "app_autorestart") == 0 ) {
		if (coltxt)
			uiapp->autorestart= strdup(coltxt);
		else
			uiapp->autorestart = NULL;
	} else if (strcmp(colname, "app_taskmanage") == 0 ) {
		if (coltxt)
			uiapp->taskmanage = strdup(coltxt);
		else
			uiapp->taskmanage = NULL;
	} else if (strcmp(colname, "app_enabled") == 0 ) {
		if (coltxt)
			uiapp->enabled= strdup(coltxt);
		else
			uiapp->enabled = NULL;
	} else if (strcmp(colname, "app_hwacceleration") == 0 ) {
		if (coltxt)
			uiapp->hwacceleration = strdup(coltxt);
		else
			uiapp->hwacceleration = NULL;
	} else if (strcmp(colname, "app_screenreader") == 0 ) {
		if (coltxt)
			uiapp->screenreader = strdup(coltxt);
		else
			uiapp->screenreader = NULL;
	} else if (strcmp(colname, "app_mainapp") == 0 ) {
		if (coltxt)
			uiapp->mainapp = strdup(coltxt);
		else
			uiapp->mainapp = NULL;
	} else if (strcmp(colname, "app_recentimage") == 0 ) {
		if (coltxt)
			uiapp->recentimage = strdup(coltxt);
		else
			uiapp->recentimage = NULL;
	} else if (strcmp(colname, "app_launchcondition") == 0 ) {
		if (coltxt)
			uiapp->launchcondition = strdup(coltxt);
		else
			uiapp->launchcondition = NULL;
	} else if (strcmp(colname, "app_indicatordisplay") == 0){
		if (coltxt)
			uiapp->indicatordisplay = strdup(coltxt);
		else
			uiapp->indicatordisplay = NULL;
	} else if (strcmp(colname, "app_portraitimg") == 0){
		if (coltxt)
			uiapp->portraitimg = strdup(coltxt);
		else
			uiapp->portraitimg = NULL;
	} else if (strcmp(colname, "app_landscapeimg") == 0){
		if (coltxt)
			uiapp->landscapeimg = strdup(coltxt);
		else
			uiapp->landscapeimg = NULL;
	} else if (strcmp(colname, "app_effectimage_type") == 0){
		if (coltxt)
			uiapp->effectimage_type = strdup(coltxt);
		else
			uiapp->effectimage_type = NULL;
	} else if (strcmp(colname, "app_guestmodevisibility") == 0){
		if (coltxt)
			uiapp->guestmode_visibility = strdup(coltxt);
		else
			uiapp->guestmode_visibility = NULL;
	} else if (strcmp(colname, "app_permissiontype") == 0 ) {
		if (coltxt)
			uiapp->permission_type = strdup(coltxt);
		else
			uiapp->permission_type = NULL;
	} else if (strcmp(colname, "app_preload") == 0 ) {
		if (coltxt)
			uiapp->preload = strdup(coltxt);
		else
			uiapp->preload = NULL;
	} else if (strcmp(colname, "app_submode") == 0 ) {
		if (coltxt)
			uiapp->submode = strdup(coltxt);
		else
			uiapp->submode = NULL;
	} else if (strcmp(colname, "app_submode_mainid") == 0 ) {
		if (coltxt)
			uiapp->submode_mainid = strdup(coltxt);
		else
			uiapp->submode_mainid = NULL;
	} else if (strcmp(colname, "app_installed_storage") == 0 ) {
		if (coltxt)
			uiapp->installed_storage = strdup(coltxt);
		else
			uiapp->installed_storage = NULL;
	} else if (strcmp(colname, "app_process_pool") == 0 ) {
		if (coltxt)
			uiapp->process_pool = strdup(coltxt);
		else
			uiapp->process_pool = NULL;
	} else if (strcmp(colname, "app_multi_instance") == 0 ) {
		if (coltxt)
			uiapp->multi_instance = strdup(coltxt);
		else
			uiapp->multi_instance = NULL;
	} else if (strcmp(colname, "app_multi_instance_mainid") == 0 ) {
		if (coltxt)
			uiapp->multi_instance_mainid = strdup(coltxt);
		else
			uiapp->multi_instance_mainid = NULL;
	} else if (strcmp(colname, "app_multi_window") == 0 ) {
		if (coltxt)
			uiapp->multi_window = strdup(coltxt);
		else
			uiapp->multi_window = NULL;
	} else if (strcmp(colname, "app_support_disable") == 0 ) {
		if (coltxt)
			uiapp->support_disable= strdup(coltxt);
		else
			uiapp->support_disable = NULL;
	} else if (strcmp(colname, "app_ui_gadget") == 0 ) {
		if (coltxt)
			uiapp->ui_gadget = strdup(coltxt);
		else
			uiapp->ui_gadget = NULL;
	} else if (strcmp(colname, "app_removable") == 0 ) {
		if (coltxt)
			uiapp->removable = strdup(coltxt);
		else
			uiapp->removable = NULL;
	} else if (strcmp(colname, "app_support_mode") == 0 ) {
		if (coltxt)
			uiapp->support_mode = strdup(coltxt);
		else
			uiapp->support_mode = NULL;
	} else if (strcmp(colname, "app_support_feature") == 0 ) {
		if (coltxt)
			uiapp->support_feature = strdup(coltxt);
		else
			uiapp->support_feature = NULL;
	} else if (strcmp(colname, "component_type") == 0 ) {
		if (coltxt)
			uiapp->component_type = strdup(coltxt);
		else
			uiapp->component_type = NULL;
	} else if (strcmp(colname, "package") == 0 ) {
		if (coltxt)
			uiapp->package = strdup(coltxt);
		else
			uiapp->package = NULL;
	} else if (strcmp(colname, "app_package_type") == 0 ) {
		if (coltxt)
			uiapp->package_type = strdup(coltxt);
		else
			uiapp->package_type = NULL;
	} else if (strcmp(colname, "app_package_system") == 0 ) {
		if (coltxt)
			uiapp->package_system = strdup(coltxt);
		else
			uiapp->package_system = NULL;
	} else if (strcmp(colname, "app_package_installed_time") == 0 ) {
		if (coltxt)
			uiapp->package_installed_time = strdup(coltxt);
		else
			uiapp->package_installed_time = NULL;

	/*end of package_app_info table*/

	} else if (strcmp(colname, "app_locale") == 0 ) {
		if (coltxt) {
			uiapp->icon->lang = strdup(coltxt);
			uiapp->label->lang = strdup(coltxt);
		}
		else {
			uiapp->icon->lang = NULL;
			uiapp->label->lang = NULL;
		}
	} else if (strcmp(colname, "app_label") == 0 ) {
		if (coltxt)
			uiapp->label->text = strdup(coltxt);
		else
			uiapp->label->text = NULL;
	} else if (strcmp(colname, "app_icon") == 0) {
		if (coltxt)
			uiapp->icon->text = strdup(coltxt);
		else
			uiapp->icon->text = NULL;
	/*end of package_app_localized_info table*/

	} else if (strcmp(colname, "category") == 0 ) {
		if (coltxt)
			uiapp->category->name = strdup(coltxt);
		else
			uiapp->category->name = NULL;
	/*end of package_app_category_info table*/

	} else if (strcmp(colname, "md_key") == 0 ) {
		if (coltxt)
			uiapp->metadata->key = strdup(coltxt);
		else
			uiapp->metadata->key = NULL;
	} else if (strcmp(colname, "md_value") == 0 ) {
		if (coltxt)
			uiapp->metadata->value = strdup(coltxt);
		else
			uiapp->metadata->value = NULL;
	/*end of package_app_metadata_info table*/

	} else if (strcmp(colname, "pm_type") == 0 ) {
		if (coltxt)
			uiapp->permission->type= strdup(coltxt);
		else
			uiapp->permission->type = NULL;
	} else if (strcmp(colname, "pm_value") == 0 ) {
		if (coltxt)
			uiapp->permission->value = strdup(coltxt);
		else
			uiapp->permission->value = NULL;
	/*end of package_app_permission_info table*/

	} else if (strcmp(colname, "app_image") == 0) {
		if (coltxt)
			uiapp->image->text= strdup(coltxt);
		else
			uiapp->image->text = NULL;
	} else if (strcmp(colname, "app_image_section") == 0) {
		if (coltxt)
			uiapp->image->section= strdup(coltxt);
		else
			uiapp->image->section = NULL;
	/*end of package_app_image_info table*/
	}
}

static void __update_localed_label_for_list(sqlite3_stmt *stmt, pkgmgr_pkginfo_x *info)
{
	int i = 0;
	int ncols = 0;
	char *colname = NULL;
	char *coltxt = NULL;

//	uiapplication_x *ptr1 = NULL;

//	LISTHEAD(info->manifest_info->uiapplication, ptr1);

	ncols = sqlite3_column_count(stmt);

	for(i = 0; i < ncols; i++)
	{
		colname = (char *)sqlite3_column_name(stmt, i);
		if (colname && strcmp(colname, "app_label") == 0 ){
			coltxt = (char *)sqlite3_column_text(stmt, i);
			FREE_AND_STRDUP(coltxt, info->manifest_info->uiapplication->label->text);
		}
	}
}

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

int __appinfo_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)data;
	int i = 0;
	icon_x *icon = NULL;
	label_x *label = NULL;
	category_x *category = NULL;
	metadata_x *metadata = NULL;
	permission_x *permission = NULL;
	image_x *image = NULL;

	icon = calloc(1, sizeof(icon_x));
	retvm_if(icon == NULL, PMINFO_R_ERROR, "out of memory");
	LISTADD(info->uiapp_info->icon, icon);
	label = calloc(1, sizeof(label_x));
	retvm_if(label == NULL, PMINFO_R_ERROR, "out of memory");
	LISTADD(info->uiapp_info->label, label);
	category = calloc(1, sizeof(category_x));
	retvm_if(category == NULL, PMINFO_R_ERROR, "out of memory");
	LISTADD(info->uiapp_info->category, category);
	metadata = calloc(1, sizeof(metadata_x));
	retvm_if(metadata == NULL, PMINFO_R_ERROR, "out of memory");
	LISTADD(info->uiapp_info->metadata, metadata);
	permission = calloc(1, sizeof(permission_x));
	retvm_if(permission == NULL, PMINFO_R_ERROR, "out of memory");
	LISTADD(info->uiapp_info->permission, permission);
	image = calloc(1, sizeof(image_x));
	retvm_if(image == NULL, PMINFO_R_ERROR, "out of memory");
	LISTADD(info->uiapp_info->image, image);

	for(i = 0; i < ncols; i++)
	{
		__get_appinfo_from_db(colname[i], coltxt[i], info->uiapp_info);
	}
	return 0;
}

int __uiapp_list_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)data;
	int i = 0;
	uiapplication_x *uiapp = NULL;
	icon_x *icon = NULL;
	label_x *label = NULL;

	uiapp = calloc(1, sizeof(uiapplication_x));
	retvm_if(uiapp == NULL, PMINFO_R_ERROR, "out of memory");
	LISTADD(info->manifest_info->uiapplication, uiapp);
	icon = calloc(1, sizeof(icon_x));
	retvm_if(icon == NULL, PMINFO_R_ERROR, "out of memory");
	LISTADD(info->manifest_info->uiapplication->icon, icon);
	label = calloc(1, sizeof(label_x));
	retvm_if(label == NULL, PMINFO_R_ERROR, "out of memory");
	LISTADD(info->manifest_info->uiapplication->label, label);

	for(i = 0; i < ncols; i++)
	{
		__get_appinfo_from_db(colname[i], coltxt[i], info->manifest_info->uiapplication);
	}
	return 0;
}

static void __get_appinfo_for_list(sqlite3_stmt *stmt, pkgmgr_pkginfo_x *udata)
{
	int i = 0;
	int ncols = 0;
	char *colname = NULL;
	char *coltxt = NULL;

	uiapplication_x *uiapp = NULL;

	uiapp = calloc(1, sizeof(uiapplication_x));
	retvm_if(uiapp == NULL, PMINFO_R_ERROR, "out of memory");
	uiapp->icon= calloc(1, sizeof(icon_x));
	uiapp->label= calloc(1, sizeof(label_x));

	LISTADD(udata->manifest_info->uiapplication, uiapp);

	ncols = sqlite3_column_count(stmt);

	for(i = 0; i < ncols; i++)
	{
		colname = (char *)sqlite3_column_name(stmt, i);
		coltxt = (char *)sqlite3_column_text(stmt, i);

//		_LOGE("field value	:: %s = %s \n", colname, coltxt);
		__get_appinfo_from_db(colname, coltxt, udata->manifest_info->uiapplication);
	}
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
	FREE_AND_NULL(info);
	return locale_new;
catch:
	FREE_AND_NULL(info);
	return NULL;
}

char* __get_app_locale_by_fallback(sqlite3 *db, const char *appid)
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
		   FREE_AND_NULL(locale);
		   if (locale_new == NULL)
			   locale_new =  strdup(DEFAULT_LOCALE);
		   return locale_new;
	}

	/* default locale */
	FREE_AND_NULL(locale);
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

static int __sat_ui_is_enabled(const char *appid, bool *enabled)
{
	retvm_if(appid == NULL, PMINFO_R_EINVAL, "appid is NULL");
	retvm_if(enabled == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");

	if ((strncmp(appid, SAT_UI_APPID_1, strlen(SAT_UI_APPID_1)) == 0) || (strncmp(appid, SAT_UI_APPID_2, strlen(SAT_UI_APPID_2)) == 0)) {
		char info_file[MAX_PACKAGE_STR_SIZE] = {'\0', };

		snprintf(info_file, MAX_PACKAGE_STR_SIZE, "%s/%s", PKG_DATA_PATH, appid);
		if (access(info_file, F_OK)==0) {
			*enabled = 1;
		} else {
			*enabled = 0;
		}
		return PMINFO_R_OK;
	}
	return PMINFO_R_EINVAL;
}

static int __sat_ui_get_label(pkgmgrinfo_appinfo_h  handle, char **label)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(label == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");

	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	retvm_if(info->uiapp_info->appid == NULL, PMINFO_R_EINVAL, "appid is NULL");

	if ((strncmp((char *)info->uiapp_info->appid, SAT_UI_APPID_1, strlen(SAT_UI_APPID_1)) == 0) || (strncmp((char *)info->uiapp_info->appid, SAT_UI_APPID_2, strlen(SAT_UI_APPID_2)) == 0)) {
		char info_file[MAX_PACKAGE_STR_SIZE] = {'\0', };

		snprintf(info_file, MAX_PACKAGE_STR_SIZE, "%s/%s", PKG_DATA_PATH, (char *)info->uiapp_info->appid);
		if (access(info_file, F_OK)==0) {
			FILE *fp;
			char buf[MAX_PACKAGE_STR_SIZE] = {0};

			fp = fopen(info_file, "r");
			if (fp == NULL){
				_LOGE("fopen[%s] fail\n", info_file);
				return PMINFO_R_ERROR;
			}

			fgets(buf, MAX_PACKAGE_STR_SIZE, fp);
			if (buf[0] == '\0') {
				_LOGE("[%s] use db info\n", (char *)info->uiapp_info->appid);
				fclose(fp);
				return PMINFO_R_ERROR;
			}

			FREE_AND_STRDUP(buf, info->uiapp_info->satui_label);
			*label = info->uiapp_info->satui_label;

			fclose(fp);
			return PMINFO_R_OK;
		}
	}
	return PMINFO_R_ERROR;
}

API int pkgmgrinfo_appinfo_get_list(pkgmgrinfo_pkginfo_h handle, pkgmgrinfo_app_component component,
						pkgmgrinfo_app_list_cb app_func, void *user_data)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL");
	retvm_if(app_func == NULL, PMINFO_R_EINVAL, "callback pointer is NULL");
	retvm_if((component != PMINFO_UI_APP) && (component != PMINFO_SVC_APP) && (component != PMINFO_ALL_APP), PMINFO_R_EINVAL, "Invalid App Component Type");
	retvm_if(component == PMINFO_SVC_APP, PMINFO_R_OK, "PMINFO_SVC_APP is done" );

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

	snprintf(query, MAX_QUERY_LEN, "select DISTINCT * " \
			"from package_app_info where " \
			"package='%s' and app_component='%s' and app_disable='false'",
			info->manifest_info->package,"uiapp");

	/*Populate ui app info */
	ret = __exec_db_query(appinfo_db, query, __uiapp_list_cb, (void *)info);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info list retrieval failed");

	uiapplication_x *tmp = NULL;
	SAFE_LISTHEAD(info->manifest_info->uiapplication, tmp);

	/*Populate localized info for default locales and call callback*/
	/*If the callback func return < 0 we break and no more call back is called*/
	while(tmp != NULL)
	{
		if (locale)
			appinfo->locale = strdup(locale);
		appinfo->uiapp_info = tmp;
		if (strcmp(appinfo->uiapp_info->type,"c++app") == 0){
			FREE_AND_NULL(locale);
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
		snprintf(query, MAX_QUERY_LEN, "select * from package_app_app_category where app_id='%s'", appinfo->uiapp_info->appid);
		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Category Info DB Information retrieval failed");

		/*Populate app metadata*/
		memset(query, '\0', MAX_QUERY_LEN);
		snprintf(query, MAX_QUERY_LEN, "select * from package_app_app_metadata where app_id='%s'", appinfo->uiapp_info->appid);
		ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
		tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Metadata Info DB Information retrieval failed");

		SAFE_LISTHEAD(appinfo->uiapp_info->icon, ptr1);
		SAFE_LISTHEAD(appinfo->uiapp_info->label, ptr2);
		SAFE_LISTHEAD(appinfo->uiapp_info->category, ptr3);
		SAFE_LISTHEAD(appinfo->uiapp_info->metadata, ptr4);
		SAFE_LISTHEAD(appinfo->uiapp_info->permission, ptr5);
		SAFE_LISTHEAD(appinfo->uiapp_info->image, ptr6);

		ret = app_func((void *)appinfo, user_data);
		if (ret < 0){
			FREE_AND_NULL(appinfo->locale);
			break;
		}

		FREE_AND_NULL(appinfo->locale);
		tmp = tmp->next;
	}

	ret = PMINFO_R_OK;

catch:
	FREE_AND_NULL(locale);
	FREE_AND_NULL(appinfo);
	__cleanup_pkginfo(allinfo);
	sqlite3_close(appinfo_db);
	return ret;
}

API int pkgmgrinfo_appinfo_get_install_list(pkgmgrinfo_app_list_cb app_func, void *user_data)
{
	retvm_if(app_func == NULL, PMINFO_R_EINVAL, "callback function is NULL");

	int ret = PMINFO_R_OK;
	char *query = NULL;
	pkgmgr_pkginfo_x *info = NULL;
	pkgmgr_appinfo_x *appinfo = NULL;
	uiapplication_x *ptr1 = NULL;
	sqlite3 *appinfo_db = NULL;
	sqlite3_stmt *stmt = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	retvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*calloc pkginfo*/
	info = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*calloc manifest_info*/
	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(info->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*calloc appinfo*/
	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*Start constructing query*/
	query = sqlite3_mprintf("select * from package_app_info where app_disable='false'");

	/*prepare query*/
	ret = sqlite3_prepare_v2(appinfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	/*step query*/
	while(1) {
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {
			__get_appinfo_for_list(stmt, info);
		} else {
			break;
		}
	}

	/*head up*/
	SAFE_LISTHEAD(info->manifest_info->uiapplication, ptr1);

	/*UI Apps*/
	for(ptr1 = info->manifest_info->uiapplication; ptr1; ptr1 = ptr1->next)
	{
		appinfo->uiapp_info = ptr1;

		ret = app_func((void *)appinfo, user_data);
		if (ret < 0) {
			break;
		}
	}
	ret = PMINFO_R_OK;

catch:
	sqlite3_free(query);
	sqlite3_finalize(stmt);
	sqlite3_close(appinfo_db);
	FREE_AND_NULL(appinfo);
	__cleanup_pkginfo(info);
	return ret;
}


API int pkgmgrinfo_appinfo_get_installed_list(pkgmgrinfo_app_list_cb app_func, void *user_data)
{
	retvm_if(app_func == NULL, PMINFO_R_EINVAL, "callback function is NULL");

	int ret = PMINFO_R_OK;
	char *query = NULL;
	char *locale = NULL;
	char appid[MAX_QUERY_LEN] = {0,};
	char pre_appid[MAX_QUERY_LEN] = {0,};

	pkgmgr_pkginfo_x *info = NULL;
	pkgmgr_appinfo_x *appinfo = NULL;
	uiapplication_x *ptr1 = NULL;

	sqlite3 *appinfo_db = NULL;
	sqlite3_stmt *stmt = NULL;


	/*calloc manifest_info*/
	info = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	retvm_if(info == NULL, PMINFO_R_ERROR, "Out of Memory!!!");

	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(info->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "db_util_open[%s] failed!", MANIFEST_DB);

	/*Start constructing query*/
	locale = __convert_system_locale_to_manifest_locale();
	query = sqlite3_mprintf("select * from package_app_info LEFT OUTER JOIN package_app_localized_info "\
					"ON package_app_info.app_id=package_app_localized_info.app_id "\
					"where package_app_info.app_disable='false' and package_app_localized_info.app_locale IN (%Q, %Q)", DEFAULT_LOCALE, locale);

	/*prepare query*/
	ret = sqlite3_prepare_v2(appinfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	/*step query*/
	while(1) {
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {

			memset(appid, 0, MAX_QUERY_LEN);
			strncpy(appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

			if (strlen(pre_appid) != 0) {
				if (strcmp(pre_appid, appid) == 0) {
					/*if same appid is found, then it is about exact matched locale*/
					__update_localed_label_for_list(stmt, info);

					memset(pre_appid, 0, MAX_QUERY_LEN);
					strncpy(pre_appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

					continue;
				} else {
					memset(pre_appid, 0, MAX_QUERY_LEN);
					strncpy(pre_appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
				}
			} else {
				strncpy(pre_appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
			}

			__get_appinfo_for_list(stmt, info);
		} else {
			break;
		}
	}

	/*head up*/
	SAFE_LISTHEAD(info->manifest_info->uiapplication, ptr1);

	/*UI Apps*/
	for(ptr1 = info->manifest_info->uiapplication; ptr1; ptr1 = ptr1->next)
	{
		if (locale && appinfo->locale == NULL)
			appinfo->locale = strdup(locale);
		appinfo->uiapp_info = ptr1;

		ret = __appinfo_check_installed_storage(appinfo);
		if(ret < 0)
			continue;

		ret = app_func((void *)appinfo, user_data);
		if (ret < 0)
			break;
		FREE_AND_NULL(appinfo->locale);
	}
	ret = PMINFO_R_OK;

catch:
	FREE_AND_NULL(locale);
	sqlite3_free(query);
	sqlite3_finalize(stmt);
	sqlite3_close(appinfo_db);
	FREE_AND_NULL(appinfo);
	__cleanup_pkginfo(info);
	return ret;
}

API int pkgmgrinfo_appinfo_get_mounted_list(pkgmgrinfo_app_list_cb app_func, void *user_data)
{
	retvm_if(app_func == NULL, PMINFO_R_EINVAL, "callback function is NULL");

	int ret = PMINFO_R_OK;
	char *query = NULL;
	char appid[MAX_QUERY_LEN] = {0,};
	char pre_appid[MAX_QUERY_LEN] = {0,};
	char *locale = NULL;
	pkgmgr_pkginfo_x *info = NULL;
	pkgmgr_appinfo_x *appinfo = NULL;
	uiapplication_x *ptr1 = NULL;
	sqlite3 *appinfo_db = NULL;
	sqlite3_stmt *stmt = NULL;

	/*get system locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "manifest locale is NULL");

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*calloc pkginfo*/
	info = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*calloc manifest_info*/
	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(info->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*calloc appinfo*/
	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*Start constructing query*/
	query = sqlite3_mprintf("select * from package_app_info LEFT OUTER JOIN package_app_localized_info " \
								"ON package_app_info.app_id=package_app_localized_info.app_id " \
								"where app_installed_storage='installed_external' and package_app_info.app_disable='false' and package_app_localized_info.app_locale IN (%Q, %Q)", DEFAULT_LOCALE, locale);

	/*prepare query*/
	ret = sqlite3_prepare_v2(appinfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	/*step query*/
	while(1) {
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {

			memset(appid, 0, MAX_QUERY_LEN);
			strncpy(appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

			if (strlen(pre_appid) != 0) {
				if (strcmp(pre_appid, appid) == 0) {
					/*if same appid is found, then it is about exact matched locale*/
					__update_localed_label_for_list(stmt, info);

					memset(pre_appid, 0, MAX_QUERY_LEN);
					strncpy(pre_appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

					continue;
				} else {
					memset(pre_appid, 0, MAX_QUERY_LEN);
					strncpy(pre_appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
				}
			} else {
				strncpy(pre_appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
			}

			__get_appinfo_for_list(stmt, info);
		} else {
			break;
		}
	}

	/*head up*/
	SAFE_LISTHEAD(info->manifest_info->uiapplication, ptr1);

	/*UI Apps*/
	for(ptr1 = info->manifest_info->uiapplication; ptr1; ptr1 = ptr1->next)
	{
		appinfo->locale = strdup(locale);
		appinfo->uiapp_info = ptr1;

		ret = __appinfo_check_installed_storage(appinfo);
		if(ret < 0) {
			FREE_AND_NULL(appinfo->locale);
			continue;
		}

		ret = app_func((void *)appinfo, user_data);
		if(ret < 0) {
			FREE_AND_NULL(appinfo->locale);
			break;
		}

		FREE_AND_NULL(appinfo->locale);
	}
	ret = PMINFO_R_OK;

catch:
	FREE_AND_NULL(locale);
	sqlite3_free(query);
	sqlite3_finalize(stmt);
	sqlite3_close(appinfo_db);
	FREE_AND_NULL(appinfo);
	__cleanup_pkginfo(info);
	return ret;
}

API int pkgmgrinfo_appinfo_get_unmounted_list(pkgmgrinfo_app_list_cb app_func, void *user_data)
{
	retvm_if(app_func == NULL, PMINFO_R_EINVAL, "callback function is NULL");

	int ret = PMINFO_R_OK;
	char *query = NULL;
	char appid[MAX_QUERY_LEN] = {0,};
	char pre_appid[MAX_QUERY_LEN] = {0,};
	char *locale = NULL;
	pkgmgr_pkginfo_x *info = NULL;
	pkgmgr_appinfo_x *appinfo = NULL;
	uiapplication_x *ptr1 = NULL;
	sqlite3 *appinfo_db = NULL;
	sqlite3_stmt *stmt = NULL;

	/*get system locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "manifest locale is NULL");

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*calloc pkginfo*/
	info = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*calloc manifest_info*/
	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(info->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*calloc appinfo*/
	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!");

	/*Start constructing query*/
	query = sqlite3_mprintf("select * from package_app_info LEFT OUTER JOIN package_app_localized_info " \
								"ON package_app_info.app_id=package_app_localized_info.app_id " \
								"where app_installed_storage='installed_external' and package_app_info.app_disable='false' and package_app_localized_info.app_locale IN (%Q, %Q)", DEFAULT_LOCALE, locale);

	/*prepare query*/
	ret = sqlite3_prepare_v2(appinfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	/*step query*/
	while(1) {
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {

			memset(appid, 0, MAX_QUERY_LEN);
			strncpy(appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

			if (strlen(pre_appid) != 0) {
				if (strcmp(pre_appid, appid) == 0) {
					/*if same appid is found, then it is about exact matched locale*/
					__update_localed_label_for_list(stmt, info);

					memset(pre_appid, 0, MAX_QUERY_LEN);
					strncpy(pre_appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

					continue;
				} else {
					memset(pre_appid, 0, MAX_QUERY_LEN);
					strncpy(pre_appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
				}
			} else {
				strncpy(pre_appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
			}

			__get_appinfo_for_list(stmt, info);
		} else {
			break;
		}
	}

	/*head up*/
	SAFE_LISTHEAD(info->manifest_info->uiapplication, ptr1);

	/*UI Apps*/
	for(ptr1 = info->manifest_info->uiapplication; ptr1; ptr1 = ptr1->next)
	{
		appinfo->locale = strdup(locale);
		appinfo->uiapp_info = ptr1;

		ret = app_func((void *)appinfo, user_data);
		if(ret < 0) {
			FREE_AND_NULL(appinfo->locale);
			break;
		}

		FREE_AND_NULL(appinfo->locale);
	}
	ret = PMINFO_R_OK;

catch:
	FREE_AND_NULL(locale);
	sqlite3_free(query);
	sqlite3_finalize(stmt);
	sqlite3_close(appinfo_db);
	FREE_AND_NULL(appinfo);
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
	char *alias_id = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	retvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*Get the alias id*/
	alias_id = __get_aliasid_from_db(appinfo_db,appid);

	/*check alias_id exist on db*/
	query = sqlite3_mprintf("select exists(select * from package_app_info where app_id=%Q and app_disable='false')", alias_id);
	ret = __exec_db_query(appinfo_db, query, _pkgmgrinfo_validate_cb, (void *)&exist);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "sqlite3_exec fail");
	tryvm_if(exist == 0, ret = PMINFO_R_ERROR, "Appid[%s] not found in DB", alias_id);

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
	query = sqlite3_mprintf("select * from package_app_info where app_id=%Q and app_disable='false' ", alias_id);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	query = sqlite3_mprintf("select * from package_app_localized_info where app_id=%Q and app_locale=%Q", alias_id, locale);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	/*Also store the values corresponding to default locales*/
	query = sqlite3_mprintf("select * from package_app_localized_info where app_id=%Q and app_locale=%Q", alias_id, DEFAULT_LOCALE);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

	/*Populate app category*/
	query = sqlite3_mprintf("select * from package_app_app_category where app_id=%Q", alias_id);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Category Info DB Information retrieval failed");

	/*Populate app metadata*/
	query = sqlite3_mprintf("select * from package_app_app_metadata where app_id=%Q", alias_id);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Metadata Info DB Information retrieval failed");

	/*Populate app permission*/
	query = sqlite3_mprintf("select * from package_app_app_permission where app_id=%Q", alias_id);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App permission Info DB Information retrieval failed");

	/*store setting notification icon section*/
	query = sqlite3_mprintf("select * from package_app_icon_section_info where app_id=%Q", alias_id);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App icon section Info DB Information retrieval failed");

	/*store app preview image info*/
	query = sqlite3_mprintf("select app_image_section, app_image from package_app_image_info where app_id=%Q", alias_id);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App image Info DB Information retrieval failed");

	SAFE_LISTHEAD(appinfo->uiapp_info->label, tmp1);
	SAFE_LISTHEAD(appinfo->uiapp_info->icon, tmp2);
	SAFE_LISTHEAD(appinfo->uiapp_info->category, tmp3);
	SAFE_LISTHEAD(appinfo->uiapp_info->metadata, tmp4);
	SAFE_LISTHEAD(appinfo->uiapp_info->permission, tmp5);
	SAFE_LISTHEAD(appinfo->uiapp_info->image, tmp6);

	ret = PMINFO_R_OK;

catch:
	if (ret == PMINFO_R_OK)
		*handle = (void*)appinfo;
	else {
		*handle = NULL;
		__cleanup_appinfo(appinfo);
	}

	sqlite3_close(appinfo_db);
	FREE_AND_NULL(locale);
	FREE_AND_NULL(alias_id);
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
	char *alias_id = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	retvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*Get the alias id*/
	alias_id = __get_aliasid_from_db(appinfo_db,appid);

	/*check alias_id exist on db*/
	query = sqlite3_mprintf("select exists(select * from package_app_info where app_id=%Q and app_disable='false')", alias_id);
	ret = __exec_db_query(appinfo_db, query, _pkgmgrinfo_validate_cb, (void *)&exist);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "sqlite3_exec fail");
	if (exist == 0) {
		_LOGS("Appid[%s] not found in DB", alias_id);
		ret = PMINFO_R_ERROR;
		goto catch;
	}

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
	query = sqlite3_mprintf("select * from package_app_info where app_id=%Q and app_disable='false' ", alias_id);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	query = sqlite3_mprintf("select * from package_app_localized_info where app_id=%Q and app_locale=%Q", alias_id, locale);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	/*Also store the values corresponding to default locales*/
	query = sqlite3_mprintf("select * from package_app_localized_info where app_id=%Q and app_locale=%Q", alias_id, DEFAULT_LOCALE);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Localized Info DB Information retrieval failed");

	/*Populate app category*/
	query = sqlite3_mprintf("select * from package_app_app_category where app_id=%Q", alias_id);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Category Info DB Information retrieval failed");

	/*Populate app metadata*/
	query = sqlite3_mprintf("select * from package_app_app_metadata where app_id=%Q", alias_id);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Metadata Info DB Information retrieval failed");

	/*Populate app permission*/
	query = sqlite3_mprintf("select * from package_app_app_permission where app_id=%Q", alias_id);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App permission Info DB Information retrieval failed");

	/*store setting notification icon section*/
	query = sqlite3_mprintf("select * from package_app_icon_section_info where app_id=%Q", alias_id);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App icon section Info DB Information retrieval failed");

	/*store app preview image info*/
	query = sqlite3_mprintf("select app_image_section, app_image from package_app_image_info where app_id=%Q", alias_id);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App image Info DB Information retrieval failed");

	ret = __appinfo_check_installed_storage(appinfo);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "[%s] is installed external, but is not in mmc", appinfo->uiapp_info->package);

	SAFE_LISTHEAD(appinfo->uiapp_info->label, tmp1);
	SAFE_LISTHEAD(appinfo->uiapp_info->icon, tmp2);
	SAFE_LISTHEAD(appinfo->uiapp_info->category, tmp3);
	SAFE_LISTHEAD(appinfo->uiapp_info->metadata, tmp4);
	SAFE_LISTHEAD(appinfo->uiapp_info->permission, tmp5);
	SAFE_LISTHEAD(appinfo->uiapp_info->image, tmp6);

	ret = PMINFO_R_OK;

catch:
	if (ret == PMINFO_R_OK)
		*handle = (void*)appinfo;
	else {
		*handle = NULL;
		__cleanup_appinfo(appinfo);
	}

	sqlite3_close(appinfo_db);
	FREE_AND_NULL(locale);
	FREE_AND_NULL(alias_id);
	return ret;
}


API int pkgmgrinfo_appinfo_get_appid(pkgmgrinfo_appinfo_h  handle, char **appid)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(appid == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	*appid = (char *)info->uiapp_info->appid;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_pkgname(pkgmgrinfo_appinfo_h  handle, char **pkg_name)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(pkg_name == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	*pkg_name = (char *)info->uiapp_info->package;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_pkgid(pkgmgrinfo_appinfo_h  handle, char **pkgid)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(pkgid == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	*pkgid = (char *)info->uiapp_info->package;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_pkgtype(pkgmgrinfo_appinfo_h  handle, char **pkgtype)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(pkgtype == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	*pkgtype = (char *)info->uiapp_info->package_type;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_exec(pkgmgrinfo_appinfo_h  handle, char **exec)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(exec == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	*exec = (char *)info->uiapp_info->exec;

	return PMINFO_R_OK;
}


API int pkgmgrinfo_appinfo_get_ambient_support(pkgmgrinfo_appinfo_h  handle, bool *ambient_support)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(ambient_support == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	char *val = NULL;
	bool is_support = false;

	val = (char *)info->uiapp_info->ambient_support;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			is_support = true;
	}

	*ambient_support = is_support;

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

	start = info->uiapp_info->icon;

    for(ptr = start; ptr != NULL; ptr = ptr->next)
    {
            if (ptr->lang) {
                    if (strcmp(ptr->lang, locale) == 0) {
						if (ptr->text) {
                            *icon = (char *)ptr->text;
                            if (strcasecmp(*icon, PKGMGR_PARSER_EMPTY_STR) == 0) {
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
	int ret = 0;
	char *locale = NULL;
	label_x *ptr = NULL;
	label_x *start = NULL;
	*label = NULL;

	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	locale = info->locale;
	retvm_if(locale == NULL, PMINFO_R_ERROR, "manifest locale is NULL");

	ret = __sat_ui_get_label(handle, label);
	retvm_if(ret == PMINFO_R_OK, PMINFO_R_OK, "sat ui(%s) is enabled", (char *)info->uiapp_info->appid);

	start = info->uiapp_info->label;

	for(ptr = start; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->lang) {
			if (strcmp(ptr->lang, locale) == 0) {
				if (ptr->text) {
					*label = (char *)ptr->text;
					if (strcasecmp(*label, PKGMGR_PARSER_EMPTY_STR) == 0) {
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
					if (strcasecmp(*label, PKGMGR_PARSER_EMPTY_STR) == 0) {
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

	*component = PMINFO_UI_APP;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_apptype(pkgmgrinfo_appinfo_h  handle, char **app_type)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(app_type == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	*app_type = (char *)info->uiapp_info->type;

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

	val = (char*)info->uiapp_info->permission_type;

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
		if (strcasecmp(val, "off") == 0)
			*hwacceleration = PMINFO_HWACCELERATION_OFF;
		else if (strcasecmp(val, "on") == 0)
			*hwacceleration = PMINFO_HWACCELERATION_ON;
		else
			*hwacceleration = PMINFO_HWACCELERATION_DEFAULT;
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

	*portrait_img = (char *)info->uiapp_info->portraitimg;
	*landscape_img = (char *)info->uiapp_info->landscapeimg;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_effectimage_type(pkgmgrinfo_appinfo_h  handle, char **effectimg_type)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(effectimg_type == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	*effectimg_type = (char *)info->uiapp_info->effectimage_type;

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
	char *query = NULL;

	retvm_if(appid == NULL || locale == NULL || label == NULL, PMINFO_R_EINVAL, "Argument is NULL");

	sqlite3_stmt *stmt = NULL;
	sqlite3 *pkgmgr_parser_db = NULL;
	char *alias_id = NULL;

	ret = db_util_open(MANIFEST_DB, &pkgmgr_parser_db, 0);
	if (ret != SQLITE_OK) {
		_LOGE("DB open fail\n");
		return -1;
	}

	/*Get the alias id*/
	alias_id = __get_aliasid_from_db(pkgmgr_parser_db,appid);

	query = sqlite3_mprintf("select app_label from package_app_localized_info where app_id=%Q and app_locale=%Q", alias_id, locale);


	ret = sqlite3_prepare_v2(pkgmgr_parser_db, query, strlen(query), &stmt, NULL);
	if (ret != SQLITE_OK) {
		_LOGE("prepare_v2 fail\n");
		sqlite3_close(pkgmgr_parser_db);
		sqlite3_free(query);
		FREE_AND_NULL(alias_id);
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
				FREE_AND_NULL(localed_label);
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
		query = sqlite3_mprintf("select app_label from package_app_localized_info where app_id=%Q and app_locale=%Q", alias_id, DEFAULT_LOCALE);
		ret = sqlite3_prepare_v2(pkgmgr_parser_db, query, strlen(query), &stmt, NULL);
		if (ret != SQLITE_OK) {
			_LOGE("prepare_v2 fail\n");
			sqlite3_close(pkgmgr_parser_db);
			sqlite3_free(query);
			FREE_AND_NULL(alias_id);
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
					FREE_AND_NULL(localed_label);
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

	FREE_AND_NULL(alias_id);
	sqlite3_finalize(stmt);
	sqlite3_close(pkgmgr_parser_db);
	sqlite3_free(query);

	if (localed_label == NULL) {
		return PMINFO_R_ERROR;
	} else {
		return PMINFO_R_OK;
	}
}

API int pkgmgrinfo_appinfo_get_metadata_value(pkgmgrinfo_appinfo_h handle, const char *metadata_key, char **metadata_value)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(metadata_key == NULL, PMINFO_R_EINVAL, "metadata_key is NULL");
	retvm_if(metadata_value == NULL, PMINFO_R_EINVAL, "metadata_value is NULL");

	metadata_x *ptr = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	ptr = info->uiapp_info->metadata;

	for (; ptr; ptr = ptr->next) {
		if (ptr->key) {
			if (strcasecmp(ptr->key, metadata_key) == 0)
			{
				*metadata_value = (char*)ptr->value;
				return PMINFO_R_OK;
			}
		}
	}

	return PMINFO_R_EINVAL;
}

API int pkgmgrinfo_appinfo_get_multi_instance_mainid(pkgmgrinfo_appinfo_h  handle, char **multi_instance_mainid)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(multi_instance_mainid == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	*multi_instance_mainid = (char *)info->uiapp_info->multi_instance_mainid;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_datacontrol_info(const char *providerid, const char *type, char **appid, char **access)
{
	retvm_if(providerid == NULL, PMINFO_R_EINVAL, "Argument supplied is NULL\n");
	retvm_if(type == NULL, PMINFO_R_EINVAL, "Argument supplied is NULL\n");
	retvm_if(appid == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	retvm_if(access == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	int ret = PMINFO_R_OK;
	char *query = NULL;
	sqlite3 *appinfo_db = NULL;
	sqlite3_stmt *stmt = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	retvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*Start constructing query*/
	query = sqlite3_mprintf("select * from package_app_data_control where providerid=%Q and type=%Q", providerid, type);

	/*prepare query*/
	ret = sqlite3_prepare_v2(appinfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	/*step query*/
	ret = sqlite3_step(stmt);
	tryvm_if((ret != SQLITE_ROW) || (ret == SQLITE_DONE), ret = PMINFO_R_ERROR, "No records found");

	*appid = strdup((char *)sqlite3_column_text(stmt, 0));
	*access = strdup((char *)sqlite3_column_text(stmt, 2));

	ret = PMINFO_R_OK;

catch:
	sqlite3_free(query);
	sqlite3_finalize(stmt);
	sqlite3_close(appinfo_db);
	return ret;
}

API int pkgmgrinfo_appinfo_get_datacontrol_appid(const char *providerid, char **appid)
{
	retvm_if(providerid == NULL, PMINFO_R_EINVAL, "Argument supplied is NULL\n");
	retvm_if(appid == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	int ret = PMINFO_R_OK;
	char *query = NULL;
	sqlite3 *appinfo_db = NULL;
	sqlite3_stmt *stmt = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	retvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*Start constructing query*/
	query = sqlite3_mprintf("select * from package_app_data_control where providerid=%Q", providerid);

	/*prepare query*/
	ret = sqlite3_prepare_v2(appinfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	/*step query*/
	ret = sqlite3_step(stmt);
	tryvm_if((ret != SQLITE_ROW) || (ret == SQLITE_DONE), ret = PMINFO_R_ERROR, "No records found");

	*appid = strdup((char *)sqlite3_column_text(stmt, 0));

	ret = PMINFO_R_OK;

catch:
	sqlite3_free(query);
	sqlite3_finalize(stmt);
	sqlite3_close(appinfo_db);
	return ret;
}

API int pkgmgrinfo_appinfo_get_support_mode(pkgmgrinfo_appinfo_h  handle, int *support_mode)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(support_mode == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");

	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	if (info->uiapp_info->support_mode)
		*support_mode = atoi(info->uiapp_info->support_mode);
	else
		*support_mode = 0;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_support_feature(pkgmgrinfo_appinfo_h  handle, int *support_feature)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(support_feature == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");

	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	if (info->uiapp_info->support_feature)
		*support_feature = atoi(info->uiapp_info->support_feature);
	else
		*support_feature = 0;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_get_uginfo(const char *ug_name, pkgmgrinfo_appinfo_h *handle)
{
	retvm_if(ug_name == NULL, PMINFO_R_EINVAL, "ug_name is NULL");
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");

	pkgmgr_appinfo_x *appinfo = NULL;
	int ret = -1;
	char *query = NULL;
	sqlite3 *appinfo_db = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	retvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*calloc appinfo*/
	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for appinfo");

	/*calloc app_component*/
	appinfo->uiapp_info = (uiapplication_x *)calloc(1, sizeof(uiapplication_x));
	tryvm_if(appinfo->uiapp_info == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for uiapp info");

	/*populate app_info from DB*/
	query = sqlite3_mprintf("select * from package_app_info where app_ui_gadget='true' and app_exec like '%%%s'", ug_name);
	ret = __exec_db_query(appinfo_db, query, __appinfo_cb, (void *)appinfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "App Info DB Information retrieval failed");

	ret = __appinfo_check_installed_storage(appinfo);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "[%s] is installed external, but is not in mmc", appinfo->uiapp_info->package);

	ret = PMINFO_R_OK;

catch:
	if (ret == PMINFO_R_OK)
		*handle = (void*)appinfo;
	else {
		*handle = NULL;
		__cleanup_appinfo(appinfo);
	}

	sqlite3_close(appinfo_db);

	return ret;
}

/*Get the alias id for an appid from pkgmgr DB*/
API int  pkgmgrinfo_appinfo_get_aliasid(const char *appid, char **alias_id)
{

	sqlite3 *appinfo_db = NULL;
	int ret = PMINFO_R_OK;

	retvm_if(appid == NULL, PMINFO_R_EINVAL, "appid is NULL");
	retvm_if(alias_id == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");

	/*open db */
	ret = db_util_open(MANIFEST_DB, &appinfo_db, 0);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	*alias_id = __get_aliasid_from_db(appinfo_db,appid);
catch:

	sqlite3_close(appinfo_db);
	return ret;
}

API int pkgmgrinfo_appinfo_get_installed_time(pkgmgrinfo_appinfo_h handle, int *installed_time)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(installed_time == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	if (info->uiapp_info->package_installed_time)
		*installed_time = atoi(info->uiapp_info->package_installed_time);
	else
		return PMINFO_R_ERROR;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_foreach_permission(pkgmgrinfo_appinfo_h handle,
			pkgmgrinfo_app_permission_list_cb permission_func, void *user_data)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(permission_func == NULL, PMINFO_R_EINVAL, "Callback function is NULL");
	int ret = -1;
	permission_x *ptr = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	ptr = info->uiapp_info->permission;

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

	ptr = info->uiapp_info->category;

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

	ptr = info->uiapp_info->metadata;

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
	appsvc_x *appsvc = NULL;
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
		FREE_AND_NULL(manifest);
		return PMINFO_R_ERROR;
	}
	FREE_AND_NULL(manifest);
	ptr  = calloc(1, sizeof(pkgmgrinfo_appcontrol_x));
	if (ptr == NULL) {
		_LOGE("Out of Memory!!!\n");
		pkgmgr_parser_free_manifest_xml(mfx);
		return PMINFO_R_ERROR;
	}
	/*Get Operation, Uri, Mime*/
	if (mfx->uiapplication) {
		if (mfx->uiapplication->appsvc) {
			appsvc = mfx->uiapplication->appsvc;
		}
	}

	for (; appsvc; appsvc = appsvc->next) {
		op = appsvc->operation;
		for (; op; op = op->next)
			oc = oc + 1;
		op = appsvc->operation;

		ui = appsvc->uri;
		for (; ui; ui = ui->next)
			uc = uc + 1;
		ui = appsvc->uri;

		mi = appsvc->mime;
		for (; mi; mi = mi->next)
			mc = mc + 1;
		mi = appsvc->mime;

		sa = appsvc->subapp;
		for (; sa; sa = sa->next)
			sc = sc + 1;
		sa = appsvc->subapp;

		operation = (char **)calloc(oc, sizeof(char *));
		if (operation == NULL) {
			_LOGE("out of memory");
			break;
		}

		for (i = 0; i < oc; i++) {
			operation[i] = strndup(op->name, PKG_STRING_LEN_MAX - 1);
			op = op->next;
		}

		uri = (char **)calloc(uc, sizeof(char *));
		if (uri == NULL) {
			_LOGE("out of memory");
			for (i = 0; i < oc; i++) {
				FREE_AND_NULL(operation[i]);
			}
			FREE_AND_NULL(operation);
			break;
		}

		for (i = 0; i < uc; i++) {
			uri[i] = strndup(ui->name, PKG_STRING_LEN_MAX - 1);
			ui = ui->next;
		}

		mime = (char **)calloc(mc, sizeof(char *));
		if (mime == NULL) {
			_LOGE("out of memory");
			for (i = 0; i < uc; i++) {
				FREE_AND_NULL(uri[i]);
			}
			FREE_AND_NULL(uri);
			for (i = 0; i < oc; i++) {
				FREE_AND_NULL(operation[i]);
			}
			FREE_AND_NULL(operation);
			break;
		}

		for (i = 0; i < mc; i++) {
			mime[i] = strndup(mi->name, PKG_STRING_LEN_MAX - 1);
			mi = mi->next;
		}

		subapp = (char **)calloc(sc, sizeof(char *));
		if (subapp == NULL) {
			_LOGE("out of memory");
			for (i = 0; i < mc; i++) {
				FREE_AND_NULL(mime[i]);
			}
			FREE_AND_NULL(mime);
			for (i = 0; i < uc; i++) {
				FREE_AND_NULL(uri[i]);
			}
			FREE_AND_NULL(uri);
			for (i = 0; i < oc; i++) {
				FREE_AND_NULL(operation[i]);
			}
			FREE_AND_NULL(operation);
			break;
		}

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
			FREE_AND_NULL(operation[i]);
		}
		FREE_AND_NULL(operation);
		for (i = 0; i < uc; i++) {
			FREE_AND_NULL(uri[i]);
		}
		FREE_AND_NULL(uri);
		for (i = 0; i < mc; i++) {
			FREE_AND_NULL(mime[i]);
		}
		FREE_AND_NULL(mime);
		for (i = 0; i < sc; i++) {
			FREE_AND_NULL(subapp[i]);
		}
		FREE_AND_NULL(subapp);
		if (ret < 0)
			break;
		uc = 0;
		mc = 0;
		oc = 0;
		sc = 0;
	}
	pkgmgr_parser_free_manifest_xml(mfx);
	FREE_AND_NULL(ptr);
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
	int ret = 0;
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	ret = __sat_ui_is_enabled((char *)info->uiapp_info->appid, enabled);
	retvm_if(ret == PMINFO_R_OK, PMINFO_R_OK, "sat ui(%s) is enabled", (char *)info->uiapp_info->appid);

	val = (char *)info->uiapp_info->enabled;

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

	val = (char *)info->uiapp_info->onboot;

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

	val = (char *)info->uiapp_info->autorestart;

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

API int pkgmgrinfo_appinfo_is_category_exist(pkgmgrinfo_appinfo_h handle, const char *category, bool *exist)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL");
	retvm_if(category == NULL, PMINFO_R_EINVAL, "category is NULL");
	retvm_if(exist == NULL, PMINFO_R_EINVAL, "exist is NULL");

	category_x *ptr = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;

	*exist = 0;

	ptr = info->uiapp_info->category;

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

API int pkgmgrinfo_appinfo_is_multi_instance(pkgmgrinfo_appinfo_h handle, bool *multi_instance)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(multi_instance == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->multi_instance;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*multi_instance = 1;
		else if (strcasecmp(val, "false") == 0)
			*multi_instance = 0;
		else
			*multi_instance = 0;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_is_multi_window(pkgmgrinfo_appinfo_h handle, bool *multi_window)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(multi_window == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->multi_window;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*multi_window = 1;
		else if (strcasecmp(val, "false") == 0)
			*multi_window = 0;
		else
			*multi_window = 0;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_is_support_disable(pkgmgrinfo_appinfo_h handle, bool *support_disable)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(support_disable == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->support_disable;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*support_disable = 1;
		else if (strcasecmp(val, "false") == 0)
			*support_disable = 0;
		else
			*support_disable = 0;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_is_ui_gadget(pkgmgrinfo_appinfo_h handle, bool *ui_gadget)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(ui_gadget == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->ui_gadget;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*ui_gadget = 1;
		else if (strcasecmp(val, "false") == 0)
			*ui_gadget = 0;
		else
			*ui_gadget = 0;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_is_removable(pkgmgrinfo_appinfo_h handle, bool *removable)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(removable == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->removable;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*removable = 1;
		else if (strcasecmp(val, "false") == 0)
			*removable = 0;
		else
			*removable = 0;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_appinfo_is_system(pkgmgrinfo_appinfo_h handle, bool *system)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(system == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	char *val = NULL;
	pkgmgr_appinfo_x *info = (pkgmgr_appinfo_x *)handle;
	val = (char *)info->uiapp_info->package_system;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*system = 1;
		else if (strcasecmp(val, "false") == 0)
			*system = 0;
		else
			*system = 0;
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
		FREE_AND_NULL(node);
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
		FREE_AND_NULL(node);
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
		val = strndup("uiapp", PKG_STRING_LEN_MAX - 1);
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
			FREE_AND_NULL(node);
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

	int ret = 0;
	int filter_count = 0;

	char *locale = NULL;
	char *condition = NULL;
	char query[MAX_QUERY_LEN] = {'\0'};
	char where[MAX_QUERY_LEN] = {'\0'};

	uiapplication_x *ptr1 = NULL;
	pkgmgr_pkginfo_x *info = NULL;
	pkgmgr_appinfo_x *appinfo = NULL;
	pkgmgrinfo_filter_x *filter = (pkgmgrinfo_filter_x*)handle;

	sqlite3 *pkginfo_db = NULL;
	sqlite3_stmt *stmt = NULL;
	GSList *list;


	/*open db*/
	ret = db_util_open(MANIFEST_DB, &pkginfo_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "db_util_open[%s] failed!", MANIFEST_DB);

	/*calloc*/
	info = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(info->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	/*Start constructing query*/
	locale = __convert_system_locale_to_manifest_locale();
	snprintf(query, MAX_QUERY_LEN - 1, FILTER_QUERY_COUNT_APP, locale);

	/*Get where clause*/
	for (list = filter->list; list; list = g_slist_next(list)) {
		__get_filter_condition(list->data, &condition);
		if (condition) {
			strncat(where, condition, sizeof(where) - strlen(where) -1);
			where[sizeof(where) - 1] = '\0';
			FREE_AND_NULL(condition);
		}
		if (g_slist_next(list)) {
			strncat(where, " and ", sizeof(where) - strlen(where) - 1);
			where[sizeof(where) - 1] = '\0';
		}
	}

	if (strstr(where, "package_app_info.app_disable") == NULL) {
		if (strlen(where) > 0) {
			strncat(where, " and package_app_info.app_disable IN ('false','False')", sizeof(where) - strlen(where) - 1);
			where[sizeof(where) - 1] = '\0';
		}
	}
	_LOGS("where = %s\n", where);

	if (strlen(where) > 0) {
		strncat(query, where, sizeof(query) - strlen(query) - 1);
		query[sizeof(query) - 1] = '\0';
	}
	_LOGS("query = %s\n", query);

	/*prepare query*/
	ret = sqlite3_prepare_v2(pkginfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	/*step query*/
	while(1) {
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {
			__get_appinfo_for_list(stmt, info);
		} else {
			break;
		}
	}

	/*head up*/
	SAFE_LISTHEAD(info->manifest_info->uiapplication, ptr1);

	/*If the callback func return < 0 we break and no more call back is called*/
	while(ptr1 != NULL)
	{
		appinfo->uiapp_info = ptr1;
		ret = __appinfo_check_installed_storage(appinfo);
		if(ret < 0) {
			ptr1 = ptr1->next;
			continue;
		}
		ptr1 = ptr1->next;
		filter_count++;
	}

	*count = filter_count;

	ret = PMINFO_R_OK;
catch:
	FREE_AND_NULL(locale);
	sqlite3_finalize(stmt);
	sqlite3_close(pkginfo_db);
	FREE_AND_NULL(appinfo);
	__cleanup_pkginfo(info);
	return ret;
}

API int pkgmgrinfo_appinfo_filter_foreach_appinfo(pkgmgrinfo_appinfo_filter_h handle,
				pkgmgrinfo_app_list_cb app_cb, void * user_data)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	retvm_if(app_cb == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");

	int ret = 0;

	char *locale = NULL;
	char *condition = NULL;
	char query[MAX_QUERY_LEN] = {'\0'};
	char where[MAX_QUERY_LEN] = {'\0'};

	char appid[MAX_QUERY_LEN] = {0,};
	char pre_appid[MAX_QUERY_LEN] = {0,};

	pkgmgr_pkginfo_x *info = NULL;
	pkgmgr_appinfo_x *appinfo = NULL;
	pkgmgrinfo_filter_x *filter = (pkgmgrinfo_filter_x*)handle;
	uiapplication_x *ptr1 = NULL;

	sqlite3 *pkginfo_db = NULL;
	sqlite3_stmt *stmt = NULL;
	GSList *list;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &pkginfo_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*calloc*/
	info = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(info->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	/*Start constructing query*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "locale is NULL\n");
	snprintf(query, MAX_QUERY_LEN - 1, FILTER_QUERY_LIST_APP, DEFAULT_LOCALE, locale);

	/*Get where clause*/
	for (list = filter->list; list; list = g_slist_next(list)) {
		__get_filter_condition(list->data, &condition);
		if (condition) {
			strncat(where, condition, sizeof(where) - strlen(where) -1);
			where[sizeof(where) - 1] = '\0';
			FREE_AND_NULL(condition);
		}
		if (g_slist_next(list)) {
			strncat(where, " and ", sizeof(where) - strlen(where) - 1);
			where[sizeof(where) - 1] = '\0';
		}
	}

	if (strstr(where, "package_app_info.app_disable") == NULL) {
		if (strlen(where) > 0) {
			strncat(where, " and package_app_info.app_disable IN ('false','False')", sizeof(where) - strlen(where) - 1);
			where[sizeof(where) - 1] = '\0';
		}
	}

	_LOGS("where = %s\n", where);

	if (strlen(where) > 0) {
		strncat(query, where, sizeof(query) - strlen(query) - 1);
		query[sizeof(query) - 1] = '\0';
	}
	_LOGS("query = %s\n", query);

	/*prepare query*/
	ret = sqlite3_prepare_v2(pkginfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	/*step query*/
	while(1) {
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {

			memset(appid, 0, MAX_QUERY_LEN);
			strncpy(appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

			if (strlen(pre_appid) != 0) {
				if (strcmp(pre_appid, appid) == 0) {
					/*if same appid is found, then it is about exact matched locale*/
					__update_localed_label_for_list(stmt, info);

					memset(pre_appid, 0, MAX_QUERY_LEN);
					strncpy(pre_appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

					continue;
				} else {
					memset(pre_appid, 0, MAX_QUERY_LEN);
					strncpy(pre_appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
				}
			} else {
				strncpy(pre_appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
			}

			__get_appinfo_for_list(stmt, info);
		} else {
			break;
		}
	}

	/*head up*/
	SAFE_LISTHEAD(info->manifest_info->uiapplication, ptr1);

	/*If the callback func return < 0 we break and no more call back is called*/
	while(ptr1 != NULL)
	{
		appinfo->locale = strdup(locale);
		appinfo->uiapp_info = ptr1;

		ret = __appinfo_check_installed_storage(appinfo);
		if(ret < 0) {
			FREE_AND_NULL(appinfo->locale);
			ptr1 = ptr1->next;
			continue;
		}

		ret = app_cb((void *)appinfo, user_data);
		if (ret < 0){
			FREE_AND_NULL(appinfo->locale);
			break;
		}

		FREE_AND_NULL(appinfo->locale);
		ptr1 = ptr1->next;
	}
	ret = PMINFO_R_OK;

catch:
	FREE_AND_NULL(locale);
	sqlite3_finalize(stmt);
	sqlite3_close(pkginfo_db);
	FREE_AND_NULL(appinfo);
	__cleanup_pkginfo(info);
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
		FREE_AND_NULL(node->key);
		FREE_AND_NULL(node->value);
		FREE_AND_NULL(node);
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
	char query[MAX_QUERY_LEN] = {'\0'};
	char where[MAX_QUERY_LEN] = {'\0'};
	char appid[MAX_QUERY_LEN] = {0,};
	char pre_appid[MAX_QUERY_LEN] = {0,};
	GSList *list;
	int ret = 0;
	pkgmgr_pkginfo_x *info = NULL;
	pkgmgr_appinfo_x *appinfo = NULL;
	uiapplication_x *ptr1 = NULL;
	pkgmgrinfo_filter_x *filter = (pkgmgrinfo_filter_x*)handle;
	sqlite3 *pkginfo_db = NULL;
	sqlite3_stmt *stmt = NULL;

	/*open db*/
	ret = db_util_open(MANIFEST_DB, &pkginfo_db, 0);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*calloc*/
	info = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(info->manifest_info == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

	appinfo = (pkgmgr_appinfo_x *)calloc(1, sizeof(pkgmgr_appinfo_x));
	tryvm_if(appinfo == NULL, ret = PMINFO_R_ERROR, "Out of Memory!!!\n");

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
			FREE_AND_NULL(condition);
		}
		if (g_slist_next(list)) {
			strncat(where, METADATA_FILTER_QUERY_UNION_CLAUSE, sizeof(where) - strlen(where) - 1);
		}
	}

	if (strstr(where, "package_app_info.app_disable") == NULL) {
		if (strlen(where) > 0) {
			strncat(where, " and package_app_info.app_disable IN ('false','False')", sizeof(where) - strlen(where) - 1);
			where[sizeof(where) - 1] = '\0';
		}
	}

	_LOGE("where = %s (%d)\n", where, strlen(where));

	if (strlen(where) > 0) {
		strncat(query, where, sizeof(query) - strlen(query) - 1);
	}
	_LOGE("query = %s (%d)\n", query, strlen(query));

	/*prepare query*/
	ret = sqlite3_prepare_v2(pkginfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	/*step query*/
	while(1) {
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {

			memset(appid, 0, MAX_QUERY_LEN);
			strncpy(appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

			if (strlen(pre_appid) != 0) {
				if (strcmp(pre_appid, appid) == 0) {
					/*if same appid is found, then it is about exact matched locale*/
					__update_localed_label_for_list(stmt, info);

					memset(pre_appid, 0, MAX_QUERY_LEN);
					strncpy(pre_appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

					continue;
				} else {
					memset(pre_appid, 0, MAX_QUERY_LEN);
					strncpy(pre_appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
				}
			} else {
				strncpy(pre_appid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
			}

			__get_appinfo_for_list(stmt, info);
		} else {
			break;
		}
	}

	/*head up*/
	SAFE_LISTHEAD(info->manifest_info->uiapplication, ptr1);

	/*UI Apps*/
	while (ptr1 != NULL) {
		appinfo->locale = strdup(locale);
		appinfo->uiapp_info = ptr1;

		ret = __appinfo_check_installed_storage(appinfo);
		if(ret < 0) {
			FREE_AND_NULL(appinfo->locale);
			ptr1 = ptr1->next;
			continue;
		}

		ret = app_cb((void *)appinfo, user_data);
		if (ret < 0) {
			FREE_AND_NULL(appinfo->locale);
			break;
		}

		FREE_AND_NULL(appinfo->locale);
		ptr1 = ptr1->next;
	}

	ret = PMINFO_R_OK;
catch:
	FREE_AND_NULL(locale);
	sqlite3_finalize(stmt);
	sqlite3_close(pkginfo_db);
	FREE_AND_NULL(appinfo);
	__cleanup_pkginfo(info);
	return ret;
}
