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

#include "pkgmgrinfo_private.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "PKGMGR_INFO"

#define FILTER_QUERY_LIST_PACKAGE	"select * from package_info LEFT OUTER JOIN package_localized_info " \
				"ON package_info.package=package_localized_info.package " \
				"and package_localized_info.package_locale IN ('%s', '%s') where "


typedef struct _pkgmgr_cert_x {
	char *pkgid;
	int cert_id;
} pkgmgr_cert_x;

extern int strverscmp (const char *, const char *);

static void __destroy_each_node(gpointer data, gpointer user_data)
{
	ret_if(data == NULL);
	pkgmgrinfo_node_x *node = (pkgmgrinfo_node_x*)data;
	FREE_AND_NULL(node->value);
	FREE_AND_NULL(node->key);
	FREE_AND_NULL(node);
}

static void __get_pkginfo_from_db(char *colname, char *coltxt, manifest_x *manifest_info)
{
	GList *tmp = NULL;
	label_x* lbl = NULL;
	author_x* author = NULL;
	description_x* description = NULL;
	icon_x* icon = NULL;

	if((tmp = g_list_last(manifest_info->label))) {
		lbl = (label_x*) tmp->data;
		if (lbl == NULL) {
			_LOGE("lbl is NULL.");
			return;
		}
	}

	if((tmp = g_list_last(manifest_info->author))) {
		author = (author_x*)tmp->data;
		if (author == NULL) {
			_LOGE("author is NULL.");
			return;
		}
	}

	if((tmp = g_list_last(manifest_info->description))) {
		description = (description_x*)tmp->data;
		if (description == NULL) {
			_LOGE("description is NULL.");
			return;
		}
	}

	if((tmp = g_list_last(manifest_info->icon))) {
		icon = (icon_x*)tmp->data;
		if (icon == NULL) {
			_LOGE("icon is NULL.");
			return;
		}
	}

	if (strcmp(colname, "package") == 0) {
		if (manifest_info->package)
			return;
		manifest_info->package = strdup(coltxt);

	} else if (strcmp(colname, "package_type") == 0) {
		manifest_info->type = strdup(coltxt);

	} else if (strcmp(colname, "package_version") == 0) {
		manifest_info->version = strdup(coltxt);

	} else if (strcmp(colname, "package_api_version") == 0) {
		manifest_info->api_version = strdup(coltxt);

	} else if (strcmp(colname, "install_location") == 0) {
		manifest_info->installlocation = strdup(coltxt);

	} else if (strcmp(colname, "package_size") == 0) {
		manifest_info->package_size = strdup(coltxt);

	} else if (strcmp(colname, "package_removable") == 0 ){
		manifest_info->removable = strdup(coltxt);

	} else if (strcmp(colname, "package_preload") == 0 ){
		manifest_info->preload = strdup(coltxt);

	} else if (strcmp(colname, "package_readonly") == 0 ){
		manifest_info->readonly = strdup(coltxt);

	} else if (strcmp(colname, "package_update") == 0 ){
		manifest_info->update= strdup(coltxt);

	} else if (strcmp(colname, "package_system") == 0 ){
		manifest_info->system= strdup(coltxt);

	} else if (strcmp(colname, "package_appsetting") == 0 ){
		manifest_info->appsetting = strdup(coltxt);

	} else if (strcmp(colname, "installed_time") == 0 ){
		manifest_info->installed_time = strdup(coltxt);

	} else if (strcmp(colname, "installed_storage") == 0 ){
		manifest_info->installed_storage = strdup(coltxt);

	} else if (strcmp(colname, "mainapp_id") == 0 ){
		manifest_info->mainapp_id = strdup(coltxt);

	} else if (strcmp(colname, "storeclient_id") == 0 ){
		manifest_info->storeclient_id = strdup(coltxt);

	} else if (strcmp(colname, "package_url") == 0 ){
		manifest_info->package_url = strdup(coltxt);

	} else if (strcmp(colname, "root_path") == 0 ){
		manifest_info->root_path = strdup(coltxt);

	} else if (strcmp(colname, "csc_path") == 0 ){
		manifest_info->csc_path = strdup(coltxt);

	} else if (strcmp(colname, "package_support_disable") == 0 ){
		manifest_info->support_disable = strdup(coltxt);

	} else if (strcmp(colname, "package_mother_package") == 0 ){
		manifest_info->mother_package = strdup(coltxt);

	} else if (strcmp(colname, "package_support_mode") == 0 ){
		manifest_info->support_mode = strdup(coltxt);

	} else if (strcmp(colname, "package_backend_installer") == 0 ){
		manifest_info->backend_installer = strdup(coltxt);

	} else if (strcmp(colname, "package_custom_smack_label") == 0 ){
		manifest_info->custom_smack_label = strdup(coltxt);

	} else if (strcmp(colname, "package_reserve3") == 0 ){
		manifest_info->groupid = strdup(coltxt);
	/*end package_info table*/

	} else if (strcmp(colname, "author_email") == 0 ){
		if (author)
			author->email = strdup(coltxt);

	} else if (strcmp(colname, "author_href") == 0 ){
		if (author)
			author->href = strdup(coltxt);

	} else if (strcmp(colname, "package_label") == 0 ){
		if (lbl)
			lbl->text = strdup(coltxt);

	} else if (strcmp(colname, "package_icon") == 0 ){
		if (icon)
			icon->text = strdup(coltxt);

	} else if (strcmp(colname, "package_description") == 0 ){
		if (description)
			description->text = strdup(coltxt);

	} else if (strcmp(colname, "package_author") == 0 ){
		if (author)
			author->text = strdup(coltxt);

	} else if (strcmp(colname, "package_locale") == 0 ){
		if (author)
			author->lang = strdup(coltxt);
		if (icon)
			icon->lang = strdup(coltxt);
		if (lbl)
			lbl->lang = strdup(coltxt);
		if (description)
			description->lang = strdup(coltxt);
		/*end package_localized_info table*/

	} else if (strcmp(colname, "privilege") == 0 ){
		manifest_info->privileges = g_list_append(manifest_info->privileges,strdup(coltxt));

#ifdef _APPFW_FEATURE_EXPANSION_PKG_INSTALL
	}  else if (strcmp(colname, "package_tep_name") == 0 ) {
		manifest_info->tep_name = strdup(coltxt);
#endif

	}
	/*end package_privilege_info table*/

}

static void __get_pkginfo_for_list(sqlite3_stmt *stmt, GList **udata)
{
	int i = 0;
	int ncols = 0;
	char *colname = NULL;
	char *coltxt = NULL;
	pkgmgr_pkginfo_x *info = NULL;
	icon_x* icon = NULL;
	label_x *label = NULL;
	author_x* author = NULL;
	description_x *description = NULL;

	info = calloc(1, sizeof(pkgmgr_pkginfo_x));
	if (!info) {
		_LOGE("malloc failed");
		goto catch;
	}

	info->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	if (!info->manifest_info) {
		_LOGE("malloc failed");
		goto catch;
	}

	ncols = sqlite3_column_count(stmt);
	*udata = g_list_append(*udata, info);

	for(i = 0; i < ncols; i++)
	{
		colname = (char *)sqlite3_column_name(stmt, i);
		coltxt = (char *)sqlite3_column_text(stmt, i);

		if(!coltxt || !colname)
			continue;

		if((strcmp(colname, "package_label") == 0 || strcmp(colname, "package_locale") == 0 ) && label == NULL) {
			label = calloc(1, sizeof(label_x));
			trym_if(label == NULL, "Out of memory: label");
			info->manifest_info->label = g_list_append(info->manifest_info->label, label);
		}

		if((strcmp(colname, "package_icon") == 0 || strcmp(colname, "package_locale") == 0 ) && icon == NULL) {
			icon = calloc(1, sizeof(icon_x));
			trym_if(icon == NULL, "Out of memory: icon");
			info->manifest_info->icon = g_list_append(info->manifest_info->icon, icon);
		}

		if((strcmp(colname, "package_description") == 0 || strcmp(colname, "package_locale") == 0 ) && description == NULL) {
			description = calloc(1, sizeof(description_x));
			trym_if(description == NULL, "Out of memory: description");
			info->manifest_info->description = g_list_append(info->manifest_info->description, description);
		}

		if((strcmp(colname, "package_author") == 0 || strcmp(colname, "author_email") == 0 || strcmp(colname, "author_href") == 0
			|| strcmp(colname, "package_locale") == 0 ) && author == NULL) {
			author = calloc(1, sizeof(author_x));
			trym_if(author == NULL, "Out of memory: author");
			info->manifest_info->author = g_list_append(info->manifest_info->author, author);
		}

//		_LOGE("field value  :: %s = %s \n", colname, coltxt);
		__get_pkginfo_from_db(colname, coltxt, info->manifest_info);
	}

	return;

catch:
	//for exceptional cases
	if (info)
		FREE_AND_NULL(info->manifest_info);
	FREE_AND_NULL(info);
	FREE_AND_NULL(icon);
	FREE_AND_NULL(label);
	FREE_AND_NULL(author);
	FREE_AND_NULL(description);
	return;
}

static void __update_localed_label_for_list(sqlite3_stmt *stmt, pkgmgr_pkginfo_x *pkginfo)
{
	int i = 0;
	int ncols = 0;
	char *colname = NULL;
	char *coltxt = NULL;

	label_x *label = NULL;

	ncols = sqlite3_column_count(stmt);

	for(i = 0; i < ncols; i++)
	{
		colname = (char *)sqlite3_column_name(stmt, i);
		if (strcmp(colname, "package_label") == 0 ){
			coltxt = (char *)sqlite3_column_text(stmt, i);
			label = (label_x*)pkginfo->manifest_info->label->data;
			FREE_AND_STRDUP(coltxt, label->text);
		}
	}
}

int __pkginfo_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)data;
	int i = 0;
	author_x *author = NULL;
	icon_x *icon = NULL;
	label_x *label = NULL;
	description_x *description = NULL;

	for(i = 0; i < ncols; i++)
	{
		if(!coltxt[i] || !colname[i])
			continue;

		if((strcmp(colname[i], "package_label") == 0 || strcmp(colname[i], "package_locale") == 0 ) && label == NULL) {
			label = calloc(1, sizeof(label_x));
			retvm_if(label == NULL, PMINFO_R_ERROR, "Out of memory: label");
			info->manifest_info->label = g_list_append(info->manifest_info->label, label);
		}

		if((strcmp(colname[i], "package_icon") == 0 || strcmp(colname[i], "package_locale") == 0 ) && icon == NULL) {
			icon = calloc(1, sizeof(icon_x));
			retvm_if(icon == NULL, PMINFO_R_ERROR, "Out of memory: icon");
			info->manifest_info->icon = g_list_append(info->manifest_info->icon, icon);
		}

		if((strcmp(colname[i], "package_description") == 0 || strcmp(colname[i], "package_locale") == 0 ) && description == NULL) {
			description = calloc(1, sizeof(description_x));
			retvm_if(description == NULL, PMINFO_R_ERROR, "Out of memory: description");
			info->manifest_info->description = g_list_append(info->manifest_info->description, description);
		}

		if((strcmp(colname[i], "package_author") == 0 || strcmp(colname[i], "author_email") == 0 || strcmp(colname[i], "author_href") == 0
			|| strcmp(colname[i], "package_locale") == 0) && author == NULL) {
			author = calloc(1, sizeof(author_x));
			retvm_if(author == NULL, PMINFO_R_ERROR, "Out of memory: author");
			info->manifest_info->author = g_list_append(info->manifest_info->author, author);
		}

		//_LOGE("field value  :: %s = %s \n", colname[i], coltxt[i]);
		__get_pkginfo_from_db(colname[i], coltxt[i], info->manifest_info);
	}

	return 0;
}

static int __cert_cb(void *data, int ncols, char **coltxt, char **colname)
{
	pkgmgr_cert_x *info = (pkgmgr_cert_x *)data;
	int i = 0;

	for(i = 0; i < ncols; i++)
	{
		if (strcmp(colname[i], "author_signer_cert") == 0) {
			if (coltxt[i])
				info->cert_id = atoi(coltxt[i]);
			else
				info->cert_id = 0;
		} else if (strcmp(colname[i], "package") == 0) {
			FREE_AND_NULL(info->pkgid);
			if (coltxt[i])
				info->pkgid= strdup(coltxt[i]);
		} else
			continue;
	}
	return 0;
}

API int pkgmgrinfo_pkginfo_get_list(pkgmgrinfo_pkg_list_cb pkg_list_cb, void *user_data)
{
	retvm_if(pkg_list_cb == NULL, PMINFO_R_EINVAL, "callback function is NULL\n");

	int ret = PMINFO_R_OK;
	char *locale = NULL;
	char *query = NULL;

	char pkgid[MAX_QUERY_LEN] = {0,};
	char pre_pkgid[MAX_QUERY_LEN] = {0,};

	sqlite3 *pkginfo_db = NULL;
	sqlite3_stmt *stmt = NULL;

	pkgmgr_pkginfo_x *pkginfo = NULL;
	GList *list_pkginfo = NULL;
	GList *node = NULL;

	/*open db*/
	ret = _pminfo_db_open(MANIFEST_DB, &pkginfo_db);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "_pminfo_db_open[%s] failed!", MANIFEST_DB);

	locale = __convert_system_locale_to_manifest_locale();
	retvm_if(locale == NULL, PMINFO_R_ERROR, "convert system locale to manifest locale fail");

	query = sqlite3_mprintf("select * from package_info LEFT OUTER JOIN package_localized_info " \
								"ON package_info.package=package_localized_info.package "\
								"where package_info.package_disable='false' and package_localized_info.package_locale IN (%Q, %Q)", DEFAULT_LOCALE, locale);

	/*prepare query*/
	ret = sqlite3_prepare_v2(pkginfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	/*step query*/
	while(1) {
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {

			memset(pkgid, 0, MAX_QUERY_LEN);
			strncpy(pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

			if (strlen(pre_pkgid) != 0) {
				if (strcmp(pre_pkgid, pkgid) == 0) {
					node = g_list_last(list_pkginfo);
					if (node)
						__update_localed_label_for_list(stmt, (pkgmgr_pkginfo_x *)node->data);

					memset(pre_pkgid, 0, MAX_QUERY_LEN);
					strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

					continue;
				} else {
					memset(pre_pkgid, 0, MAX_QUERY_LEN);
					strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
				}
			} else {
				strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
			}
			__get_pkginfo_for_list(stmt, &list_pkginfo);
		} else {
			break;
		}
	}

	for(node = list_pkginfo; node; node = node->next) {
		pkginfo = (pkgmgr_pkginfo_x *)node->data;
		pkginfo->locale = strdup(locale);

		ret = __pkginfo_check_installed_storage(pkginfo);
		if(ret < 0)
			continue;

		ret = pkg_list_cb( (void *)pkginfo, user_data);
		if(ret < 0) {
			_LOGE("callback is stopped.");
			break;
		}
	}

	ret = PMINFO_R_OK;

catch:
	sqlite3_free(query);
	sqlite3_finalize(stmt);
	sqlite3_close(pkginfo_db);
	FREE_AND_NULL(locale);

	__cleanup_list_pkginfo(&list_pkginfo);
	node = NULL;

	return ret;
}

API int pkgmgrinfo_pkginfo_get_disabled_list(pkgmgrinfo_pkg_list_cb pkg_list_cb, void *user_data)
{
	retvm_if(pkg_list_cb == NULL, PMINFO_R_EINVAL, "callback function is NULL\n");

	int ret = PMINFO_R_OK;
	char *locale = NULL;
	char *query = NULL;

	char pkgid[MAX_QUERY_LEN] = {0,};
	char pre_pkgid[MAX_QUERY_LEN] = {0,};

	sqlite3 *pkginfo_db = NULL;
	sqlite3_stmt *stmt = NULL;

	pkgmgr_pkginfo_x *pkginfo = NULL;
	GList *node = NULL;
	GList *list_pkginfo = NULL;

	/*open db*/
	ret = _pminfo_db_open(MANIFEST_DB, &pkginfo_db);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "_pminfo_db_open[%s] failed!", MANIFEST_DB);

	locale = __convert_system_locale_to_manifest_locale();
	retvm_if(locale == NULL, PMINFO_R_ERROR, "convert system locale to manifest locale fail");

	/*Start constructing query*/
	query = sqlite3_mprintf("select * from package_info LEFT OUTER JOIN package_localized_info " \
								"ON package_info.package=package_localized_info.package " \
								"where package_info.package_disable='true' and package_localized_info.package_locale IN (%Q, %Q)", DEFAULT_LOCALE, locale);

	/*prepare query*/
	ret = sqlite3_prepare_v2(pkginfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	/*step query*/
	while(1) {
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {

			memset(pkgid, 0, MAX_QUERY_LEN);
			strncpy(pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

			if (strlen(pre_pkgid) != 0) {
				if (strcmp(pre_pkgid, pkgid) == 0) {
					node = g_list_last(list_pkginfo);
					if (node)
						__update_localed_label_for_list(stmt, (pkgmgr_pkginfo_x *)node->data);

					memset(pre_pkgid, 0, MAX_QUERY_LEN);
					strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

					continue;
				} else {
					memset(pre_pkgid, 0, MAX_QUERY_LEN);
					strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
				}
			} else {
				strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
			}

			__get_pkginfo_for_list(stmt, &list_pkginfo);

		} else {
			break;
		}
	}

	for(node = list_pkginfo; node ; node = node->next) {
		pkginfo = (pkgmgr_pkginfo_x *)node->data;
		pkginfo->locale = strdup(locale);

		ret = __pkginfo_check_installed_storage(pkginfo);
		if(ret < 0)
			continue;

		ret = pkg_list_cb( (void *)pkginfo, user_data);
		if(ret < 0) {
			_LOGE("callback is stopped.");
			break;
		}
	}

	ret = PMINFO_R_OK;

catch:
	sqlite3_free(query);
	sqlite3_finalize(stmt);
	sqlite3_close(pkginfo_db);
	FREE_AND_NULL(locale);

	__cleanup_list_pkginfo(&list_pkginfo);
	node = NULL;

	return ret;
}

API int pkgmgrinfo_pkginfo_get_mounted_list(pkgmgrinfo_pkg_list_cb pkg_list_cb, void *user_data)
{
	retvm_if(pkg_list_cb == NULL, PMINFO_R_EINVAL, "callback function is NULL\n");

	int ret = PMINFO_R_OK;
	char *locale = NULL;
	char *query = NULL;

	char pkgid[MAX_QUERY_LEN] = {0,};
	char pre_pkgid[MAX_QUERY_LEN] = {0,};

	sqlite3 *pkginfo_db = NULL;
	sqlite3_stmt *stmt = NULL;

	GList *node = NULL;
	pkgmgr_pkginfo_x *pkginfo = NULL;
	GList *list_pkginfo = NULL;

	/*open db*/
	ret = _pminfo_db_open(MANIFEST_DB, &pkginfo_db);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "_pminfo_db_open[%s] failed!", MANIFEST_DB);

	locale = __convert_system_locale_to_manifest_locale();
	retvm_if(locale == NULL, PMINFO_R_ERROR, "convert system locale to manifest locale fail");

	query = sqlite3_mprintf("select * from package_info LEFT OUTER JOIN package_localized_info " \
								"ON package_info.package=package_localized_info.package "\
								"where installed_storage='installed_external' and package_info.package_disable='false' and package_localized_info.package_locale IN (%Q, %Q)", DEFAULT_LOCALE, locale);

	/*prepare query*/
	ret = sqlite3_prepare_v2(pkginfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	/*step query*/
	while(1) {
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {

			memset(pkgid, 0, MAX_QUERY_LEN);
			strncpy(pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

			if (strlen(pre_pkgid) != 0) {
				if (strcmp(pre_pkgid, pkgid) == 0) {
					node = g_list_last(list_pkginfo);
					if (node)
						__update_localed_label_for_list(stmt, (pkgmgr_pkginfo_x *)node->data);

					memset(pre_pkgid, 0, MAX_QUERY_LEN);
					strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
					continue;
				} else {
					memset(pre_pkgid, 0, MAX_QUERY_LEN);
					strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
				}
			} else {
				strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
			}

			__get_pkginfo_for_list(stmt, &list_pkginfo);

		} else {
			break;
		}
	}

	for(node = list_pkginfo; node ; node = node->next) {
		pkginfo = (pkgmgr_pkginfo_x *)node->data;
		pkginfo->locale = strdup(locale);

		ret = __pkginfo_check_installed_storage(pkginfo);
		if(ret < 0)
			continue;

		ret = pkg_list_cb( (void *)pkginfo, user_data);
		if(ret < 0) {
			_LOGE("callback is stopped.");
			break;
		}
	}
	ret = PMINFO_R_OK;

catch:
	sqlite3_free(query);
	sqlite3_finalize(stmt);
	sqlite3_close(pkginfo_db);
	FREE_AND_NULL(locale);

	__cleanup_list_pkginfo(&list_pkginfo);
	node = NULL;

	return ret;
}


API int pkgmgrinfo_pkginfo_get_unmounted_list(pkgmgrinfo_pkg_list_cb pkg_list_cb, void *user_data)
{
	retvm_if(pkg_list_cb == NULL, PMINFO_R_EINVAL, "callback function is NULL\n");

	int ret = PMINFO_R_OK;
	char *locale = NULL;
	char *query = NULL;

	char pkgid[MAX_QUERY_LEN] = {0,};
	char pre_pkgid[MAX_QUERY_LEN] = {0,};

	sqlite3 *pkginfo_db = NULL;
	sqlite3_stmt *stmt = NULL;

	GList *node = NULL;
	pkgmgr_pkginfo_x *pkginfo = NULL;
	GList *list_pkginfo = NULL;

	/*open db*/
	ret = _pminfo_db_open(MANIFEST_DB, &pkginfo_db);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "_pminfo_db_open[%s] failed!", MANIFEST_DB);

	locale = __convert_system_locale_to_manifest_locale();
	retvm_if(locale == NULL, PMINFO_R_ERROR, "convert system locale to manifest locale fail");

	query = sqlite3_mprintf("select * from package_info LEFT OUTER JOIN package_localized_info " \
								"ON package_info.package=package_localized_info.package "\
								"where installed_storage='installed_external' and package_info.package_disable='false' and package_localized_info.package_locale IN (%Q, %Q)", DEFAULT_LOCALE, locale);

	/*prepare query*/
	ret = sqlite3_prepare_v2(pkginfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	/*step query*/
	while(1) {
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {

			memset(pkgid, 0, MAX_QUERY_LEN);
			strncpy(pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

			if (strlen(pre_pkgid) != 0) {
				if (strcmp(pre_pkgid, pkgid) == 0) {
					node = g_list_last(list_pkginfo);
					if (node)
						__update_localed_label_for_list(stmt, (pkgmgr_pkginfo_x *)node->data);

					memset(pre_pkgid, 0, MAX_QUERY_LEN);
					strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

					continue;
				} else {
					memset(pre_pkgid, 0, MAX_QUERY_LEN);
					strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
				}
			} else {
				strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
			}

			__get_pkginfo_for_list(stmt, &list_pkginfo);

		} else {
			break;
		}
	}

	for(node = list_pkginfo; node ; node = node->next) {
		pkginfo = (pkgmgr_pkginfo_x *)node->data;
		pkginfo->locale = strdup(locale);

		ret = pkg_list_cb( (void *)pkginfo, user_data);
		if(ret < 0) {
			_LOGE("callback is stopped.");
			break;
		}
	}
	ret = PMINFO_R_OK;

catch:
	sqlite3_free(query);
	sqlite3_finalize(stmt);
	sqlite3_close(pkginfo_db);
	FREE_AND_NULL(locale);

	__cleanup_list_pkginfo(&list_pkginfo);
	node = NULL;

	return ret;
}

API int pkgmgrinfo_pkginfo_get_unmounted_pkginfo(const char *pkgid, pkgmgrinfo_pkginfo_h *handle)
{
	retvm_if(pkgid == NULL, PMINFO_R_EINVAL, "pkgid is NULL\n");
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *pkginfo = NULL;
	int ret = PMINFO_R_OK;
	char *query = NULL;
	char *locale = NULL;
	int exist = 0;
	sqlite3 *pkginfo_db = NULL;

	/*validate pkgid*/
	ret = _pminfo_db_open(MANIFEST_DB, &pkginfo_db);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*check pkgid exist on db*/
	query= sqlite3_mprintf("select exists(select * from package_info where package=%Q and package_disable='false')", pkgid);
	ret = __exec_db_query(pkginfo_db, query, _pkgmgrinfo_validate_cb, (void *)&exist);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "sqlite3_exec[%s] fail", pkgid);
	tryvm_if(exist == 0, ret = PMINFO_R_ERROR, "pkgid[%s] not found in DB", pkgid);

	/*get system locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "manifest locale is NULL");

	pkginfo = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(pkginfo == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for pkginfo");

	pkginfo->locale = strdup(locale);

	pkginfo->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(pkginfo->manifest_info == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for manifest info");

	pkginfo->manifest_info->package = strdup(pkgid);

	/*populate manifest_info from DB*/
	query= sqlite3_mprintf("select * from package_info where package=%Q and package_disable='false'", pkgid);
	ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");

	/*populate privilege_info from DB*/
	query= sqlite3_mprintf("select * from package_privilege_info where package=%Q ", pkgid);
	ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Privilege Info DB Information retrieval failed");

	query= sqlite3_mprintf("select * from package_localized_info where package=%Q and package_locale=%Q", pkgid, locale);
	ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");

	/*Also store the values corresponding to default locales*/
	query= sqlite3_mprintf("select * from package_localized_info where package=%Q and package_locale=%Q", pkgid, DEFAULT_LOCALE);
	ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");


catch:
	if (ret == PMINFO_R_OK)
		*handle = (void*)pkginfo;
	else {
		*handle = NULL;
		__cleanup_pkginfo(pkginfo);
	}
	sqlite3_close(pkginfo_db);

	FREE_AND_NULL(locale);
	return ret;
}

API int pkgmgrinfo_pkginfo_get_pkginfo(const char *pkgid, pkgmgrinfo_pkginfo_h *handle)
{
	retvm_if(pkgid == NULL, PMINFO_R_EINVAL, "pkgid is NULL\n");
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *pkginfo = NULL;
	int ret = PMINFO_R_OK;
	char *query = NULL;
	char *locale = NULL;
	int exist = 0;
	sqlite3 *pkginfo_db = NULL;

	/*validate pkgid*/
	ret = _pminfo_db_open(MANIFEST_DB, &pkginfo_db);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*check pkgid exist on db*/
	query= sqlite3_mprintf("select exists(select * from package_info where package=%Q and package_disable='false')", pkgid);
	ret = __exec_db_query(pkginfo_db, query, _pkgmgrinfo_validate_cb, (void *)&exist);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "sqlite3_exec[%s] fail", pkgid);
	if (exist == 0) {
		_LOGS("pkgid[%s] not found in DB", pkgid);
		ret = PMINFO_R_ERROR;
		goto catch;
	}

	/*get system locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "manifest locale is NULL");

	pkginfo = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(pkginfo == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for pkginfo");

	pkginfo->locale = strdup(locale);

	pkginfo->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(pkginfo->manifest_info == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for manifest info");

	pkginfo->manifest_info->package = strdup(pkgid);

	/*populate manifest_info from DB*/
	query= sqlite3_mprintf("select * from package_info where package=%Q and package_disable='false'", pkgid);
	ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");

	/*populate privilege_info from DB*/
	query= sqlite3_mprintf("select * from package_privilege_info where package=%Q ", pkgid);
	ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Privilege Info DB Information retrieval failed");

	query= sqlite3_mprintf("select * from package_localized_info where package=%Q and package_locale=%Q", pkgid, locale);
	ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");

	/*Also store the values corresponding to default locales*/
	query= sqlite3_mprintf("select * from package_localized_info where package=%Q and package_locale=%Q", pkgid, DEFAULT_LOCALE);
	ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");


	ret = __pkginfo_check_installed_storage(pkginfo);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "[%s] is installed external, but is not in mmc", pkgid);

catch:
	if (ret == PMINFO_R_OK)
		*handle = (void*)pkginfo;
	else {
		*handle = NULL;
		__cleanup_pkginfo(pkginfo);
	}
	sqlite3_close(pkginfo_db);

	FREE_AND_NULL(locale);
	return ret;
}

API int pkgmgrinfo_pkginfo_get_disabled_pkginfo(const char *pkgid, pkgmgrinfo_pkginfo_h *handle)
{
	retvm_if(pkgid == NULL, PMINFO_R_EINVAL, "pkgid is NULL\n");
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *pkginfo = NULL;
	int ret = PMINFO_R_OK;
	char *query = NULL;
	char *locale = NULL;
	int exist = 0;
	sqlite3 *pkginfo_db = NULL;

	/*validate pkgid*/
	ret = _pminfo_db_open(MANIFEST_DB, &pkginfo_db);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	/*check pkgid exist on db*/
	query= sqlite3_mprintf("select exists(select * from package_info where package=%Q and package_disable='true')", pkgid);
	ret = __exec_db_query(pkginfo_db, query, _pkgmgrinfo_validate_cb, (void *)&exist);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "sqlite3_exec[%s] fail", pkgid);
	if (exist == 0) {
		_LOGS("pkgid[%s] not found in DB", pkgid);
		ret = PMINFO_R_ERROR;
		goto catch;
	}

	/*get system locale*/
	locale = __convert_system_locale_to_manifest_locale();
	tryvm_if(locale == NULL, ret = PMINFO_R_ERROR, "manifest locale is NULL");

	pkginfo = (pkgmgr_pkginfo_x *)calloc(1, sizeof(pkgmgr_pkginfo_x));
	tryvm_if(pkginfo == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for pkginfo");

	pkginfo->locale = strdup(locale);

	pkginfo->manifest_info = (manifest_x *)calloc(1, sizeof(manifest_x));
	tryvm_if(pkginfo->manifest_info == NULL, ret = PMINFO_R_ERROR, "Failed to allocate memory for manifest info");

	pkginfo->manifest_info->package = strdup(pkgid);

	/*populate manifest_info from DB*/
	query= sqlite3_mprintf("select * from package_info where package=%Q and package_disable='true'", pkgid);
	ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");

	/*populate privilege_info from DB*/
	query= sqlite3_mprintf("select * from package_privilege_info where package=%Q ", pkgid);
	ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Privilege Info DB Information retrieval failed");

	query= sqlite3_mprintf("select * from package_localized_info where package=%Q and package_locale=%Q", pkgid, locale);
	ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");

	/*Also store the values corresponding to default locales*/
	query= sqlite3_mprintf("select * from package_localized_info where package=%Q and package_locale=%Q", pkgid, DEFAULT_LOCALE);
	ret = __exec_db_query(pkginfo_db, query, __pkginfo_cb, (void *)pkginfo);
	sqlite3_free(query);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "Package Info DB Information retrieval failed");


	ret = __pkginfo_check_installed_storage(pkginfo);
	tryvm_if(ret == -1, ret = PMINFO_R_ERROR, "[%s] is installed external, but is not in mmc", pkgid);

catch:
	if (ret == PMINFO_R_OK)
		*handle = (void*)pkginfo;
	else {
		*handle = NULL;
		__cleanup_pkginfo(pkginfo);
	}
	sqlite3_close(pkginfo_db);

	FREE_AND_NULL(locale);
	return ret;
}

API int pkgmgrinfo_pkginfo_get_pkgname(pkgmgrinfo_pkginfo_h handle, char **pkg_name)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(pkg_name == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	if (info->manifest_info->package)
		*pkg_name = (char *)info->manifest_info->package;
	else
		return PMINFO_R_ERROR;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_pkgid(pkgmgrinfo_pkginfo_h handle, char **pkgid)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(pkgid == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	if (info->manifest_info->package)
		*pkgid = (char *)info->manifest_info->package;
	else
		return PMINFO_R_ERROR;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_type(pkgmgrinfo_pkginfo_h handle, char **type)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(type == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	if (info->manifest_info->type)
		*type = (char *)info->manifest_info->type;
	else
		*type = "rpm";
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_version(pkgmgrinfo_pkginfo_h handle, char **version)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(version == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	*version = (char *)info->manifest_info->version;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_api_version(pkgmgrinfo_pkginfo_h handle, char **api_version)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(api_version == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	*api_version = (char *)info->manifest_info->api_version;
	return PMINFO_R_OK;
}

#ifdef _APPFW_FEATURE_EXPANSION_PKG_INSTALL
API int pkgmgrinfo_pkginfo_get_tep_name(pkgmgrinfo_pkginfo_h handle, char **tep_name)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "appinfo handle is NULL\n");
	retvm_if(tep_name == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	if (info->manifest_info && info->manifest_info->tep_name)
		*tep_name = (char *)(info->manifest_info->tep_name);
	else
		return PMINFO_R_ERROR;

	return PMINFO_R_OK;
}
#endif

API int pkgmgrinfo_pkginfo_check_api_version(pkgmgrinfo_pkginfo_h handle, const char *api_version, int *result)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(api_version == NULL, PMINFO_R_EINVAL, "api_version is NULL\n");
	retvm_if(result == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	int ret;
	char *pkg_api_version = NULL;

	ret = pkgmgrinfo_pkginfo_get_api_version(handle, &pkg_api_version);
	if (ret != PMINFO_R_OK) {
		_LOGE("pkgmgrinfo_pkginfo_get_api_version is failed\n");
		return PMINFO_R_ERROR;
	}

	if (pkg_api_version == NULL) {
		_LOGE("pkg_api_version is null\n");
		return PMINFO_R_ERROR;
	}

	ret = strverscmp(pkg_api_version, api_version);

	*result = ret;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_install_location(pkgmgrinfo_pkginfo_h handle, pkgmgrinfo_install_location *location)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(location == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	char *val = NULL;
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	val = (char *)info->manifest_info->installlocation;
	if (val) {
		if (strcmp(val, "internal-only") == 0)
			*location = PMINFO_INSTALL_LOCATION_INTERNAL_ONLY;
		else if (strcmp(val, "prefer-external") == 0)
			*location = PMINFO_INSTALL_LOCATION_PREFER_EXTERNAL;
		else
			*location = PMINFO_INSTALL_LOCATION_AUTO;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_icon(pkgmgrinfo_pkginfo_h handle, char **icon)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL");
	retvm_if(icon == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	char *locale = NULL;
	icon_x *ptr = NULL;
	GList *start = NULL;
	*icon = NULL;

	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;

	locale = info->locale;
	retvm_if(locale == NULL, PMINFO_R_ERROR, "manifest locale is NULL");

	for(start = info->manifest_info->icon; start != NULL; start = start->next)
	{
		ptr = (icon_x*)start->data;
		if (ptr && ptr->lang) {
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

	retvm_if(*icon == NULL, PMINFO_R_ERROR, "Failed to find icon");

	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_label(pkgmgrinfo_pkginfo_h handle, char **label)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL");
	retvm_if(label == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");
	int ret = PMINFO_R_OK;
	char *locale = NULL;
	label_x *ptr = NULL;
	GList *lbl_list = NULL;
	*label = NULL;

	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	locale = info->locale;
	retvm_if(locale == NULL, PMINFO_R_ERROR, "manifest locale is NULL");

	for(lbl_list = info->manifest_info->label; lbl_list != NULL; lbl_list = lbl_list->next)
	{
		ptr = (label_x*)lbl_list->data;
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
			} else if (strcmp(ptr->lang, DEFAULT_LOCALE) == 0) {
				*label = (char *)ptr->text;
				break;
			}
		}
	}

	return ret;
}

API int pkgmgrinfo_pkginfo_get_description(pkgmgrinfo_pkginfo_h handle, char **description)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(description == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	char *locale = NULL;
	description_x *ptr = NULL;
	GList *desc_list = NULL;
	*description = NULL;

	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	locale = info->locale;
	retvm_if(locale == NULL, PMINFO_R_ERROR, "manifest locale is NULL");

	for(desc_list = info->manifest_info->description; desc_list != NULL; desc_list = desc_list->next)
	{
		ptr = (description_x*)desc_list->data;
		if(ptr){
			if (ptr->lang) {
				if (strcmp(ptr->lang, locale) == 0) {
					*description = (char *)ptr->text;
					if (strcasecmp(*description, PKGMGR_PARSER_EMPTY_STR) == 0) {
						locale = DEFAULT_LOCALE;
						continue;
					} else
						break;
				} else if (strcmp(ptr->lang, DEFAULT_LOCALE) == 0) {
					*description = (char *)ptr->text;
					break;
				}
			}
		}
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_author_name(pkgmgrinfo_pkginfo_h handle, char **author_name)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(author_name == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	char *locale = NULL;
	author_x *ptr = NULL;
	*author_name = NULL;
	GList *ath_list = NULL;

	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	locale = info->locale;
	retvm_if(locale == NULL, PMINFO_R_ERROR, "manifest locale is NULL");

	for(ath_list = info->manifest_info->author; ath_list != NULL; ath_list = ath_list->next)
	{
		ptr = (author_x*)ath_list->data;
		if (ptr->lang) {
			if (strcmp(ptr->lang, locale) == 0) {
				*author_name = (char *)ptr->text;
				if (strcasecmp(*author_name, PKGMGR_PARSER_EMPTY_STR) == 0) {
					locale = DEFAULT_LOCALE;
					continue;
				} else
					break;
			} else if (strcmp(ptr->lang, DEFAULT_LOCALE) == 0) {
				*author_name = (char *)ptr->text;
				break;
			}
		}
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_author_email(pkgmgrinfo_pkginfo_h handle, char **author_email)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(author_email == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	author_x* author = (author_x*)info->manifest_info->author->data;
	*author_email = (char *)author->email;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_author_href(pkgmgrinfo_pkginfo_h handle, char **author_href)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(author_href == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	author_x* author = (author_x*)info->manifest_info->author->data;
	*author_href = (char *)author->href;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_installed_storage(pkgmgrinfo_pkginfo_h handle, pkgmgrinfo_installed_storage *storage)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(storage == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;

	 if (strcmp(info->manifest_info->installed_storage,"installed_internal") == 0)
	 	*storage = PMINFO_INTERNAL_STORAGE;
	 else if (strcmp(info->manifest_info->installed_storage,"installed_external") == 0)
		 *storage = PMINFO_EXTERNAL_STORAGE;
	 else
		 return PMINFO_R_ERROR;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_installed_time(pkgmgrinfo_pkginfo_h handle, int *installed_time)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(installed_time == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	if (info->manifest_info->installed_time)
		*installed_time = atoi(info->manifest_info->installed_time);
	else
		return PMINFO_R_ERROR;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_storeclientid(pkgmgrinfo_pkginfo_h handle, char **storeclientid)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(storeclientid == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	*storeclientid = (char *)info->manifest_info->storeclient_id;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_mainappid(pkgmgrinfo_pkginfo_h handle, char **mainappid)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(mainappid == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	*mainappid = (char *)info->manifest_info->mainapp_id;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_url(pkgmgrinfo_pkginfo_h handle, char **url)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(url == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	*url = (char *)info->manifest_info->package_url;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_root_path(pkgmgrinfo_pkginfo_h handle, char **path)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(path == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	if (info->manifest_info->root_path)
		*path = (char *)info->manifest_info->root_path;
	else
		return PMINFO_R_ERROR;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_csc_path(pkgmgrinfo_pkginfo_h handle, char **path)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(path == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	if (info->manifest_info->csc_path)
		*path = (char *)info->manifest_info->csc_path;
	else
		return PMINFO_R_ERROR;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_support_mode(pkgmgrinfo_pkginfo_h handle, int *support_mode)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(support_mode == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	if (info->manifest_info->support_mode)
		*support_mode = atoi(info->manifest_info->support_mode);
	else
		*support_mode = 0;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_groupid(pkgmgrinfo_pkginfo_h handle, char **groupid)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(groupid == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	if (info->manifest_info->groupid)
		*groupid = (char *)info->manifest_info->groupid;
	else
		return PMINFO_R_ERROR;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_backend_installer(pkgmgrinfo_pkginfo_h handle, char **backend_installer)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(backend_installer == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	if (info->manifest_info->backend_installer)
		*backend_installer = (char *)info->manifest_info->backend_installer;
	else
		return PMINFO_R_ERROR;

	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_get_custom_smack_label(pkgmgrinfo_pkginfo_h handle, char **smack_label)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(smack_label == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	*smack_label = (char *)info->manifest_info->custom_smack_label;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_foreach_privilege(pkgmgrinfo_pkginfo_h handle,
			pkgmgrinfo_pkg_privilege_list_cb privilege_func, void *user_data)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL");
	retvm_if(privilege_func == NULL, PMINFO_R_EINVAL, "Callback function is NULL");
	int ret = -1;
	GList *list = NULL;
	char *ptr = NULL;
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	list = info->manifest_info->privileges;
	for (; list; list = list->next) {
		ptr = (char*)list->data;
		if (ptr){
			ret = privilege_func(ptr, user_data);
			if (ret < 0)
				break;
		}
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_compare_pkg_cert_info(const char *lhs_package_id, const char *rhs_package_id, pkgmgrinfo_cert_compare_result_type_e *compare_result)
{
	retvm_if(lhs_package_id == NULL, PMINFO_R_EINVAL, "lhs package ID is NULL");
	retvm_if(rhs_package_id == NULL, PMINFO_R_EINVAL, "rhs package ID is NULL");
	retvm_if(compare_result == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");

	int ret = PMINFO_R_OK;
	char *query = NULL;
	char *error_message = NULL;
	pkgmgr_cert_x *info= NULL;
	int lcert = 0;
	int rcert = 0;
	int exist = -1;
	sqlite3 *pkgmgr_cert_db = NULL;

	*compare_result = PMINFO_CERT_COMPARE_ERROR;
	info = (pkgmgr_cert_x *)calloc(1, sizeof(pkgmgr_cert_x));
	retvm_if(info == NULL, PMINFO_R_ERROR, "Out of Memory!!!");

	ret = _pminfo_db_open_with_options(CERT_DB, &pkgmgr_cert_db, SQLITE_OPEN_READONLY);
	if (ret != SQLITE_OK) {
		_LOGE("connect db [%s] failed!\n", CERT_DB);
		ret = PMINFO_R_ERROR;
		goto err;
	}

	query = sqlite3_mprintf("select exists(select * from package_cert_info where package=%Q)", lhs_package_id);
	if (SQLITE_OK !=
	    sqlite3_exec(pkgmgr_cert_db, query, _pkgmgrinfo_validate_cb, (void *)&exist, &error_message)) {
		_LOGE("Don't execute query = %s error message = %s\n", query,
		       error_message);
		ret = PMINFO_R_ERROR;
		sqlite3_free(query);
		goto err;
	}

	if (exist == 0) {
		lcert = 0;
	} else {
		sqlite3_free(query);
		query = sqlite3_mprintf("select author_signer_cert from package_cert_info where package=%Q", lhs_package_id);
		if (SQLITE_OK !=
			sqlite3_exec(pkgmgr_cert_db, query, __cert_cb, (void *)info, &error_message)) {
			_LOGE("Don't execute query = %s error message = %s\n", query,
				   error_message);
			ret = PMINFO_R_ERROR;
			sqlite3_free(query);
			goto err;
		}
		lcert = info->cert_id;
	}

	sqlite3_free(query);
	query = sqlite3_mprintf("select exists(select * from package_cert_info where package=%Q)", rhs_package_id);
	if (SQLITE_OK !=
		sqlite3_exec(pkgmgr_cert_db, query, _pkgmgrinfo_validate_cb, (void *)&exist, &error_message)) {
		_LOGE("Don't execute query = %s error message = %s\n", query,
			   error_message);
		ret = PMINFO_R_ERROR;
		sqlite3_free(query);
		goto err;
	}

	if (exist == 0) {
		rcert = 0;
	} else {
		sqlite3_free(query);
		query = sqlite3_mprintf("select author_signer_cert from package_cert_info where package=%Q", rhs_package_id);
		if (SQLITE_OK !=
			sqlite3_exec(pkgmgr_cert_db, query, __cert_cb, (void *)info, &error_message)) {
			_LOGE("Don't execute query = %s error message = %s\n", query,
				   error_message);
			ret = PMINFO_R_ERROR;
			sqlite3_free(query);
			goto err;
		}
		rcert = info->cert_id;
		sqlite3_free(query);
	}

	if ((lcert == 0) || (rcert == 0))
	{
		if ((lcert == 0) && (rcert == 0))
			*compare_result = PMINFO_CERT_COMPARE_BOTH_NO_CERT;
		else if (lcert == 0)
			*compare_result = PMINFO_CERT_COMPARE_LHS_NO_CERT;
		else if (rcert == 0)
			*compare_result = PMINFO_CERT_COMPARE_RHS_NO_CERT;
	} else {
		if (lcert == rcert)
			*compare_result = PMINFO_CERT_COMPARE_MATCH;
		else
			*compare_result = PMINFO_CERT_COMPARE_MISMATCH;
	}

err:
	sqlite3_free(error_message);
	sqlite3_close(pkgmgr_cert_db);
	if (info) {
		FREE_AND_NULL(info->pkgid);
		FREE_AND_NULL(info);
	}
	return ret;
}


API int pkgmgrinfo_pkginfo_compare_app_cert_info(const char *lhs_app_id, const char *rhs_app_id, pkgmgrinfo_cert_compare_result_type_e *compare_result)
{
	retvm_if(lhs_app_id == NULL, PMINFO_R_EINVAL, "lhs app ID is NULL");
	retvm_if(rhs_app_id == NULL, PMINFO_R_EINVAL, "rhs app ID is NULL");
	retvm_if(compare_result == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL");

	int ret = PMINFO_R_OK;
	char *query = NULL;
	char *error_message = NULL;
	pkgmgr_cert_x *info= NULL;
 	int exist = -1;
	char *lpkgid = NULL;
	char *rpkgid = NULL;
	sqlite3 *pkginfo_db = NULL;

	info = (pkgmgr_cert_x *)calloc(1, sizeof(pkgmgr_cert_x));
	retvm_if(info == NULL, PMINFO_R_ERROR, "Out of Memory!!!");

	ret = _pminfo_db_open(MANIFEST_DB, &pkginfo_db);
	tryvm_if(ret != SQLITE_OK, ret = PMINFO_R_ERROR, "connect db [%s] failed!", MANIFEST_DB);

	query = sqlite3_mprintf("select exists(select * from package_app_info where app_id=%Q)", lhs_app_id);
	if (SQLITE_OK !=
	    sqlite3_exec(pkginfo_db, query, _pkgmgrinfo_validate_cb, (void *)&exist, &error_message)) {
		_LOGE("Don't execute query = %s error message = %s\n", query,
		       error_message);
		ret = PMINFO_R_ERROR;
		sqlite3_free(query);
		goto catch;
	}

	if (exist == 0) {
		lpkgid = NULL;
	} else {
		sqlite3_free(query);
		query = sqlite3_mprintf("select package from package_app_info where app_id=%Q", lhs_app_id);
		if (SQLITE_OK !=
			sqlite3_exec(pkginfo_db, query, __cert_cb, (void *)info, &error_message)) {
			_LOGE("Don't execute query = %s error message = %s\n", query,
				   error_message);
			ret = PMINFO_R_ERROR;
			sqlite3_free(query);
			goto catch;
		}
		lpkgid = strdup(info->pkgid);
		if (lpkgid == NULL) {
			_LOGE("Out of Memory\n");
			ret = PMINFO_R_ERROR;
			sqlite3_free(query);
			goto catch;
		}
		free(info->pkgid);
		info->pkgid = NULL;
	}

	sqlite3_free(query);
	query = sqlite3_mprintf("select exists(select * from package_app_info where app_id=%Q)", rhs_app_id);
	if (SQLITE_OK !=
	    sqlite3_exec(pkginfo_db, query, _pkgmgrinfo_validate_cb, (void *)&exist, &error_message)) {
		_LOGE("Don't execute query = %s error message = %s\n", query,
		       error_message);
		ret = PMINFO_R_ERROR;
		sqlite3_free(query);
		goto catch;
	}

	if (exist == 0) {
		rpkgid = NULL;
	} else {
		sqlite3_free(query);
		query = sqlite3_mprintf("select package from package_app_info where app_id=%Q", rhs_app_id);
		if (SQLITE_OK !=
			sqlite3_exec(pkginfo_db, query, __cert_cb, (void *)info, &error_message)) {
			_LOGE("Don't execute query = %s error message = %s\n", query,
				   error_message);
			ret = PMINFO_R_ERROR;
			sqlite3_free(query);
			goto catch;
		}
		rpkgid = strdup(info->pkgid);
		if (rpkgid == NULL) {
			_LOGE("Out of Memory\n");
			ret = PMINFO_R_ERROR;
			sqlite3_free(query);
			goto catch;
		}
		sqlite3_free(query);
		FREE_AND_NULL(info->pkgid);
	}
	ret = pkgmgrinfo_pkginfo_compare_pkg_cert_info(lpkgid, rpkgid, compare_result);

 catch:
	sqlite3_free(error_message);
	sqlite3_close(pkginfo_db);
	if (info) {
		FREE_AND_NULL(info->pkgid);
		FREE_AND_NULL(info);
	}
	FREE_AND_NULL(lpkgid);
	FREE_AND_NULL(rpkgid);
	return ret;
}

API int pkgmgrinfo_pkginfo_is_accessible(pkgmgrinfo_pkginfo_h handle, bool *accessible)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(accessible == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	*accessible = 1;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_is_removable(pkgmgrinfo_pkginfo_h handle, bool *removable)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(removable == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	char *val = NULL;
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	val = (char *)info->manifest_info->removable;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*removable = 1;
		else if (strcasecmp(val, "false") == 0)
			*removable = 0;
		else
			*removable = 1;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_is_movable(pkgmgrinfo_pkginfo_h handle, bool *movable)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(movable == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	char *val = NULL;
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;

	val = (char *)info->manifest_info->installlocation;
	if (val) {
		if (strcmp(val, "internal-only") == 0)
			*movable = 0;
		else if (strcmp(val, "prefer-external") == 0)
			*movable = 1;
		else
			*movable = 1;
	}

	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_is_preload(pkgmgrinfo_pkginfo_h handle, bool *preload)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(preload == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	char *val = NULL;
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	val = (char *)info->manifest_info->preload;
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

API int pkgmgrinfo_pkginfo_is_system(pkgmgrinfo_pkginfo_h handle, bool *system)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(system == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	char *val = NULL;
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	val = (char *)info->manifest_info->system;
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

API int pkgmgrinfo_pkginfo_is_readonly(pkgmgrinfo_pkginfo_h handle, bool *readonly)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(readonly == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");
	char *val = NULL;
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	val = (char *)info->manifest_info->readonly;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*readonly = 1;
		else if (strcasecmp(val, "false") == 0)
			*readonly = 0;
		else
			*readonly = 0;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_is_update(pkgmgrinfo_pkginfo_h handle, bool *update)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(update == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	char *val = NULL;
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	val = (char *)info->manifest_info->update;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*update = 1;
		else if (strcasecmp(val, "false") == 0)
			*update = 0;
		else
			*update = 1;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_is_support_disable(pkgmgrinfo_pkginfo_h handle, bool *support_disable)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(support_disable == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	char *val = NULL;
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	val = (char *)info->manifest_info->support_disable;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*support_disable = 1;
		else if (strcasecmp(val, "false") == 0)
			*support_disable = 0;
		else
			*support_disable = 1;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_is_mother_package(pkgmgrinfo_pkginfo_h handle, bool *mother_package)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	retvm_if(mother_package == NULL, PMINFO_R_EINVAL, "Argument supplied to hold return value is NULL\n");

	char *val = NULL;
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	val = (char *)info->manifest_info->mother_package;
	if (val) {
		if (strcasecmp(val, "true") == 0)
			*mother_package = 1;
		else if (strcasecmp(val, "false") == 0)
			*mother_package = 0;
		else
			*mother_package = 1;
	}
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_destroy_pkginfo(pkgmgrinfo_pkginfo_h handle)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "pkginfo handle is NULL\n");
	pkgmgr_pkginfo_x *info = (pkgmgr_pkginfo_x *)handle;
	__cleanup_pkginfo(info);
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_filter_create(pkgmgrinfo_pkginfo_filter_h *handle)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Filter handle output parameter is NULL\n");
	*handle = NULL;
	pkgmgrinfo_filter_x *filter = (pkgmgrinfo_filter_x*)calloc(1, sizeof(pkgmgrinfo_filter_x));
	if (filter == NULL) {
		_LOGE("Out of Memory!!!");
		return PMINFO_R_ERROR;
	}
	*handle = filter;
	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_filter_destroy(pkgmgrinfo_pkginfo_filter_h handle)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	pkgmgrinfo_filter_x *filter = (pkgmgrinfo_filter_x*)handle;
	if (filter->list){
		g_slist_foreach(filter->list, __destroy_each_node, NULL);
		g_slist_free(filter->list);
	}
	FREE_AND_NULL(filter);

	return PMINFO_R_OK;
}

API int pkgmgrinfo_pkginfo_filter_add_int(pkgmgrinfo_pkginfo_filter_h handle,
				const char *property, const int value)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	retvm_if(property == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	char buf[PKG_VALUE_STRING_LEN_MAX] = {'\0'};
	char *val = NULL;
	GSList *link = NULL;
	int prop = -1;
	prop = _pminfo_pkginfo_convert_to_prop_int(property);
	if (prop < E_PMINFO_PKGINFO_PROP_PACKAGE_MIN_INT ||
		prop > E_PMINFO_PKGINFO_PROP_PACKAGE_MAX_INT) {
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
	if (link) {
		_pminfo_destroy_node((gpointer)link->data);
		filter->list = g_slist_delete_link(filter->list, link);
	}
	filter->list = g_slist_append(filter->list, (gpointer)node);
	return PMINFO_R_OK;

}

API int pkgmgrinfo_pkginfo_filter_add_bool(pkgmgrinfo_pkginfo_filter_h handle,
				const char *property, const bool value)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	retvm_if(property == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	char *val = NULL;
	GSList *link = NULL;
	int prop = -1;
	prop = _pminfo_pkginfo_convert_to_prop_bool(property);
	if (prop < E_PMINFO_PKGINFO_PROP_PACKAGE_MIN_BOOL ||
		prop > E_PMINFO_PKGINFO_PROP_PACKAGE_MAX_BOOL) {
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
	if (link) {
		_pminfo_destroy_node((gpointer)link->data);
		filter->list = g_slist_delete_link(filter->list, link);
	}
	filter->list = g_slist_append(filter->list, (gpointer)node);
	return PMINFO_R_OK;

}

API int pkgmgrinfo_pkginfo_filter_add_string(pkgmgrinfo_pkginfo_filter_h handle,
				const char *property, const char *value)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	retvm_if(property == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	retvm_if(value == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	char *val = NULL;
	GSList *link = NULL;
	int prop = -1;
	prop = _pminfo_pkginfo_convert_to_prop_str(property);
	if (prop < E_PMINFO_PKGINFO_PROP_PACKAGE_MIN_STR ||
		prop > E_PMINFO_PKGINFO_PROP_PACKAGE_MAX_STR) {
		_LOGE("Invalid String Property\n");
		return PMINFO_R_EINVAL;
	}
	pkgmgrinfo_filter_x *filter = (pkgmgrinfo_filter_x*)handle;
	pkgmgrinfo_node_x *node = (pkgmgrinfo_node_x*)calloc(1, sizeof(pkgmgrinfo_node_x));
	if (node == NULL) {
		_LOGE("Out of Memory!!!\n");
		return PMINFO_R_ERROR;
	}
	if (strcmp(value, PMINFO_PKGINFO_INSTALL_LOCATION_AUTO) == 0)
		val = strndup("auto", PKG_STRING_LEN_MAX - 1);
	else if (strcmp(value, PMINFO_PKGINFO_INSTALL_LOCATION_INTERNAL) == 0)
		val = strndup("internal-only", PKG_STRING_LEN_MAX - 1);
	else if (strcmp(value, PMINFO_PKGINFO_INSTALL_LOCATION_EXTERNAL) == 0)
		val = strndup("prefer-external", PKG_STRING_LEN_MAX - 1);
	else if (strcmp(value, "installed_internal") == 0)
		val = strndup("installed_internal", PKG_STRING_LEN_MAX - 1);
	else if (strcmp(value, "installed_external") == 0)
		val = strndup("installed_external", PKG_STRING_LEN_MAX - 1);
	else
		val = strndup(value, PKG_STRING_LEN_MAX - 1);
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
	if (link) {
		_pminfo_destroy_node((gpointer)link->data);
		filter->list = g_slist_delete_link(filter->list, link);
	}
	filter->list = g_slist_append(filter->list, (gpointer)node);
	return PMINFO_R_OK;

}

API int pkgmgrinfo_pkginfo_filter_count(pkgmgrinfo_pkginfo_filter_h handle, int *count)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	retvm_if(count == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");

	int ret = 0;
	int filter_count = 0;

	char pkgid[MAX_QUERY_LEN] = {0,};
	char pre_pkgid[MAX_QUERY_LEN] = {0,};

	char *locale = NULL;
	char *condition = NULL;
	char query[MAX_QUERY_LEN] = {'\0'};
	char where[MAX_QUERY_LEN] = {'\0'};
	GSList *list;

	pkgmgr_pkginfo_x *pkginfo = NULL;
	pkgmgrinfo_filter_x *filter = (pkgmgrinfo_filter_x*)handle;

	GList *node = NULL;
	GList *list_pkginfo = NULL;

	sqlite3 *pkginfo_db = NULL;
	sqlite3_stmt *stmt = NULL;


	/*open db*/
	ret = _pminfo_db_open(MANIFEST_DB, &pkginfo_db);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "_pminfo_db_open[%s] failed!", MANIFEST_DB);

	/*Start constructing query*/
	locale = __convert_system_locale_to_manifest_locale();
	sqlite3_snprintf(MAX_QUERY_LEN - 1, query, FILTER_QUERY_LIST_PACKAGE, DEFAULT_LOCALE, locale);

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

	if (strstr(where, "package_info.package_disable") == NULL) {
		if (strlen(where) > 0) {
			strncat(where, " and package_info.package_disable IN ('false','False')", sizeof(where) - strlen(where) - 1);
			where[sizeof(where) - 1] = '\0';
		}
	}

	_LOGE("where = %s\n", where);
	if (strlen(where) > 0) {
		strncat(query, where, sizeof(query) - strlen(query) - 1);
		query[sizeof(query) - 1] = '\0';
	}
	_LOGE("query = %s\n", query);

	ret = sqlite3_prepare_v2(pkginfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	while(1) {
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {

			memset(pkgid, 0, MAX_QUERY_LEN);
			strncpy(pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

			if (strlen(pre_pkgid) != 0) {
				if (strcmp(pre_pkgid, pkgid) == 0) {
					node = g_list_last(list_pkginfo);
					if (node)
						__update_localed_label_for_list(stmt, (pkgmgr_pkginfo_x *)node->data);

					memset(pre_pkgid, 0, MAX_QUERY_LEN);
					strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

					continue;
				} else {
					memset(pre_pkgid, 0, MAX_QUERY_LEN);
					strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
				}
			} else {
				strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
			}

			__get_pkginfo_for_list(stmt, &list_pkginfo);
		} else {
			break;
		}
	}

	for(node = list_pkginfo; node ; node = node->next) {
		pkginfo = (pkgmgr_pkginfo_x *)node->data;
		ret = __pkginfo_check_installed_storage(pkginfo);
		if(ret < 0)
			continue;
		filter_count++;
	}

	*count = filter_count;
	ret = PMINFO_R_OK;

catch:
	FREE_AND_NULL(locale);
	sqlite3_finalize(stmt);
	sqlite3_close(pkginfo_db);

	__cleanup_list_pkginfo(&list_pkginfo);
	node = NULL;

	return ret;
}

API int pkgmgrinfo_pkginfo_filter_foreach_pkginfo(pkgmgrinfo_pkginfo_filter_h handle,
				pkgmgrinfo_pkg_list_cb pkg_cb, void *user_data)
{
	retvm_if(handle == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	retvm_if(pkg_cb == NULL, PMINFO_R_EINVAL, "Filter handle input parameter is NULL\n");
	char *locale = NULL;
	char *condition = NULL;
	char query[MAX_QUERY_LEN] = {'\0'};
	char where[MAX_QUERY_LEN] = {'\0'};
	GSList *list;
	int ret = 0;

	char pkgid[MAX_QUERY_LEN] = {0,};
	char pre_pkgid[MAX_QUERY_LEN] = {0,};

	pkgmgr_pkginfo_x *pkginfo = NULL;
	pkgmgrinfo_filter_x *filter = (pkgmgrinfo_filter_x*)handle;

	GList *node = NULL;
	GList *list_pkginfo = NULL;

	sqlite3 *pkginfo_db = NULL;
	sqlite3_stmt *stmt = NULL;

	/*open db*/
	ret = _pminfo_db_open(MANIFEST_DB, &pkginfo_db);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "_pminfo_db_open[%s] failed!", MANIFEST_DB);

	/*Start constructing query*/
	locale = __convert_system_locale_to_manifest_locale();
	retvm_if(locale == NULL, PMINFO_R_ERROR, "convert system locale to manifest locale fail");

	sqlite3_snprintf(MAX_QUERY_LEN - 1, query, FILTER_QUERY_LIST_PACKAGE, DEFAULT_LOCALE, locale);

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

	if (strstr(where, "package_info.package_disable") == NULL) {
		if (strlen(where) > 0) {
			strncat(where, " and package_info.package_disable IN ('false','False')", sizeof(where) - strlen(where) - 1);
			where[sizeof(where) - 1] = '\0';
		}
	}

	_LOGE("where = %s\n", where);

	if (strlen(where) > 0) {
		strncat(query, where, sizeof(query) - strlen(query) - 1);
		query[sizeof(query) - 1] = '\0';
	}
	_LOGE("query = %s\n", query);

	ret = sqlite3_prepare_v2(pkginfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	while(1) {
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {

			memset(pkgid, 0, MAX_QUERY_LEN);
			strncpy(pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

			if (strlen(pre_pkgid) != 0) {
				if (strcmp(pre_pkgid, pkgid) == 0) {
					node = g_list_last(list_pkginfo);
					if (node)
						__update_localed_label_for_list(stmt, (pkgmgr_pkginfo_x *)node->data);

					memset(pre_pkgid, 0, MAX_QUERY_LEN);
					strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);

					continue;
				} else {
					memset(pre_pkgid, 0, MAX_QUERY_LEN);
					strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
				}
			} else {
				strncpy(pre_pkgid, (const char *)sqlite3_column_text(stmt, 0), MAX_QUERY_LEN - 1);
			}

			__get_pkginfo_for_list(stmt, &list_pkginfo);
		} else {
			break;
		}
	}

	for(node = list_pkginfo ; node ; node = node->next) {
		pkginfo = (pkgmgr_pkginfo_x *)node->data;
		pkginfo->locale = strdup(locale);

		ret = __pkginfo_check_installed_storage(pkginfo);
		if(ret < 0)
			continue;

		ret = pkg_cb( (void *)pkginfo, user_data);
		if(ret < 0) {
			_LOGE("callback is stopped.");
			break;
		}
	}
	ret = PMINFO_R_OK;

catch:
	FREE_AND_NULL(locale);
	sqlite3_finalize(stmt);
	sqlite3_close(pkginfo_db);

	__cleanup_list_pkginfo(&list_pkginfo);
	node = NULL;

	return ret;
}

API int pkgmgrinfo_pkginfo_privilege_filter_foreach(const char *privilege,
			pkgmgrinfo_pkg_list_cb pkg_list_cb, void *user_data)
{
	retvm_if(privilege == NULL, PMINFO_R_EINVAL, "privilege is NULL\n");
	retvm_if(pkg_list_cb == NULL, PMINFO_R_EINVAL, "callback function is NULL\n");

	int ret = PMINFO_R_OK;
	char *query = NULL;

	sqlite3 *pkginfo_db = NULL;
	sqlite3_stmt *stmt = NULL;

	pkgmgr_pkginfo_x *pkginfo = NULL;

	GList *node = NULL;
	GList *list_pkginfo = NULL;

	/*open db*/
	ret = _pminfo_db_open(MANIFEST_DB, &pkginfo_db);
	retvm_if(ret != SQLITE_OK, PMINFO_R_ERROR, "_pminfo_db_open[%s] failed!", MANIFEST_DB);

	/*Start constructing query*/
	query = sqlite3_mprintf("select package_info.* from package_info LEFT OUTER JOIN package_privilege_info " \
			"ON package_privilege_info.package=package_info.package " \
			"where package_privilege_info.privilege=%Q and package_info.package_disable='false'", privilege);

	/*prepare query*/
	ret = sqlite3_prepare_v2(pkginfo_db, query, strlen(query), &stmt, NULL);
	tryvm_if(ret != PMINFO_R_OK, ret = PMINFO_R_ERROR, "sqlite3_prepare_v2 failed[%s]\n", query);

	/*step query*/
	while(1) {
		ret = sqlite3_step(stmt);
		if(ret == SQLITE_ROW) {
			__get_pkginfo_for_list(stmt, &list_pkginfo);
		} else {
			break;
		}
	}

	for(node = list_pkginfo; node ; node = node->next) {
		pkginfo = (pkgmgr_pkginfo_x *)node->data;
		ret = pkg_list_cb( (void *)pkginfo, user_data);
		if(ret < 0) {
			_LOGE("callback is stopped.");
			break;
		}
	}
	ret = PMINFO_R_OK;

catch:
	sqlite3_free(query);
	sqlite3_finalize(stmt);
	sqlite3_close(pkginfo_db);
	__cleanup_list_pkginfo(&list_pkginfo);
	node = NULL;

	return ret;
}

API int pkgmgrinfo_updateinfo_register(const char* updateinfo)
{
	retvm_if(updateinfo == NULL, PMINFO_R_EINVAL, "updateinfo is NULL");


#if 0
	int ret = 0;
	ret = vconf_set_str(VCONFKEY_PKGMGR_UPDATE_INFO, updateinfo);
	retvm_if(ret < 0, PMINFO_R_ERROR, "vconf_set_str(%s) fail", updateinfo);
#endif

	return PMINFO_R_OK;
}

API int pkgmgrinfo_updateinfo_check_update(const char* pkgid)
{
	retvm_if(pkgid == NULL, PMINFO_R_ERROR, "pkgid is NULL");

	int ret = -1;
#if 0
	char *vconf_str = NULL;
	char pkgid_is[PKG_STRING_LEN_MAX] = {0,};

	vconf_str = vconf_get_str(VCONFKEY_PKGMGR_UPDATE_INFO);
	if (vconf_str == NULL) {
		return PMINFO_R_ERROR;
	}

	strncat(pkgid_is, pkgid, strlen(pkgid));
	strncat(pkgid_is, ":", strlen(":"));

	if (strstr(vconf_str, pkgid_is)) {
		_LOGS("pkgid[%s] has update version", pkgid);
		ret = PMINFO_R_OK;
	} else {
		ret = PMINFO_R_ERROR;
	}

	FREE_AND_NULL(vconf_str);
#endif
	return ret;
}

API int pkgmgrinfo_updateinfo_remove(const char* pkgid)
{
	retvm_if(pkgid == NULL, PMINFO_R_EINVAL, "pkgid is NULL");

	int ret = 0;
#if 0
	int len = 0;
	char *vconf_str = NULL;
	char pkgid_is[PKG_STRING_LEN_MAX] = {0,};
	char new_info[PKG_STRING_LEN_MAX] = {0,};

	const char* head = NULL;
	const char* tail = NULL;

	vconf_str = vconf_get_str(VCONFKEY_PKGMGR_UPDATE_INFO);
	if (vconf_str == NULL) {
		return PMINFO_R_ERROR;
	}

	strncat(pkgid_is, ":", strlen(":"));
	strncat(pkgid_is, pkgid, strlen(pkgid));
	strncat(pkgid_is, ":", strlen(":"));
	tryvm_if(strstr(vconf_str, pkgid_is) == NULL, ret = PMINFO_R_ERROR, "pkgid is already removed");

	memset(pkgid_is, 0, PKG_STRING_LEN_MAX);
	strncat(pkgid_is, pkgid, strlen(pkgid));
	strncat(pkgid_is, ":", strlen(":"));

	head = strstr(vconf_str, pkgid_is);
	if (head != NULL) {
		len = strlen(vconf_str) - strlen(head);
		strncat(new_info, vconf_str, len);
	}

	tail = head + strlen(pkgid) + 1;
	if (tail != NULL) {
		strncat(new_info, tail, strlen(tail));
	}

	ret = vconf_set_str(VCONFKEY_PKGMGR_UPDATE_INFO, new_info);
	tryvm_if(ret < 0, ret = PMINFO_R_ERROR, "vconf_set_str fail");
#endif

	ret = PMINFO_R_OK;

//catch:
	//FREE_AND_NULL(vconf_str);
	return ret;
}
